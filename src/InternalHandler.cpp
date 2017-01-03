#include "ndlcom/InternalHandler.hpp"

using namespace ndlcom;

/**
 * BridgeHandler:
 */
void BridgeHandler::sendRaw(const struct NDLComHeader *header,
                            const void *payload) {
    ndlcomBridgeSendRaw(&caller, header, payload);
}

void BridgeHandler::registerHandler() {
    ndlcomBridgeRegisterBridgeHandler(&caller, &internal);
}

void BridgeHandler::deregisterHandler() {
    ndlcomBridgeDeregisterBridgeHandler(&caller, &internal);
}

/**
 * NodeHandler:
 */
void NodeHandler::send(const NDLComId receiverId, const void *payload,
                       const size_t length) {
    ndlcomNodeSend(&caller, receiverId, payload, length);
}

void NodeHandler::registerHandler() {
    ndlcomNodeRegisterNodeHandler(&caller, &internal);
}

void NodeHandler::deregisterHandler() {
    ndlcomNodeDeregisterNodeHandler(&caller, &internal);
}

const NDLComId NodeHandler::getOwnDeviceId() const {
    return handler.node->headerConfig.mOwnSenderId;
}
