#include "ndlcom/InternalHandler.hpp"

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

using namespace ndlcom;

/**
 * BridgeHandler:
 */
void BridgeHandler::sendRaw(const struct NDLComHeader *header,
                            const void *payload) {
    // TODO: would need a check if this class is actually registered at the
    // "caller" as there is a small time-window during the creation of this
    // class in the factory, where it is not registered.
    //
    // but this might not be a problem in this class, assuming the type of
    // "caller" is an actual "struct NDLComBridge"
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
    return ndlcomNodeGetOwnDeviceId(handler.node);
}
