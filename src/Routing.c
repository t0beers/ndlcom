#include "ndlcom/Routing.h"

void ndlcomRoutingTableInit(struct NDLComRoutingTable *routingTable) {

    /* before we know anything about the world we have to default to "all
     * interfaces" for all deviceIds */
    int i;
    for (i = 0; i < NDLCOM_MAX_NUMBER_OF_DEVICES; ++i)
        routingTable->table[i] = NDLCOM_ROUTING_ALL_INTERFACES;
}

void *ndlcomRoutingGetDestination(const struct NDLComRoutingTable *routingTable,
                                  const NDLComId receiverId) {
    /* handle broadcastId special */
    if (receiverId == NDLCOM_ADDR_BROADCAST)
        return NDLCOM_ROUTING_ALL_INTERFACES;
    /* note that no special range-checks are needed, as the index "receiverId"
     * cannot get bigger or smaller than the array itself */
    return routingTable->table[receiverId];
}

void ndlcomRoutingTableUpdate(struct NDLComRoutingTable *routingTable,
                              const NDLComId senderId, void *pInterface) {
    /* This does never put broadcast senderIds into the routing table. It
     * simply does not make any sense. Note that, by never populating the
     * entry, we automatically reply NDLCOM_ROUTING_ALL_INTERFACES in case
     * someone queries the table for the NDLCOM_ADDR_BROADCAST.
     *
     * could add a check to not use our own senderId accidentally... */
    if (senderId == NDLCOM_ADDR_BROADCAST)
        return;
    routingTable->table[senderId] = pInterface;
}
