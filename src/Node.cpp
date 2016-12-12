#include "ndlcom/Node.hpp"
#include <iomanip>

using namespace ndlcom;

Node::Node(struct NDLComBridge &bridge, NDLComId ownDeviceId) {
    // this call will also register the node to the bridge
    ndlcomNodeInit(&node, &bridge, ownDeviceId);
}

Node::~Node() {
    allHandler.clear();
    ndlcomNodeDeinit(&node);
}

void Node::printStatus(std::ostream &out) {
    out << "Node (receiverId " << std::setfill('0') << std::showbase << std::hex
        << std::setfill('0') << std::setw(4) << std::internal
        << (int)node.headerConfig.mOwnSenderId << "):\n";
    for (auto it : allHandler) {
        out << "- " << it->label << "\n";
    }
}

NDLComId Node::getOwnDeviceId() const { return node.headerConfig.mOwnSenderId; }

void Node::send(const NDLComId receiverId, const void *payload,
                size_t payloadSize) {
    ndlcomNodeSend(&node, receiverId, payload, payloadSize);
}
