#include "ndlcom/BridgeHandler.h"

void ndlcomBridgeHandlerInit(struct NDLComBridgeHandler *bridgeHandler,
                             const NDLComBridgeHandlerFkt handler,
                             const uint8_t flags, void *context) {
    bridgeHandler->context = context;
    bridgeHandler->handler = handler;
    ndlcomBridgeHandlerSetFlags(bridgeHandler, flags);
    // this handler is not connected to any "bridge" yet.
    bridgeHandler->bridge = 0;

    INIT_LIST_HEAD(&bridgeHandler->list);
}

void ndlcomBridgeHandlerSetFlags(struct NDLComBridgeHandler *bridgeHandler,
                                 const uint8_t flags) {
    bridgeHandler->flags = flags;
}
