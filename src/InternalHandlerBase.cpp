#include "ndlcom/InternalHandlerBase.hpp"

ndlcom::BridgeHandler::BridgeHandler(NDLComBridge &_bridge, uint8_t flags)
    : bridge(_bridge) {
    ndlcomInternalHandlerInit(&internal, BridgeHandler::handleWrapper, flags,
                              this);
    ndlcomBridgeRegisterInternalHandler(&bridge, &internal);
}

ndlcom::BridgeHandler::~BridgeHandler() {
    ndlcomBridgeDeregisterInternalHandler(&bridge, &internal);
}

void ndlcom::BridgeHandler::handleWrapper(void *context,
                                          const struct NDLComHeader *header,
                                          const void *payload) {
    class BridgeHandler *self = static_cast<class BridgeHandler *>(context);
    self->handle(header, payload);
}
