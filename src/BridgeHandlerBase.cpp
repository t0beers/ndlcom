#include "ndlcom/BridgeHandlerBase.hpp"

#include "ndlcom/Bridge.h"

using namespace ndlcom;

BridgeHandlerBase::BridgeHandlerBase(NDLComBridge &_bridge, std::ostream &_out)
    : bridge(_bridge), out(_out) {
    ndlcomBridgeHandlerInit(&internal, BridgeHandlerBase::handleWrapper,
                              NDLCOM_BRIDGE_HANDLER_FLAGS_DEFAULT, this);
    ndlcomBridgeRegisterBridgeHandler(&bridge, &internal);
}

BridgeHandlerBase::~BridgeHandlerBase() {
    ndlcomBridgeDeregisterBridgeHandler(&bridge, &internal);
}

void BridgeHandlerBase::handleWrapper(void *context,
                                      const struct NDLComHeader *header,
                                      const void *payload, const void *origin) {
    class BridgeHandlerBase *self =
        static_cast<class BridgeHandlerBase *>(context);
    self->handle(header, payload, origin);
}
