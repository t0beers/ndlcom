#include "ndlcom/ExternalInterfaceParseUri.hpp"

#include "ndlcom/ExternalInterface.hpp"

const std::string serial = "serial://";
const std::string udp = "udp://";
const std::string pipe = "pipe://";
const std::string fpga = "fpga://";
const std::string pty = "pty://";

/**
 * TODO:
 * - datastructure with functions to be called for specific match-strings.
 *   allows to be extendeble by defining additional functions...
 */

#define DEFAULT_SERIAL_BAUDRATE 921600
#define DEFAULT_UDP_SRCPORT 34000
#define DEFAULT_UDP_DSTPORT 34001

class ndlcom::ExternalInterfaceBase *
ndlcom::ParseUriAndCreateExternalInterface(std::ostream &out,
                                           struct NDLComBridge &bridge,
                                           std::string uri, uint8_t flags) {

    if (uri.compare(0, serial.length(), serial) == 0) {
        size_t begin_device = uri.find(serial) + serial.size();
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
        return new NDLComBridgeSerial(bridge, device, baudrate, flags);

    } else if (uri.compare(0, udp.length(), udp) == 0) {
        size_t begin_hostname = uri.find(udp) + udp.size();
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
        if (inport == 0) {
            inport = DEFAULT_UDP_SRCPORT;
            out << "falling back to default inport of '" << inport << "'\n";
        }
        if (outport == 0) {
            outport = DEFAULT_UDP_DSTPORT;
            out << "falling back to default outport of '" << outport << "'\n";
        }
        out << "opening udp '" << hostname << "' with inport " << inport
            << " and outport " << outport << "\n";
        return new NDLComBridgeUdp(bridge, hostname, inport, outport, flags);

    } else if (uri.compare(0, pipe.length(), pipe) == 0) {
        size_t begin_pipename = uri.find(pipe) + pipe.size();
        std::string pipename(uri.substr(begin_pipename));
        out << "opening pipe '" << pipename << "'\n";
        return new NDLComBridgeNamedPipe(bridge, pipename, flags);

    } else if (uri.compare(0, fpga.length(), fpga) == 0) {
        size_t begin_fpganame = uri.find(fpga) + fpga.size();
        std::string fpganame(uri.substr(begin_fpganame));
        if (fpganame.empty())
            fpganame = "/dev/NDLCom";
        out << "opening fpga '" << fpganame << "'\n";
        return new NDLComBridgeFpga(bridge, fpganame);

    } else if (uri.compare(0, pty.length(), pty) == 0) {
        size_t begin_ptyname = uri.find(pty) + pty.size();
        std::string ptyname(uri.substr(begin_ptyname));
        out << "opening pty master '" << ptyname << "'\n";
        return new ExternalInterfacePty(bridge, ptyname);
    }

    // when reaching here, nothing was created
    return NULL;
}
