/**
 * @file src/Parser.c
 * @date 2011
 */
#include "ndlcom/Parser.h"
#include "ndlcom/Crc.h"

/**
 * @brief Used to extract protocol-packets out of a continuously flowing raw
 * data-stream, which may have been received by your serial connection
 */

const char *ndlcomParserStateName[] = {"ERROR",
                                       "WAIT_HEADER",
                                       "WAIT_DATA",
                                       "WAIT_FIRST_CRC_BYTE",
                                       "WAIT_SECOND_CRC_BYTE",
                                       "COMPLETE",
                                       0};

/** the size needed by the "struct NDLComParser" */
#define NDLCOM_PARSER_MIN_BUFFER_SIZE (sizeof(struct NDLComParser))

struct NDLComParser *ndlcomParserCreate(void *pBuffer, size_t dataBufSize) {
    struct NDLComParser *parser = (struct NDLComParser *)pBuffer;

    /* we enforce a correct length: having less memory will lead to
     * buffer-overflows on big packets. */
    if (!parser || dataBufSize < NDLCOM_PARSER_MIN_BUFFER_SIZE) {
        return 0;
    }

    /* call all necessary initialization functions */
    ndlcomParserDestroyPacket(parser);
    ndlcomParserResetNumberOfCRCFails(parser);

    return parser;
}

void ndlcomParserDestroy(struct NDLComParser *parser) {
    /* may be used later */
    parser->mState = mcERROR;
}

int16_t ndlcomParserReceive(struct NDLComParser *parser, const void *newData,
                            uint16_t newDataLen) {
    int16_t dataRead = 0;
    const uint8_t *in = (uint8_t *)newData;

    while (newDataLen--) {
        uint8_t c = *in;
        in++;
        dataRead++;

        /* abort a packet _always_ after reading a START_STOP_FLAG. (See
         * RFC1549, Sec. 4): */
        if (c == NDLCOM_START_STOP_FLAG) {
            ndlcomParserDestroyPacket(parser);
            continue;
        }

        /* handle a byte after NDLCOM_ESC_CHAR */
        if (parser->mLastWasESC) {
            parser->mLastWasESC = 0;

            /* handle as normal data below, but with complemented bit 6: */
            c ^= 0x20;
        }
        /* handle an escape byte */
        else if (c == NDLCOM_ESC_CHAR) {
            /* do nothing now. Wait for next byte to decide action */
            parser->mLastWasESC = 1;
            continue;
        }

        switch (parser->mState) {
        case mcWAIT_HEADER:
            *(parser->mpHeaderWritePos++) = c;
            parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);
            if (parser->mpHeaderWritePos - parser->mHeader.raw ==
                NDLCOM_HEADERLEN) {
                /* check if there is actual data to come... */
                if (parser->mHeader.hdr.mDataLen) {
                    parser->mState = mcWAIT_DATA;
                }
                /* ...else we have a degenerate packet with no payload, proceed
                 * directly */
                else {
                    parser->mState = mcWAIT_FIRST_CRC_BYTE;
                }
            }
            break;
        case mcWAIT_DATA:
            *(parser->mpDataWritePos++) = c;
            parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);

            /* no out-of-bound check is performed. Since we guarded the
             * buffer-size in ndlcomParserCreate() to be big enough, this
             * will hopefully never fail... */

            /* did we read "enough" data -- as was advertised in the header? */
            if (parser->mpDataWritePos ==
                parser->mpData + parser->mHeader.hdr.mDataLen) {
                parser->mState = mcWAIT_FIRST_CRC_BYTE;
            }
            break;
/* the CRC arrives in two separate bytes in the 16bit FCS case.  handling them
 * one after the other */
#ifndef NDLCOM_CRC16
        case mcWAIT_FIRST_CRC_BYTE:
        case mcWAIT_SECOND_CRC_BYTE:
            if (c == parser->mDataCRC) {
                parser->mState = mcCOMPLETE;
            } else {
                parser->mNumberOfCRCFails++;
                ndlcomParserDestroyPacket(parser);
            }
            break;
#else
        case mcWAIT_FIRST_CRC_BYTE:
            parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);
            parser->mState = mcWAIT_SECOND_CRC_BYTE;
            break;
        /* only after the second one was received and stuffed into the
         * CRC-chain, we can decide whether we got something good. */
        case mcWAIT_SECOND_CRC_BYTE:
            parser->mDataCRC = ndlcomDoCrc(parser->mDataCRC, &c);
            if (parser->mDataCRC == NDLCOM_CRC_REAL_GOOD_VALUE) {
                parser->mState = mcCOMPLETE;
            } else {
                parser->mNumberOfCRCFails++;
                ndlcomParserDestroyPacket(parser);
            }
            break;
#endif
        case mcCOMPLETE:
            /* we reach here if we are "mcCOMPLETE" before any byte was parsed,
             * since there is an "if" for the same state after the switch-case.
             *
             * HINT: you have to manually obtain the packet-data using
             * "ndlcomParserGetPacket()" and "ndlcomParserGetHeader()".  Then
             * call ndlcomParserDestroyPacket() to reset the state-machine and
             * prepare it for further processing */
            return 0;
            break;
        case mcERROR:
            ndlcomParserDestroyPacket(parser);
            break;
            /* TODO */
        }

        /* abort processing if a complete packet was received */
        if (parser->mState == mcCOMPLETE) {
            return dataRead;
        }
    }

    return dataRead;
}

char ndlcomParserHasPacket(struct NDLComParser *parser) {
    return parser->mState == mcCOMPLETE;
}

const NDLComHeader *ndlcomParserGetHeader(struct NDLComParser *parser) {
    return parser->mState == mcCOMPLETE ? &parser->mHeader.hdr : 0;
}

const void *ndlcomParserGetPacket(struct NDLComParser *parser) {
    return parser->mState == mcCOMPLETE ? parser->mpData : 0;
}

void ndlcomParserDestroyPacket(struct NDLComParser *parser) {
    parser->mState = mcWAIT_HEADER;
    parser->mDataCRC = NDLCOM_CRC_INITIAL_VALUE;
    parser->mpHeaderWritePos = parser->mHeader.raw;
    parser->mpDataWritePos = parser->mpData;
    parser->mLastWasESC = 0;
}

void ndlcomParserGetState(struct NDLComParser *parser,
                          struct NDLComParserState *output) {
    output->mState = parser->mState;
    output->mNumberOfCRCFails = parser->mNumberOfCRCFails;
}

uint32_t ndlcomParserGetNumberOfCRCFails(struct NDLComParser *parser) {
    return parser->mNumberOfCRCFails;
}

void ndlcomParserResetNumberOfCRCFails(struct NDLComParser *parser) {
    parser->mNumberOfCRCFails = 0;
}
