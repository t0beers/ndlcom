/**
 * @file src/Encoder.c
 * @date 2011
 */

#include "ndlcom/Encoder.h"

/**
 * @brief Used to pack some payload into a distinct data-format, which may be sent over
 * your serial connection.
 *
 * Encoding scheme based on RFC1662, see https://tools.ietf.org/html/rfc1662.html
 *
 * FCS - frame check sequence (aka crc) is calculated over all bytes of the
 * packet, excluding the FCS itself and the start/stop flag. the FCS is done
 * before escaping the start/stop and escape-flags.
 */

size_t ndlcomEncode(void *pOutputBuffer, const size_t outputBufferSize,
                    const NDLComHeader *pHeader, const void *pData) {
    const uint8_t* pRead;
    const uint8_t* pHeaderEnd;
    const uint8_t* pDataEnd;
    const uint8_t* pCrcEnd;
    uint8_t* pWritePos = (uint8_t*)pOutputBuffer;
    NDLComCrc crc = NDLCOM_CRC_INITIAL_VALUE;

    /**
     * this calulation in done for the worst-case in "ndlcom/Types.h", but
     * calculating the needed buffer size here, based on the given mDataLen
     * allows smaller buffers for smaller packets.
     *
     * return "0" if it fails, noting "we did not do anything".
     */
    if (outputBufferSize <
        (2 + 2 * (sizeof(NDLComHeader) + pHeader->mDataLen + sizeof(NDLComCrc)))) {
        return 0;
    }

    /* start byte */
    *pWritePos++ = NDLCOM_START_STOP_FLAG;

    /* header */
    pRead = (const uint8_t*)pHeader;
    pHeaderEnd = pRead + sizeof(NDLComHeader);
    while (pRead != pHeaderEnd)
    {
        const uint8_t d = *pRead;

        /* NOTE: calculating the checksum prior to escaping */
        crc = ndlcomDoCrc(crc, &d);

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG)
        {
            /* we need to send an escaped byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    /* no further checks if pWritePos is within bound, since we enforced a
     * maximum buffer-size before */

    /* data */
    pRead = (const uint8_t*)pData;
    pDataEnd = pRead + pHeader->mDataLen;
    while (pRead != pDataEnd)
    {
        const uint8_t d = *pRead;

        /* NOTE: calculating the checksum prior to escaping */
        crc = ndlcomDoCrc(crc, &d);

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG)
        {
            /* we need to send an escaped data byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
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
    pRead = (const uint8_t*)&crc;
    pCrcEnd = pRead + sizeof(NDLComCrc);
    while (pRead != pCrcEnd)
    {
        const uint8_t d = *pRead;

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG)
        {
            /* we need to send an escaped data byte here: */
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    /* flag at end of packet */
    *pWritePos++ = NDLCOM_START_STOP_FLAG;

    return pWritePos - (uint8_t*)pOutputBuffer;
}

