#ifndef NDLCOM_NODE_H
#define NDLCOM_NODE_H

#include <stddef.h>

#include "ndlcom/Bridge.h"
#include "ndlcom/BridgeHandler.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Types.h"
#include "ndlcom/list.h"

struct NDLComNodeHandler;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief a NDLComNode has its own NDLComId and is connected to an NDLComBridge
 *
 * Every active participant in a NDLCom network needs its own NDLComId which is
 * used as the NDLComHeader::mSenderId in the when transmitting a message.  The
 * sender of a message is also responsible to correctly increase the packet
 * counter in NDLComHeader::mCounter for every single new message transmitted to
 * a specific receiver. Finally, the node has to be able to act on messages
 * directed at its own NDLComId, and additionally the special broadcast address,
 * NDLCOM_ADDR_BROADCAST. All this is provided by the NDLComNode.
 *
 * An NDLComNode is wrapped around a NDLComBridgeHandler and registers at an
 * NDLComBridge. It uses the the NDLComBridgeHandler::handler callback to decide
 * for every passing message if its own handlers, kept in
 * NDLComNode::nodeHandlerList have to be called.
 *
 * Naturally, an NDLComNode can be only be connected to one single NDLComBridge.
 */
struct NDLComNode {
    /**
     * The Node is also responsible to create headers with correct packet
     * counters. This structure is also used to store the own NDLComId as
     * NDLComHeaderConfig::ownSenderId
     */
    struct NDLComHeaderConfig headerConfig;
    /**
     * These handlers are called for _each_ single message directed at its own
     * NDLComId, and broadcasts, after it was decoded, prior to possibly being
     * forwarded on external interfaces.
     */
    struct list_head nodeHandlerList;
    /**
     * The NDLComBridgeHandler which will be registered at the NDLComBridge and
     * is used to filter out messages not directed at us. This handler will then
     * call all the handlers in our NDLComNode::nodeHandlerList for message
     * directed at us.
     *
     * NOTE: This struct provides a pointer to the attached NDLComBridge. Is
     * this still needed?
     */
    struct NDLComBridgeHandler bridgeHandler;
};

/**
 * @brief Initializes the data structure
 *
 * Note: In order to be used by the NDLComBridge, you still have to call
 * ndlcomNodeRegister() and provide a suitable pointer after initialization.
 *
 * @param node pointer to an NDLComNode which has to be initialized.
 * @param ownSenderId provide a NDLComId during initialization to be used as
 *                    our personality
 */
void ndlcomNodeInit(struct NDLComNode *node, const NDLComId ownSenderId);

/**
 * @brief Attach the NDLComBridgeHandler inside this struct to the NDLComBridge
 *
 * Will also mark our own deviceId as "internally used" to prevent routing
 * of incoming messages directed at us.
 *
 * @param node pointer to our data structure
 * @param bridge pointer to the bridge where to connect to
 */
void ndlcomNodeRegister(struct NDLComNode *node, struct NDLComBridge *bridge);

/**
 * @brief Remove the callback of this NDLComNode from the list in NDLComBridge
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
 * @brief query the current deviceId
 *
 * @param node the NDLComNode to query
 * @return the currently used NDLComId of the given node
 */
NDLComId ndlcomNodeGetOwnDeviceId(const struct NDLComNode* node);

/**
 * @brief Create a new package and send it away
 *
 * Sending messages from the internal side to whomever they may concern.
 * automatically creates and fills NDLComHeader.
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
 * Add a new NDLComNodeHandler to the internal list of this node. The
 * NDLComNodeHandler::handler will be called for every message directed at this
 * node.
 *
 * @param node The node to register at
 * @param nodeHandler The handler which is to be registered
 */
void ndlcomNodeRegisterNodeHandler(struct NDLComNode *node,
                                   struct NDLComNodeHandler *nodeHandler);
/**
 * @brief Deregister node handler
 *
 * Remove the NDLComNodeHandler from the list of handlers
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
