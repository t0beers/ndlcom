#ifndef NDLCOMBRIDGEPARSEURI_H
#define NDLCOMBRIDGEPARSEURI_H

#include <sstream>

#include "ndlcom/ExternalInterfaceBase.hpp"

/**
 * @brief parse string containing uri, create "ndlcom::ExternalInterfaceBase"
 *
 * this function is a tooling function which reads a string and tries to parse
 * it into information about which kind of specialization for the low-level
 * "ExternalInterface" to create. suitable for commandline-parsing. knows about
 * the following types:
 *
 * "udp://localhost:$SRCPORT:$DSTPORT (default: 34000 and 34001)
 * "fpga:///dev/NDLCom"
 * "serial:///dev/ttyUSB:$BAUDRATE" (default: 921600)
 * "pipe:///tmp/testpipe"
 * "pty:///tmp/testpty"
 *
 * for information about the specific behaviour of returned interface classes
 * see their respective header.
 *
 * @param out stream for writing status messages
 * @param bridge reference to the bridge to attach the new interface to
 * @param uri user-given string saying which kind of interface to create and return
 * @param flags settings for the low-level "struct ndlcomExternalInterface"
 *
 * @return NULL on failure to parse, an "ndlcom::ExternalInterfaceBase" object
 *         in case parsing was successfull
 *
 */
class ndlcom::ExternalInterfaceBase *parseUriAndCreateInterface(
    std::ostream &out, struct NDLComBridge &bridge, std::string uri,
    uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

#endif /*NDLCOMBRIDGEPARSEURI_H*/
