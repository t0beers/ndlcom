#include "ndlcomBridgeInternalHandler.hpp"

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

// printf
#include <cstdio>

NDLComBridgeHandler::NDLComBridgeHandler(NDLComBridge &_bridge, uint8_t flags)
    : bridge(_bridge) {
    ndlcomInternalHandlerInit(&internal, NDLComBridgeHandler::handleWrapper,
                              flags, this);
    ndlcomBridgeRegisterInternalHandler(&bridge, &internal);
}

NDLComBridgeHandler::~NDLComBridgeHandler() {
    ndlcomBridgeDeregisterInternalHandler(&bridge, &internal);
}

void NDLComBridgeHandler::handleWrapper(void *context,
                                        const struct NDLComHeader *header,
                                        const void *payload) {
    class NDLComBridgeHandler *self =
        static_cast<class NDLComBridgeHandler *>(context);
    self->handle(header, payload);
}

NDLComNodeHandler::NDLComNodeHandler(NDLComNode &_node) : node(_node) {
    ndlcomInternalHandlerInit(&internal, NDLComNodeHandler::handleWrapper,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, this);
    ndlcomNodeRegisterInternalHandler(&node, &internal);
}

NDLComNodeHandler::~NDLComNodeHandler() {
    ndlcomNodeDeregisterInternalHandler(&node, &internal);
}

void NDLComNodeHandler::handleWrapper(void *context,
                                      const struct NDLComHeader *header,
                                      const void *payload) {
    class NDLComNodeHandler *self =
        static_cast<class NDLComNodeHandler *>(context);
    self->handle(header, payload);
}

void NDLComNodeHandler::send(const NDLComId destination, const void *payload,
                             const size_t length) {
    ndlcomNodeSend(&node, destination, payload, length);
}

void NDLComBridgePrintAll::handle(const struct NDLComHeader *header,
                                  const void *payload) {
    printf("bridge saw message from 0x%02x to 0x%02x with %3u bytes of payload\n",
           header->mSenderId, header->mReceiverId, header->mDataLen);
}

void NDLComBridgePrintMissEvents::handle(const struct NDLComHeader *header,
                                         const void *payload) {
    // broadcast receiver dont make sense... skip 'em
    if (header->mSenderId == NDLCOM_ADDR_BROADCAST) {
        return;
    }

    /* allow configuring smaller value for MAX_NUMBER_OF_DEVICES. is
     * always false by default and should be optimized by the compiler */
    if (header->mSenderId >= NDLCOM_MAX_NUMBER_OF_DEVICES ||
        header->mReceiverId >= NDLCOM_MAX_NUMBER_OF_DEVICES)
        return;

    if (!alreadySeen.test(header->mSenderId << sizeof(NDLComId) * 8 |
                          header->mReceiverId)) {
        // this is the first time we see a packet for this combination of
        // sender and receiver. therefore it does not make sense to count an
        // eventually unexpected packet counter as miss-event. we did not know
        // what to expect in the first place.
        alreadySeen.set(header->mSenderId << sizeof(NDLComId) * 8 |
                        header->mReceiverId);
    } else {
        // if we saw this combination before we can go and test of the
        // packet-counter matches our expectation.
        if (expectedNextPacketCounter[header->mSenderId][header->mReceiverId] !=
            header->mCounter) {
            // output the "event"
            printf("miss event in message from 0x%02x to 0x%02x -- diff: %i\n",
                   header->mSenderId, header->mReceiverId,
                   header->mCounter -
                       expectedNextPacketCounter[header->mSenderId]
                                                [header->mReceiverId]);
            // count the number of events as well
            numberOfPacketMissEvents[header->mSenderId][header->mReceiverId]++;
        }
    }

    // in any case we remember the next expected packet counter of this
    // connecetion
    expectedNextPacketCounter[header->mSenderId][header->mReceiverId] =
        header->mCounter + 1;
}

void NDLComNodePrintOwnId::handle(const struct NDLComHeader *header,
                                  const void *payload) {
    printf(
        "listener 0x%02x got message from 0x%02x with %3u bytes of payload\n",
        node.headerConfig.mOwnSenderId, header->mSenderId, header->mDataLen);
}
