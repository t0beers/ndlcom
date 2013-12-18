/**
 * @file src/Encoder.c
 * @date 2011
 */

#include "ndlcom_core/Protocol.h"

/**
 * @brief Used to pack some payload into a distinct data-format, which may be sent over
 * your serial connection.
 *
 * Encoding sheme based on RFC1662, see https://tools.ietf.org/html/rfc1662.html
 *
 * FCS - frame check sequence (aka crc) is calculated over all bytes of the
 * packet, excluding the FCS itself and the start/stop flag. the FCS is done
 * before exscaping the start/stop and escape-flags.
 */


int16_t ndlcomEncode(void* pOutputBuffer,
                     uint16_t outputBufferSize,
                     const NDLComHeader* pHeader,
                     const void* pData)
{
    uint8_t headerRaw[NDLCOM_HEADERLEN];
    uint8_t* pWritePos = (uint8_t*)pOutputBuffer;
    NDLComCrc crc = NDLCOM_CRC_INITIAL_VALUE;

    /* we need at least: 2bytes for start/stop-flags, the header itself, the
     * actual data with an unencoded maximum of 255bytes and the crc. since
     * each byte (except the start/stop flags) _could_ be escaped in theory, we
     * need doubled number of bytes... */
    if (outputBufferSize < 2+2*(sizeof(NDLComHeader) + pHeader->mDataLen + sizeof(NDLComCrc)))
    {
        return -1;
    }

    headerRaw[0] = pHeader->mReceiverId;
    headerRaw[1] = pHeader->mSenderId;
    headerRaw[2] = pHeader->mCounter;
    headerRaw[3] = pHeader->mDataLen;

    //start byte
    *pWritePos++ = NDLCOM_START_STOP_FLAG;
    const uint8_t* pRead;

    //header
    pRead = (const uint8_t*)headerRaw;
    const uint8_t* pHeaderEnd = pRead + NDLCOM_HEADERLEN;
    while (pRead != pHeaderEnd)
    {
        const uint8_t d = *pRead;

        crc = ndlcomDoCrc(crc, &d);

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG)
        {
            //we need to send an escaped byte here:
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    //no further checks if pWritePos is within bound, since we enforced a
    //maximum buffer-size before

    //data
    pRead = (const uint8_t*)pData;
    const uint8_t* pDataEnd = pRead + pHeader->mDataLen;
    while (pRead != pDataEnd)
    {
        const uint8_t d = *pRead;

        crc = ndlcomDoCrc(crc, &d);

        if (d == NDLCOM_ESC_CHAR || d == NDLCOM_START_STOP_FLAG)
        {
            //we need to send an escaped data byte here:
            *pWritePos++ = NDLCOM_ESC_CHAR;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
            *pWritePos++ = d;
        }
        ++pRead;
    }

    //crc
    if (crc == NDLCOM_ESC_CHAR || crc == NDLCOM_START_STOP_FLAG)
    {
        *pWritePos++ = NDLCOM_ESC_CHAR;
        *pWritePos++ = 0x20 ^ crc;
    }
    else
    {
        *pWritePos++ = crc;
    }

    //flag at end of packet
    *pWritePos++ = NDLCOM_START_STOP_FLAG;

    return pWritePos - (uint8_t*)pOutputBuffer;
}
