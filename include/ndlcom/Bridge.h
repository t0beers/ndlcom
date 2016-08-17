#ifndef NDLCOM_BRIDGE_H
#define NDLCOM_BRIDGE_H

#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"
#include "ndlcom/ExternalInterface.h"
#include "ndlcom/InternalHandler.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * A flag which specifies whether forwarding is enabled or not
 */
#define NDLCOM_BRIDGE_FLAGS_FORWARDING_ENABLED 0x01

/**
 * @brief Encapsulate sending, receiving and routing of NDLCom messages
 *
 * A NDLComBridge has one RoutingTable and a number of ExternalInterfaces as
 * well as InternalHandlers.
 *
 * All processing is done after calling a single-point-of-entry. Implementers
 * have to provide function-pointers to non-blocking IO for actually reading
 * and writing the escaped byte-streams from the hardware.
 *
 * ExternalInterfaces use their "write" and "read" functions to handle a raw
 * byte-stream consisting of fully escaped NDLCom messages. InternalHandlers
 * provide a "handle" function which is called for each decoded message with
 * the "header,payload".
 *
 * There are special ExternalInterfaces called "mirrors" which get a copy
 * of every message passing through the bridge. Messages originating from a
 * "mirror" are not used for updating the RoutingTable. They are useful for
 * debugging the complete data streaming through a bridge or for log file
 * record/replay.
 *
 * Each valid message received by one of the ExternalInterfaces is seen by
 * every InternalHandler. To give a bridge a "personality" so that it only
 * listens to messages directed at its own "deviceId" (and all broadcasts) use
 * a NDLComNode, see "ndlcom/Node.h".
 *
 * NOTE: No cut-through, no forwarding of still escaped bytes! Routing and
 * handling of messages _always_ leads to complete de-escaping them, and the
 * routing and forwarding is then done in the "header,payload" form. After the
 * destination is known, the messages is escaped again and then transmitted on
 * the respective interface(s).
 *
 * If the routing table has no entry for a given "receiverId", the
 * message is transmitted on all known external interfaces.
 *
 * NOTE: The routing table will contain pointers to the respective
 * ExternalInterface data structure, to be able to call the provided function
 * pointers. There is the additional special-case for messages directed at the
 * bridge itself which store the "bridge" pointer inside the routing table.
 * This is a hack at best.
 */
struct NDLComBridge {
    /** The whole bridge has one RoutingTable */
    struct NDLComRoutingTable routingTable;
    /**
     * These handlers are called for _each_ single message after it was
     * decoded, prior to being forwarded.
     */
    struct list_head internalHandlerList;
    /**
     * These are the actual hardware interfaces which are used to
     * receive/transmit an escaped byte stream to/from the real world. The
     * "senderId" of messages received on one of these interfaces is used to
     * update the routing table.
     *
     * Contains entries for debug-interfaces and normal interfaces.
     */
    struct list_head externalInterfaceList;
    /**
     * Current set of flags which are handled by the bridge functions/processes
     */
    uint8_t flags;
};

/**
 * @brief Initializes the bridge data structure
 *
 * Clears out the linked-lists and initializes the RoutingTable.
 * NOTE: Per default, forwarding is enabled
 *
 * @param bridge Pointer to the bridge which shall be initialized.
 */
void ndlcomBridgeInit(struct NDLComBridge *bridge);

/**
 * @brief Sets the flags of the bridge
 *
 * Currently, the only flag is the flag to enable/disable forwarding
 *
 * @param bridge The bridge to use
 * @param flags The flags to be set
 */
void ndlcomBridgeSetFlags(struct NDLComBridge *bridge, const uint8_t flags);

/**
 * @brief Encode and transmit messages to the outside
 *
 * Uses the routing table to determine which interface to use. If the
 * destination for the "receiverId" in the given NDLComHeader is unknown, the
 * message will be sent on every external interface.
 *
 * NOTE: The messages written here will be seen by the internal handlers as
 * well (after they where written out to the correct external interfaces). Be
 * carefull to not have a internal handler responding to its own message!
 *
 * NOTE: The packet length in the given header is used to copy data from the
 * given pointer.
 *
 * @param bridge The bridge to use
 * @param header The message header, prepared with packet counter and length
 * @param payload The memory containing actual payload
 */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header,
                         const void *payload);

/**
 * @brief Process all data which can be read on each interface
 *
 * Keeps processing as long as data can be read from any interface. When this
 * function returns, the incoming queues of every interface are empty
 *
 * @param bridge The bridge to process
 * @return The sum of all bytes which where processed. When returning from this
 *         function, no bytes are waiting to be processed.
 */
size_t ndlcomBridgeProcess(struct NDLComBridge *bridge);

/**
 * @brief Process each interface of the bridge once
 *
 * Calls "read()" for every interface and processes eventually resulting
 * packets. This function will return in a finite amount of time, but there
 * might still be data available.
 *
 * @param bridge The bridge to process
 * @return The sum of all bytes which where processed by the bridge. Call this
 *         function again if this is greater than zero, as this means there
 *         might still be bytes waiting.
 */
size_t ndlcomBridgeProcessOnce(struct NDLComBridge *bridge);

/**
 * @brief Tell the bridge about deviceIds used by an InternalHandler
 *
 * Messages to this deviceId are not longer considered as "unknown destinations"
 * and not forwarded to external interfaces anymore. They are supposed to not
 * be transmitted to the outside.
 *
 * @param bridge The bridge to work on
 * @param deviceId The deviceId which belongs to an InternalHandler
 */
void ndlcomBridgeMarkDeviceIdAsInternal(struct NDLComBridge *bridge,
                                        const NDLComId deviceId);

/**
 * @brief Clear a previously internally used deviceId
 *
 * The given deviceId was used connected to an InternalHandler, probably via
 * an NDLComNode, and will no longer be used. This function will set the
 * destination in the RoutingTable from "internal" to "unknown" again.
 *
 * @param bridge The bridge to work on
 * @param deviceId The previously "internally" used deviceId
 */
void ndlcomBridgeClearInternalDeviceId(struct NDLComBridge *bridge,
                                       const NDLComId deviceId);

/**
 * @brief Register additional InternalHandlers
 *
 * Does nothing if the interface is already part of the bridge
 *
 * @param bridge The bridge to use
 * @param internalHandler The InternalHandler to register
 */
void ndlcomBridgeRegisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *internalHandler);

/**
 * @brief Register additional ExternalInterfaces
 *
 * Does nothing if the ExternalInterface is already part of the bridge
 *
 * @param bridge The bridge to use
 * @param externalInterface The ExternalInterface to register
 */
void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

/**
 * @brief Remove existing InternalHandler
 *
 * does nothing if the handler is not part of the bridge
 *
 * @param bridge The bridge to use
 * @param internalHandler The InternalHandler to deregister
 */
void ndlcomBridgeDeregisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *internalHandler);

/**
 * @brief Remove existing ExternalInterface
 *
 * does nothing if the interface is not part of the bridge
 *
 * @param bridge The bridge to use
 * @param externalInterface The ExternalInterface to deregister
 */
void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

/**
 * @brief Check if an InternalHandler is part of a bridge
 *
 * @param bridge The bridge to use
 * @param internalHandler The handler to check
 * @return true if InternalHandler is already registered
 */
uint8_t ndlcomBridgeCheckInternalHandler(
    struct NDLComBridge *bridge,
    struct NDLComInternalHandler *internalHandler);

/**
 * @brief Check if an ExternalInterface is part of a bridge
 *
 * @param bridge The bridge to use
 * @param externalInterface The interface to check
 * @return true if ExternalInterface is already registered
 */
uint8_t ndlcomBridgeCheckExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_BRIDGE_H*/
