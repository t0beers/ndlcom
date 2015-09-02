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

void ndlcomBridgeInit(struct NDLComBridge *bridge, const NDLComId ownSenderId) {

    /* initialize all the list we have */
    INIT_LIST_HEAD(&bridge->internalHandlerList);
    INIT_LIST_HEAD(&bridge->externalInterfaceList);
    INIT_LIST_HEAD(&bridge->ownIdInternelHandlerList);
    INIT_LIST_HEAD(&bridge->debugMirrorInterfaceList);

    /* calling this will also initialize the routing table */
    ndlcomBridgeSetOwnSenderId(bridge, ownSenderId);
}

/* initializes internal structs with correct values. nearly resets the whole
 * state */
void ndlcomBridgeSetOwnSenderId(struct NDLComBridge *bridge,
                                const NDLComId ownSenderId) {

    /* resets the packet-counters to use for each receiver, and is used to
     * store our own deviceId at a convenient place */
    ndlcomHeaderPrepareInit(&bridge->headerConfig, ownSenderId);

    /* we have to (re-)initialize the routing table, as we probably changed our
     * position inside the network. this also gets rid of our new "own id",
     * which might be already there */
    ndlcomRoutingTableInit(&bridge->routingTable);

    /**
     * NOTE: putting our own "deviceId" into the routingtable. this is a hack, to be
     * able to detect messages going "to us" and not process them in the
     * outgoing side
     */
    ndlcomRoutingTableUpdate(&bridge->routingTable, ownSenderId, bridge);
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
    if (header->mReceiverId == bridge->headerConfig.mOwnSenderId)
        return;
    }

    /**
     * do _not_ send messages directed explicitly at us to the outside. if we
     * would try to, the routing table would say "unknown destination" and copy
     * the message to all interfaces.
     *
     * this is a problem when we want to have _no_ explicit ownDeviceId!
     */
#if 0
    if (header->mReceiverId == bridge->headerConfig.mOwnSenderId)
        return;
#endif

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
    /* second case: we know the specific interface where to send the message to */
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
            printf("bridge is skipping directed to the origin\n");
        } else if(destination == (void*)bridge) {
            /* would send to ourselfes... skip. or call internal handlers? */
            printf("bridge is skipping directed to us\n");
        }
        /* think about this... routing table can still return "0" when asked
         * where to route packets for _us_ */
        else {
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

    /**
     * NOTE: this function here is called for messages from internal handlers.
     * so we have to prevent looping by checking their origin. we only give a
     * message to a handler if its not the origin of the message
     */

    /* first call the InternalHandlers which handle _all_ messages. */
    list_for_each_entry(internalHandler, &bridge->internalHandlerList, list) {
        /* check origin */
        if (internalHandler != origin) {
            internalHandler->handler(internalHandler->context, header, payload);
        }
    }

    /* call the handlers which only want to see messages intended for "us" */
    if ((header->mReceiverId == bridge->headerConfig.mOwnSenderId) ||
        (header->mReceiverId == NDLCOM_ADDR_BROADCAST)) {
        list_for_each_entry(internalHandler, &bridge->ownIdInternelHandlerList,
                            list) {
            /* check origin */
            if (internalHandler != origin) {
                internalHandler->handler(internalHandler->context, header,
                                         payload);
            }
        }
    }

    /* after feeding the decoded message through all handlers we finally give
     * it to the actual external interfaces to be written out */
    ndlcomBridgeProcessOutgoingMessage(bridge, header, payload, origin);
}

/* sending a payload to a receiver */
void ndlcomBridgeSend(struct NDLComBridge *bridge, const NDLComId receiverId,
                      const void *payload, const size_t payloadSize) {
    /* preparation of the header */
    struct NDLComHeader header;
    ndlcomHeaderPrepare(&bridge->headerConfig, &header, receiverId, payloadSize);
    /**
     * in the end, transmitting a message from internal is nothing but
     * "forwarding" a message with the bridge as origin.
     *
     * NOTE: this "origin" does not have to be set to the "bridge" pointer...
     * the forwarder will also look at the senderId. but therefore it needs a
     * unique (nonzero) pointer... little bit dangerous since it might be
     * casted to an "struct NDLComExternalInterface"...
     */
    ndlcomBridgeProcessDecodedMessage(bridge, &header, payload, &bridge);
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
                 * never put our own deviceId into the RoutingTable... would
                 * break how we detect messages intended for us.
                 *
                 * updating the table before processing the message allows
                 * responding on the right interface. */
                if ((bridge->headerConfig.mOwnSenderId != header->mSenderId) &&
                    !(externalInterface->flags &
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

    if (internalHandler->flags & NDLCOM_INTERNAL_HANDLER_FLAGS_ONLY_OWN_ID) {
        list_add(&internalHandler->list, &bridge->ownIdInternelHandlerList);
    } else {
        list_add(&internalHandler->list, &bridge->internalHandlerList);
    }
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
