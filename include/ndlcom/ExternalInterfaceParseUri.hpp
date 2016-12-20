#ifndef NDLCOMBRIDGEPARSEURI_H
#define NDLCOMBRIDGEPARSEURI_H

#include <sstream>
#include <memory>
#include <regex>

#include "ndlcom/ExternalInterface.hpp"

namespace ndlcom {

/**
 * @brief Parse string containing "uri", create "ndlcom::ExternalInterfaceBase"
 *
 * This function is a tooling function which reads a string and tries to parse
 * it into information about which kind of specialization for the low-level
 * "ExternalInterface" to create. Suitable for commandline-parsing. Knows about
 * the following types:
 *
 * "udp://localhost:$SRCPORT:$DSTPORT (default: 34000 and 34001)
 * "fpga:///dev/NDLCom"
 * "serial:///dev/ttyUSB0:$BAUDRATE" (default: 921600)
 * "pipe:///tmp/testpipe"
 * "pty:///tmp/testpty"
 * "tcpclient://localhost:$PORT" (default: 2000)
 *
 * Every uri might have a trailing string specifying apriori information
 * concerning the NDLComRoutingTable for this ExternalInterface in the format
 * "&1,2,3".
 *
 * For information about the specific behaviour of returned interface classes
 * see their respective header.
 *
 * This function does not take ownership of the returned objects.
 *
 * @param out stream for writing status messages
 * @param bridge reference to the bridge to attach the new interface to
 * @param uri string stating which kind of interface to create and return
 * @param flags settings for the low-level "struct ndlcomExternalInterface"
 *
 * @return nullptr on failure, unique_ptr of an ExternalInterfaceBase otherwise
 */
std::unique_ptr<class ndlcom::ExternalInterfaceBase>
ParseUriAndCreateExternalInterface(
    std::ostream &out, struct NDLComBridge &bridge, const std::string &uri,
    uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

/**
 * similar function which tries to create the interface-type given as
 * template-argument by parsing the given string.
 *
 * This function is implemented for all known interfaces classes and  actually
 * used by ParseUriAndCreateExternalInterface.
 *
 * Will also register the new ExternalInterface at the given NDLComBridge
 *
 * Returns an object created on the stack
 */
template <typename T, typename = typename std::enable_if<std::is_base_of<
                          class ndlcom::ExternalInterfaceBase, T>::value>::type>
T createByParse(std::ostream &out, struct NDLComBridge &bridge,
                const std::string uri,
                uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

/**
 * Regexes to validate valid uri strings for each different interface type.
 *
 * Granted, the naming and location is kinda stupid...
 */
extern const std::regex pty_rx;
extern const std::regex pipe_rx;
extern const std::regex udp_rx;
extern const std::regex fpga_rx;
extern const std::regex serial_rx;
extern const std::regex tcpClient_rx;

} // of namespace

#endif /*NDLCOMBRIDGEPARSEURI_H*/
