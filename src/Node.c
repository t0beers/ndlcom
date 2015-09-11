#include "ndlcom/Node.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"

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

    /* initialize handler which this Node is going to register in the bridge */
    ndlcomInternalHandlerInit(&node->myIdHandler, ndlcomNodeMessageHandler,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, node);
    /* na, what did I say? */
    ndlcomBridgeRegisterInternalHandler(bridge, &node->myIdHandler);
}

void ndlcomNodeDeinit(struct NDLComNode *node) {
    /* we do not want to be called anymore in the future */
    ndlcomBridgeDeregisterInternalHandler(node->bridge, &node->myIdHandler);
    /* additionally inform the bridge that our deviceId is no longer used
     * internally */
    ndlcomBridgeClearInternalDeviceId(node->bridge,
                                      node->headerConfig.mOwnSenderId);
}

/*
 * Initializes internal structs with correct values.
 */
void ndlcomNodeSetOwnSenderId(struct NDLComNode *node,
                              const NDLComId ownSenderId) {
    /*
     * resets the packet-counters to use for each receiver, and is used to
     * store our own deviceId at a convenient place
     */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);
    /*
     * we have to (re-)initialize the routing table, as we probably changed our
     * position inside the network. this also gets rid of our new "own id",
     * which might be already there
     * 
     * FIXME: This is wrong!
     */
    ndlcomRoutingTableInit(&node->bridge->routingTable);
    /*
     * informing the bridge that our "deviceId" is now internally used. messages
     * for this deviceId shall no longer be forwarded to the outside
     */
    ndlcomBridgeMarkDeviceIdAsInternal(node->bridge, ownSenderId);
}

void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload) {
    struct NDLComNode *node = (struct NDLComNode *)context;
    const struct NDLComInternalHandler *internalHandler;
    /*
     * calls our own handlers only for messages directed at "us"
     */
    if ((header->mReceiverId == node->headerConfig.mOwnSenderId) ||
        (header->mReceiverId == NDLCOM_ADDR_BROADCAST)) {
        list_for_each_entry(internalHandler, &node->internalHandlerList, list) {
            /* internal interfaces will see their own broadcast messages... */
            internalHandler->handler(internalHandler->context, header, payload);
        }
    }
}

/*
 * Sending a payload to a receiver
 */
void ndlcomNodeSend(struct NDLComNode *node, const NDLComId receiverId,
                    const void *payload, const size_t payloadSize) {
    /*
     * preparation of the header, filling in packet counter
     */
    struct NDLComHeader header;
    ndlcomHeaderPrepare(&node->headerConfig, &header, receiverId, payloadSize);
    /*
     * transmitting a message using the bridge
     */
    ndlcomBridgeSendRaw(node->bridge, &header, payload);
}

void ndlcomNodeRegisterInternalHandler(
    struct NDLComNode *node, struct NDLComInternalHandler *internalHandler) {

    list_add(&internalHandler->list, &node->internalHandlerList);
}

void ndlcomNodeDeregisterInternalHandler(
    struct NDLComNode *node, struct NDLComInternalHandler *internalHandler) {

    list_del(&internalHandler->list);
}
