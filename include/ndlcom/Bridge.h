#ifndef NDLCOM_BRIDGE_H
#define NDLCOM_BRIDGE_H

#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"
#include "ndlcom/Interfaces.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief encapsulate routing and handling of NDLCom messages
 *
 * A bridge has a number of internal and external interfaces. A bridge has one
 * routing table and one senderId (personality)
 *
 * todo:
 * - nice testing would be two bridges in one programm, talking to each other.
 *   use this for benchmarking and stress testing...
 * benchmarking:
 * - need commandline options for "multiInterfaceTest"
 * - measure average _user_ time spent in "process" via commandline option?
 * - time is strongly kernel dependent...
 * - shell script sets up some bridge processes with pipes, connect pipes via
 *   "tail -f"
 * - fix this strange "pipe does only work after bridge restart" problem...
 *
 * questions:
 * - does a bridge have one and only one ownSenderId?
 * - does a bridge have one and only one receiverId? maybe we wanna listen to
 *   more than one...
 *
 * design decisions
 * - receiving of packages _always_ leads to complete de-escaping them.
 *   forwarding is done in the "header,payload" form. no cut-through
 *   forwarding of escaped bytes...
 * - always keep embedded/bare-metal in mind: try to avoid calls to
 *   malloc!
 * - processing done after single-point-of-entry, no threading, non-blocking
 *   IO... effectively polling the interfaces...
 * - iff this sticks to c++, go through the code and root-out exceptions by
 *   enforcing "noexcept"
 */
struct NDLComBridge {

    /* the whole bridge has one global RoutingTable */
    struct NDLComRoutingTable routingTable;
    /**
     * it also encodes correct packetCounters. this struct is also used to
     * store the ownSenderId.
     */
    struct NDLComHeaderConfig headerConfig;

    struct listI_t {
        struct NDLComInternalHandler* handler;
        struct list_t* next;
    } listI;
    struct listE_t {
        struct NDLComExternalInterface* interface;
        struct list_t* next;
    } listE;

    /* and linked lists of internal and external interfaces */
    struct NDLComInternalHandler *internalInterfaces;
    struct NDLComExternalInterface *externalInterfaces;
};

/**
 * @brief initializes the datastructure
 *
 * @param bridge
 * @param ownSenderId
 */
void ndlcomBridgeInit(struct NDLComBridge *bridge, const NDLComId ownSenderId);

/**
 * @brief create a new package and send it away
 *
 * sending messages from the internal side to whomever they may concern.
 * automatically fill header.
 *
 * creates a new header and puts it with the still unencoded payload into the
 * ndlcomBridgeSendRaw() function
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
 * NOTE: the messages written here will not be seen from any of the internal
 * handlers. they go straight out to the external interfaces
 *
 * @param bridge
 * @param receiverId
 * @param payload
 * @param payloadSize
 */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header, const void *payload,
                         const size_t size);

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

/* TODO: */
void ndlcomBridgeDeregisterInternalHandler(
    struct NDLComBridge *bridge, const struct NDLComInternalHandler *interface);
void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    const struct NDLComExternalInterface *interface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_BRIDGE_H*/
