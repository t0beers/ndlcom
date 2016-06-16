/**
 * @file include/ndlcom/Routing.h
 */
#ifndef NDLCOM_ROUTING_H
#define NDLCOM_ROUTING_H

#include "ndlcom/Types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Dynamic routing table stores a "void*" identifier for every known deviceId.
 *
 * In reality, each distinct pointer represents a specific ExternalInterface.
 * Upon lookup of a deviceId in the table, the corresponding interface
 * identifier is returned.
 *
 * For unknown deviceIds, the value NDLCOM_ROUTING_ALL_INTERFACES is stored.
 * Additionally, the (special) broadcastId uses the same identifier. After
 * initialization, the whole table contains this value.
 */
struct NDLComRoutingTable {
    void *table[NDLCOM_MAX_NUMBER_OF_DEVICES];
};

/**
 * This is the default interface id to be used while a certain senderId has not
 * been observed yet. This means we have no knowledge where to route for a
 * given receiverId until we observer a proper specimen.
 *
 * NOTE: This is also the value which is implicitly returned for broadcast
 * packages
 */
#define NDLCOM_ROUTING_ALL_INTERFACES 0

/**
 * @brief Initializing all entries to the NDLCOM_ROUTING_ALL_INTERFACES
 *
 * After calling this function, no senderIds will be known to the routing
 * table. Nothing is known.
 */
void ndlcomRoutingTableInit(struct NDLComRoutingTable *routingTable);

/**
 * @brief Routing Table look up for dynamic routing
 *
 * This function handles broadcast (and unknown) packages automagically
 * by returning the special value NDLCOM_ROUTING_ALL_INTERFACES. For known
 * receiverIds the void* stored into the table is returned.
 *
 * @param routingTable The routing table to work on
 * @param receiverId The destination address to look up
 * @return Identifier of the interface to use. NDLCOM_ROUTING_ALL_INTERFACES
 *         for broadcast and unknown ids.
 */
void *ndlcomRoutingGetDestination(const struct NDLComRoutingTable *routingTable,
                                  const NDLComId receiverId);

/**
 * @brief Update an entry in the routing table
 *
 * @param routingTable The table to work on
 * @param senderId The id of the sender of an observed packet
 * @param pInterface The id of the interface on which the packet was observed
 */
void ndlcomRoutingTableUpdate(struct NDLComRoutingTable *routingTable,
                              const NDLComId senderId, void *pInterface);

/**
 * @brief Removing interfaces from the routing table
 *
 * After phyiscally disconnecting an existing interface, the routing table may
 * still contain an entry for a previously existing interface. This function
 * clears all entries containing the given pInterface from the given
 * routingTable, sets them to NDLCOM_ROUTING_ALL_INTERFACES again.
 *
 * @param routingTable the table to work on
 * @param pInterface the entry which will be overwritten with
 *        NDLCOM_ROUTING_ALL_INTERFACES
 */
void
ndlcomRoutingTableInvalidateInterface(struct NDLComRoutingTable *routingTable,
                                      const void *pInterface);

#if defined(__cplusplus)
}
#endif

#endif/*NDLCOM_ROUTING_H*/
