#include "ndlcom/ExternalInterfaceParseUri.hpp"

#include "ndlcom/ExternalInterface.hpp"

#include <vector>
#include <limits>
#include <stdlib.h>
#include <iomanip>
#include <regex>

/* clang-format off */
const std::regex ndlcom::pty_rx("^pty://([^:&]*)(?:&(.*))?$");
const std::regex ndlcom::pipe_rx("^pipe://([^:&]*)(?:&(.*))?$");
const std::regex ndlcom::udp_rx("^udp://([^:&]*)(?::(\\d+))?(?::(\\d+))?(?:&(.*))?$");
const std::regex ndlcom::fpga_rx("^fpga://([^:&]*)(?:&:(.*))?$");
const std::regex ndlcom::serial_rx("^serial://([^:&]*)(?::(\\d+))?(?:&(.*))?$");
const std::regex ndlcom::tcpClient_rx("^tcpclient://([^:&]*)(?::(\\d+))?(?:&(.*))?$");
/* clang-format on */

const int defaultSerialBaudrate = 921600;
const int defaultUdpSrcPort = 34000;
const int defaultUdpDstPort = 34001;
const int defaultTcpPort = 2000;

/**
 * TODO:
 * - datastructure with functions to be called for specific match-strings.
 *   allows to be extendeble by defining additional functions... regexes would
 *   be nice...
 * - think about generic factory to allow creation from other places
 */

// split one "delim" separated string into several substrings
std::vector<std::string> splitStringIntoStrings(std::string s,
                                                const char delim) {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<NDLComId> convertStringToIds(std::vector<std::string> numbers,
                                         std::ostream &out) {
    std::vector<NDLComId> retval;
    for (auto it : numbers) {
        int number = std::stoi(it);
        if (number == NDLCOM_ADDR_BROADCAST) {
            out << "ParseUri: will ignore broadcast\n";
            continue;
        }
        if (number < std::numeric_limits<NDLComId>::min()) {
            out << "ParseUri: too small a deviceId '" << it << "'\n";
        }
        if (number > std::numeric_limits<NDLComId>::max()) {
            out << "ParseUri: too large a deviceId '" << it << "'\n";
        }
        retval.push_back(number);
    }
    return retval;
}

/**
 * will also register the ExternalInterface at the Bridge
 */
template <typename T>
void setRouting(T &retval, std::string conn, std::ostream &out) {
    // HAVE to register the interface, so that we can modify the routing table
    // if we are asked. has to be done after the ctor is finished to prevent
    // premature calling of handler.
    retval.registerHandler();

    std::vector<NDLComId> ids =
        convertStringToIds(splitStringIntoStrings(conn, ','), out);

    for (auto it : ids) {
        out << "ParseUri: set routingTable to use '" << retval.label
            << "' for deviceId 0x" << std::setfill('0') << std::hex
            << std::setw(2) << (int)(it) << std::dec << "\n";
        retval.setRoutingForDeviceId(it);
    }
}

namespace ndlcom {
template <>
class ndlcom::ExternalInterfaceSerial
createByParse(std::ostream &out, struct NDLComBridge &bridge,
              const std::string uri, uint8_t flags) {

    std::smatch matches;
    std::regex_match(uri, matches, serial_rx);

    speed_t baudrate = matches[2].length() ? std::stoi(matches[2].str())
                                           : defaultSerialBaudrate;

    class ndlcom::ExternalInterfaceSerial retval(bridge, matches[1], baudrate,
                                                 flags);
    setRouting(retval, matches[3], out);
    return retval;
}

template <>
class ndlcom::ExternalInterfaceUdp
createByParse(std::ostream &out, struct NDLComBridge &bridge,
              const std::string uri, uint8_t flags) {

    std::smatch matches;
    std::regex_match(uri, matches, udp_rx);

    int srcPort =
        matches[2].length() ? std::stoi(matches[2].str()) : defaultUdpSrcPort;
    int dstPort =
        matches[3].length() ? std::stoi(matches[3].str()) : defaultUdpDstPort;

    class ndlcom::ExternalInterfaceUdp retval(bridge, matches[1], srcPort,
                                              dstPort, flags);
    setRouting(retval, matches[4], out);
    return retval;
}

template <>
class ndlcom::ExternalInterfaceTcpClient
createByParse(std::ostream &out, struct NDLComBridge &bridge,
              const std::string uri, uint8_t flags) {

    std::smatch matches;
    std::regex_match(uri, matches, tcpClient_rx);

    int port =
        matches[2].length() ? std::stoi(matches[2].str()) : defaultTcpPort;

    class ndlcom::ExternalInterfaceTcpClient retval(bridge, matches[1], port,
                                                    flags);
    setRouting(retval, matches[3], out);
    return retval;
}

template <>
class ndlcom::ExternalInterfacePipe
createByParse(std::ostream &out, struct NDLComBridge &bridge,
              const std::string uri, uint8_t flags) {

    std::smatch matches;
    std::regex_match(uri, matches, pipe_rx);

    class ndlcom::ExternalInterfacePipe retval(bridge, matches[1], flags);
    setRouting(retval, matches[2], out);
    return retval;
}

template <>
class ndlcom::ExternalInterfaceFpga
createByParse(std::ostream &out, struct NDLComBridge &bridge,
              const std::string uri, uint8_t flags) {

    std::smatch matches;
    std::regex_match(uri, matches, fpga_rx);

    class ndlcom::ExternalInterfaceFpga retval(bridge, matches[1], flags);
    setRouting(retval, matches[2], out);
    return retval;
}

template <>
class ndlcom::ExternalInterfacePty
createByParse(std::ostream &out, struct NDLComBridge &bridge,
              const std::string uri, uint8_t flags) {

    std::smatch matches;
    std::regex_match(uri, matches, pty_rx);

    class ndlcom::ExternalInterfacePty retval(bridge, matches[1], flags);
    setRouting(retval, matches[2], out);
    return retval;
}
}

std::unique_ptr<class ndlcom::ExternalInterfaceBase>
ndlcom::ParseUriAndCreateExternalInterface(std::ostream &out,
                                           struct NDLComBridge &bridge,
                                           const std::string &uri,
                                           uint8_t flags) {

    std::unique_ptr<class ExternalInterfaceBase> retval; // initialized to zero

    // try all different regexes for a match
    if (std::regex_match(uri, serial_rx)) {
        retval.reset(new ExternalInterfaceSerial(
            createByParse<ExternalInterfaceSerial>(out, bridge, uri, flags)));
    } else if (std::regex_match(uri, udp_rx)) {
        retval.reset(new ExternalInterfaceUdp(
            createByParse<ExternalInterfaceUdp>(out, bridge, uri, flags)));
    } else if (std::regex_match(uri, pipe_rx)) {
        retval.reset(new ExternalInterfacePipe(
            createByParse<ExternalInterfacePipe>(out, bridge, uri, flags)));
    } else if (std::regex_match(uri, fpga_rx)) {
        retval.reset(new ExternalInterfaceFpga(
            createByParse<ExternalInterfaceFpga>(out, bridge, uri, flags)));
    } else if (std::regex_match(uri, pty_rx)) {
        retval.reset(new ExternalInterfacePty(
            createByParse<ExternalInterfacePty>(out, bridge, uri, flags)));
    } else if (std::regex_match(uri, tcpClient_rx)) {
        retval.reset(new ExternalInterfaceTcpClient(
            createByParse<ExternalInterfaceTcpClient>(out, bridge, uri,
                                                      flags)));
    }

    if (!retval) {
        out << "ParseUri: could not create any interface from string '" + uri +
                   "'\n";
        return retval;
    }

    // done...
    return retval;
}
