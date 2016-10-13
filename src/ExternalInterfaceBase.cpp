#include "ndlcom/ExternalInterfaceBase.hpp"

#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cstdio>
#include <sstream>

using namespace ndlcom;

ExternalInterfaceBase::ExternalInterfaceBase(struct NDLComBridge &_bridge,
                                             std::ostream &_out, uint8_t flags)
    : paused(false), bytesTransmitted(0), bytesReceived(0), bridge(_bridge),
      out(_out) {
    ndlcomExternalInterfaceInit(&external, ExternalInterfaceBase::writeWrapper,
                                ExternalInterfaceBase::readWrapper, flags,
                                this);
}

// static wrapper function for the C-callback
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

void ExternalInterfaceBase::reportRuntimeError(const std::string &error,
                                               const std::string &file,
                                               const int &line) const {
    std::stringstream ss;
    ss << file << ":" << line << " -- " << error;
    throw std::runtime_error(ss.str());
}
