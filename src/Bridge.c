#include "ndlcom/Bridge.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"

/**
 * Size if the temporary rxBuffer which is the block size of data when calling
 * "read()" of the external interfaces during parsing. The size of one encoded
 * message seems like a good guess...
 *
 * NOTE: Can be influenced at compile-time to safe some space on the stack.
 */
#ifndef NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE
#define NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE NDLCOM_MAX_ENCODED_MESSAGE_SIZE
#endif

void ndlcomBridgeInit(struct NDLComBridge *bridge) {

    /* initialize all the list we have */
    INIT_LIST_HEAD(&bridge->internalHandlerList);
    INIT_LIST_HEAD(&bridge->externalInterfaceList);

    /* initialize the routing table */
    ndlcomRoutingTableInit(&bridge->routingTable);
}

/* Helper function. Cements the hack of putting the bridge-pointer itself into
 * the routing table */
int deviceIdIsNotInternallyUsed(const struct NDLComBridge *bridge,
                                const NDLComId deviceId) {
    return (ndlcomRoutingGetDestination(&bridge->routingTable, deviceId) !=
            bridge);
}

/*
 * After messages where received by an external interface or after they are
 * assembled externally and inserted using "ndlcomBridgeSendRaw()" they pass
 * through this function.
 */
void ndlcomBridgeProcessOutgoingMessage(struct NDLComBridge *bridge,
                                        const struct NDLComHeader *header,
                                        const void *payload, void *origin) {
    /* used as loop-variable for the lists */
    struct NDLComExternalInterface *externalInterface;
    /* return-value for the routing table */
    struct NDLComExternalInterface *destination;

    /*
     * At first the message needs to be encoded. We do not know yet if we have
     * to encode it at all: receiving from only one outgoing interface for
     * example. This is inefficient...
     *
     * NOTE: Variable length array, so this will eventually safe some stack?
     */
    uint8_t txBuffer[NDLCOM_MAX_ENCODED_MESSAGE_SIZE_FOR_PACKET(header)];
    size_t len = ndlcomEncode(txBuffer, sizeof(txBuffer), header, payload);

    /* these external interfaces want to get _all_ messages, no matter what */
    list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                        list) {
        /* don't echo messages back to their origin */
        if (origin != externalInterface) {
            /* only debug-interfaces! */
            if (externalInterface->flags &
                NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR) {
                externalInterface->write(externalInterface->context, txBuffer,
                                         len);
            }
        }
    }

    /* asking the routing table where to forward to */
    destination = (struct NDLComExternalInterface *)ndlcomRoutingGetDestination(
        &bridge->routingTable, header->mReceiverId);

    /*
     * First case: Broadcast or unknown destination. We are asked to send to
     * all known interfaces (except the origin of this message)
     */
    if ((header->mReceiverId == NDLCOM_ADDR_BROADCAST) ||
        (destination == NDLCOM_ROUTING_ALL_INTERFACES)) {
        /* loop through all interfaces */
        list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                            list) {
            /* don't echo messages back to their origin */
            if (origin != externalInterface) {
                /* no debug interfaces! */
                if (!(externalInterface->flags &
                    NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR)) {
                    externalInterface->write(externalInterface->context,
                                             txBuffer, len);
                }
            }
        }
    }
    /*
     * Second case: knowing the exact interface where to send the message to
     */
    else {
        if (destination == origin) {
            /**
             * When throwing random messages at one interface which is
             * connected to the bridge, this interface will slowly accumulate
             * all "deviceIds", as messages from all senders seem to originate
             * from this interface. So after some time, it is responsible for
             * all "receiverIds".
             *
             * The respective entry cannot be updated as long as all the other
             * interfaces do not receive random messages.
             */
        } else if (destination == (void *)bridge) {
            /**
             * Do _not_ try to "write" a messages destined for us at the
             * "destination" pointer. Works by having our "deviceId" inside the
             * routing table. The message was hopefully handled at some earlier
             * stage.
             */
        } else {
            /**
             * Otherwise, finally write to the external interface!
             */
            destination->write(destination->context, txBuffer, len);
        }
    }
}

/**
 * Called for messages which:
 * - where successfully received from an external interface, after the update
 *   to the routing table
 * - after a new message is summoned in "ndlcomBridgeSendRaw()"
 *
 * NOTE: "origin" can be either be a pointer to one of the external interfaces
 * or the "bridge" pointer itself, if it comes from internal.
 */
void ndlcomBridgeProcessDecodedMessage(struct NDLComBridge *bridge,
                                       const struct NDLComHeader *header,
                                       const void *payload, void *origin) {
    /* used as loop-variable for the lists */
    struct NDLComInternalHandler *internalHandler;

    /*
     * First thing to do: forward/transmit outgoing messages on the actual
     * external interfaces. For example: send a broadcast on every interface.
     */
    ndlcomBridgeProcessOutgoingMessage(bridge, header, payload, origin);

    /*
     * NOTE: to exclude _all_ internal handler from seeing messages from
     * internal, check that the "origin" is not "bridge", as this is used in
     * the "send" functions.
     */
    // if (origin == bridge) {
    // return;
    //}

    /* call the internal handlers which handle _all_ messages. */
    list_for_each_entry(internalHandler, &bridge->internalHandlerList, list) {
        /*
         * checking the "origin" for internal handlers does not make sense...
         * internal handler will always see their own messages, if they are the
         * ones who did create the message in the first place...
         */
        internalHandler->handler(internalHandler->context, header, payload);
    }
}

/* inserting new messages into the bridge */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge, const struct NDLComHeader
        *header, const void *payload) {
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

/* reading and parsing bytes from one external interface */
size_t ndlcomBridgeProcessExternalInterface(
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
             * TODO: take care that no one can override "deviceIds" which
             * are actually used by a node from "us".
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
        }

    } while (bytesRead != bytesProcessed);
    return bytesRead;
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
        bytesRead = ndlcomBridgeProcessFair(bridge);
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
    struct NDLComExternalInterface *externalInterface;
    list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                        list) {
        bytesReadOverall +=
            ndlcomBridgeProcessExternalInterface(bridge, externalInterface);
    }
    return bytesReadOverall;
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

void ndlcomBridgeRegisterInternalHandler(
    struct NDLComBridge *bridge,
    struct NDLComInternalHandler *internalHandler) {

    list_add(&internalHandler->list, &bridge->internalHandlerList);
}

void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {

    list_add(&externalInterface->list, &bridge->externalInterfaceList);
}

void ndlcomBridgeDeregisterInternalHandler(
    struct NDLComBridge *bridge,
    struct NDLComInternalHandler *internalHandler) {

    list_del_init(&internalHandler->list);
}

void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {

    list_del_init(&externalInterface->list);
}
