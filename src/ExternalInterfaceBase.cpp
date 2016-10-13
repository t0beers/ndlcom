#include "ndlcom/ExternalInterfaceBase.hpp"

#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cstdio>
#include <sstream>

using namespace ndlcom;

ExternalInterfaceBase::ExternalInterfaceBase(struct NDLComBridge &_bridge,
                                             std::ostream &_out, uint8_t flags)
    : bridge(_bridge), out(_out) {
    ndlcomExternalInterfaceInit(&external, ExternalInterfaceBase::writeWrapper,
                                ExternalInterfaceBase::readWrapper, flags,
                                this);
}

// static wrapper function for the C-callback
void ExternalInterfaceBase::writeWrapper(void *context, const void *buf,
                                         const size_t count) {
    class ExternalInterfaceBase *self =
        static_cast<class ExternalInterfaceBase *>(context);
    self->writeEscapedBytes(buf, count);
}

// static wrapper function for the C-callback
size_t ExternalInterfaceBase::readWrapper(void *context, void *buf,
                                          const size_t count) {
    class ExternalInterfaceBase *self =
        static_cast<class ExternalInterfaceBase *>(context);
    return self->readEscapedBytes(buf, count);
}

void ExternalInterfaceBase::reportRuntimeError(const std::string &error,
                                               const std::string &file,
                                               const int &line) const {
    std::stringstream ss;
    ss << file << ":" << line << " -- " << error;
    throw std::runtime_error(ss.str());
}
