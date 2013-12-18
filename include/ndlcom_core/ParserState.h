/**
 * @file include/ndlcom_core/ParserState.h
 * @date 2011
 */

#ifndef NDLCOM_PARSER_STATE_H
#define NDLCOM_PARSER_STATE_H

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

/**
 * @brief actual state of the parser, useful for a gui
 */
struct NDLComParserState
{
    int8_t mState;/**< actual state of the parser */
    int16_t mNumberOfCRCFails;/**< how often the crc failed */
    int8_t mLastError;/**< cause of the last error */
};

#if defined (__cplusplus)
}
#endif

#endif
