#include "ndlcom/Node.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"

/* forward declaration */
void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin);

void ndlcomNodeInit(struct NDLComNode *node, struct NDLComBridge *bridge,
                    const NDLComId ownSenderId) {

    /* safe the bridge-pointer which we'll gonna use to transmit messages */
    node->bridge = bridge;

    /* initialize all the list we have */
    INIT_LIST_HEAD(&node->nodeHandlerList);

    /* also initialize the packet counter used for headers */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);

    /* calling this will also initialize the routing table */
    ndlcomNodeSetOwnSenderId(node, ownSenderId);

    /* initialize handler which this Node is going to register in the bridge */
    ndlcomBridgeHandlerInit(&node->myIdHandler, ndlcomNodeMessageHandler,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, node);
    /* na, what did I say? */
    ndlcomBridgeRegisterBridgeHandler(bridge, &node->myIdHandler);
}

void ndlcomNodeDeinit(struct NDLComNode *node) {
    /* we do not want to be called anymore in the future */
    ndlcomBridgeDeregisterBridgeHandler(node->bridge, &node->myIdHandler);
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
     * disabling the old senderId, which is still stored in the headerConfig
     */
    ndlcomBridgeClearInternalDeviceId(node->bridge,
                                      node->headerConfig.mOwnSenderId);
    /*
     * resets the packet-counters to use for each receiver, and is used to
     * store our own deviceId at a convenient place
     */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);
    /*
     * NOTE: putting our own "deviceId" into the routingtable. this is a hack,
     * to be able to detect messages going "to us" and not process them in the
     * outgoing side
     *
     * TODO: check that using "bridge" as the origin will work here...
     */
    ndlcomBridgeMarkDeviceIdAsInternal(node->bridge, ownSenderId);
}

void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin) {
    struct NDLComNode *node = (struct NDLComNode *)context;
    const struct NDLComNodeHandler *nodeHandler;
    /*
     * calls our own handlers only for messages directed at "us"
     */
    if ((header->mReceiverId == node->headerConfig.mOwnSenderId) ||
        (header->mReceiverId == NDLCOM_ADDR_BROADCAST)) {
        list_for_each_entry(nodeHandler, &node->nodeHandlerList, list)
        {
            /* NodeHandler will see their own broadcast messages if this is
             * not disabled by a special config-flag...  Messages from the
             * internal side can be detected by checking "origin" to be _not_
             * zero
             *
             * FIXME: the flag is not checked!
             */
            nodeHandler->handler(nodeHandler->context, header, payload,
                                     origin);
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

void ndlcomNodeRegisterNodeHandler(
    struct NDLComNode *node, struct NDLComNodeHandler *nodeHandler) {

    list_add(&nodeHandler->list, &node->nodeHandlerList);
    nodeHandler->node = node;
}

void ndlcomNodeDeregisterNodeHandler(
    struct NDLComNode *node, struct NDLComNodeHandler *nodeHandler) {

    list_del_init(&nodeHandler->list);
    nodeHandler->node = 0;
}
