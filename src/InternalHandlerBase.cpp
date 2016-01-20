#include "ndlcom/InternalHandlerBase.hpp"

ndlcom::BridgeHandler::BridgeHandler(NDLComBridge &_bridge, std::ostream &_out)
    : bridge(_bridge), out(_out) {
    ndlcomInternalHandlerInit(&internal, BridgeHandler::handleWrapper,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, this);
    ndlcomBridgeRegisterInternalHandler(&bridge, &internal);
}

ndlcom::BridgeHandler::~BridgeHandler() {
    ndlcomBridgeDeregisterInternalHandler(&bridge, &internal);
}

void ndlcom::BridgeHandler::handleWrapper(void *context,
                                          const struct NDLComHeader *header,
                                          const void *payload) {
    class BridgeHandler *self = static_cast<class BridgeHandler *>(context);
    self->handle(header, payload);
}

ndlcom::NodeHandler::NodeHandler(NDLComNode &_node, std::ostream &_out)
    : node(_node), out(_out) {
    ndlcomInternalHandlerInit(&internal, NodeHandler::handleWrapper,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, this);
    ndlcomNodeRegisterInternalHandler(&node, &internal);
}

ndlcom::NodeHandler::~NodeHandler() {
    ndlcomNodeDeregisterInternalHandler(&node, &internal);
}

// static wrapper function for the C-callback
void ndlcom::NodeHandler::handleWrapper(void *context,
                                const struct NDLComHeader *header,
                                const void *payload) {
    class NodeHandler *self = static_cast<class NodeHandler *>(context);
    self->handle(header, payload);
}

void ndlcom::NodeHandler::send(const NDLComId destination, const void *payload,
                       const size_t length) {
    ndlcomNodeSend(&node, destination, payload, length);
}
