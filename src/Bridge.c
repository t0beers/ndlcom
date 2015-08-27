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

void ndlcomBridgeProcessExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *external);

/* processing packages after they are in "header,payload" form. decide
 * where to put them. also used for newly sent packages from the inside. */
void ndlcomBridgeProcessOutgoingMessage(struct NDLComBridge *bridge,
                                        const struct NDLComHeader *header,
                                        const void *payload, void *origin);


void ndlcomBridgeInit(struct NDLComBridge *bridge, const NDLComId ownSenderId) {

    /* NOTE: using the routing table to accept messages directed to us is
     * dangerous, somebody might override it from outside and prevent us from
     * receiving messages...
     */
    ndlcomRoutingTableInit(&bridge->routingTable);

    ndlcomHeaderPrepareInit(&bridge->headerConfig, ownSenderId);

    bridge->internalInterfaces = 0;
    bridge->externalInterfaces = 0;
}

/* after messages where received on a external interface or after they are
 * assembled at the internal interface go through this function.
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

        /* start looping the linked list */
        struct NDLComExternalInterface *external = bridge->externalInterfaces;
        while (external) {
            /* don't echo messages back to their origin */
            if (origin != external)
                external->write(external->context, txBuffer, len);
            /* step on */
            external = external->next;
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
         * where to route packets for _us_
         */
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
     * forwarding with the internal interface as origin
     *
     * NOTE: this origin does not have to be used... the forwarder can also
     * look at the senderId. but it needs a unique (nonzero) pointer... little
     * bit dangerous since it might be casted to an "struct
     * NDLComExternalInterface"...
     */
    ndlcomBridgeProcessOutgoingMessage(bridge, header, payload, &bridge);
}

/* the "main" function: going sequentially through all interfaces and
 * trying to read bytes for processing. handle all detected messages.
 */
void ndlcomBridgeProcess(struct NDLComBridge *bridge) {

    struct NDLComExternalInterface *external = bridge->externalInterfaces;
    while (external) {
        ndlcomBridgeProcessExternalInterface(bridge, external);
        external = external->next;
    }
}

void ndlcomBridgeProcessExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *external) {

    uint8_t rawReadBuffer[NDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE];
    size_t bytesRead;
    size_t bytesProcessed;

    const struct NDLComHeader *header;
    const void *payload;
    const struct NDLComInternalHandler *inter;

    do {
        bytesProcessed = 0;
        bytesRead = external->read(external->context, rawReadBuffer,
                                   sizeof(rawReadBuffer));

        do {
            bytesProcessed += ndlcomParserReceive(
                &external->parser, rawReadBuffer + bytesProcessed,
                bytesRead - bytesProcessed);

            if (ndlcomParserHasPacket(&external->parser)) {

                header = ndlcomParserGetHeader(&external->parser);
                payload = ndlcomParserGetPacket(&external->parser);

                /* always update the routing table with the pointer to the
                 * ExternalInterface where the message came from.
                 */
                ndlcomRoutingTableUpdate(&bridge->routingTable,
                                         header->mSenderId, external);

                /* first this: call the internal handlers. they accept the
                 * already decoded message:
                 */
                inter = bridge->internalInterfaces;
                while (inter) {
                    inter->handler(inter->context, header, payload);
                    inter = inter->next;
                }

                /* try to forward the message */
                ndlcomBridgeProcessOutgoingMessage(bridge, header, payload,
                                                   external);

                /* and cleaning up the parser, for the next loop */
                ndlcomParserDestroyPacket(&external->parser);
            }

        } while (bytesRead != bytesProcessed);
    } while (bytesRead > 0);
}

void ndlcomBridgeRegisterInternalHandler(
    struct NDLComBridge *bridge, struct NDLComInternalHandler *interface) {

    /* note the given datastructure in the "NDLComBridge": traverse linked list
     * of known internal interfaces from "bridge" until leaf
     */
    if (bridge->internalInterfaces) {
        struct NDLComInternalHandler *leaf = bridge->internalInterfaces;
        while (leaf->next) {
            leaf = leaf->next;
        }
        leaf->next = interface;
    }
    /* first interface added... */
    else {
        bridge->internalInterfaces = interface;
    }
}

void ndlcomBridgeRegisterExternalInterface(
    struct NDLComBridge *bridge, struct NDLComExternalInterface *interface) {

    /* note the given datastructure in the "NDLComBridge": traverse linked list
     * of known internal interfaces from "bridge" until leaf
     */
    if (bridge->externalInterfaces) {
        struct NDLComExternalInterface *leaf = bridge->externalInterfaces;
        while (leaf->next) {
            leaf = leaf->next;
        }
        leaf->next = interface;
    }
    /* first interface added... */
    else {
        bridge->externalInterfaces = interface;
    }
}
