#include "ndlcom/Bridge.h"

#include "ndlcom/BridgeHandler.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/ExternalInterface.h"
#include "ndlcom/Parser.h"
#include "ndlcom/Routing.h"

/**
 * Size if the temporary rxBuffer which is the block size of data when calling
 * "read()" of the external interfaces during parsing. The size of one encoded
 * message seems like a good guess...
 *
 * NOTE: Could be influenced at compile-time to safe some space on the stack.
 */
#ifndef NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE
#define NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE NDLCOM_MAX_ENCODED_MESSAGE_SIZE
#endif

/**
 * @brief Helper macro written to solve a problem at hand...
 *
 * Problem is that an BridgeHandler (like a "post register value write
 * callback") could decide to remove an existing ExternalInterface from the
 * bridge. Which is bad, it can cause endless loops if the wrong one is removed
 * because it is no longer connected to the rest of the list, but just an
 * "empty linked list" on its own, "next" and "prev" pointing to itself.
 *
 * Ua, I hope this trickery just works...
 */
#define CHECK_LIST_IN_LOOP(first, second, listname)                            \
    if (list_empty(&first->listname)) {                                        \
        first = second;                                                        \
        continue;                                                              \
    } else if (list_empty(&second->listname)) {                                \
        second = first;                                                        \
        continue;                                                              \
    }

/* Helper function. Cements the hack of putting the bridge-pointer itself into
 * the routing table */
static inline int deviceIdIsNotInternallyUsed(const struct NDLComBridge *bridge,
                                              const NDLComId deviceId) {
    return (ndlcomRoutingGetDestination(&bridge->routingTable, deviceId) !=
            bridge);
}

/*
 * After messages where received by an external interface or after they are
 * assembled externally and inserted using "ndlcomBridgeSendRaw()" they pass
 * through this function.
 */
static void
ndlcomBridgeProcessOutgoingMessage(struct NDLComBridge *bridge,
                                   const struct NDLComHeader *header,
                                   const void *payload, void *origin) {
    /* used as loop-variable for the lists */
    struct NDLComExternalInterface *externalInterface;
    /* return-value for the routing table */
    struct NDLComExternalInterface *destination;

    /** Asking the routing table where to forward to */
    destination = (struct NDLComExternalInterface *)ndlcomRoutingGetDestination(
        &bridge->routingTable, header->mReceiverId);
    /**
     * At first the message needs to be encoded. We do not know yet if we have
     * to encode it at all: Receiving a message from the only ExternalInterface
     * for example where the received message would never have to be
     * re-encoded, as it can only be handled internally. This is inefficient
     * but makes the code easier to reason...
     *
     * NOTE: Variable length array, so this will eventually safe some stack?
     */
    uint8_t txBuffer[NDLCOM_MAX_ENCODED_MESSAGE_SIZE_FOR_PACKET(header)];
    size_t len = ndlcomEncode(txBuffer, sizeof(txBuffer), header, payload);

    /**
     * Some ExternalInterface are "mirrors", they want to get _all_ messages,
     * no matter what.
     */
    list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                        list) {
        /* don't echo messages back to their origin */
        if (origin == externalInterface) {
            continue;
        }
        /* only debug-interfaces! */
        if (!(externalInterface->flags &
              NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR)) {
            continue;
        }
        externalInterface->write(externalInterface->context, txBuffer, len);
    }

    /**
     * MS: Suppress further handling iff
     * the origin is not the bridge itself,
     * the destination is not the bridge itself, and
     * forwarding has been disabled
     */
    if (
            (!(bridge->flags & NDLCOM_BRIDGE_FLAGS_FORWARDING_ENABLED))
            && (origin != (void *)bridge)
            && (destination != (void *)bridge)
       )
        return;

    /**
     * First case: Broadcast or unknown destination. We are asked to send to
     * all known interfaces excluding the originating interface of the message.
     */
    if (destination == NDLCOM_ROUTING_ALL_INTERFACES) {
        /* loop through all interfaces */
        list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                            list) {
            /* don't echo messages back to their originating interface. */
            if (origin == externalInterface) {
                continue;
            }
            /* skip debug interfaces, they already got the message! */
            if (externalInterface->flags &
                NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR) {
                continue;
            }
            /* finally, write the shit: */
            externalInterface->write(externalInterface->context, txBuffer, len);
        }
    }
    /**
     * Second case: Knowing the exact interface where to send the message to.
     */
    else {
        if (destination == origin) {
            /**
             * When throwing random messages at one interface which is
             * connected to the bridge, the routing table will slowly
             * accumulate this interface for every deviceId, as messages from
             * all senders seem to originate from this interface. So after some
             * time, it is responsible for every deviceId.
             *
             * The respective entries cannot be updated as long as all the
             * other interfaces do not receive random messages. Spoils random
             * testing a bit.
             *
             * This also happens when a device changes its location in the
             * network updating the routing table after receiving this message
             * in the first place and reaching here during handling. So
             * virtually never...
             */
        } else if (destination == (void *)bridge) {
            /**
             * Do _not_ try to "write" a message destined for us at the
             * "destination" pointer. Works by having our "deviceId" inside the
             * routing table. The message was hopefully handled at some earlier
             * stage.
             *
             * This specific if-branch indicates a problem somewhere else.
             */
        } else {
            /**
             * Finally write the ExternalInterface using its function pointer.
             */
            destination->write(destination->context, txBuffer, len);
        }
    }
}

/**
 * Called for messages which:
 * - where successfully received from an external interface, after the update
 *   to the routing table
 * - after a new message is summoned in "ndlcomBridgeSendRaw()", from the
 *   internal side directed at the world
 *
 * NOTE: "origin" can be either be a pointer to one of the external interfaces
 * or the "bridge" pointer itself, if it comes from internal.
 */
static void ndlcomBridgeProcessDecodedMessage(struct NDLComBridge *bridge,
                                              const struct NDLComHeader *header,
                                              const void *payload,
                                              void *origin) {
    /* used as loop-variable for the lists */
    struct NDLComBridgeHandler *bridgeHandler, *temp;

    /*
     * First thing to do: forward/transmit outgoing messages on the actual
     * external interfaces. For example: send a broadcast on every interface.
     */
    ndlcomBridgeProcessOutgoingMessage(bridge, header, payload, origin);

    /* call the internal handlers to handle the message */
    list_for_each_entry_safe(bridgeHandler, temp, &bridge->bridgeHandlerList,
                             list) {
        /*
         * Every BridgeHandler can opt-out from seeing messages sent by other
         * callers connected on the internal side. In this case (if the flag is
         * set), we compare the "origin" to be the "bridge" pointer itself to
         * not handle these messages.
         */
        if ((bridgeHandler->flags &
             NDLCOM_BRIDGE_HANDLER_FLAGS_NO_MESSAGES_FROM_INTERNAL) &&
            (origin == bridge)) {
            continue;
        }
        /* will pass NULL if the origin was "internal", eg "bridge" */
        // TODO: may change signature of "origin" to be a promised pointer to the actual NDLComExternalInterface
        bridgeHandler->handler(bridgeHandler->context, header, payload,
                               origin == bridge ? NULL : origin);
        /* guard against removal of handlers by other handlers... */
        CHECK_LIST_IN_LOOP(bridgeHandler, temp, list);
    }
}

/* reading and parsing bytes from one ExternalInterface */
static size_t ndlcomBridgeProcessExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {

    /* some variables we'll need later */
    uint8_t rawReadBuffer[NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE];
    size_t bytesRead;
    size_t bytesProcessed = 0;
    const struct NDLComHeader *header;
    const void *payload;

    bytesRead = externalInterface->read(externalInterface->context,
                                        rawReadBuffer, sizeof(rawReadBuffer));

    do {
        bytesProcessed += ndlcomParserReceive(&externalInterface->parser,
                                              rawReadBuffer + bytesProcessed,
                                              bytesRead - bytesProcessed);

        if (ndlcomParserHasPacket(&externalInterface->parser)) {

            header = ndlcomParserGetHeader(&externalInterface->parser);
            payload = ndlcomParserGetPacket(&externalInterface->parser);

            /*
             * Got a packet!
             *
             * Only update the routing table when processing
             * non-debug-ports with the pointer to the external interface
             * where the message came from.
             *
             * Updating the table before processing the message allows
             * readily responding on the right interface.
             *
             * TODO: take care that no one can override "deviceIds" in the
             * RoutingTable which are actually used by a node from "us".
             */
            if (!(externalInterface->flags &
                  NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR)) {
                /* additionally guard the deviceIds consided as "internal"
                 * from accidental update from outside. this can only
                 * happen if there is a rouge device claiming to be one of
                 * our Nodes */
                if (deviceIdIsNotInternallyUsed(bridge, header->mSenderId)) {
                    ndlcomRoutingTableUpdate(&bridge->routingTable,
                                             header->mSenderId,
                                             externalInterface);
                }
            }
            /*
             * Try to forward the message. We are not sure if we can, yet.
             */
            ndlcomBridgeProcessDecodedMessage(bridge, header, payload,
                                              externalInterface);
            /*
             * Clean up the parser of this interface, to be ready for the
             * next packet.
             */
            ndlcomParserDestroyPacket(&externalInterface->parser);

            /**
             * Problem: If some BridgeHandler deregistered this
             * ExternalInterface, we should not proceed parsing any residual
             * bytes, mayhem would ensue: The interface-pointer will be
             * reinserted into the routing table!
             *
             * So we need to double-check if this interface is still connected
             * to the ExternalInterface list. It is not connected if the
             * ExternalInterface itself forms an empty list, as list_del_init()
             * is called below.
             */
            if (list_empty(&externalInterface->list)) {
                break;
            }
        }

    } while (bytesRead != bytesProcessed);
    return bytesProcessed;
}

void ndlcomBridgeInit(struct NDLComBridge *bridge) {

    /* Initialize all the lists we have */
    INIT_LIST_HEAD(&bridge->bridgeHandlerList);
    INIT_LIST_HEAD(&bridge->externalInterfaceList);

    /* And initialize the RoutingTable */
    ndlcomRoutingTableInit(&bridge->routingTable);

    /* Per default, enable forwarding */
    bridge->flags = NDLCOM_BRIDGE_FLAGS_FORWARDING_ENABLED;
}

void ndlcomBridgeSetFlags(struct NDLComBridge *bridge, const uint8_t flags)
{
    bridge->flags = flags;
}

/* inserting new messages into the bridge */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header,
                         const void *payload) {
    /*
     * Inserting new messages into the bridge. By using the "bridge" itself as
     * origin, we can later detect messages which are not coming from one of
     * the external interfaces.
     *
     * NOTE: We could also call "ndlcomBridgeProcessOutgoingMessage()" instead.
     * This would prevent internal interfaces from being able to see messages
     * originating from inside...
     */
    ndlcomBridgeProcessDecodedMessage(bridge, header, payload, bridge);
}

/*
 * The "main" processing function: trigger processing all interfaces as long as
 * data is available. Handle all detected messages.
 *
 * There are two kind of interfaces:
 * - the ordinary ones susceptible to routing and everything
 * - the "debug mirrors", which get _all_ messages
 */
size_t ndlcomBridgeProcess(struct NDLComBridge *bridge) {
    size_t bytesReadOverall = 0;
    size_t bytesRead;
    do {
        bytesRead = ndlcomBridgeProcessOnce(bridge);
        bytesReadOverall += bytesRead;
    } while (bytesRead > 0);
    return bytesReadOverall;
}

/*
 * query all interfaces for data available and process these chunks. return
 * afterwards.
 */
size_t ndlcomBridgeProcessOnce(struct NDLComBridge *bridge) {

    size_t bytesReadOverall = 0;
    struct NDLComExternalInterface *externalInterface, *temp;
    list_for_each_entry_safe(externalInterface, temp,
                             &bridge->externalInterfaceList, list) {
        bytesReadOverall +=
            ndlcomBridgeProcessExternalInterface(bridge, externalInterface);
        /* guard against removal of handlers by other handlers... */
        CHECK_LIST_IN_LOOP(externalInterface, temp, list);
    }
    return bytesReadOverall;
}

void ndlcomBridgeAddRoutingInformationForDeviceId(
    struct NDLComBridge *bridge, const NDLComId deviceId,
    struct NDLComExternalInterface *externalInterface) {

    ndlcomRoutingTableUpdate(&bridge->routingTable, deviceId,
                             externalInterface);
}

void ndlcomBridgeMarkDeviceIdAsInternal(struct NDLComBridge *bridge,
                                        const NDLComId deviceId) {

    ndlcomRoutingTableUpdate(&bridge->routingTable, deviceId, bridge);
}

void ndlcomBridgeClearInternalDeviceId(struct NDLComBridge *bridge,
                                       const NDLComId deviceId) {
    ndlcomRoutingTableUpdate(&bridge->routingTable, deviceId,
                             NDLCOM_ROUTING_ALL_INTERFACES);
}

void
ndlcomBridgeRegisterBridgeHandler(struct NDLComBridge *bridge,
                                  struct NDLComBridgeHandler *bridgeHandler) {
    /* check that the given handler is not yet part of the bridge */
    if (ndlcomBridgeCheckBridgeHandler(bridge, bridgeHandler)) {
        return;
    }
    /* and now we can add it */
    list_add(&bridgeHandler->list, &bridge->bridgeHandlerList);
    bridgeHandler->bridge = bridge;
}

void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {
    /* check that the given handler is not yet part of the bridge */
    if (ndlcomBridgeCheckExternalInterface(bridge, externalInterface)) {
        return;
    }
    /* and now we can add it */
    list_add(&externalInterface->list, &bridge->externalInterfaceList);
    externalInterface->bridge = bridge;
}

void
ndlcomBridgeDeregisterBridgeHandler(struct NDLComBridge *bridge,
                                    struct NDLComBridgeHandler *bridgeHandler) {
    /* check that the given handler is really part of the bridge */
    if (!ndlcomBridgeCheckBridgeHandler(bridge, bridgeHandler)) {
        return;
    }
    /* and now we can delete it */
    list_del_init(&bridgeHandler->list);
    bridgeHandler->bridge = 0;
}

void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {
    /* check that the given interface is really part of the bridge */
    if (!ndlcomBridgeCheckExternalInterface(bridge, externalInterface)) {
        return;
    }
    /* at first remove the known destination from the routing table */
    ndlcomRoutingTableInvalidateInterface(&bridge->routingTable,
                                          externalInterface);
    /* and now we can delete it */
    list_del_init(&externalInterface->list);
    externalInterface->bridge = 0;
}

uint8_t ndlcomBridgeCheckExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {
    /* iterate all interfaces, check if the one in the argument is present */
    struct NDLComExternalInterface *it;
    list_for_each_entry(it, &bridge->externalInterfaceList, list) {
        if (it == externalInterface) {
            return 1;
        }
    }
    return 0;
}

uint8_t
ndlcomBridgeCheckBridgeHandler(struct NDLComBridge *bridge,
                               struct NDLComBridgeHandler *bridgeHandler) {
    /* iterate all handlers, check if the one in the argument is present */
    struct NDLComBridgeHandler *it;
    list_for_each_entry(it, &bridge->bridgeHandlerList, list) {
        if (it == bridgeHandler) {
            return 1;
        }
    }
    return 0;
}
