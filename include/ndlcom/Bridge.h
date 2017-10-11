#ifndef NDLCOM_BRIDGE_H
#define NDLCOM_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "ndlcom/Routing.h"
#include "ndlcom/Types.h"
#include "ndlcom/list.h"

struct NDLComBridgeHandler;
struct NDLComExternalInterface;

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
 * A NDLComBridge contains one global NDLComRoutingTable and a number of
 * NDLComExternalInterface for communication with the outside as well as
 * NDLComBridgeHandler for internal handling. The task of a bridge is to decode
 * incoming messages, handle them internally if needed and forward them to the
 * correct outgoing interface if needed. If the routing table has no entry for
 * a given NDLComHeader::mReceiverId, the message is transmitted on all known
 * external interfaces.
 *
 * All processing is done after calling ndlcomBridgeProcess(), which is the
 * single point of entry.
 *
 * For each type of interface implementers have to provide function pointers for
 * NDLComExternalInterface::read and NDLComExternalInterface::write to facilitate
 * non-blocking IO of raw (escaped) byte-streams from hardware. Each
 * NDLComBridgeHandler has to provide a "handle" function which is called for
 * each passing decoded message with the "header"+"payload" of the decoded
 * message.
 *
 * There are special NDLComExternalInterface called "mirrors" which get a copy
 * of every message passing through the bridge. Messages originating from a
 * "mirror" are not used for updating the RoutingTable. They are useful for
 * debugging the complete data streaming through a bridge or for log file
 * record/replay.
 *
 * A special kind of NDLComBridgeHandler is the NDLComNode which gives a bridge
 * a "personality". It filters the passing messages and only messages directed
 * at its own deviceId (and broadcasts) are passed to its own
 * NDLComNodeHandler. The handle function of every node handler is called for
 * each message directed at the owning NDLComNode.
 *
 * ---
 *
 * NOTE: No cut-through routing is performed, there is no forwarding of still
 * escaped bytes! Routing and handling of a message _always_ leads to complete
 * de-escaping, and the routing and forwarding is then done in the
 * "header"+"payload" form.  After the destination is known, the messages is
 * escaped again and then transmitted on the respective interface(s). This is
 * inefficient, but makes the implementation way easier.
 */
struct NDLComBridge {
    /**
     * The whole bridge has one RoutingTable
     */
    struct NDLComRoutingTable routingTable;
    /**
     * These handlers are called for _each_ single message after it was
     * decoded, prior to possibly being forwarded to external interfaces.
     */
    struct list_head bridgeHandlerList;
    /**
     * These are the actual hardware interfaces which are used to
     * receive/transmit a stream of escaped bytes to/from the real world. When
     * a message is received and successfully decoded its NDLCom::mSenderId is
     * used to update the routing table with the external interface where it
     * was being read.
     *
     * Contains entries for mirror interfaces as well as normal interfaces.
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
 * Clears out the linked-lists and initializes the routing table.
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
 * Uses the NDLComRoutingTable to determine which NDLComExternalInterface to
 * use. If the destination named in the NDLComHeader::mReceiverId is unknown,
 * the message will be sent on every external interface.
 *
 * NOTE: The messages written here will be seen by the NDLComBridgeHandler as
 * well (after they where written out to the correct external interfaces). Be
 * carefull to not have a handler responding to its own message!
 *
 * NOTE: The packet length in the NDLComHeader::mDataLen field is used to copy
 * bytes from the given void pointer.
 *
 * NOTE: For messages sent with this function the packet counter in the header
 * will not be prepared correctly. The normal way to send messages is to the
 * ndlcomNodeSend()
 *
 * @param bridge The bridge to use
 * @param header The message header, fully valid and prepared with packet
 *               counter and length
 * @param payload Memory containing the actual payload
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
 * Calls "read()" for every interface once and processes eventually resulting
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
 * @brief Update NDLComRoutingTable with known interface for given deviceId
 *
 * This is helpful in case the bridge will receive a large amount of data
 * directed to a deviceId which is either not active at all or send packets
 * very irregularly. In case the probable location of such a device in the
 * network is known, initializing the routing table will prevent handling all
 * traffic as having an unknown direction. Thus, slower interfaces will not be
 * overloaded with traffic not directed at them.
 *
 * Similar to ndlcomBridgeMarkDeviceIdAsInternal() and friends.
 *
 * @param bridge The bridge to work on
 * @param externalInterface Pointer to the interface where deviceId shall be
 *        reached. Pointer cannot be const as it is stored as "void*" in the
 *        NDLComRoutingTable.
 * @param deviceId The deviceId which is reachable via the given interface
 */
void ndlcomBridgeAddRoutingInformationForDeviceId(
    struct NDLComBridge *bridge, const NDLComId deviceId,
    struct NDLComExternalInterface *externalInterface);

/**
 * @brief Tell the bridge about deviceIds used to send messages from internal
 *
 * Messages directed as this deviceId are not longer considered as having an
 * "unknown destination" and are hence not forwarded to all existing
 * NDLComExternalInterface anymore. This effectivly disables routing of
 * messages for this deviceId to the outside as they will hopefully be handled
 * on the inside. To be used by an NDLComNode.
 *
 * @param bridge The bridge to work on
 * @param deviceId The deviceId which belongs to the internal side
 */
void ndlcomBridgeMarkDeviceIdAsInternal(struct NDLComBridge *bridge,
                                        const NDLComId deviceId);

/**
 * @brief Clear a previously internally used deviceId
 *
 * The given deviceId was used by an internal node like NDLComNode, but is no
 * longer be used. This function will set the destination in the RoutingTable
 * from "internal" to "unknown" again. Messages for this deviceId will be
 * forwarded again.
 *
 * @param bridge The bridge to work on
 * @param deviceId The previously "internally" used deviceId
 */
void ndlcomBridgeClearInternalDeviceId(struct NDLComBridge *bridge,
                                       const NDLComId deviceId);

/**
 * @brief Register additional BridgeHandler
 *
 * Adds the given handler to the internal NDLComBridge::bridgeHandlerList. Does
 * nothing if the handler is already part of the bridge.
 *
 * @param bridge The bridge to use
 * @param bridgeHandler The handler to register
 */
void
ndlcomBridgeRegisterBridgeHandler(struct NDLComBridge *bridge,
                                  struct NDLComBridgeHandler *bridgeHandler);

/**
 * @brief Register additional NDLComExternalInterface
 *
 * Adds the given handler to the internal NDLComBridge::externalInterfaceList.
 * Does nothing if the NDLComExternalInterface is already part of the bridge
 *
 * @param bridge The bridge to use
 * @param externalInterface The interface to register
 */
void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

/**
 * @brief Remove existing BridgeHandler
 *
 * Clears the handler from the linked list. Does nothing if the handler is not
 * part of the bridge
 *
 * @param bridge The bridge to use
 * @param bridgeHandler The handler to deregister
 */
void
ndlcomBridgeDeregisterBridgeHandler(struct NDLComBridge *bridge,
                                    struct NDLComBridgeHandler *bridgeHandler);

/**
 * @brief Remove existing NDLComExternalInterface
 *
 * Clears the handler from the linked list. Does nothing if the interface is
 * not part of the bridge
 *
 * @param bridge The bridge to use
 * @param externalInterface The interface to deregister
 */
void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

/**
 * @brief Check if a handler is part of a bridge
 *
 * Checks if the given handler is contained in NDLComBridge::bridgeHandlerList
 *
 * @param bridge The bridge to use
 * @param bridgeHandler The handler to check
 * @return true if BridgeHandler is already registered
 */
uint8_t
ndlcomBridgeCheckBridgeHandler(struct NDLComBridge *bridge,
                               struct NDLComBridgeHandler *bridgeHandler);

/**
 * @brief Check if an NDLComExternalInterface is part of a bridge
 *
 * Checks if the given interface is contained in
 * NDLComBridge::externalInterfaceList
 *
 * @param bridge The bridge to use
 * @param externalInterface The interface to check
 * @return true if NDLComExternalInterface is already registered
 */
uint8_t ndlcomBridgeCheckExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_BRIDGE_H*/
