#include "ndlcomBridgeInternalHandler.hpp"

#include "ndlcom/Bridge.h"

#include <cstdio>

NDLComBridgeInternalHandler::NDLComBridgeInternalHandler(NDLComBridge &_bridge)
    : bridge(_bridge) {
    ndlcomInternalHandlerInit(&internal,
                              NDLComBridgeInternalHandler::handleWrapper, this);
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
    if (bridge.headerConfig.mOwnSenderId == header->mReceiverId ||
        bridge.headerConfig.mOwnSenderId == NDLCOM_ADDR_BROADCAST) {
        printf("saw message from 0x%02x to ME with %3u bytes of payload\n",
               header->mSenderId, header->mDataLen);
    }
}
