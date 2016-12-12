#include "ndlcom/Node.hpp"

using namespace ndlcom;

Node::Node(struct NDLComBridge &bridge, NDLComId ownDeviceId) {
    // this call will also add the node to the bridge
    ndlcomNodeInit(&node, &bridge, ownDeviceId);
}

Node::~Node() {
    allHandler.clear();
    ndlcomNodeDeinit(&node);
}

NDLComId Node::getOwnDeviceId() const { return node.headerConfig.mOwnSenderId; }

void Node::send(const NDLComId receiverId, const void *payload,
                size_t payloadSize) {
    ndlcomNodeSend(&node, receiverId, payload, payloadSize);
}
