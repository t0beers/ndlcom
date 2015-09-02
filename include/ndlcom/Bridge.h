#ifndef NDLCOM_BRIDGE_H
#define NDLCOM_BRIDGE_H

#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"
#include "ndlcom/Interfaces.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief encapsulate routing and handling of NDLCom messages
 *
 * A "NDLComBridge" has a number of internal and external interfaces. A
 * "NDLComBridge" has one routing table and one senderId (eg: a personality).
 *
 * TODO:
 * - fix this strange "pipe does only work after bridge restart" problem...
 * - really nice would be a cpp-class for doing the packet-statistics of the
 *   CommStat2 widget. thinking about an ncurses interface for the
 *   ndlcomBridge...
 *
 * open questions:
 * - does a bridge have one and only one ownSenderId?
 * - does it make sense to have a bridge _without_ ownId? and then implement
 *   the whole "own packet handling" as part of an "InternalHandler"...
 *   gut-feeling says: this would lead to problems in routing, where we have to
 *   treat the "internal" stuff special. broadcasts for example? this might
 *   lead to problems when different "internal" things send messages... do they
 *   see each other? should not be the case... or?
 *
 * explicit design decisions
 * - receiving of packages _always_ leads to complete de-escaping them.
 *   forwarding is done in the "header,payload" form. no cut-through
 *   forwarding of still escaped bytes...
 * - always keep embedded/bare-metal in mind: try to avoid calls to
 *   malloc in the "core", keep c++ out
 * - processing done after calling a single-point-of-entry. no threading, users
 *   have to provide non-blocking IO... effectively polling the interfaces...
 */
struct NDLComBridge {

    /** the whole bridge has one global RoutingTable */
    struct NDLComRoutingTable routingTable;
    /**
     * it also encodes correct packetCounters. this struct is also used to
     * store the ownSenderId.
     */
    struct NDLComHeaderConfig headerConfig;

    /**
     * these are called for _each_ single message after it was decoded, prior
     * to beeing forwarded.
     */
    struct list_head internalHandlerList;

    /**
     * only called if the receiver is _our_ node
     */
    struct list_head ownIdInternelHandlerList;

    /**
     * these are the interfaces which are used to receive and transmit bytes
     * from the real world
     */
    struct list_head externalInterfaceList;

    /**
     * used to mirror all messages, independent from the routing table
     */
    struct list_head debugMirrorInterfaceList;
};

/**
 * @brief initializes the datastructure
 *
 * @param bridge pointer to "struct NDLComBridge" which has to be initialized.
 * @param ownSenderId provide an "NDLComId" during initialization. can be changed later.
 */
void ndlcomBridgeInit(struct NDLComBridge *bridge, const NDLComId ownSenderId);

/**
 * @brief change the id of the given bridge used for new packages
 *
 * re-initializes the routing table as well.
 *
 * @param bridge
 * @param ownSenderId
 */
void ndlcomBridgeSetOwnSenderId(struct NDLComBridge *bridge,
                                const NDLComId ownSenderId);

/**
 * @brief create a new package and send it away
 *
 * sending messages from the internal side to whomever they may concern.
 * automatically creates and fills header.
 *
 * NOTE: messages will be seen by all internal handlers.
 *
 * @param bridge
 * @param receiverId
 * @param payload
 * @param payloadSize
 */
void ndlcomBridgeSend(struct NDLComBridge *bridge, const NDLComId receiverId,
                      const void *payload, const size_t payloadSize);

/**
 * @brief put arbritrary messages to external interfaces
 *
 * NOTE: the messages written here will NOT be seen from any of the internal
 * handlers. they go straight out to the external interfaces
 *
 * @param bridge
 * @param receiverId
 * @param payload
 * @param payloadSize
 */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header, const void *payload,
                         const size_t payloadSize);

/**
 * @brief churn the data from all interfaces...
 * @param bridge object to process
 */
void ndlcomBridgeProcess(struct NDLComBridge *bridge);

/**
 * @brief register internal handlers
 * @param bridge
 * @param interface
 */
void ndlcomBridgeRegisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *interface);
/**
 * @brief register external interfaces
 * @param bridge
 * @param interface
 */
void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *interface);

void ndlcomBridgeDeregisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *interface);

void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *interface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_BRIDGE_H*/
