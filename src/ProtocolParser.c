/**
 * @file src/ProtocolParser.c
 * @date 2011
 */
#include "NDLCom/Protocol.h"
#include <stdio.h>

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 */

/**
 * @defgroup Communication_NDLCom_C_Parser NDLCom C Parser
 *
 * Used to extract protocol-pakets out of a continously flowing raw data-stream,
 * which may have been received by your serial connection
 *
 * @{
 */

/**
 * @brief State for protocolParser functions.
 *
 * Since a packet may be distributed over many sequential calls of
 * protocolParserReceive a buffer and some state variables are
 * required to reconstruct the packet information.
 * Since only a pointer to this struct is used in the functions listed
 * in include/NDLCom/Protocol.h the struct definition is not needed to use this
 * library.
 */
struct ProtocolParser
{
    /** Last completely received header.
     * After receiving all header bytes and converting the byte order the
     * header data is stored here. \see protocolParserGetHeader
     */
    struct ProtocolHeader mHeader;
     /** temporary storage for raw header data (without start bytes) */
    uint8_t mHeaderRaw[PROTOCOL_HEADERLEN];
    uint8_t* mHeaderRawWritePos; /**< Current write position while receiving header data. */
    uint8_t* mpData; /**< Pointer where the packet content should be written. */
    uint8_t* mpDataWritePos; /**< Current write position of next data byte while receiving user data. */
    uint8_t  mDataCRC; /**< Checksum of data (header + packet content) while receiving. */
    uint16_t mDataBufSize; /**< Size of buffer (\see mpData) */
    /** different states the parser may have. */
    enum State
    {
        mcERROR=0,
        mcWAIT_STARTFLAG,
        mcWAIT_HEADER,
        mcWAIT_DATA,
        mcWAIT_CHECKSUM,
        mcCOMPLETE,
    } mState;
    int8_t mLastWasESC;/**< stores if the last received byte was a crc. used to detect escaped bytes */
    int16_t mNumberOfCRCFails;/**< how often a bad crc was received */
    /** why the parser went into error-state */
    enum ErrorReason
    {
        mcNOERROR=0,
        mcBAD_CRC,
        mcDESTROYED,
    } mLastError;
    int mFlags;
};

const char* protocolParserStateName[] = {
    "ERROR",
    "WAIT_STARTFLAG",
    "WAIT_HEADER",
    "WAIT_DATA",
    "WAIT_CHECKSUM",
    "COMPLETE",
    0
};

const char* protocolParserErrorString[] = {
    "NOERROR", "BAD_CRC", "DESTROYED", 0
};

struct ProtocolParser* protocolParserCreate(void* pBuffer, uint16_t dataBufSize)
{
    if (!pBuffer || dataBufSize <= sizeof(struct ProtocolParser))
    {
        return 0;
    }

    struct ProtocolParser* parser = (struct ProtocolParser*)pBuffer;
    parser->mpData = pBuffer + sizeof(struct ProtocolParser);
    parser->mDataBufSize = dataBufSize - sizeof(struct ProtocolParser);
    parser->mState = mcWAIT_STARTFLAG;
    parser->mLastError = mcNOERROR;
    parser->mDataCRC = 0;
    parser->mLastWasESC = 0;
    parser->mNumberOfCRCFails = 0;
    parser->mHeaderRawWritePos = (uint8_t*)&parser->mHeaderRaw;
    parser->mFlags = 0;
    return parser;
}

void protocolParserSetFlag(struct ProtocolParser* parser, int flag)
{
  parser->mFlags |= flag;
}

void protocolParserClearFlag(struct ProtocolParser* parser, int flag)
{
  parser->mFlags &= ~flag;
}

void protocolParserDestroy(struct ProtocolParser* parser)
{
    //may be used later
    parser->mState = mcERROR;
}

int16_t protocolParserReceive(
    struct ProtocolParser* parser,
    const void* newData,
    uint16_t newDataLen)
{
    int16_t dataRead = 0;
    const uint8_t* in = (uint8_t*)newData;

    if (parser->mFlags & PROTOCOL_PARSER_DISABLE_FRAMING)
    {
      //start of a frame without flags in each function call.
      parser->mLastError = mcNOERROR;
      parser->mDataCRC = 0;
      parser->mLastWasESC = 0;
      parser->mNumberOfCRCFails = 0;
      parser->mState = mcWAIT_HEADER;
      parser->mHeaderRawWritePos = (uint8_t*)&parser->mHeaderRaw;
    }

    while (newDataLen--)
    {
        uint8_t c = *in;
        in++;
        dataRead++;

        //handle char after ESC
        if(parser->mLastWasESC)
        {
            parser->mLastWasESC = 0;

            //a packet was aborted (See RFC1549, Sec. 4):
            if (c == PROTOCOL_FLAG)
            {
                protocolParserDestroyPacket(parser);
                continue;
            }
            else
            {
                //handle as normal data below, but with
                //complemented bit 6:
                c ^= 0x20;
            }
        }
        //handle an escape byte
        else if (c == PROTOCOL_ESC && !(parser->mFlags & PROTOCOL_PARSER_DISABLE_FRAMING))
        {
            //do nothing now. wait for next byte
            //to decide action
            parser->mLastWasESC = 1;
            continue;
        }
        //handle a protocol flag
        else if (c == PROTOCOL_FLAG && !(parser->mFlags & PROTOCOL_PARSER_DISABLE_FRAMING))
        {
            //an unescaped FLAG is interpreted as a packet start
            protocolParserDestroyPacket(parser);
            continue;
        }

        switch (parser->mState)
        {
            case mcWAIT_HEADER:
                *(parser->mHeaderRawWritePos++) = c;
                parser->mDataCRC ^= c;
                if (parser->mHeaderRawWritePos - parser->mHeaderRaw == PROTOCOL_HEADERLEN)
                {
                    parser->mHeader.mReceiverId = parser->mHeaderRaw[0];
                    parser->mHeader.mSenderId   = parser->mHeaderRaw[1];
                    parser->mHeader.mCounter    = parser->mHeaderRaw[2];
                    parser->mHeader.mDataLen    = parser->mHeaderRaw[3];
                    parser->mpDataWritePos      = parser->mpData;
                    /* check if there is actual data to come... */
                    if (parser->mHeader.mDataLen)
                    {
                        parser->mState = mcWAIT_DATA;
                    }
                    /* ...else we have a degenerate packet, proceed directly */
                    else
                    {
                        parser->mState = mcWAIT_CHECKSUM;
                    }
                }
                break;
            case mcWAIT_DATA:
                *(parser->mpDataWritePos++) = c;
                parser->mDataCRC ^= c;
                // check that our raw-buffer is still in bounds
                if (parser->mpDataWritePos == parser->mpData + parser->mDataBufSize)
                {
                    //TODO armin: nice error handling in switch-case
                    parser->mState = mcERROR;
                    return dataRead;
                }
                // check if we read "enough" data -- as was advertised in the header
                if (parser->mpDataWritePos == parser->mpData + parser->mHeader.mDataLen)
                {
                    if (parser->mFlags & PROTOCOL_PARSER_DISABLE_FRAMING)
                    {
                        parser->mState = mcCOMPLETE;
                    }
                    else
                    {
                        parser->mState = mcWAIT_CHECKSUM;
                    }
                }
                break;
            case mcWAIT_CHECKSUM:
                if (c == parser->mDataCRC)
                {
                    parser->mState = mcCOMPLETE;
                }
                else
                {
                    parser->mNumberOfCRCFails++;
                    parser->mState = mcWAIT_STARTFLAG;
                }
                break;
            case mcCOMPLETE:
                /* we reach here if we are "mcCOMPLETE" before any byte was
                 * parsed, since there is an "if" for the same state after the
                 * switch-case.
                 * HINT: you have to manually call
                 * protocolParserDestroyPacket() after reading data to reset
                 * the state-machine */
                return 0;
                break;
            case mcERROR:
                protocolParserDestroyPacket(parser);
                break;
            case mcWAIT_STARTFLAG:
                break;
                ;
                //TODO
        } //of switch

        //abort processing if a complete packet was received
        if (parser->mState == mcCOMPLETE)
        {
            return dataRead;
        }
    }

    return dataRead;
}


char protocolParserHasPacket(struct ProtocolParser* parser)
{
    return parser->mState == mcCOMPLETE;
}

const struct ProtocolHeader* protocolParserGetHeader(struct ProtocolParser* parser)
{
    return &parser->mHeader;
}

const void* protocolParserGetPacket(struct ProtocolParser* parser)
{
    return parser->mState == mcCOMPLETE ? parser->mpData : 0;
}

void protocolParserDestroyPacket(struct ProtocolParser* parser)
{
    parser->mState = mcWAIT_HEADER;
    parser->mDataCRC = 0;
    parser->mHeaderRawWritePos = (uint8_t*)&parser->mHeaderRaw;
    parser->mLastWasESC = 0;
}

void protocolParserGetState(struct ProtocolParser* parser,
                            struct ProtocolParserState* output)
{
    output->mState = parser->mState;
    output->mNumberOfEscapes = parser->mLastWasESC;
    output->mNumberOfCRCFails = parser->mNumberOfCRCFails;
    output->mLastError = parser->mLastError;
}

/**
 * @}
 */

/**
 * @}
 * @}
 */
