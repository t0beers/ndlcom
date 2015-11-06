#ifndef NDLCOMBRIDGEPARSEURI_H
#define NDLCOMBRIDGEPARSEURI_H

#include "ndlcomBridgeExternalInterface.hpp"

class NDLComBridgeExternalInterface *parseUriAndCreateInterface(
    struct NDLComBridge &bridge, std::string uri,
    uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

#endif /*NDLCOMBRIDGEPARSEURI_H*/
