#ifndef NDLCOM_NODE_H
#define NDLCOM_NODE_H

#include "ndlcom/Bridge.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief a "NDLComNode" has a "deviceId" and is connected to a NDLComBridge
 *
 * Every active participant in a NDLCom network has a "deviceId" which is used
 * in the senderId in the message header. The Node is responsible to correctly
 * increase the packet counter in the header for every new message transmitted.
 *
 * Additionally a Node can receive messages directed at its "deviceId" (or the
 * special broadcast messages). Therefore, the Node will register a handler in
 * the bridge and decide for every message if its own handlers should be
 * called.
 *
 * A Node can be only be connected to one single NDLComBridge.
 *
 */
struct NDLComNode {
    /**
     * The bridge used by this node.
     */
    struct NDLComBridge *bridge;
    /**
     * The Node is also responsible to create headers with correct packet
     * counters. This structure is also used to store the "ownSenderId".
     */
    struct NDLComHeaderConfig headerConfig;
    /**
     * These are called for _each_ single message intended for us after it was
     * decoded, prior to being forwarded.
     */
    struct list_head internalHandlerList;
    /**
     * Handler which will be registered at the NDLComBridge and is used to
     * filter out messages not directed at us. This handler in turn will call
     * all the handlers in our own list for message directed at us.
     */
    struct NDLComInternalHandler myIdHandler;
};

/**
 * @brief Initializes the data structure
 *
 * @param node pointer to "struct NDLComNode" which has to be initialized.
 * @param bridge Pointer to the bridge this Node should connect to
 * @param ownSenderId provide an "NDLComId" during initialization
 */
void ndlcomNodeInit(struct NDLComNode *node, struct NDLComBridge *bridge,
                    const NDLComId ownSenderId);

/**
 * @brief Remove the callback of this Node from the bridges list
 *
 * Since we register at the "bridge", we have to unregister as well... We also
 * have to disable the entry in the routing table.
 *
 * @param node pointer to our data structure
 */
void ndlcomNodeDeinit(struct NDLComNode *node);

/**
 * @brief Change the "deviceId" of the given node used for new packages
 *
 * Re-initializes the routing table of the linked bridge as well.
 *
 * @param node pointer to the data structure to use
 * @param ownSenderId The new NDLComId to be used by the given Node
 */
void ndlcomNodeSetOwnSenderId(struct NDLComNode *node,
                              const NDLComId ownSenderId);

/**
 * @brief Create a new package and send it away
 *
 * Sending messages from the internal side to whomever they may concern.
 * automatically creates and fills header.
 *
 * NOTE: Messages going through a bridge will be seen by all internal handlers.
 * Since we have a general handler in the bridge which is also listening to
 * Broadcasts, every broadcast sent here will be seen by the handlers
 * registered in this Node.
 *
 * @param node
 * @param receiverId
 * @param payload
 * @param payloadSize
 */
void ndlcomNodeSend(struct NDLComNode *node, const NDLComId receiverId,
                    const void *payload, const size_t payloadSize);

/**
 * @brief Register internal handler
 *
 * Will be called for every messages directed at this Node
 *
 * @param node
 * @param interface
 */
void ndlcomNodeRegisterInternalHandler(
    struct NDLComNode *node, struct NDLComInternalHandler *internalHandler);
/**
 * @brief De-register internal handler
 *
 * @param node
 * @param interface
 */
void ndlcomNodeDeregisterInternalHandler(
    struct NDLComNode *node, struct NDLComInternalHandler *internalHandler);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_NODE_H*/
