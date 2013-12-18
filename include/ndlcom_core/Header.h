/**
 * @file include/ndlcom_core/Header.h
 * @date 2011
 */

#ifndef NDLCOM_HEADER_H
#define NDLCOM_HEADER_H

#include "ndlcom_core/Types.h"

#if defined (__cplusplus)
extern "C" {
#endif

/**
 * @brief The byteformat of a header
 *
 * Contains all used data-structures.
 */
typedef struct
{
    /** id of receiver. 0xff for broadcast, 0x00 reserved (error). */
    NDLComId mReceiverId;
    /** id of the sender of the packet. */
    NDLComId mSenderId;
    /** Frame counter. */
    NDLComCounter mCounter;
    /** Length of following data structure, limited to 255 bytes. */
    NDLComDataLen mDataLen;
} __attribute__((packed)) NDLComHeader;

/*
 * Keeps track of used counters and has the default sender id which
 * should be used in packet headers.
 * Do not access the members directly, use the function below to do so.
 */
typedef struct
{
    NDLComId mSenderId;
    NDLComId mCounterForReceiver[NDLCOM_MAX_NUMBER_OF_DEVICES];
} NDLComHeaderConfig;

/*
 * There is one static instance of NDLComHeaderConfig.
 */
extern NDLComHeaderConfig ndlcomHeaderConfigDefault;

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
static void ndlcomHeaderPrepareWithConfig(NDLComHeader* pHeader,
                                          NDLComId recv,
                                          NDLComDataLen length,
                                          NDLComHeaderConfig* pConfig)
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
 * Wrapper for ndlcomHeaderPrepareWithConfig using a common config structure.
 * @param pHeader This data structure will be modified
 * @param recv Receiver Id.
 * @param length Length of data packet (often the size of a c-struct).
 */
static inline void ndlcomHeaderPrepare(NDLComHeader* pHeader,
                                       NDLComId recv,
                                       NDLComDataLen length)
{
    return ndlcomHeaderPrepareWithConfig(pHeader, recv, length, &ndlcomHeaderConfigDefault);
}

/**
 * @brief Set default sender id.
 */
static inline void ndlcomHeaderConfigDefaultSenderId(NDLComId id)
{
    ndlcomHeaderConfigDefault.mSenderId = id;
}



#if defined (__cplusplus)
}
#endif

#endif
