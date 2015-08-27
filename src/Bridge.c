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

    INIT_LIST_HEAD(&bridge->internalHandlerList);
    INIT_LIST_HEAD(&bridge->externalInterfaceList);

    // this will also initialize the routing table
    ndlcomBridgeSetOwnSenderId(bridge, ownSenderId);
}

void ndlcomBridgeSetOwnSenderId(struct NDLComBridge *bridge,
                                const NDLComId ownSenderId) {

    /* resets the packet-counters to use for each receiver, and is used to
     * store our own deviceId at a convenient place */
    ndlcomHeaderPrepareInit(&bridge->headerConfig, ownSenderId);

    /* we have to (re-)initialize the routing table, as we probably changed our
     * position inside the network. this also gets rid of our new "own id",
     * which might be already there */
    ndlcomRoutingTableInit(&bridge->routingTable);
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
    /* at first the message needs to be encoded
     *
     * NOTE: variable length array, so eventually safe some stack?
     */
    uint8_t txBuffer[NDLCOM_MAX_ENCODED_MESSAGE_SIZE_FOR_PACKET(header)];
    size_t len = ndlcomEncode(txBuffer, sizeof(txBuffer), header, payload);

    /* ask routing table */
    struct NDLComExternalInterface *destination =
        (struct NDLComExternalInterface *)ndlcomRoutingGetDestination(
            &bridge->routingTable, header->mReceiverId);

    /* this function does _not_ handle messages with our Id as receiver */
    if (header->mReceiverId == bridge->headerConfig.mOwnSenderId)
        return;

    /* first case: send to all known interfaces */
    if ((header->mReceiverId == NDLCOM_ADDR_BROADCAST) ||
        (destination == NDLCOM_ROUTING_ALL_INTERFACES)) {

        /* loop through all interfaces */
        struct NDLComExternalInterface *externalInterface;
        list_for_each_entry(externalInterface, &bridge->externalInterfaceList,
                            list) {
            /* don't echo messages back to their origin */
            if (origin != externalInterface) {
                externalInterface->write(externalInterface->context, txBuffer,
                                         len);
            }
        }

    }
    /* TODO: if somehow our "ownSenderId" is added into the routing table?  */
    else {
        /* NOTE: when throwing random messages at one interface,
         * this interface will slowly accumulate all deviceIds. so
         * after some time, it is responsible for all receiverIds.
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
        }
        /* think about this... routing table can still return "0" when asked
         * where to route packets for _us_ */
        else {
            destination->write(destination->context, txBuffer, len);
        }
    }
}

void ndlcomBridgeSend(struct NDLComBridge *bridge, const NDLComId receiverId,
                      const void *payload, const size_t payloadSize) {

    struct NDLComHeader header;
    ndlcomHeaderPrepare(&bridge->headerConfig, &header, receiverId, payloadSize);

    ndlcomBridgeSendRaw(bridge, &header, payload, payloadSize);
}

/* send with custom header */
void ndlcomBridgeSendRaw(struct NDLComBridge *bridge,
                         const struct NDLComHeader *header, const void *payload,
                         const size_t size) {
    /* in the end, transmitting a message from internal is nothing but
     * forwarding with the bridge as origin.
     *
     * NOTE: this origin does not have to be used... the forwarder can also
     * look at the senderId. but it needs a unique (nonzero) pointer... little
     * bit dangerous since it might be casted to an "struct
     * NDLComExternalInterface"...
     */
    ndlcomBridgeProcessOutgoingMessage(bridge, header, payload, &bridge);
}

void ndlcomBridgeProcessExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *externalInterface) {

    uint8_t rawReadBuffer[NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE];
    size_t bytesRead;
    size_t bytesProcessed;
    const struct NDLComInternalHandler *internalHandler;

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

                /* always update the routing table with the pointer to the
                 * ExternalInterface where the message came from.
                 */
                ndlcomRoutingTableUpdate(&bridge->routingTable,
                                         header->mSenderId, externalInterface);

                /* first this: call the InternalHandlers. they accept the
                 * already decoded message:
                 */
                list_for_each_entry(internalHandler,
                                    &bridge->internalHandlerList, list) {
                    internalHandler->handler(internalHandler->context, header,
                                             payload);
                }

                /* try to forward the message */
                ndlcomBridgeProcessOutgoingMessage(bridge, header, payload,
                                                   externalInterface);

                /* and cleaning up the parser, for the next loop */
                ndlcomParserDestroyPacket(&externalInterface->parser);
            }

        } while (bytesRead != bytesProcessed);
    } while (bytesRead > 0);
}

/* the "main" function: going sequentially through all interfaces and
 * trying to read bytes for processing. handle all detected messages.
 */
void ndlcomBridgeProcess(struct NDLComBridge *bridge) {

    struct NDLComExternalInterface *externalInterface;
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

    list_add(&externalInterface->list, &bridge->externalInterfaceList);
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
