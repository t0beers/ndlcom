#include "ndlcom/Interfaces.h"

void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internal,
                               NDLComHandlerFkt handler, void *context) {
    internal->context = context;
    internal->handler = handler;
    internal->next = 0;
}

void ndlcomExternalInterfaceInit(struct NDLComExternalInterface *external,
                                 NDLComWriteEscapedBytes write,
                                 NDLComReadEscapedBytes read, void *context) {
    external->context = context;
    external->read = read;
    external->write = write;
    external->next = 0;
    ndlcomParserCreate(&external->parser, sizeof(struct NDLComParser));
}
