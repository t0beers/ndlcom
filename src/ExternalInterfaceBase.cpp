#include "ndlcom/ExternalInterfaceBase.hpp"

#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cstdio>

NDLComBridgeExternalInterface::NDLComBridgeExternalInterface(
    NDLComBridge &_bridge, uint8_t flags)
    : bridge(_bridge) {
    ndlcomExternalInterfaceInit(
        &external, NDLComBridgeExternalInterface::writeWrapper,
        NDLComBridgeExternalInterface::readWrapper, flags, this);

    ndlcomBridgeRegisterExternalInterface(&bridge, &external);
}

NDLComBridgeExternalInterface::~NDLComBridgeExternalInterface() {
    ndlcomBridgeDeregisterExternalInterface(&bridge, &external);
}

void NDLComBridgeExternalInterface::writeWrapper(void *context, const void *buf,
                                                 const size_t count) {
    class NDLComBridgeExternalInterface *self =
        static_cast<class NDLComBridgeExternalInterface *>(context);
    self->writeEscapedBytes(buf, count);
}

size_t NDLComBridgeExternalInterface::readWrapper(void *context, void *buf,
                                                  const size_t count) {
    class NDLComBridgeExternalInterface *self =
        static_cast<class NDLComBridgeExternalInterface *>(context);
    return self->readEscapedBytes(buf, count);
}
