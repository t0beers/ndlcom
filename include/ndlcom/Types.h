/**
 * @file include/ndlcom/Types.h
 * @date 2011
 */
#ifndef NDLCOM_TYPES_H
#define NDLCOM_TYPES_H

/**
 * this header-file declares the size/type of "NDLComCrc", depending on which
 * mode is chosen, 8bit XOR or 16bit FCS */
#include "ndlcom/Crc.h"

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Type for sender and receiver ids in the header. */
typedef uint8_t NDLComId;

/** Type for counter field in the header. */
typedef uint8_t NDLComCounter;

/** Type for the data length field in the header */
typedef uint8_t NDLComDataLen;

/* Calculate the number of possible devices */
enum { NDLCOM_MAX_NUMBER_OF_DEVICES = (1 << (sizeof(NDLComId) * 8)) };

/* how much payload a packet can carry in maximum */
enum { NDLCOM_MAX_PAYLOAD_SIZE = (1 << (sizeof(NDLComDataLen) * 8)) };

/**
 * @brief The byte format of a header
 *
 * Contains all used data-structures.
 */
typedef struct NDLComHeader {
    /** id of receiver. 0xff for broadcast, 0x00 reserved (error). */
    NDLComId mReceiverId;
    /** id of the sender of the packet. */
    NDLComId mSenderId;
    /** Frame counter. */
    NDLComCounter mCounter;
    /** Length of following data structure, limited to 255 bytes. */
    NDLComDataLen mDataLen;
} __attribute__((packed)) NDLComHeader;

/**
 * @brief current escape-byte
 */
#define NDLCOM_ESC_CHAR 0x7d

/**
 * @brief current flag to denote an escaped byte
 */
#define NDLCOM_START_STOP_FLAG 0x7e

/**
 * @brief current broadcast address
 */

#define NDLCOM_ADDR_BROADCAST 0xff

/**
 * @brief length of the current header
 */
#define NDLCOM_HEADERLEN (sizeof(NDLComHeader))

/**
 * @brief worst-case size of rx-buffer
 *
 * an decoded message can contain upto 255byte, a header and the crc. no bytes
 * are escaped
 */
#define NDLCOM_MAX_DECODED_MESSAGE_SIZE                                        \
    (NDLCOM_HEADERLEN + NDLCOM_MAX_PAYLOAD_SIZE + sizeof(NDLComCrc))

/**
 * @brief worst-case size of tx-buffer
 *
 * in an encoded message, the worst case would be to escape _each_ single byte
 * of an decoded message, plus the initial start-flag and the optional stop
 * flag. The CRC is included in the decoded message, and can be escaped as well
 */
#define NDLCOM_MAX_ENCODED_MESSAGE_SIZE                                        \
    (2 + NDLCOM_MAX_DECODED_MESSAGE_SIZE * 2)

#if defined(__cplusplus)
}
#endif

#endif
