#include "ndlcom/Node.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"

/* forward declaration */
void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin);

void ndlcomNodeInit(struct NDLComNode *node,
                    const NDLComId ownSenderId) {

    /* initialize all the list we have */
    INIT_LIST_HEAD(&node->nodeHandlerList);

    /* also initialize the packet counter used for headers */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);

    /* initialize handler which this Node is going to register in the bridge */
    ndlcomBridgeHandlerInit(&node->bridgeHandler, ndlcomNodeMessageHandler,
                            NDLCOM_BRIDGE_HANDLER_FLAGS_DEFAULT, node);

    /* initialize our own NDLComHeaderConfig */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);
}

/* register our own NDLComBridgeHandler at the NDLComBridge */
void ndlcomNodeRegister(struct NDLComNode *node, struct NDLComBridge *bridge) {
    /* na, what did I say? */
    ndlcomBridgeRegisterBridgeHandler(bridge, &node->bridgeHandler);
    /*
     * putting our own "deviceId" into the routingtable. this is a hack in the
     * NDLComBridge, to be able to detect messages which have to go "to us" and
     * not process them in the outgoing side
     */
    ndlcomBridgeMarkDeviceIdAsInternal(node->bridgeHandler.bridge,
                                       node->headerConfig.mOwnSenderId);
}

/* disconnect our handler form the bridge */
void ndlcomNodeDeregister(struct NDLComNode *node) {
    /* inform the bridge that deviceId is no longer to be used internally */
    ndlcomBridgeClearInternalDeviceId(node->bridgeHandler.bridge,
                                      node->headerConfig.mOwnSenderId);
    /* we do not want to be called by anymore in the future */
    ndlcomBridgeDeregisterBridgeHandler(node->bridgeHandler.bridge,
                                        &node->bridgeHandler);
}

/* Initializes internal structs with correct values.  */
void ndlcomNodeSetOwnSenderId(struct NDLComNode *node,
                              const NDLComId ownSenderId) {
    /* inform the bridge that deviceId is no longer to be used internally */
    ndlcomBridgeClearInternalDeviceId(node->bridgeHandler.bridge,
                                      node->headerConfig.mOwnSenderId);
    /*
     * Since we are asked to become a new personality we have to reset the
     * packet-counters to use for each receiver. This function is also used to
     * store our own deviceId at a convenient place
     */
    ndlcomHeaderPrepareInit(&node->headerConfig, ownSenderId);
    /* and update the routing table */
    ndlcomBridgeMarkDeviceIdAsInternal(node->bridgeHandler.bridge,
                                       node->headerConfig.mOwnSenderId);
}

/**
 * Handler will be called by the NDLComBridge, here is where the filtering
 * for messages directed at "us" happens: Our receiverId and the Broadcast.
 */
void ndlcomNodeMessageHandler(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin) {
    struct NDLComNode *node = (struct NDLComNode *)context;
    const struct NDLComNodeHandler *nodeHandler;
    /*
     * calls our own handlers only for messages directed at "us"
     */
    if ((header->mReceiverId == node->headerConfig.mOwnSenderId) ||
        (header->mReceiverId == NDLCOM_ADDR_BROADCAST)) {
        list_for_each_entry(nodeHandler, &node->nodeHandlerList, list) {
            /**
             * NDLComNodeHandler would see their own messages if this is
             * not disabled by a special config-flag... Messages from the
             * internal side can be detected by checking "origin" to be _not_
             * zero.
             */
            if (!origin &&
                (nodeHandler->flags &
                 NDLCOM_NODE_HANDLER_FLAGS_NO_MESSAGES_FROM_INTERNAL)) {
                continue;
            }
            nodeHandler->handler(nodeHandler->context, header, payload, origin);
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
    ndlcomBridgeSendRaw(node->bridgeHandler.bridge, &header, payload);
}

void ndlcomNodeRegisterNodeHandler(struct NDLComNode *node,
                                   struct NDLComNodeHandler *nodeHandler) {

    list_add(&nodeHandler->list, &node->nodeHandlerList);
    nodeHandler->node = node;
}

void ndlcomNodeDeregisterNodeHandler(struct NDLComNode *node,
                                     struct NDLComNodeHandler *nodeHandler) {

    list_del_init(&nodeHandler->list);
    nodeHandler->node = 0;
}
