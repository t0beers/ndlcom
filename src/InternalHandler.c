#include "ndlcom/InternalHandler.h"

void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internalHandler,
                               NDLComInternalHandlerFkt handler,
                               const uint8_t flags, void *context) {
    internalHandler->context = context;
    internalHandler->handler = handler;
    ndlcomInternalHandlerSetFlags(internalHandler, flags);
    // this handler is not connected to any "node" yet.
    internalHandler->node = 0;

    INIT_LIST_HEAD(&internalHandler->list);
}

void
ndlcomInternalHandlerSetFlags(struct NDLComInternalHandler *internalHandler,
                              const uint8_t flags) {
    internalHandler->flags = flags;
}
