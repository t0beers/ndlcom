#include "ndlcom/Routing.h"
#include "ndlcom/Protocol.h"

#include <string.h>

/* Due to DK (Donkey Kong :P): This macro determines the routing table size by
 * the size of the NDLComHeader.mReceiverId size */
#define NDLCOM_ROUTING_TABLE_SIZE (1 << (sizeof(NDLComId) * 8))

/* This array contains the interface given the receiver_id as index */
static void *routingTable[NDLCOM_ROUTING_TABLE_SIZE];

void ndlcomInitRoutingTable() {
    /* Default to all interfaces for all entries */
    int i;
    for (i = 0; i < NDLCOM_ROUTING_TABLE_SIZE; ++i)
        routingTable[i] = NDLCOM_ROUTING_ALL_INTERFACES;
}

void *ndlcomGetInterfaceByReceiverId(const NDLComId receiverId) {
    return routingTable[receiverId];
}

void ndlcomUpdateRoutingTable(const NDLComId senderId, void *pInterface) {
    if (senderId == NDLCOM_ADDR_BROADCAST)
        return;
    routingTable[senderId] = pInterface;
}
