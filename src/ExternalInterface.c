#include "ndlcom/ExternalInterface.h"

void
ndlcomExternalInterfaceInit(struct NDLComExternalInterface *externalInterface,
                            NDLComExternalInterfaceWriteEscapedBytes write,
                            NDLComExternalInterfaceReadEscapedBytes read,
                            const uint8_t flags, void *context) {

    externalInterface->context = context;
    externalInterface->flags = flags;
    externalInterface->read = read;
    externalInterface->write = write;

    ndlcomParserCreate(&externalInterface->parser, sizeof(struct NDLComParser));

    INIT_LIST_HEAD(&externalInterface->list);
}

uint32_t ndlcomExternalInterfaceGetCrcFails(
    const struct NDLComExternalInterface *externalInterface) {
    return ndlcomParserGetNumberOfCRCFails(&externalInterface->parser);
}
