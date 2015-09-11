#include "ndlcom/InternalHandler.h"

void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internalHandler,
                               NDLComInternalHandlerFkt handler,
                               const uint8_t flags, void *context) {
    internalHandler->context = context;
    internalHandler->flags = flags;
    internalHandler->handler = handler;

    INIT_LIST_HEAD(&internalHandler->list);
}
