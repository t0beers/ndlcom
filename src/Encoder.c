/**
 * @file src/Encoder.c
 * @date 2011
 */

#include "ndlcom/Protocol.h"

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


int16_t ndlcomEncode(void* pOutputBuffer,
                     uint16_t outputBufferSize,
                     const NDLComHeader* pHeader,
                     const void* pData)
{
    uint8_t headerRaw[NDLCOM_HEADERLEN];
    const uint8_t* pRead;
    const uint8_t* pHeaderEnd;
    const uint8_t* pDataEnd;
    const uint8_t* pCrcEnd;
    uint8_t* pWritePos = (uint8_t*)pOutputBuffer;
    NDLComCrc crc = NDLCOM_CRC_INITIAL_VALUE;

    /* We need at least: 2bytes for start/stop-flags, the header itself, the
     * actual data with an decoded maximum of 255bytes and the CRC. Since
     * each byte (except the start/stop flags) _could_ be escaped in theory, we
     * need doubled number of bytes...
     *
     * Calculating the needed buffer size based on the given mDataLen allows
     * smaller buffers for smaller packets */
    if (outputBufferSize < 2+2*(sizeof(NDLComHeader) + pHeader->mDataLen + sizeof(NDLComCrc)))
    {
        return -1;
    }

    headerRaw[0] = pHeader->mReceiverId;
    headerRaw[1] = pHeader->mSenderId;
    headerRaw[2] = pHeader->mCounter;
    headerRaw[3] = pHeader->mDataLen;

    /* start byte */
    *pWritePos++ = NDLCOM_START_STOP_FLAG;

    /* header */
    pRead = (const uint8_t*)headerRaw;
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

