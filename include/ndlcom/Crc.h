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

/** 8bit XOR */
#ifndef NDLCOM_CRC16
typedef uint8_t NDLComCrc;

#define NDLCOM_CRC_INITIAL_VALUE 0x00

/** @brief small function which does the CRC-calculation
 *
 * relatively simple, to be used incrementally.
 *
 * the algorithm is actually a simple concatenated XOR...
 */
NDLComCrc ndlcomDoCrc(const NDLComCrc currentCrc, const uint8_t *c);

/** 16bit FCS */
#else
typedef uint16_t NDLComCrc;

/**
 * @brief when we calculate the CRC, a convention on the initial value of the
 * memory location is needed. You are standing right in front of it. Behold,
 * the initial value:
 */
#define NDLCOM_CRC_INITIAL_VALUE 0xffff
#define NDLCOM_CRC_GOOD_VALUE 0xf0b8

/** don't ask me, it seems that this is also needed: */
#define NDLCOM_CRC_REAL_GOOD_VALUE 0x0000

/**
 * @brief and doing the CRC, as 16bit FCS
 *
 * @param currentCrc starting value of the FCS. Allows incremental calls
 * @param c pointer to the data-byte to be processed
 * @return the new CRC for the given values
 */
NDLComCrc ndlcomDoCrc(const NDLComCrc currentCrc, const uint8_t *c);

#endif

#if defined (__cplusplus)
}
#endif

#endif/*NDLCOM_CRC_H*/

