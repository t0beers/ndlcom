/**
 * @file include/NDLCom/header.h
 * @date 2011
 */

#ifndef NDLCOM_HEADER_H
#define NDLCOM_HEADER_H

/**
 * @addtogroup Communication_Protocol Protocol
 * @{
 *
 */

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif


/**
 * @brief The byteformat of a header
 *
 * Contains all used data-structures.
 */
struct ProtocolHeader
{
    /** id of receiver. 0x00 for broadcast. */
    uint8_t mReceiverId;
    /** id of the sender of the packet. */
    uint8_t mSenderId;
    /** Frame counter. */
    uint8_t mCounter;
    /** Length of following data structure, limited to 255 bytes. */
    uint8_t mDataLen;
} __attribute__((packed));


#if defined (__cplusplus)
}
#endif

/**
 * @}
 */

#endif
