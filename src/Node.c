#include "ndlcom/Node.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"
#include "ndlcom/Interfaces.h"

/* forward declaration */
void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload);

void ndlcomNodeInit(struct NDLComNode *node, struct NDLComBridge *bridge,
                    const NDLComId ownSenderId) {

    /* safe the bridge-pointer which we'll gonna use to transmit messages */
    node->bridge = bridge;

    /* initialize all the list we have */
    INIT_LIST_HEAD(&node->internalHandlerList);

    /* calling this will also initialize the routing table */
    ndlcomNodeSetOwnSenderId(node, ownSenderId);

    ndlcomInternalHandlerInit(&node->myIdHandler, ndlcomNodeMessageHandler,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, node);
}

void ndlcomNodeDeinit(struct NDLComNode *node) {
    ndlcomBridgeDeregisterInternalHandler(node->bridge, &node->myIdHandler);
}

/* initializes internal structs with correct values. nearly resets the whole
 * state */
void ndlcomNodeSetOwnSenderId(struct NDLComNode *node,
                              const NDLComId ownSenderId) {

    /* resets the packet-counters to use for each receiver, and is used to
     * store our own deviceId at a convenient place */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);

    /* we have to (re-)initialize the routing table, as we probably changed our
     * position inside the network. this also gets rid of our new "own id",
     * which might be already there */
    ndlcomRoutingTableInit(&node->bridge->routingTable);

    /**
     * NOTE: putting our own "deviceId" into the routingtable. this is a hack,
     * to be able to detect messages going "to us" and not process them in the
     * outgoing side
     *
     * TODO: check that using "node" as the origin will work here...
     */
    ndlcomRoutingTableUpdate(&node->bridge->routingTable, ownSenderId, node);
}

void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload) {
    struct NDLComNode *node = (struct NDLComNode *)context;

    const struct NDLComInternalHandler *internalHandler;

    /* call the handlers which only want to see messages intended for "us" */
    if ((header->mReceiverId == node->headerConfig.mOwnSenderId) ||
        (header->mReceiverId == NDLCOM_ADDR_BROADCAST)) {
        list_for_each_entry(internalHandler, &node->internalHandlerList, list) {
            /* checking the "origin" for internal handlers does not make
             * sense... internal interfaces will always see their own
             * messages... */
            internalHandler->handler(internalHandler->context, header, payload);
        }
    }
}

/* sending a payload to a receiver. message will be handled by internal handlers
 */
void ndlcomNodeSend(struct NDLComNode *node, const NDLComId receiverId,
                    const void *payload, const size_t payloadSize) {
    /**
     * preparation of the header
     */
    struct NDLComHeader header;
    ndlcomHeaderPrepare(&node->headerConfig, &header, receiverId, payloadSize);
    /**
     * transmitting a message using the bridge
     */
    ndlcomBridgeSendRaw(node->bridge, &header, payload, payloadSize);
}

void ndlcomNodeRegisterInternalHandler(
    struct NDLComNode *node, struct NDLComInternalHandler *internalHandler) {

    list_add(&internalHandler->list, &node->internalHandlerList);
}

void ndlcomNodeDeregisterInternalHandler(
    struct NDLComNode *node, struct NDLComInternalHandler *internalHandler) {

    list_del(&internalHandler->list);
}
