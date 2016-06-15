/**
 * German Research Center for Artificial Intelligence\n
 * Project: iStruct
 *
 * @date 11.11.2011
 *
 * @author Schilling, Zenzes
 */

#ifndef NDLCOM_ROUTING_H_
#define NDLCOM_ROUTING_H_

#include "ndlcom/Types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *
 * stores a "void*" identifier for every known deviceId.
 *
 */
struct NDLComRoutingTable {
    void *table[NDLCOM_MAX_NUMBER_OF_DEVICES];
};

/**
 * This is the default interface id, which means, that a packet with a certain
 * senderId has not been observed yet. this means someone should send the
 * packet to _all_ interfaces, until we get a proper response.
 *
 * NOTE: this is also the value which is implicitly returned for broadcast
 * packages
 */
#define NDLCOM_ROUTING_ALL_INTERFACES 0

/**
 * @brief Setting all entries, using all interfaces as default
 */
void ndlcomRoutingTableInit(struct NDLComRoutingTable *routingTable);

/**
 * @brief Routing Table look up
 *
 * this functions handles broadcast (and unknown) packages automagically
 * by return the special value NDLCOM_ROUTING_ALL_INTERFACES in this case
 *
 * @param routingTable the routing table to process
 * @param receiverId the destination of the packet
 * @return identifier of the interface to use. NULL for "any interface"
 */
void *ndlcomRoutingGetDestination(const struct NDLComRoutingTable *routingTable,
                                  const NDLComId receiverId);

/**
 * @brief Updates the routing table entries
 *
 * @param routingTable the table to work on
 * @param senderId the id of the sender of an incoming packet
 * @param pInterface the id of the interface on which the packet was received
 */
void ndlcomRoutingTableUpdate(struct NDLComRoutingTable *routingTable,
                              const NDLComId senderId, void *pInterface);

#if defined(__cplusplus)
}
#endif

#endif
