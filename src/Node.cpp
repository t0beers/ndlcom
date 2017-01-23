#include <ostream>
#include <string>

#include "ndlcom/HandlerCommon.hpp"
#include "ndlcom/Node.hpp"

using namespace ndlcom;

Node::Node(struct NDLComBridge &bridge, const NDLComId ownDeviceId)
    : BridgeHandlerBase(bridge, node.bridgeHandler,
                        "Node-" + std::to_string(ownDeviceId)) {
    // this call will also register the node to the bridge
    ndlcomNodeInit(&node, ownDeviceId);
}

Node::~Node() {
    /** no iterators, as the call inside the loop would invalidate them */
    while (!allHandler.empty()) {
        destroyNodeHandler(
            std::weak_ptr<ndlcom::NodeHandlerBase>(allHandler.front()));
    }
    /** compare c list with c++ vector. both should be empty! */
    if (!list_empty(&node.nodeHandlerList)) {
        out << "Attention, something went wrong during teardown, "
               "c-NodeHandlerList is not empty\n";
    }
}

void Node::registerHandler() { ndlcomNodeRegister(&node, &caller); }
void Node::deregisterHandler() { ndlcomNodeDeregister(&node); }

void Node::printStatus() {
    out << label << ":\n";
    for (auto it : allHandler) {
        out << "- " << it->label << "\n";
    }
}

const NDLComId Node::getOwnDeviceId() const {
    return ndlcomNodeGetOwnDeviceId(&node);
}

void Node::send(const NDLComId receiverId, const void *payload,
                size_t payloadSize) {
    ndlcomNodeSend(&node, receiverId, payload, payloadSize);
}

void Node::setOwnDeviceId(const NDLComId ownDeviceId) {
    ndlcomNodeSetOwnSenderId(&node, ownDeviceId);
}
