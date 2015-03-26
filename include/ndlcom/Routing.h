/**
 * German Research Center for Artificial Intelligence\n
 * Project: iStruct
 *
 * @date 11.11.2011
 *
 * @author Schilling
 */

#ifndef NDLCOM_ROUTING_H_
#define NDLCOM_ROUTING_H_

#include "ndlcom/Types.h"

/** This is the default interface id, which means, that a packet with a certain
 * senderId has not been observed yet*/
#define INTERFACE_ID_ALL 0

#if defined (__cplusplus)
extern "C" {
#endif

/**
 * @brief Setting all entries, using all interfaces as default
 */
void ndlcomInitRoutingTable();

/**
 * @brief Routing Table look up
 *
 * Should be used by comm.c
 *
 * @param receiver_id the destination of the packet
 * @return the interface to use
 */
void *ndlcomGetInterfaceByReceiverId(NDLComId receiverId);

/**
 * @brief Updates the routing table entries
 *
 * Should be used by usart.c
 *
 * @param sender_id the id of the sender of an incoming packet
 * @param interface the id of the interface on which the packet was received
 * @return none
 */
void ndlcomUpdateRoutingTable(NDLComId senderId, void *pInterface);

#if defined (__cplusplus)
}
#endif

#endif
