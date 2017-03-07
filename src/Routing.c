/**
 * @file src/Routing.c
 */
#include "ndlcom/Routing.h"

void ndlcomRoutingTableInit(struct NDLComRoutingTable *routingTable) {
    int i;
    /* before we know anything about the world we have to default to "all
     * interfaces" for all deviceIds */
    for (i = 0; i < NDLCOM_MAX_NUMBER_OF_DEVICES; ++i) {
        routingTable->table[i] = NDLCOM_ROUTING_ALL_INTERFACES;
    }
}

void *ndlcomRoutingGetDestination(const struct NDLComRoutingTable *routingTable,
                                  const NDLComId receiverId) {
    /* if we can lookup the deviceId in the table do so */
    if (receiverId < NDLCOM_MAX_NUMBER_OF_DEVICES) {
        return routingTable->table[receiverId];
    } else {
        /* otherwise we have to fallback to the ALL_INTERFACES. this allows to
         * be compiled with less available device ids, saving some memory */
        return NDLCOM_ROUTING_ALL_INTERFACES;
    }
}

void ndlcomRoutingTableUpdate(struct NDLComRoutingTable *routingTable,
                              const NDLComId senderId, void *pInterface) {
    /* never put broadcast senderIds into the routing table. It simply does not
     * make any sense. also, in case this library is compiled with reduced
     * number if devices, ids outside the range will be ignored.
     */
    if (senderId >= NDLCOM_MAX_NUMBER_OF_DEVICES) {
        return;
    } else {
        routingTable->table[senderId] = pInterface;
    }
}

void
ndlcomRoutingTableInvalidateInterface(struct NDLComRoutingTable *routingTable,
                                      const void *pInterface) {
    int i;
    /* someone wants the given interface to vanish from the current routing
     * table... easy enough, lets go: */
    for (i = 0; i < NDLCOM_MAX_NUMBER_OF_DEVICES; ++i) {
        if (routingTable->table[i] == pInterface) {
            routingTable->table[i] = NDLCOM_ROUTING_ALL_INTERFACES;
        }
    }
}
