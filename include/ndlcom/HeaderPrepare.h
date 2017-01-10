/**
 * @file include/ndlcom/HeaderPrepare.h
 */
#ifndef NDLCOM_HEADER_PREPARE_H
#define NDLCOM_HEADER_PREPARE_H

#include "ndlcom/Types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Every NDLCom packet carries a packet counter
 *
 * It has to be incremented for each packet transmitted to a specific
 * receiverId -- regardless of the payload used in the packet. Therefore, a
 * table is used to fill the last used packet counter for every receiver.
 *
 * It keeps track of used counters and also stored the default ownSenderIid
 * which should be used in packet headers. Do not modify the members directly,
 * use the functions below to do so.
 */
struct NDLComHeaderConfig {
    /** a table with the current counter for each target-device */
    NDLComId mCounterForReceiver[NDLCOM_MAX_NUMBER_OF_DEVICES];
    /** this will define which Id to use as _our_ senderId */
    NDLComId mOwnSenderId;
};

/**
 * @brief Set ownSenderId and initialize the packet counters
 *
 * NOTE: When changing the senderId (personality), the routing table has to be
 * cleared as well. This is _not_ done in this function!
 *
 * @param config Pointer to the struct to initialize
 * @param ownSenderId The new deviceId to store
 */
void ndlcomHeaderPrepareInit(struct NDLComHeaderConfig *config,
                             const NDLComId ownSenderId);

/**
 * @brief Set all fields of the header.
 *
 * Since this function increments the packet counter in the header,
 * you should actually send the packet to the receiver.
 *
 * @param config Pointer to a structure holding the sender id and
 *                keeping track of packet counters.
 * @param header This data structure will be modified
 * @param receiverId Intended receiverId.
 * @param dataLength Length of the payload to be sent
 */
void ndlcomHeaderPrepare(struct NDLComHeaderConfig *config,
                         struct NDLComHeader *header, const NDLComId receiverId,
                         const NDLComDataLen dataLength);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_HEADER_PREPARE_H*/
