#include "ndlcom/NodeHandler.h"

void ndlcomNodeHandlerInit(struct NDLComNodeHandler *nodeHandler,
                           const NDLComNodeHandlerFkt handler, const uint8_t flags,
                           void *context) {
    nodeHandler->context = context;
    nodeHandler->handler = handler;
    ndlcomNodeHandlerSetFlags(nodeHandler, flags);
    // this handler is not connected to any "node" yet.
    nodeHandler->node = 0;

    INIT_LIST_HEAD(&nodeHandler->list);
}

void ndlcomNodeHandlerSetFlags(struct NDLComNodeHandler *internalHandler,
                               const uint8_t flags) {
    internalHandler->flags = flags;
}
