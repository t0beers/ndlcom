#ifndef NDLCOM_NODE_H
#define NDLCOM_NODE_H

#include "ndlcom/Bridge.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct NDLComNode {
    /**
     * the bridge used by this node.
     */
    struct NDLComBridge* bridge;
    /**
     * it also encodes correct packetCounters. this struct is also used to
     * store the ownSenderId.
     */
    struct NDLComHeaderConfig headerConfig;
    /**
     * these are called for _each_ single message intended for us after it was
     * decoded, prior to beeing forwarded.
     */
    struct list_head internalHandlerList;
    /**
     * handler which will be registered at the bridge and used to filter out
     * messages not directed at us. will call all the handlers in our own
     * list for message directed at us.
     */
    struct NDLComInternalHandler myIdHandler;
};

/**
 * @brief initializes the datastructure
 *
 * @param node pointer to "struct NDLComBridge" which has to be initialized.
 * @param ownSenderId provide an "NDLComId" during initialization. can be
 * changed later.
 */
void ndlcomNodeInit(struct NDLComNode *node, struct NDLComBridge *bridge,
                    const NDLComId ownSenderId);

/**
 * @brief remove this strucs callback from the bridges list
 *
 * since we register at the "bridge", we have to deregister as well... at
 * least in theory
 */
void ndlcomNodeDeinit(struct NDLComNode *node);

/**
 * @brief change the id of the given node used for new packages
 *
 * re-initializes the routing table of the linked bridge as well.
 *
 * @param node
 * @param ownSenderId
 */
void ndlcomNodeSetOwnSenderId(struct NDLComNode *node,
                              const NDLComId ownSenderId);

/**
 * @brief create a new package and send it away
 *
 * sending messages from the internal side to whomever they may concern.
 * automatically creates and fills header.
 *
 * NOTE: messages will be seen by all internal handlers. this behaviour can be
 * changed, though.
 *
 * @param node
 * @param receiverId
 * @param payload
 * @param payloadSize
 */
void ndlcomNodeSend(struct NDLComNode *node, const NDLComId receiverId,
                    const void *payload, const size_t payloadSize);

/**
 * @brief register internal handler
 * @param node
 * @param interface
 */
void ndlcomNodeRegisterInternalHandler(struct NDLComNode *node,
                                       struct NDLComInternalHandler *handler);
/**
 * @brief deregister internal handler
 * @param node
 * @param interface
 */
void ndlcomNodeDeregisterInternalHandler(struct NDLComNode *node,
                                         struct NDLComInternalHandler *handler);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_NODE_H*/
