#include "ndlcom/Interfaces.h"

void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internalHandler,
                               NDLComHandlerFkt handler, const uint8_t flags,
                               void *context) {
    internalHandler->context = context;
    internalHandler->flags = flags;
    internalHandler->handler = handler;

    INIT_LIST_HEAD(&internalHandler->list);
}

void ndlcomExternalInterfaceInit(
    struct NDLComExternalInterface *externalInterface,
    NDLComWriteEscapedBytes write, NDLComReadEscapedBytes read,
    const uint8_t flags, void *context) {

    externalInterface->context = context;
    externalInterface->flags = flags;
    externalInterface->read = read;
    externalInterface->write = write;

    ndlcomParserCreate(&externalInterface->parser, sizeof(struct NDLComParser));

    INIT_LIST_HEAD(&externalInterface->list);
}
