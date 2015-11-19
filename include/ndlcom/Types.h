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

/**
 * @brief The byte format of a header
 *
 * Contains all used data-structures.
 */
struct NDLComHeader {
    /** id of receiver. 0xff for broadcast, 0x00 reserved (error). */
    NDLComId mReceiverId;
    /** id of the sender of the packet. */
    NDLComId mSenderId;
    /** Frame counter. */
    NDLComCounter mCounter;
    /** Length of following data structure, limited to 255 bytes. */
    NDLComDataLen mDataLen;
    /* is "packed" because the header will be used in ndlcom-messages */
} __attribute__((packed));

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
 * calculate the number of possible devices, 256 by default
 *
 * this can be altered from outside to cut away the upper Ids from the global
 * adressspace. safes memory in routing and header tables.
 *
 * TODO: correct?
 * - packets exceeding these limits should not disturb the parser, they just
 *   cannot be received.
 * - the encoder does not care
 */
#ifndef NDLCOM_MAX_NUMBER_OF_DEVICES
#define NDLCOM_MAX_NUMBER_OF_DEVICES (1 << (sizeof(NDLComId) * 8))
#endif

/**
 * how much payload a packet can carry in maximum
 *
 * can be overridden to restrict datastructures to lower maximum packet sizes,
 * safes memory
 *
 * TODO: correct?
 * - packets exceeding these limits should not disturb the parser, they just
 *   cannot be received.
 * - the encoder does not care
 */
#ifndef NDLCOM_MAX_PAYLOAD_SIZE
#define NDLCOM_MAX_PAYLOAD_SIZE (1 << (sizeof(NDLComDataLen) * 8))
#endif

/**
 * @brief worst-case size of rx-buffer
 *
 * an decoded message can contain upto 255byte, a header and the crc. no bytes
 * are escaped and there are no start/stop flags. this results in 261bytes by
 * default.
 */
#define NDLCOM_MAX_DECODED_MESSAGE_SIZE                                        \
    (sizeof(struct NDLComHeader) + NDLCOM_MAX_PAYLOAD_SIZE + sizeof(NDLComCrc))

/**
 * @brief size to store a complete given packet
 *
 * this macro is probably useless
 */
#define NDLCOM_MAX_DECODED_MESSAGE_SIZE_FOR_PACKET(header)                     \
    (sizeof(struct NDLComHeader) + header->mDataLen + sizeof(NDLComCrc))

/**
 * @brief worst-case size of tx-buffer
 *
 * in an encoded message, the worst case would be to escape _each_ single byte
 * of an decoded message, plus the initial start-flag and the optional stop
 * flag. The CRC is included in the decoded message, and can be escaped as
 * well. this is 524 by default
 */
#define NDLCOM_MAX_ENCODED_MESSAGE_SIZE                                        \
    (2 + 2 * NDLCOM_MAX_DECODED_MESSAGE_SIZE)

/**
 * @brief how much space it needed to encode a given packet in worst case
 *
 * probably less.
 */
#define NDLCOM_MAX_ENCODED_MESSAGE_SIZE_FOR_PACKET(header)                     \
    (2 + 2 * NDLCOM_MAX_DECODED_MESSAGE_SIZE_FOR_PACKET(header))

#if defined(__cplusplus)
}
#endif

#endif
