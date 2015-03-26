#include "ndlcom/Routing.h"
#include "ndlcom/Protocol.h"

#include <string.h>

/* Due to DK (Donkey Kong :P): This macro determines the routing table size by
 * the size of the NDLComHeader.mReceiverId size */
#define ROUTING_TABLE_SIZE (1 << (sizeof(NDLComId) * 8))

/* This array contains the interface given the receiver_id as index */
static void *routingTable[ROUTING_TABLE_SIZE];

void ndlcomInitRoutingTable() {
    /* Default to all interfaces for all entries */
    int i = 0;
    for (; i < ROUTING_TABLE_SIZE; ++i)
        routingTable[i] = INTERFACE_ID_ALL;
}

void *ndlcomGetInterfaceByReceiverId(NDLComId receiverId) {
    return routingTable[receiverId];
}

void ndlcomUpdateRoutingTable(NDLComId senderId, void *pInterface) {
    if (senderId == NDLCOM_ADDR_BROADCAST)
        return;
    routingTable[senderId] = pInterface;
}
