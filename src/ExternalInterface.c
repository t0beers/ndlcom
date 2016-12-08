#include "ndlcom/ExternalInterface.h"

void
ndlcomExternalInterfaceInit(struct NDLComExternalInterface *externalInterface,
                            NDLComExternalInterfaceWriteEscapedBytes write,
                            NDLComExternalInterfaceReadEscapedBytes read,
                            const uint8_t flags, void *context) {

    externalInterface->context = context;
    externalInterface->read = read;
    externalInterface->write = write;
    // this interface is not connected to a "bridge" yet.
    externalInterface->bridge = 0;

    ndlcomExternalInterfaceSetFlags(externalInterface, flags);
    ndlcomParserCreate(&externalInterface->parser, sizeof(struct NDLComParser));

    INIT_LIST_HEAD(&externalInterface->list);
}

uint32_t ndlcomExternalInterfaceGetCrcFails(
    const struct NDLComExternalInterface *externalInterface) {
    return ndlcomParserGetNumberOfCRCFails(&externalInterface->parser);
}

void ndlcomExternalInterfaceSetFlags(
    struct NDLComExternalInterface *externalInterface,
    const uint8_t flags) {
    externalInterface->flags = flags;
}
