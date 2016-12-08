#include "ndlcom/NodeHandler.hpp"

// input/output
#include <iostream>
#include <iomanip>

using namespace ndlcom;

NodeHandlerPrintOwnId::NodeHandlerPrintOwnId(struct NDLComNode &_node, std::ostream &_out)
    : NodeHandlerBase(_node, _out) {}

void NodeHandlerPrintOwnId::handle(const struct NDLComHeader *header,
                                   const void *payload, const void *origin) {
    out << std::string("listener ");
    out << std::showbase << std::hex;
    out << std::setfill('0') << std::setw(4) << std::internal
        << (int)node.headerConfig.mOwnSenderId;
    out << std::string(" got message from ");
    out << std::setfill('0') << std::setw(4) << std::internal
        << (int)header->mSenderId;
    out << std::string(" with ");
    out << std::noshowbase << std::dec;
    out << std::setw(3) << std::setfill(' ') << std::right
        << (int)header->mDataLen;
    out << std::string(" bytes of payload\n");
}
