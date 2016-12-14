#ifndef NDLCOM_NODE_H
#define NDLCOM_NODE_H

#include "ndlcom/Bridge.h"
#include "ndlcom/NodeHandler.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief a NDLComNode has a deviceId and is connected to an NDLComBridge
 *
 * An NDLComNode is wrapped around a NDLComBridgeHandler. Every active
 * participant in a NDLCom network has a "deviceId" which is used as the
 * senderId in the message header when transmitting a message, which is
 * provided by the NDLComNode. Additionally it is responsible to correctly
 * increase the packet counter in the header for every new message transmitted
 * to a specific deviceId.
 *
 * Additionally an NDLComNode is used to handle messages directed at its
 * "deviceId" (and the special broadcast address). Therefore, the NDLComNode
 * registers a NDLComBridgeHandler at the provided bridge and uses the callback
 * handler to decide for every message if its own handlers should be called.
 *
 * A NDLComNode can be only be connected to one single NDLComBridge.
 *
 * Note: There seem to be no sensible way to enforce that a NDLComBridge has
 * only one NDLComNode for a given receiverId... But it might work...
 */
struct NDLComNode {
    /**
     * The Node is also responsible to create headers with correct packet
     * counters. This structure is also used to store the "ownSenderId".
     */
    struct NDLComHeaderConfig headerConfig;
    /**
     * These are called for _each_ single message intended for us after it was
     * decoded, prior to being forwarded.
     */
    struct list_head nodeHandlerList;
    /**
     * Handler which will be registered at the NDLComBridge and is used to
     * filter out messages not directed at us. This handler in turn will call
     * all the handlers in our own list for message directed at us.
     *
     * Note that this struct provides a pointer to the attached NDLComBridge.
     */
    struct NDLComBridgeHandler bridgeHandler;
};

/**
 * @brief Initializes the data structure
 *
 * Note: In order to be used by the NDLComBridge, you still have to call
 * ndlcomNodeRegister() and provide a suitable pointer.
 *
 * @param node pointer to an NDLComNode which has to be initialized.
 * @param ownSenderId provide an deviceId during initialization
 */
void ndlcomNodeInit(struct NDLComNode *node,
                    const NDLComId ownSenderId);

/**
 * @brief attach the NDLComBridgeHandler to the NDLComBridge
 *
 * Will also mark our own deviceId as "internally used" to allow obtaining
 * messages.
 *
 * @param node pointer to our data structure
 */
void ndlcomNodeRegister(struct NDLComNode *node, struct NDLComBridge *bridge);

/**
 * @brief Remove the callback of this Node from the list in NDLComBridge
 *
 * Since we register at a NDLComBridge, we have to unregister as well... We
 * also have to disable the special marking for our deviceId in the routing
 * table.
 *
 * @param node pointer to our data structure
 */
void ndlcomNodeDeregister(struct NDLComNode *node);

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
 * @param node the node to use when sending the message. determines the senderId
 * @param receiverId
 * @param payload
 * @param payloadSize
 */
void ndlcomNodeSend(struct NDLComNode *node, const NDLComId receiverId,
                    const void *payload, const size_t payloadSize);

/**
 * @brief Register node handler
 *
 * Will be called for every messages directed at this Node
 *
 * @param node The node to register at
 * @param nodeHandler The handler which is to be registered
 */
void ndlcomNodeRegisterNodeHandler(struct NDLComNode *node,
                                   struct NDLComNodeHandler *nodeHandler);
/**
 * @brief De-register node handler
 *
 * @param node The node to disconnect from
 * @param nodeHandler The handler to be disconnected
 */
void ndlcomNodeDeregisterNodeHandler(struct NDLComNode *node,
                                     struct NDLComNodeHandler *nodeHandler);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_NODE_H*/
