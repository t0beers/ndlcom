#include <cstdio>
#include <iosfwd>
#include <stdexcept>

#include "ndlcom/Bridge.h"
#include "ndlcom/ExternalInterfaceBase.hpp"

using namespace ndlcom;

ExternalInterfaceBase::ExternalInterfaceBase(struct NDLComBridge &bridge,
                                             std::string _label,
                                             std::ostream &_out, uint8_t flags)
    : ExternalInterfaceVeryBase(bridge, external, _label, _out), paused(false),
      bytesTransmitted(0), bytesReceived(0) {
    ndlcomExternalInterfaceInit(&external, ExternalInterfaceBase::writeWrapper,
                                ExternalInterfaceBase::readWrapper, flags,
                                this);
}

void ExternalInterfaceBase::registerHandler() {
    ndlcomBridgeRegisterExternalInterface(&caller, &handler);
}

void ExternalInterfaceBase::deregisterHandler() {
    ndlcomBridgeDeregisterExternalInterface(&caller, &handler);
}

// static wrapper function for the c-callback
void ExternalInterfaceBase::writeWrapper(void *context, const void *buf,
                                         const size_t count) {
    class ExternalInterfaceBase *self =
        static_cast<class ExternalInterfaceBase *>(context);
    if (self->paused) {
        return;
    }
    self->writeEscapedBytes(buf, count);
    self->noteOutgoingBytes(buf, count);
}

// static wrapper function for the C-callback
size_t ExternalInterfaceBase::readWrapper(void *context, void *buf,
                                          const size_t count) {
    class ExternalInterfaceBase *self =
        static_cast<class ExternalInterfaceBase *>(context);
    // reading even if paused, to empty kernel buffer
    size_t read = self->readEscapedBytes(buf, count);
    if (self->paused) {
        read = 0;
    }
    self->noteIncomingBytes(buf, read);
    return read;
}

void ExternalInterfaceBase::noteIncomingBytes(const void *buf, size_t count) {
    bytesReceived += count;
}

void ExternalInterfaceBase::noteOutgoingBytes(const void *buf, size_t count) {
    bytesTransmitted += count;
}

void ExternalInterfaceBase::setFlag(uint8_t flag, bool value) {
    uint8_t oldFlags = external.flags;
    if (value) {
        ndlcomExternalInterfaceSetFlags(&external, oldFlags | flag);
    } else {
        ndlcomExternalInterfaceSetFlags(&external, oldFlags & ~flag);
    }
}

void ExternalInterfaceBase::setRoutingForDeviceId(const NDLComId deviceId) {
    ndlcomExternalInterfaceSetRoutingForDeviceId(&external, deviceId);
}

void ExternalInterfaceBase::reportRuntimeError(const std::string &error,
                                               const std::string &file,
                                               const int &line) const {
    throw std::runtime_error(file + ":" + std::to_string(line) + " -- " +
                             error);
}
