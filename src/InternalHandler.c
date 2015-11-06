#include "ndlcom/InternalHandler.h"

void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internalHandler,
                               NDLComInternalHandlerFkt handler,
                               const uint8_t flags, void *context) {
    internalHandler->context = context;
    internalHandler->handler = handler;
    ndlcomInternalHandlerSetFlags(internalHandler, flags);

    INIT_LIST_HEAD(&internalHandler->list);
}

void
ndlcomInternalHandlerSetFlags(struct NDLComInternalHandler *internalHandler,
                              const uint8_t flags) {
    internalHandler->flags = flags;
}
