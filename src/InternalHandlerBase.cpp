#include "ndlcom/InternalHandlerBase.hpp"

using namespace ndlcom;

BridgeHandlerBase::BridgeHandlerBase(NDLComBridge &_bridge, std::ostream &_out)
    : bridge(_bridge), out(_out) {
    ndlcomInternalHandlerInit(&internal, BridgeHandlerBase::handleWrapper,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, this);
    ndlcomBridgeRegisterInternalHandler(&bridge, &internal);
}

BridgeHandlerBase::~BridgeHandlerBase() {
    ndlcomBridgeDeregisterInternalHandler(&bridge, &internal);
}

void BridgeHandlerBase::handleWrapper(void *context,
                                          const struct NDLComHeader *header,
                                          const void *payload) {
    class BridgeHandlerBase *self = static_cast<class BridgeHandlerBase *>(context);
    self->handle(header, payload);
}

NodeHandlerBase::NodeHandlerBase(NDLComNode &_node, std::ostream &_out)
    : node(_node), out(_out) {
    ndlcomInternalHandlerInit(&internal, NodeHandlerBase::handleWrapper,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, this);
    ndlcomNodeRegisterInternalHandler(&node, &internal);
}

NodeHandlerBase::~NodeHandlerBase() {
    ndlcomNodeDeregisterInternalHandler(&node, &internal);
}

// static wrapper function for the C-callback
void NodeHandlerBase::handleWrapper(void *context,
                                const struct NDLComHeader *header,
                                const void *payload) {
    class NodeHandlerBase *self = static_cast<class NodeHandlerBase *>(context);
    self->handle(header, payload);
}

void NodeHandlerBase::send(const NDLComId destination, const void *payload,
                       const size_t length) {
    ndlcomNodeSend(&node, destination, payload, length);
}
