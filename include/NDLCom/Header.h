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

#include "NDLCom/Types.h"

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
    ndlcomId mReceiverId;
    /** id of the sender of the packet. */
    ndlcomId mSenderId;
    /** Frame counter. */
    ndlcomCounter mCounter;
    /** Length of following data structure, limited to 255 bytes. */
    ndlcomDataLen mDataLen;
} __attribute__((packed));

/* The library is now named NDLCom. :-/ */
typedef struct ProtocolHeader ndlcomHeader;


/* Calculate the number of possible devices */
enum { protocolHeaderMaxNumberOfDevices = (1 << (sizeof(ndlcomId) * 8)) };

/*
 * Keeps track of used counters and has the default sender id which
 * should be used in packet headers.
 * Do not access the members directly, use the function below to do so.
 */
typedef struct
{
    ndlcomId mSenderId;
    ndlcomId mCounterForReceiver[protocolHeaderMaxNumberOfDevices];
} ndlcomHeaderConfig;

/*
 * There is one static instance of NDLComHeaderConfig.
 */
extern ndlcomHeaderConfig protocolHeaderConfigDefault;

/**
 * @brief Set all fields of the header.
 * Since this function increments the packet counter in the header,
 * you should actually send the packet to the receiver.
 * @param pHeader This data structure will be modified
 * @param recv Receiver Id.
 * @param length Length of data packet (often the size of a c-struct).
 * @param pConfig Pointer to a structure holding the sender id and
 *                keeping track of packet counters.
 */
static void protocolHeaderPrepareWithConfig(ndlcomHeader* pHeader,
                                            ndlcomId recv,
                                            ndlcomDataLen length,
                                            ndlcomHeaderConfig* pConfig)
{
    pHeader->mReceiverId = recv;
    pHeader->mSenderId = pConfig->mSenderId;
    pHeader->mCounter = pConfig->mCounterForReceiver[recv]++;
    pHeader->mDataLen = length;
}

/**
 * @brief Set all fields of the header.
 * Since this function increments the packet counter in the header,
 * you should actually send the packet to the receiver.
 * Wrapper for protocolHeaderPrepareWithConfig using a common config structure.
 * @param pHeader This data structure will be modified
 * @param recv Receiver Id.
 * @param length Length of data packet (often the size of a c-struct).
 */
static inline void protocolHeaderPrepare(ndlcomHeader* pHeader,
                                         ndlcomId recv,
                                         ndlcomDataLen length)
{
    return protocolHeaderPrepareWithConfig(pHeader, recv, length, &protocolHeaderConfigDefault);
}

/**
 * @brief Set default sender id.
 */
static inline void protocolHeaderConfigDefaultSenderId(ndlcomId id)
{
    protocolHeaderConfigDefault.mSenderId = id;
}



#if defined (__cplusplus)
}
#endif

/**
 * @}
 */

#endif
