/**
 * @file include/NDLCom/ParserState.h
 * @date 2011
 */

#ifndef NDLCOM_PARSER_STATE_H
#define NDLCOM_PARSER_STATE_H

/**
 * @addtogroup Communication_NDLCom
 */

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

/**
 * @brief actual state of the parser, useful for a gui
 */
struct ProtocolParserState
{
    int8_t mState;/**< actual state of the parser */
    int8_t mNumberOfEscapes;/**< how many escaped in a raw were received. should be something around zero, most of the time */
    int8_t mNumberOfCRCFails;/**< how often the crc failed */
    int8_t mLastError;/**< cause of the last error */
};

#if defined (__cplusplus)
}
#endif

/**
 * @}
 */

#endif
