#include "ndlcom/ExternalInterfaceBase.hpp"

#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cstdio>

using namespace ndlcom;

ExternalInterfaceBase::ExternalInterfaceBase(
    struct NDLComBridge &_bridge, uint8_t flags)
    : bridge(_bridge) {
    ndlcomExternalInterfaceInit(
        &external, ExternalInterfaceBase::writeWrapper,
        ExternalInterfaceBase::readWrapper, flags, this);

    ndlcomBridgeRegisterExternalInterface(&bridge, &external);
}

ExternalInterfaceBase::~ExternalInterfaceBase() {
    ndlcomBridgeDeregisterExternalInterface(&bridge, &external);
}

void ExternalInterfaceBase::writeWrapper(void *context, const void *buf,
                                                 const size_t count) {
    class ExternalInterfaceBase *self =
        static_cast<class ExternalInterfaceBase *>(context);
    self->writeEscapedBytes(buf, count);
}

size_t ExternalInterfaceBase::readWrapper(void *context, void *buf,
                                                  const size_t count) {
    class ExternalInterfaceBase *self =
        static_cast<class ExternalInterfaceBase *>(context);
    return self->readEscapedBytes(buf, count);
}
