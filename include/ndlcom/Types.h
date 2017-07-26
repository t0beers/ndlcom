/**
 * @file include/ndlcom/Types.h
 * @date 2011
 */
#ifndef NDLCOM_TYPES_H
#define NDLCOM_TYPES_H

/**
 * This header-file declares the size/type of "NDLComCrc", depending on which
 * mode is chosen, 8bit XOR or 16bit FCS. See the define NDLCOM_CRC16.
 */
#include "ndlcom/Crc.h"

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Type for senderId and receiverId to address devices in the header. */
typedef uint8_t NDLComId;

/** Type for the packet counter field in the header. */
typedef uint8_t NDLComCounter;

/** Type for the payload length field in the header */
typedef uint8_t NDLComDataLen;

/**
 * @brief The byte format of a header
 *
 * Keep in mind that changing this would also need corresponding changes in the
 * VHDL code.
 */
struct NDLComHeader {
    /**
     * DeviceId of the receiver this packet is directed to. 0xff for broadcast
     * (see NDLCOM_ADDR_BROADCAST), 0x00 kindof reserved (error?)
     */
    NDLComId mReceiverId;
    /**
     * deviceId of the sender of this packet.
     */
    NDLComId mSenderId;
    /**
     * Frame counter. To be increased by the sender for each packet sent to a
     * specific receiverId.
     */
    NDLComCounter mCounter;
    /**
     * Length of following payload, limited to a maximum of 255 bytes (see
     * NDLCOM_MAX_PAYLOAD_SIZE)
     */
    NDLComDataLen mDataLen;
    /* This structure is "packed" to force tight memory alignment */
} __attribute__((packed));

/**
 * @brief Escape-byte
 *
 * Denotes occurence of reserved bytes in the datastream.
 */
#define NDLCOM_ESC_CHAR 0x7d

/**
 * @brief Flag-byte
 *
 * Denotes the beginning of a new packet. Using it at the end of a packet is
 * not forbidden and helps when performing cut-through forwarding.
 */
#define NDLCOM_START_STOP_FLAG 0x7e

/**
 * @brief Broadcast address
 */
#define NDLCOM_ADDR_BROADCAST 0xff

/**
 * @brief The number of possible deviceIds, 256 by default
 *
 * This can be altered at buildtime to cut away the upper Ids from the global
 * address space. This reduces static memory consumption in NDLComRoutingTable
 * and NDLComHeaderConfig.
 *
 * - Packets exceeding these limits should not disturb the NDLComParser, they
 *   will be decoded and handled. Keep in mind that routing these packages
 *   using "store and forward" will no work, the default strategy is forwarding
 *   them on all interfaces, like a broadcast package.
 * - The BROADCAST deviceId is 255, special care is taken in HeaderPrepare.c
 *   and Routing.c
 * - It seems that the own devcieId needs to be in this range?
 */
#ifndef NDLCOM_MAX_NUMBER_OF_DEVICES
#define NDLCOM_MAX_NUMBER_OF_DEVICES (1 << (sizeof(NDLComId) * 8))
#endif

/**
 * @brief How much bytes of payload a packet can contain as maximum
 *
 * Can be overridden to restrict static memory consumption in NDLComParser and
 * stack memory consumption in ndlcomBridgeProcessExternalInterface().
 *
 * Packets exceeding this limit do not disturb the NDLComParser, they will be
 * silently discarded and thus can never be received. Keep in mind that routing
 * these packages using the implemented "store and forward" will also not work!
 *
 * The Encoder will encode as much bytes as are able to fit into the provided
 * buffer, thus packets bigger than this limits might only be partially
 * transmitted.
 */
#ifndef NDLCOM_MAX_PAYLOAD_SIZE
#define NDLCOM_MAX_PAYLOAD_SIZE ((1 << (sizeof(NDLComDataLen) * 8)) - 1)
#endif

/**
 * @brief Worst-case size of a decoded message
 *
 * A decoded message can contain up to 255bytes of payload, an NDLComHeader and
 * the NDLComCrc. This is it for the decoded version of a packet and results in
 * 261bytes by default.
 */
#define NDLCOM_MAX_DECODED_MESSAGE_SIZE                                        \
    (sizeof(struct NDLComHeader) + NDLCOM_MAX_PAYLOAD_SIZE + sizeof(NDLComCrc))

/**
 * @brief Size to store a decoded packet with known payload-size
 *
 * This macro is probably useless...
 */
#define NDLCOM_MAX_DECODED_MESSAGE_SIZE_FOR_PACKET(header)                     \
    (sizeof(struct NDLComHeader) + header->mDataLen + sizeof(NDLComCrc))

/**
 * @brief Worst-case size of encoded message
 *
 * Useful to size the raw rx/tx-buffers.
 *
 * In an encoded message, the worst case would be to escape every single byte
 * of an decoded message (see NDLCOM_MAX_DECODED_MESSAGE_SIZE), plus the
 * initial start-flag (and the optional stop flag). This results is 524 by
 * default.
 */
#define NDLCOM_MAX_ENCODED_MESSAGE_SIZE                                        \
    (2 + 2 * NDLCOM_MAX_DECODED_MESSAGE_SIZE)

/**
 * @brief Size to store a encoded packet with known payload-size
 *
 * This macro is probably useless...
 */
#define NDLCOM_MAX_ENCODED_MESSAGE_SIZE_FOR_PACKET(header)                     \
    (2 + 2 * NDLCOM_MAX_DECODED_MESSAGE_SIZE_FOR_PACKET(header))

#if defined(__cplusplus)
}
#endif

#endif
