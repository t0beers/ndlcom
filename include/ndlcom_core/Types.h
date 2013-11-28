/**
 * @file include/NDLCom/Types.h
 * @date 2011
 */

#ifndef NDLCOM_TYPES_H
#define NDLCOM_TYPES_H

/**
 * @addtogroup Communication_NDLCom NDLCom
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

/** The type used for the crc */
typedef uint8_t ndlcomCrc;

/* Calculate the number of possible devices */
enum { protocolHeaderMaxNumberOfDevices = (1 << (sizeof(ndlcomId) * 8)) };

/**
 * @}
 */

#endif
