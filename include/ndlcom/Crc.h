/**
 * @file include/ndlcom/Crc.h
 * @date 2013
 */
#ifndef NDLCOM_CRC_H
#define NDLCOM_CRC_H

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

/** crc 8*/
typedef uint8_t NDLComCrc;

/* initialization value of the checksum */
#define NDLCOM_CRC_INITIAL_VALUE 0x00

/* small function which does the calculation. relatively simple, to be used incrementally.
 *
 * the algorithm is actually a simple concatenated xor...
 */
inline NDLComCrc ndlcomDoCrc(const NDLComCrc currentCrc, const unsigned char *c);

#if defined (__cplusplus)
}
#endif

#endif/*NDLCOM_CRC_H*/
