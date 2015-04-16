/**
 * @file src/Parser.c
 * @date 2011
 */
#include "ndlcom/Protocol.h"
#include "ndlcom/Crc.h"
#include <stdio.h>

/**
 * @defgroup Communication_NDLCom_C_Parser NDLCom C Parser
 *
 * Used to extract protocol-pakets out of a continously flowing raw data-stream,
 * which may have been received by your serial connection
 *
 * @{
 */

/**
 * @brief State for ndlcomParser functions.
 *
 * Since a packet may be distributed over many sequential calls of
 * ndlcomParserReceive a buffer and some state variables are
 * required to reconstruct the packet information.
 * Since only a pointer to this struct is used in the functions listed
 * in include/NDLCom/Protocol.h the struct definition is not needed to use this
 * library.
 */
struct NDLComParser
{
    /** Last completely received header.
     * After receiving all header bytes and converting the byte order the
     * header data is stored here. \see ndlcomParserGetHeader
     */
    NDLComHeader mHeader;
     /** temporary storage for raw header data (without start bytes) */
    uint8_t mHeaderRaw[NDLCOM_HEADERLEN];
    uint8_t* mHeaderRawWritePos; /**< Current write position while receiving header data. */
    uint8_t* mpData; /**< Pointer where the packet content should be written. */
    uint8_t* mpDataWritePos; /**< Current write position of next data byte while receiving user data. */
    NDLComCrc mDataCRC; /**< Checksum of data (header + packet content) while receiving. */
    uint16_t mDataBufSize; /**< Size of buffer (\see mpData) */
    /** different states the parser may have. */
    enum State
    {
        mcERROR=0,
        mcWAIT_HEADER,
        mcWAIT_DATA,
        mcWAIT_FIRST_CRC_BYTE,
        mcWAIT_SECOND_CRC_BYTE,
        mcCOMPLETE,
    } mState;
    int8_t mLastWasESC;/**< stores if the last received byte was a crc. used to detect escaped bytes */
    uint32_t mNumberOfCRCFails;/**< how often a bad crc was received */
    int mFlags;
};

const char* ndlcomParserStateName[] = {
    "ERROR",
    "WAIT_HEADER",
    "WAIT_DATA",
    "WAIT_FIRST_CRC_BYTE",
    "WAIT_SECOND_CRC_BYTE",
    "COMPLETE",
    0
};


/** i am not that sure what to put here... the struct has to save additional
 * 255bytes, after de-escaing, nothing more?
 */
#define NDLCOM_PARSER_MIN_BUFFER_SIZE (sizeof(struct NDLComParser)+NDLCOM_MAX_PAYLOAD_SIZE)

struct NDLComParser* ndlcomParserCreate(void* pBuffer, uint16_t dataBufSize)
{
    // we enforce a correct length: having less memory will lead to
    // buffer-overflows on big packets.
    if (!pBuffer || dataBufSize <= NDLCOM_PARSER_MIN_BUFFER_SIZE)
    {
        return 0;
    }

    struct NDLComParser* parser = (struct NDLComParser*)pBuffer;
    parser->mpData = pBuffer + sizeof(struct NDLComParser);
    parser->mDataBufSize = dataBufSize - sizeof(struct NDLComParser);
    parser->mState = mcWAIT_HEADER;
    parser->mDataCRC = NDLCOM_CRC_INITIAL_VALUE;
    parser->mLastWasESC = 0;
    parser->mNumberOfCRCFails = 0;
    parser->mHeaderRawWritePos = parser->mHeaderRaw;
    parser->mFlags = 0;
    return parser;
}

void ndlcomParserSetFlag(struct NDLComParser* parser, int flag)
{
  parser->mFlags |= flag;
}

void ndlcomParserClearFlag(struct NDLComParser* parser, int flag)
{
  parser->mFlags &= ~flag;
}

void ndlcomParserDestroy(struct NDLComParser* parser)
{
    //may be used later
    parser->mState = mcERROR;
}

int16_t ndlcomParserReceive(
    struct NDLComParser* parser,
    const void* newData,
    uint16_t newDataLen)
{
    int16_t dataRead = 0;
    const uint8_t* in = (uint8_t*)newData;

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
            if (c == NDLCOM_START_STOP_FLAG)
            {
                ndlcomParserDestroyPacket(parser);
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
        else if (c == NDLCOM_ESC_CHAR)
        {
            //do nothing now. wait for next byte
            //to decide action
            parser->mLastWasESC = 1;
            continue;
        }
        //handle a protocol flag
        else if (c == NDLCOM_START_STOP_FLAG)
        {
            // NDLCOM_START_STOP_FLAG is a packet start. we'll always perform a
            // full state-reset and wait for the next byte (eg: continue) upon
            // receiving one.
            ndlcomParserDestroyPacket(parser);
            continue;
        }

        switch (parser->mState)
        {
            case mcWAIT_HEADER:
                *(parser->mHeaderRawWritePos++) = c;
                parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);
                if (parser->mHeaderRawWritePos - parser->mHeaderRaw == NDLCOM_HEADERLEN)
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
                    /* ...else we have a degenerate packet with no payload, proceed directly */
                    else
                    {
                        parser->mState = mcWAIT_FIRST_CRC_BYTE;
                    }
                }
                break;
            case mcWAIT_DATA:
                *(parser->mpDataWritePos++) = c;
                parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);

                // no out-of-bound check is performed. since we guarded the
                // buffer-size in ndlcomParserCreate to be big anough, this
                // will hopefully never fail...

                // check if we read "enough" data -- as was advertised in the header
                if (parser->mpDataWritePos == parser->mpData + parser->mHeader.mDataLen)
                {
                    parser->mState = mcWAIT_FIRST_CRC_BYTE;
                }
                break;
            // the crc arrives in two seperate bytes in the crc16 case. handling
            // them one after the other
#ifndef NDLCOM_CRC16
            case mcWAIT_FIRST_CRC_BYTE:
            case mcWAIT_SECOND_CRC_BYTE:
                if (c == parser->mDataCRC)
                {
                    parser->mState = mcCOMPLETE;
                }
                else
                {
                    parser->mNumberOfCRCFails++;
                    ndlcomParserDestroyPacket(parser);
                }
                break;
#else
            case mcWAIT_FIRST_CRC_BYTE:
                parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);
                parser->mState = mcWAIT_SECOND_CRC_BYTE;
                break;
            // only after the second one was received and stuffed into the
            // crc-chain, we can decide wether we got something good.
            case mcWAIT_SECOND_CRC_BYTE:
                parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);
                if (parser->mDataCRC == NDLCOM_CRC_REAL_GOOD_VALUE)
                {
                    parser->mState = mcCOMPLETE;
                }
                else
                {
                    parser->mNumberOfCRCFails++;
                    ndlcomParserDestroyPacket(parser);
                }
                break;
#endif
            case mcCOMPLETE:
                /* we reach here if we are "mcCOMPLETE" before any byte was
                 * parsed, since there is an "if" for the same state after the
                 * switch-case.
                 * HINT: you have to manually call
                 * ndlcomParserDestroyPacket() after reading data to reset
                 * the state-machine */
                return 0;
                break;
            case mcERROR:
                ndlcomParserDestroyPacket(parser);
                break;
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


char ndlcomParserHasPacket(struct NDLComParser* parser)
{
    return parser->mState == mcCOMPLETE;
}

const NDLComHeader* ndlcomParserGetHeader(struct NDLComParser* parser)
{
    return &parser->mHeader;
}

const void* ndlcomParserGetPacket(struct NDLComParser* parser)
{
    return parser->mState == mcCOMPLETE ? parser->mpData : 0;
}

void ndlcomParserDestroyPacket(struct NDLComParser* parser)
{
    parser->mState = mcWAIT_HEADER;
    parser->mDataCRC = NDLCOM_CRC_INITIAL_VALUE;
    parser->mHeaderRawWritePos = parser->mHeaderRaw;
    parser->mLastWasESC = 0;
}

void ndlcomParserGetState(struct NDLComParser* parser,
                          struct NDLComParserState* output)
{
    output->mState = parser->mState;
    output->mNumberOfCRCFails = parser->mNumberOfCRCFails;
}

uint32_t ndlcomParserGetNumberOfCRCFails(struct NDLComParser* parser) {
    return parser->mNumberOfCRCFails;
}

void ndlcomParserResetNumberOfCRCFails(struct NDLComParser* parser) {
    parser->mNumberOfCRCFails = 0;
}