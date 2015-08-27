#include "ndlcomBridgeInternalHandler.hpp"

#include "ndlcom/Bridge.h"

// printf
#include <cstdio>

NDLComBridgeInternalHandler::NDLComBridgeInternalHandler(NDLComBridge &_bridge,
                                                         uint8_t flags)
    : bridge(_bridge) {
    ndlcomInternalHandlerInit(
        &internal, NDLComBridgeInternalHandler::handleWrapper, flags, this);
    ndlcomBridgeRegisterInternalHandler(&bridge, &internal);
}

NDLComBridgeInternalHandler::~NDLComBridgeInternalHandler() {
    ndlcomBridgeDeregisterInternalHandler(&bridge, &internal);
}

void NDLComBridgeInternalHandler::handleWrapper(
    void *context, const struct NDLComHeader *header, const void *payload) {
    class NDLComBridgeInternalHandler *self =
        static_cast<class NDLComBridgeInternalHandler *>(context);
    self->handle(header, payload);
}

void NDLComBridgePrintAll::handle(const struct NDLComHeader *header,
                                  const void *payload) {
    printf("saw message from 0x%02x to 0x%02x with %3u bytes of payload\n",
           header->mSenderId, header->mReceiverId, header->mDataLen);
}

void NDLComBridgePrintOwnId::handle(const struct NDLComHeader *header,
                                    const void *payload) {
    printf("got message from 0x%02x with %3u bytes of payload\n",
           header->mSenderId, header->mDataLen);
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
