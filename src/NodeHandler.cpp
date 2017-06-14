#include <iomanip>
// input/output
#include <iostream>
#include <string>

#include "ndlcom/NodeHandler.hpp"
#include "ndlcom/Types.h"

using namespace ndlcom;

NodeHandlerPrintOwnId::NodeHandlerPrintOwnId(struct NDLComNode &node,
                                             std::ostream &_out)
    : NodeHandler(node, "NodeHandlerPrintOwnId", _out) {}

void NodeHandlerPrintOwnId::handle(const struct NDLComHeader *header,
                                   const void *payload, const void *origin) {
    out << std::string("listener ");
    out << std::setfill(' ') << std::setw(3) << (int)getOwnDeviceId();
    out << std::string(" got message from ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mSenderId;
    out << std::string(" with ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mDataLen;
    out << std::string(" bytes of payload\n");
}

NodeHandlerStatistics::NodeHandlerStatistics(struct NDLComNode &_caller)
    : NodeHandler(_caller, "NodeHandlerStatistics"), bytesReceived(0) {}

void NodeHandlerStatistics::handle(const struct NDLComHeader *header,
                                   const void *payload, const void *origin) {
    if (origin) {
        bytesReceived += sizeof(struct NDLComHeader) + header->mDataLen;
    } else {
        /* these are package sent by some internal handler, but handled (again)
         * by the node. either broadcast destination or (wrong?) directed at
         * us... */
    }
}

unsigned long NodeHandlerStatistics::currentBytesRx() const {
    return bytesReceived;
}

void NodeHandlerStatistics::resetBytes() { bytesReceived = 0; }
