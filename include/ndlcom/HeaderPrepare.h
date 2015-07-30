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
 * @brief The NDLCom-protocol defines the packet-counter variable.
 *
 * it has to be incremented for each packet transmitted to a received node -
 * regardless of the payload used in the packet. Therefore, a
 * table is used to fill the correct packet counter. It is essentially
 * an array with one entry for each node, storing the last used packet counter.
 *
 * Keeps track of used counters and has the default sender id which should be
 * used in packet headers.  Do not access the members directly, use the
 * function below to do so.
 */
struct NDLComHeaderConfig {
    /** a table with the current counter for each target-device */
    NDLComId mCounterForReceiver[NDLCOM_MAX_NUMBER_OF_DEVICES];
    /** this will define which Id to use as _our_ senderId */
    NDLComId mOwnSenderId;
};

/**
 * @brief Set default sender id.
 *
 * NOTE: when changing the senderId (personality), the routing table should
 * also be cleared (and it is in this function).
 *
 * @param config pointer to the struct to initialize
 * @param senderId the new id to store in the static NDLComHeaderConfig
 */
void ndlcomHeaderPrepareInit(struct NDLComHeaderConfig *config,
                             const NDLComId senderId);

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
static inline void ndlcomHeaderPrepare(struct NDLComHeaderConfig *pConfig,
                                       struct NDLComHeader *pHeader,
                                       const NDLComId receiverId,
                                       const NDLComDataLen dataLength) {
    pHeader->mReceiverId = receiverId;
    pHeader->mSenderId = pConfig->mOwnSenderId;
    pHeader->mCounter = pConfig->mCounterForReceiver[receiverId]++;
    pHeader->mDataLen = dataLength;
}

#if defined(__cplusplus)
}
#endif

#endif/*NDLCOM_HEADER_PREPARE_H*/
