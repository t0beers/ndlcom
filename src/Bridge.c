#include "ndlcom/Bridge.h"
#include "ndlcom/Encoder.h"
#include "ndlcom/HeaderPrepare.h"
#include "ndlcom/Routing.h"

/**
 * the size if the temporary rxBuffer, used between kernel and parser
 *
 * one encoded message seems like a good guess...
 *
 * can be inflluenced at compile-time
 */
#ifndef NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE
#define NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE NDLCOM_MAX_ENCODED_MESSAGE_SIZE
#endif

void ndlcomBridgeInit(struct NDLComBridge *bridge) {

    /* initialize all the list we have */
    INIT_LIST_HEAD(&bridge->internalHandlerList);
    INIT_LIST_HEAD(&bridge->externalInterfaceList);
    INIT_LIST_HEAD(&bridge->debugMirrorInterfaceList);
}

/* after messages where received by a "struct ExternalInterface" or after they
 * are assembled at the InternalHandler go through this function.
 *
 * FIXME: we do not know (yet) if the message has to go outside _at all_. this
 * might lead to encoding too much...
 */
void ndlcomBridgeProcessOutgoingMessage(struct NDLComBridge *bridge,
                                        const struct NDLComHeader *header,
                                        const void *payload, void *origin) {
    /* used as loop-variable for the lists */
    struct NDLComExternalInterface *externalInterface;
    /* return-value for the routing table */
    struct NDLComExternalInterface *destination;

    /* at first the message needs to be encoded. we do not know yet if we have
     * to encode it at all: receiving from only one outgoing interface for
     * example. this is inefficient...
     *
     * NOTE: variable length array, so this will eventually safe some stack?
     */
    uint8_t txBuffer[NDLCOM_MAX_ENCODED_MESSAGE_SIZE_FOR_PACKET(header)];
    size_t len = ndlcomEncode(txBuffer, sizeof(txBuffer), header, payload);

    /* these external interfaces want to get _all_ messages, no matter what */
    list_for_each_entry(externalInterface, &bridge->debugMirrorInterfaceList,
                        list) {
        /* don't echo messages back to their origin */
        if (origin != externalInterface) {
            externalInterface->write(externalInterface->context, txBuffer, len);
        }
    }

    /* asking the routing table where to forward to */
    destination = (struct NDLComExternalInterface *)ndlcomRoutingGetDestination(
        &bridge->routingTable, header->mReceiverId);

    /* first case: we are asked to send to all known interfaces (except the
     * origin of this message) */
    if ((header->mReceiverId == NDLCOM_ADDR_BROADCAST) ||
        (destination == NDLCOM_ROUTING_ALL_INTERFACES)) {

        /* loop through all interfaces */
        list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                            list) {
            /* don't echo messages back to their origin */
            if (origin != externalInterface) {
                externalInterface->write(externalInterface->context, txBuffer,
                                         len);
            }
        }

    }
    /* second case: knowing specific interface where to send the message to */
    else {
        /* NOTE: when throwing random messages at one interface which is
         * connected to the bridge, this interface will slowly accumulate all
         * deviceIds, as messages from all senders seem to originate from this
         * interface. so after some time, it is responsible for all
         * receiverIds.
         *
         * the respective entry cannot be updated as long as all the
         * other interfaces do not receive random messages
         */
        if (destination == origin) {
            /*
            std::cout << "bridge: ähh, sending message  from " << std::setw(3)
                      << (int)header->mSenderId << " to " << std::setw(3)
                      << (int)header->mReceiverId << " with " << std::setw(3)
                      << (int)header->mDataLen
                      << " bytes back to its origin?\n";
             */
        } else if (destination == (void *)bridge) {
            /**
             * do _not_ send messages destined for "us" to the outside. relies
             * on having "our" deviceId inside the routing table
             */
        } else {
            /* otherwise, finally write to the external interface */
            destination->write(destination->context, txBuffer, len);
        }
    }
}

/**
 * called for messages which
 * - where successfully received from an ExternalInterface, after the update to
 *   the routing table
 * - after a new message is created in "ndlcomBridgeSend()"
 */
void ndlcomBridgeProcessDecodedMessage(struct NDLComBridge *bridge,
                                       const struct NDLComHeader *header,
                                       const void *payload, void *origin) {
    const struct NDLComInternalHandler *internalHandler;

    /* first thing to do: forward/resend outgoing messages on the actual
     * external interfaces */
    ndlcomBridgeProcessOutgoingMessage(bridge, header, payload, origin);

    /* NOTE: to exclude _all_ internal handler from seeing messages from
     * internal, check that the "origin" is not "bridge", as this is used in
     * the "send" functions. */
    //if (origin != bridge) {
    //}

    /**
     * NOTE: this function here is called for messages from internal handlers.
     * so we have to prevent looping by checking their origin. we only give a
     * message to a handler if its not the origin of the message
     */

    /* first call the InternalHandlers which handle _all_ messages. */
    list_for_each_entry(internalHandler, &bridge->internalHandlerList, list) {
        /* checking the "origin" for internal handlers does not make sense...
         * internal interfaces will always see their own messages... */
        internalHandler->handler(internalHandler->context, header, payload);
    }
}

void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header, const void *payload,
                         const size_t payloadSize){
    /* ... */
    ndlcomBridgeProcessDecodedMessage(bridge, header, payload, bridge);
}

/* reading and parsig bytes from an external interface */
void ndlcomBridgeProcessExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *externalInterface) {

    uint8_t rawReadBuffer[NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE];
    size_t bytesRead;
    size_t bytesProcessed;

    const struct NDLComHeader *header;
    const void *payload;

    do {
        bytesProcessed = 0;
        bytesRead = externalInterface->read(externalInterface->context, rawReadBuffer,
                                   sizeof(rawReadBuffer));

        do {
            bytesProcessed += ndlcomParserReceive(
                &externalInterface->parser, rawReadBuffer + bytesProcessed,
                bytesRead - bytesProcessed);

            if (ndlcomParserHasPacket(&externalInterface->parser)) {

                header = ndlcomParserGetHeader(&externalInterface->parser);
                payload = ndlcomParserGetPacket(&externalInterface->parser);

                /* only update the routing table when processing
                 * non-debug-ports with the pointer to the ExternalInterface
                 * where the message came from.
                 *
                 * TODO: take care that noone can override deviceIds which are
                 * actually used by a node from "us".
                 *
                 * updating the table before processing the message allows
                 * responding on the right interface.
                 */
                if (!(externalInterface->flags &
                      NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR)) {
                    ndlcomRoutingTableUpdate(&bridge->routingTable,
                                             header->mSenderId,
                                             externalInterface);
                }

                /* try to forward the message. we are not sure if we can, yet. */
                ndlcomBridgeProcessDecodedMessage(bridge, header, payload,
                                                  externalInterface);

                /* and clean up the parser of this interface, to be ready for
                 * the next packet. */
                ndlcomParserDestroyPacket(&externalInterface->parser);
            }

        } while (bytesRead != bytesProcessed);
    } while (bytesRead > 0);
}

/* the "main" processing function: going sequentially through all known
 * interfaces and trying to read bytes for processing. handle all detected
 * messages.
 *
 * there are two kind of interfaces:
 * - the ordinary ones susceptible to routing and everything
 * - the "debug mirrors", which get _all_ messages
 */
void ndlcomBridgeProcess(struct NDLComBridge *bridge) {

    struct NDLComExternalInterface *externalInterface;
    list_for_each_entry(externalInterface, &bridge->debugMirrorInterfaceList, list) {
        ndlcomBridgeProcessExternalInterface(bridge, externalInterface);
    }
    list_for_each_entry(externalInterface, &bridge->externalInterfaceList, list) {
        ndlcomBridgeProcessExternalInterface(bridge, externalInterface);
    }

}

void ndlcomBridgeRegisterInternalHandler(
    struct NDLComBridge *bridge,
    struct NDLComInternalHandler *internalHandler) {

    list_add(&internalHandler->list, &bridge->internalHandlerList);
}

void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {

    if (externalInterface->flags &
        NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR) {
        list_add(&externalInterface->list, &bridge->debugMirrorInterfaceList);
    } else {
        list_add(&externalInterface->list, &bridge->externalInterfaceList);
    }
}

void ndlcomBridgeDeregisterInternalHandler(
    struct NDLComBridge *bridge,
    struct NDLComInternalHandler *internalHandler) {

    list_del(&internalHandler->list);
}

void ndlcomBridgeDeregisterExternalInterface(
    struct NDLComBridge *bridge,
    struct NDLComExternalInterface *externalInterface) {

    list_del(&externalInterface->list);
}
