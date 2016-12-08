#include "ndlcom/ExternalInterfaceParseUri.hpp"

#include "ndlcom/ExternalInterface.hpp"

#include <vector>
#include <limits>
#include <stdlib.h>

const std::string uri_prefix_serial = "serial://";
const std::string uri_prefix_udp = "udp://";
const std::string uri_prefix_pipe = "pipe://";
const std::string uri_prefix_fpga = "fpga://";
const std::string uri_prefix_pty = "pty://";
const std::string uri_prefix_tcpclient = "tcpclient://";

/**
 * TODO:
 * - datastructure with functions to be called for specific match-strings.
 *   allows to be extendeble by defining additional functions... regexes would
 *   be nice...
 * - think about generic factory to allow creation from other places
 */

#define DEFAULT_SERIAL_BAUDRATE 921600
#define DEFAULT_UDP_SRCPORT 34000
#define DEFAULT_UDP_DSTPORT 34001
#define DEFAULT_TCP_PORT 2000

// split one "delim" separated string into several substrings
std::vector<std::string> splitStringIntoStrings(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<NDLComId>
convertStringToIds(const std::vector<std::string> &numbers, std::ostream &out) {
    std::vector<NDLComId> retval;
    for (std::vector<std::string>::const_iterator it = numbers.begin();
         it != numbers.end(); it++) {
        int number = atoi((*it).c_str());
        if (number < std::numeric_limits<NDLComId>::min()) {
            out << "ParseUri: too small a deviceId '" << (*it) << "'\n";
        }
        if (number > std::numeric_limits<NDLComId>::max()) {
            out << "ParseUri: too large a deviceId '" << (*it) << "'\n";
        }
        retval.push_back(number);
    }
    return retval;
}

std::shared_ptr<class ndlcom::ExternalInterfaceBase >ndlcom::ParseUriAndCreateExternalInterface(
    std::ostream &out, struct NDLComBridge &bridge, const std::string &uriIn,
    uint8_t flags) {

    std::shared_ptr<class ExternalInterfaceBase> retval; // initialized to zero
    size_t ampersand_position = uriIn.find_last_of("&");
    std::string deviceIdsForRoutingTable;
    std::string uri;
    if (ampersand_position != std::string::npos) {
        uri = uriIn.substr(0, ampersand_position);
        deviceIdsForRoutingTable = uriIn.substr(ampersand_position +1);
    } else {
        uri = uriIn;
    }

    if (uri.compare(0, uri_prefix_serial.length(), uri_prefix_serial) == 0) {
        size_t begin_device =
            uri.find(uri_prefix_serial) + uri_prefix_serial.size();
        size_t begin_baud = uri.find_last_of(":") + 1;
        std::string device(
            uri.substr(begin_device, begin_baud - begin_device - 1));
        std::stringstream baudstring(uri.substr(begin_baud));
        speed_t baudrate = 0;
        baudstring >> baudrate;
        if (baudrate == 0) {
            baudrate = DEFAULT_SERIAL_BAUDRATE;
            out << "falling back to default baudrate of '" << baudrate << "'\n";
        }
        out << "opening serial '" << device << "' with " << baudrate
            << "baud\n";
        retval = std::make_shared<class ndlcom::ExternalInterfaceSerial>(
            bridge, device, baudrate, flags);

    } else if (uri.compare(0, uri_prefix_udp.length(), uri_prefix_udp) == 0) {
        size_t begin_hostname =
            uri.find(uri_prefix_udp) + uri_prefix_udp.size();
        size_t begin_inport = uri.find(":", begin_hostname) + 1;
        size_t begin_outport = uri.find(":", begin_inport) + 1;
        std::string hostname(
            uri.substr(begin_hostname, begin_inport - begin_hostname - 1));
        std::stringstream inportstring(
            uri.substr(begin_inport, begin_outport - begin_inport - 1));
        std::stringstream outportstring(uri.substr(begin_outport));
        unsigned int inport;
        unsigned int outport;
        inportstring >> inport;
        outportstring >> outport;
        if ((inportstring.str().length() == 0) || (inport == 0)) {
            inport = DEFAULT_UDP_SRCPORT;
            out << "falling back to default inport of '" << inport << "'\n";
        }
        if ((outportstring.str().length() == 0) || (outport == 0) ||
            (outport == inport)) {
            outport = DEFAULT_UDP_DSTPORT;
            out << "falling back to default outport of '" << outport << "'\n";
        }
        out << "opening udp '" << hostname << "' with inport " << inport
            << " and outport " << outport << "\n";
        retval = std::make_shared<class ndlcom::ExternalInterfaceUdp>(
            bridge, hostname, inport, outport, flags);

    } else if (uri.compare(0, uri_prefix_pipe.length(), uri_prefix_pipe) == 0) {
        size_t begin_pipename =
            uri.find(uri_prefix_pipe) + uri_prefix_pipe.size();
        std::string pipename(uri.substr(begin_pipename));
        out << "opening pipe '" << pipename << "'\n";
        retval = std::make_shared<class ndlcom::ExternalInterfacePty>(
            bridge, pipename, flags);

    } else if (uri.compare(0, uri_prefix_fpga.length(), uri_prefix_fpga) == 0) {
        size_t begin_fpganame =
            uri.find(uri_prefix_fpga) + uri_prefix_fpga.size();
        std::string fpganame(uri.substr(begin_fpganame));
        if (fpganame.empty())
            fpganame = "/dev/NDLCom";
        out << "opening fpga '" << fpganame << "'\n";
        retval = std::make_shared<class ndlcom::ExternalInterfaceFpga>(
            bridge, fpganame);

    } else if (uri.compare(0, uri_prefix_pty.length(), uri_prefix_pty) == 0) {
        size_t begin_ptyname = uri.find(uri_prefix_pty) + uri_prefix_pty.size();
        std::string ptyname(uri.substr(begin_ptyname));
        out << "opening pty master '" << ptyname << "'\n";
        retval = std::make_shared<class ndlcom::ExternalInterfacePty>(bridge, ptyname);
    } else if (uri.compare(0, uri_prefix_tcpclient.length(),
                           uri_prefix_tcpclient) == 0) {
        size_t begin_hostname =
            uri.find(uri_prefix_tcpclient) + uri_prefix_tcpclient.size();
        size_t begin_port = uri.find(":", begin_hostname) + 1;
        std::string hostname(
            uri.substr(begin_hostname, begin_port - begin_hostname - 1));
        std::stringstream portstring(uri.substr(begin_port));
        unsigned int port;
        portstring >> port;
        if ((portstring.str().length() == 0) || (port == 0)) {
            port = DEFAULT_TCP_PORT;
            out << "falling back to default port of '" << port << "'\n";
        }
        out << "opening tcpclient '" << hostname << "'\n";
        retval = std::make_shared<class ndlcom::ExternalInterfaceTcpClient>(
            bridge, hostname, port);
    }

    if (!retval) {
        out << "ParseUri: could not create any interface from string '" + uri +
                   "'";
        return NULL;
    }

    std::vector<NDLComId> ids = convertStringToIds(
        splitStringIntoStrings(deviceIdsForRoutingTable, ','), out);

    for (std::vector<NDLComId>::const_iterator it = ids.begin();
         it != ids.end(); it++) {
        out << "ParseUri: adding deviceId " << (int)(*it) << " for interface '"
            << retval->label << "'\n";
        ndlcomBridgeAddRoutingInformationForDeviceId(&bridge, *it,
                                                     retval->getInterface());
    }

    // done...
    return retval;
}
