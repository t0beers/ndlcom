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

#include "Types.h"

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
    /** id of receiver. 0xff for broadcast, 0x00 reserved (error). */
    NDLComId mReceiverId;
    /** id of the sender of the packet. */
    NDLComId mSenderId;
    /** Frame counter. */
    NDLComCounter mCounter;
    /** Length of following data structure, limited to 255 bytes. */
    NDLComDataLen mDataLen;
} __attribute__((packed));

/* The library is now named NDLCom. :-/ */
typedef struct ProtocolHeader NDLComHeader;

/*

#if defined (__cplusplus)
}
#endif

/**
 * @}
 */

#endif
