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
typedef uint8_t NDLComId;

/** Type for counter field in the header. */
typedef uint8_t NDLComCounter;

/** Type for the data length field in the header */
typedef uint8_t NDLComDataLen;

/**
 * @}
 */

#endif
