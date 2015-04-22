/**
 * @file include/ndlcom/HeaderPrepare.h
 * @date 2011
 */

#ifndef NDLCOM_HEADER_PREPARE_H
#define NDLCOM_HEADER_PREPARE_H

#include "ndlcom/Types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Keeps track of used counters and has the default sender id which should be
 * used in packet headers.  Do not access the members directly, use the
 * function below to do so.
 */
struct NDLComHeaderConfig {
    /** a table with the current counter for each target-device */
    NDLComId mCounterForReceiver[NDLCOM_MAX_NUMBER_OF_DEVICES];
    /** this will define which Id to use as _our_ senderId */
    NDLComId mSenderId;
};

extern struct NDLComHeaderConfig ndlcomHeaderConfigDefault;

/**
 * obtain the current default "senderId" used by this process, eg its
 * "personality" */
static inline NDLComId ndlcomHeaderConfigGetDefaultSenderId() {
    return ndlcomHeaderConfigDefault.mSenderId;
};

/**
 * @brief Set all fields of the header.
 * Since this function increments the packet counter in the header,
 * you should actually send the packet to the receiver.
 * @param pHeader This data structure will be modified
 * @param receiverId Receiver Id.
 * @param dataLength Length of data packet (often the size of a c-struct).
 * @param pConfig Pointer to a structure holding the sender id and
 *                keeping track of packet counters.
 */
static inline void ndlcomHeaderPrepareWithConfig(NDLComHeader *pHeader, NDLComId receiverId,
                                   NDLComDataLen dataLength,
                                   struct NDLComHeaderConfig *pConfig) {
    pHeader->mReceiverId = receiverId;
    pHeader->mSenderId = pConfig->mSenderId;
    pHeader->mCounter = pConfig->mCounterForReceiver[receiverId]++;
    pHeader->mDataLen = dataLength;
}

/**
 * @brief Set all fields of the header.
 *
 * Since this function increments the packet counter in the header, you should
 * actually send the packet to the receiver.  Wrapper for
 * "ndlcomHeaderPrepareWithConfig()" using a static NDLComHeaderConfig
 * structure.
 *
 * @param pHeader This data structure will be modified
 * @param receiverId Receiver Id.
 * @param dataLength Length of data packet (often the size of a c-struct).
 */
static inline void ndlcomHeaderPrepare(NDLComHeader *pHeader, NDLComId receiverId,
                         NDLComDataLen dataLength) {
    ndlcomHeaderPrepareWithConfig(pHeader, receiverId, dataLength,
                                  &ndlcomHeaderConfigDefault);
}

/**
 * @brief Set default sender id.
 * @param newSenderId the new id to store in the static NDLComHeaderConfig
 */
static inline void ndlcomHeaderConfigDefaultSenderId(NDLComId newSenderId) {
    ndlcomHeaderConfigDefault.mSenderId = newSenderId;

    // FIXME: when changing the senderId (personality), the routing table has
    // to be cleared?
}

#if defined(__cplusplus)
}
#endif

#endif//NDLCOM_HEADER_PREPARE_H
