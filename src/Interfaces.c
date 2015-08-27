#include "ndlcom/Interfaces.h"

void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internalHandler,
                               NDLComHandlerFkt handler, void *context) {
    internalHandler->context = context;
    internalHandler->handler = handler;

    INIT_LIST_HEAD(&internalHandler->list);
}

void ndlcomExternalInterfaceInit(
    struct NDLComExternalInterface *externalInterface,
    NDLComWriteEscapedBytes write, NDLComReadEscapedBytes read, void *context) {

    externalInterface->context = context;
    externalInterface->read = read;
    externalInterface->write = write;

    ndlcomParserCreate(&externalInterface->parser, sizeof(struct NDLComParser));

    INIT_LIST_HEAD(&externalInterface->list);
}
