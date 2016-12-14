#include "ndlcom/Node.hpp"
#include <iomanip>

using namespace ndlcom;

Node::Node(struct NDLComBridge &bridge, NDLComId ownDeviceId)
    : BridgeHandlerBase(bridge, node.bridgeHandler,
                        "Node-" + std::to_string(ownDeviceId)) {
    // this call will also register the node to the bridge
    ndlcomNodeInit(&node, &bridge, ownDeviceId);
}

Node::~Node() {
    for (auto &it : allHandler) {
        it->deregisterHandler();
    }
    allHandler.clear();
    ndlcomNodeDeinit(&node);
}

void Node::registerHandler() {}
void Node::deregisterHandler() {}

void Node::printStatus(std::ostream &out) {
    out << label << ":\n";
    for (auto it : allHandler) {
        out << "- " << it->label << "\n";
    }
}

NDLComId Node::getOwnDeviceId() const { return node.headerConfig.mOwnSenderId; }

void Node::send(const NDLComId receiverId, const void *payload,
                size_t payloadSize) {
    ndlcomNodeSend(&node, receiverId, payload, payloadSize);
}
