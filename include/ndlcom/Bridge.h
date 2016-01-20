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
 * @brief encapsulate routing of NDLCom messages
 *
 * A "NDLComBridge" has one routing table and number of external interfaces and
 * internal handlers.
 *
 * External interfaces are used to "write" and "read" a raw byte-stream with
 * escaped NDLCom messages. Internal handlers have a "handle" function which is
 * called for each decoded message with the "header,payload".
 *
 * There is care special external interfaces called "mirrors" which get a copy
 * of every message passing through the bridge. Messages originating from a
 * "mirror" are not used for updating the routing-table. They are useful for
 * debugging the complete data streaming through a bridge or for log file
 * record/replay.
 *
 * Each valid message received by one of the external interfaces is seen by
 * every internal handler. To give a Bridge a "personality" so that it only
 * listens to messages directed at its own "deviceId" (and broadcasts) see
 * "ndlcom/Node.h".
 *
 * All processing is done after calling a single-point-of-entry. Implementers
 * have to provide function-pointers to non-blocking IO for actually reading
 * and writing the escaped byte-streams from the hardware.
 *
 * NOTE: No cut-through, no forwarding of still escaped bytes! Routing and
 * handling a messages _always_ leads to complete de-escaping them, and the
 * routing and forwarding is then done in the "header,payload" form. After this
 * decision is taken, the messages is escaped again and then transmitted on the
 * respective interfaces.
 *
 * NOTE: If the routing table has no entry for a given "receiverId" the message
 * is transmitted on all known external interfaces.
 *
 * NOTE: The routing table will contain pointers to the respective external
 * interface. There is the additional special-case for messages from an dto the
 * bridge itself which store the "bridge" pointer into the routing table. This
 * is a hack at best.
 *
 * This code is written with the embedded/bare-metal case in mind: There is no
 * dynamic memory involved, no C++ in the core and no multi threading.
 *
 * TODO:
 * - Really nice would be a C++-class for doing the packet-statistics of the
 *   CommStat2 widget. Thinking about an ncurses interface for the
 *   ndlcomBridge...
 *
 */
struct NDLComBridge {
    /** The whole bridge has one global RoutingTable */
    struct NDLComRoutingTable routingTable;
    /**
     * These handlers are called for _each_ single message after it was
     * decoded, prior to being forwarded.
     */
    struct list_head internalHandlerList;
    /**
     * These are the actual interfaces which are used to receive and transmit
     * an escaped byte stream from the real world. The "senderId" of messages
     * received on one of these interfaces is used to update the routing table.
     *
     * Contains entries for debug-interfaces and normal interfaces.
     */
    struct list_head externalInterfaceList;
};

/**
 * @brief Initializes the data structure
 *
 * Clears out the linked-lists and initialized the routing table.
 *
 * @param bridge Pointer to the "struct NDLComBridge" which has to be
 *initialized.
 */
void ndlcomBridgeInit(struct NDLComBridge *bridge);

/**
 * @brief Encode and send messages to external interfaces
 *
 * Uses the routing table to determine which interface to use. If the
 * destination for the "receiverId" in the header is unknown the message will
 * be sent on every external interface.
 *
 * NOTE: The messages written here will be seen by the internal handlers as
 * well (after they where written out to the correct external interfaces). Be
 * carefull to not have a internal handler responding to its own message!
 *
 * @param bridge The object to use
 * @param header The message header, completely prepared
 * @param payload The memory containing actual payload
 * @param payloadSize Should be the same value as the "dataLen" in the header
 */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header,
                         const void *payload);

/**
 * @brief Process and handle all data which can be read on each interface
 *
 * keeps processing as long as data can be read from any interface. when this
 * function returns, the incoming queues of every interface is empty
 *
 * @param bridge Object to process
 * @return the sum of all bytes which where processed. when return from this
 *         function, no bytes are waiting to be processed.
 */
size_t ndlcomBridgeProcess(struct NDLComBridge *bridge);

/**
 * @brief Process each interface of the bridge once
 *
 * calls "read()" for every interface and processes eventually resulting
 * packets. when this function returns, there might still be data available.
 *
 * @param bridge Object to process
 * @return the sum of all bytes which where processed by the bridge. call this
 *         function again if this is greater than zero, as this means there
 *         might still be bytes waiting.
 */
size_t ndlcomBridgeProcessOnce(struct NDLComBridge *bridge);

/**
 * @brief tell the bridge about deviceIds used as internal
 *
 * messages to this deviceId are not longer consided as "unkown destinations"
 * and not forwarded to external interfaces anymore
 *
 * @param bridge
 * @param deviceId
 */
void ndlcomBridgeMarkDeviceIdAsInternal(struct NDLComBridge *bridge,
                                        const NDLComId deviceId);

/**
 * @brief clear a deviceId and make its destination "unknown" again
 *
 * @param bridge
 * @param deviceId
 */
void ndlcomBridgeClearInternalDeviceId(struct NDLComBridge *bridge,
                                       const NDLComId deviceId);

/**
 * @brief Register additional internal handlers
 *
 * @param bridge The bridge to use
 * @param internalHandler The handler to register
 */
void ndlcomBridgeRegisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *internalHandler);

/**
 * @brief Register additional external interfaces
 *
 * @param bridge The bridge to use
 * @param externalInterface The interface to register
 */
void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

void ndlcomBridgeDeregisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *internalHandler);

void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_BRIDGE_H*/
