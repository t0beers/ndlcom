/**
 * @file src/ProtocolEncode.c
 * @date 2011
 */

#include "NDLCom/Protocol.h"

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 */

/**
 * @defgroup Communication_NDLCom_Encoder Packet Encoder
 *
 * @brief Used to pack some payload into a distinct data-format, which may be sent over
 * your serial connection.
 *
 * Encoding sheme based on RFC1662, see https://tools.ietf.org/html/rfc1662.html
 *
 * FCS - frame check sequence (aka crc) is calculated over all bytes of the
 * packet, excluding the FCS itself and the start/stop flag. the FCS is done
 * before exscaping the start/stop and escape-flags.
 *
 * @{
 */


int16_t protocolEncode(void* pOutputBuffer,
                   uint16_t outputBufferSize,
                   const struct ProtocolHeader* pHeader,
                   const void* pData)
{
    uint8_t headerRaw[PROTOCOL_HEADERLEN];
    uint8_t* pWritePos = (uint8_t*)pOutputBuffer;
    const uint8_t* pWriteEnd = pWritePos + outputBufferSize;
    ndlcomCrc crc = 0;

    if (outputBufferSize < 2 + sizeof(struct ProtocolHeader) + pHeader->mDataLen + 1)
    {
        return -1;
    }

    headerRaw[0] = pHeader->mReceiverId;
    headerRaw[1] = pHeader->mSenderId;
    headerRaw[2] = pHeader->mCounter;
    headerRaw[3] = pHeader->mDataLen;

    //start byte
    *pWritePos++ = PROTOCOL_FLAG;
    const uint8_t* pRead;

    //header
    pRead = (const uint8_t*)headerRaw;
    const uint8_t* pHeaderEnd = pRead + PROTOCOL_HEADERLEN;
    while (pRead != pHeaderEnd)
    {
        const uint8_t d = *pRead;
        if (d == PROTOCOL_ESC || d == PROTOCOL_FLAG)
        {
            //we need to send an escaped byte here:
            *pWritePos++ = PROTOCOL_ESC;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
            *pWritePos++ = d;
        }
        crc ^= d;
        ++pRead;
    }

    //data
    pRead = (const uint8_t*)pData;
    const uint8_t* pDataEnd = pRead + pHeader->mDataLen;
    while (pRead != pDataEnd)
    {
        const uint8_t d = *pRead;
        if (pWritePos == pWriteEnd - (sizeof(ndlcomCrc)+1))
        {
            return -1;
        }

        if (d == PROTOCOL_ESC || d == PROTOCOL_FLAG)
        {
            //we need to send an escaped data byte here:
            *pWritePos++ = PROTOCOL_ESC;
            *pWritePos++ = 0x20 ^ d;
        }
        else
        {
            *pWritePos++ = d;
        }
        crc ^= d;
        ++pRead;
    }

    //crc
    if (crc == PROTOCOL_ESC || crc == PROTOCOL_FLAG)
    {
        *pWritePos++ = PROTOCOL_ESC;
        *pWritePos++ = 0x20 ^ crc;
    }
    else
    {
        *pWritePos++ = crc;
    }

    //flag at end of packet
    *pWritePos++ = PROTOCOL_FLAG;

    return pWritePos - (uint8_t*)pOutputBuffer;
}



int16_t protocolEncodeForUDP(void* pOutputBuffer,
                   uint16_t outputBufferSize,
                   const struct ProtocolHeader* pHeader,
                   const void* pData)
{
    uint8_t* pWritePos = (uint8_t*)pOutputBuffer;

    if (outputBufferSize < sizeof(struct ProtocolHeader) + pHeader->mDataLen)
    {
        return -1;
    }

    *pWritePos++ = pHeader->mReceiverId;
    *pWritePos++ = pHeader->mSenderId;
    *pWritePos++ = pHeader->mCounter;
    *pWritePos++ = pHeader->mDataLen;

    //data
    const uint8_t* pRead = (const uint8_t*)pData;
    const uint8_t* pDataEnd = pRead + pHeader->mDataLen;
    while (pRead != pDataEnd)
    {
        *pWritePos++ = *pRead++;
    }

    return pWritePos - (uint8_t*)pOutputBuffer;
}

/**
 * @}
 */

/**
 * @}
 * @}
 */
