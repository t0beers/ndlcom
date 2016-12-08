#include "ndlcom/NodeHandler.h"

void ndlcomNodeHandlerInit(struct NDLComNodeHandler *internalHandler,
                               NDLComNodeHandlerFkt handler,
                               const uint8_t flags, void *context) {
    internalHandler->context = context;
    internalHandler->handler = handler;
    ndlcomNodeHandlerSetFlags(internalHandler, flags);
    // this handler is not connected to any "node" yet.
    internalHandler->node = 0;

    INIT_LIST_HEAD(&internalHandler->list);
}

void
ndlcomNodeHandlerSetFlags(struct NDLComNodeHandler *internalHandler,
                              const uint8_t flags) {
    internalHandler->flags = flags;
}
