/**
 * @file include/ndlcom/ParserState.h
 * @date 2011
 */

#ifndef NDLCOM_PARSER_STATE_H
#define NDLCOM_PARSER_STATE_H

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

/**
 * @brief actual state of the parser, useful for a GUI
 */
struct NDLComParserState {
    int8_t mState;              /**< actual state of the parser */
    uint32_t mNumberOfCRCFails; /**< how often the CRC failed */
};

#if defined (__cplusplus)
}
#endif

#endif
