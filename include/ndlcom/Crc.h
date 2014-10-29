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

#define NDLCOM_CRC16

/** crc 8 */
#ifndef NDLCOM_CRC16
typedef uint8_t NDLComCrc;

#define NDLCOM_CRC_INITIAL_VALUE 0x00

/** @brief small function which does the crc-calculation
 *
 * relatively simple, to be used incrementally.
 *
 * the algorithm is actually a simple concatenated xor...
 */
inline NDLComCrc ndlcomDoCrc(const NDLComCrc currentCrc, const unsigned char *c);

/** crc 16 */
#else
typedef uint16_t NDLComCrc;

/**
 * @brief when we calculate the crc, a convention on the initial value of the memory location is
 * needed. you are standing right in front of it. behold, the initial value:
 */
#define NDLCOM_CRC_INITIAL_VALUE 0xffff
#define NDLCOM_CRC_GOOD_VALUE 0xf0b8

// don't ask me, it seems that this is also needed:
#define NDLCOM_CRC_REAL_GOOD_VALUE 0x0000

/**
 * @brief and doing the fcs, in 16bit
 *
 * @param currentFcs starting value of the fcs. allows incremental calls
 * @param cp pointer to the data-array
 * @param len length of data-array to add to the crc, in bytes
 * @return the new fcs/crc for the given start-value and the covered memory array
 */
inline NDLComCrc ndlcomDoCrc(const NDLComCrc currentCrc, const unsigned char *d);

#endif

#if defined (__cplusplus)
}
#endif

#endif/*NDLCOM_CRC_H*/
