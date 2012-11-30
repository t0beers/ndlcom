/**
 * @file include/NDLCom/Types.h
 * @date 2011
 */

#ifndef NDLCOM_TYPES_H
#define NDLCOM_TYPES_H

/**
 * @addtogroup Communication_Protocol Protocol
 * @{
 *
 */

#include <stdint.h>

/** Type for sender and receiver ids in the header. */
typedef uint8_t ndlcomId;

/** Type for counter field in the header. */
typedef uint8_t ndlcomCounter;

/** Type for the data length field in the header */
typedef uint8_t ndlcomDataLen;

/**
 * @}
 */

#endif
