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

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * This is the default interface id, which means, that a packet with a certain
 * senderId has not been observed yet. this means someone should send the
 * packet to _all_ interfaces, until we get a proper response.
 *
 * NOTE: this is also the value which is implicitly returned for broadcast
 * packages
 */
#define NDLCOM_ROUTING_ALL_INTERFACES NULL

/**
 * @brief Setting all entries, using all interfaces as default
 */
void ndlcomInitRoutingTable();

/**
 * @brief Routing Table look up
 *
 * this functions handles broadcast (and unknown) packages automagically
 * by return the special value NDLCOM_ROUTING_ALL_INTERFACES in this case
 *
 * @param receiverId the destination of the packet
 * @return identifier of the interface to use. NULL for "any interface"
 */
void *ndlcomGetInterfaceByReceiverId(const NDLComId receiverId);

/**
 * @brief Updates the routing table entries
 *
 * Should be used by usart.c
 *
 * @param senderId the id of the sender of an incoming packet
 * @param pInterface the id of the interface on which the packet was received
 * @return none
 */
void ndlcomUpdateRoutingTable(const NDLComId senderId, void *pInterface);

#if defined(__cplusplus)
}
#endif

#endif
