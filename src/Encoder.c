/**
 * @file src/Encoder.c
 * @date 2011
 */

#include "ndlcom/Encoder.h"

/* used for one single micro-optimization...
 * see http://stackoverflow.com/questions/109710
 */
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

/**
 * @brief Used to pack some payload into a distinct data-format, which may be
 * sent over your serial connection.
 *
 * Encoding scheme based on RFC1662, see
 * https://tools.ietf.org/html/rfc1662.html
 *
 * FCS - frame check sequence (aka crc) is calculated over all bytes of the
 * packet, excluding the FCS itself and the start/stop flag. the FCS is done
 * before escaping the start/stop and escape-flags.
 */

#if 0
size_t ndlcomEncode(void *pOutputBuffer, const size_t outputBufferSize,
                    const struct NDLComHeader *pHeader, const void *pData) {
    const uint8_t *pRead;
    const uint8_t *pHeaderEnd;
    const uint8_t *pDataEnd;
    const uint8_t *pCrcEnd;
    uint8_t *pWritePos = (uint8_t *)pOutputBuffer;
    NDLComCrc crc = NDLCOM_CRC_INITIAL_VALUE;

    /**
     * this calulation in done for the worst-case in "ndlcom/Types.h", but
     * calculating the needed buffer size here, based on the given mDataLen
     * allows smaller buffers for smaller packets.
     *
     * return "0" if it fails, noting "we did not do anything".
     */
    if (outputBufferSize < (2 +
                            2 * (sizeof(struct NDLComHeader) +
                                 pHeader->mDataLen + sizeof(NDLComCrc)))) {
        return 0;
    }

    /* start byte */
    *pWritePos++ = NDLCOM_START_STOP_FLAG;

    /* header */
    pRead = (const uint8_t *)pHeader;
    pHeaderEnd = pRead + sizeof(struct NDLComHeader);
    while (pRead != pHeaderEnd) {
        const uint8_t d = *pRead;

        /* NOTE: calculating the checksum prior to escaping */
        crc = ndlcomDoCrc(crc, &d);

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG) {
            /* we need to send an escaped byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        } else {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    /* no further checks if pWritePos is within bound, since we enforced a
     * maximum buffer-size before */

    /* data */
    pRead = (const uint8_t *)pData;
    pDataEnd = pRead + pHeader->mDataLen;
    while (pRead != pDataEnd) {
        const uint8_t d = *pRead;

        /* NOTE: calculating the checksum prior to escaping */
        crc = ndlcomDoCrc(crc, &d);

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG) {
            /* we need to send an escaped data byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        } else {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    /* Some algorithms want to complement the CRC after creation. Failing to do
     * so will "break" the recognition of valid CRC later by comparing it with
     * NDLCOM_FCS_GOOD_VALUE (0xf0b8). The CRC will, after going through the
     * complete parser, be zero instead, so the "GOOD_VALUE" is just different.
     */
    /*crc ^= 0xffff;*/

    /* checksum */
    pRead = (const uint8_t *)&crc;
    pCrcEnd = pRead + sizeof(NDLComCrc);
    while (pRead != pCrcEnd) {
        const uint8_t d = *pRead;

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG) {
            /* we need to send an escaped data byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        } else {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    /* flag at end of packet */
    *pWritePos++ = NDLCOM_START_STOP_FLAG;

    return pWritePos - (uint8_t *)pOutputBuffer;
}
#endif

/* internal tooling function to start a new to-be-encoded message */
size_t ndlcomEncodeInit(void *outputBuffer, const size_t outputBufferSize,
                        NDLComCrc *crc) {
    /* initialize the crc */
    *crc = NDLCOM_CRC_INITIAL_VALUE;

    /** check that nobody is trying to fool us with a very small buffer */
    if (outputBufferSize < 1) {
        return 0;
    }

    /* write start flag */
    uint8_t *pWritePos = (uint8_t *)outputBuffer;
    *pWritePos = NDLCOM_START_STOP_FLAG;

    return 1;
}

/* internal tooling function to add a chunk of memory to a half-encoded message */
size_t ndlcomEncodeAppendPayload(void *outputBuffer,
                                 const size_t outputBufferSize,
                                 const void *dataBuffer, size_t dataBufferSize,
                                 NDLComCrc *crc) {
    /* This is our reading-target pointer. It will be moved along for each
     * single byte read from the dataBuffer */
    const uint8_t *pRead = (const uint8_t *)dataBuffer;
    /* the end-mark of the dataBuffer. we read until here */
    const uint8_t *pDataEnd = pRead + dataBufferSize;
    /* the writing-target pointer. It will be moved along for each byte written
     * to the outputBuffer */
    uint8_t *pWritePos = (uint8_t *)outputBuffer;

    /* check if there is enough worst-case space in the output buffer */
    if (outputBufferSize < (2 * dataBufferSize)) {
        return 0;
    }

    /* data processing */
    while (pRead != pDataEnd) {
        const uint8_t d = *pRead;

        /**
         * calculating the checksum prior to escaping. this function can ignore
         * to update the checksum if we did not get an actual pointer.
         *
         * NOTE: very small micro-optimization... note sure if this is
         * worthwhile... no effect observable in the "encoderDecoder_pingPing"
         */
        if (likely((ptrdiff_t)crc)) {
            *crc = ndlcomDoCrc(*crc, &d);
        }

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG) {
            /* we need to send an escaped data byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        } else {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    return pWritePos - (uint8_t *)outputBuffer;
}

/* internal tooling function to finish-up a nearly-encoded message */
size_t ndlcomEncodeFinalize(void *outputBuffer, const size_t outputBufferSize) {

    /** check that nobody is trying to fool us with a very small buffer */
    if (outputBufferSize < 1) {
        return 0;
    }

    /* flag at end of packet. this is not strictly needed, but makes some things
     * easier */
    uint8_t *pWritePos = (uint8_t *)outputBuffer;
    *pWritePos = NDLCOM_START_STOP_FLAG;

    return 1;
}

size_t ndlcomEncode(void *outputBuffer, const size_t outputBufferSize,
                    const struct NDLComHeader *header, const void *data) {

    NDLComCrc crc;
    size_t wrote = 0;

    /* prepare the packet */
    wrote += ndlcomEncodeInit((uint8_t *)outputBuffer + wrote,
                              outputBufferSize - wrote, &crc);

    /* append the header itself */
    wrote += ndlcomEncodeAppendPayload((uint8_t *)outputBuffer + wrote,
                                       outputBufferSize - wrote, header,
                                       sizeof(struct NDLComHeader), &crc);

    wrote += ndlcomEncodeAppendPayload((uint8_t *)outputBuffer + wrote,
                                       outputBufferSize - wrote, data,
                                       header->mDataLen, &crc);

    /**
     * The actual content of the message is encoded at this stage, the crc has
     * its final value. It now has to be escaped and added to the output buffer
     * as well.
     *
     * NOTE: Some algorithms want to complement the CRC after creation. Failing
     * to do so will "break" the recognition of valid CRC later by comparing it
     * with NDLCOM_FCS_GOOD_VALUE (0xf0b8). The CRC will, after going through
     * the complete parser, be zero instead, so the "GOOD_VALUE" is just
     * different.
     */
    /*crc ^= 0xffff;*/
    wrote += ndlcomEncodeAppendPayload((uint8_t *)outputBuffer + wrote,
                                       outputBufferSize - wrote, &crc,
                                       sizeof(NDLComCrc), NULL);

    wrote += ndlcomEncodeFinalize((uint8_t *)outputBuffer + wrote,
                                  outputBufferSize - wrote);

    /* and finally report what we did */
    return wrote;
}
