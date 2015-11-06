#include "ndlcomBridgeParseUri.hpp"

#include <sstream>

class NDLComBridgeExternalInterface *
parseUriAndCreateInterface(struct NDLComBridge &bridge, std::string uri,
                           uint8_t flags) {

    std::string serial = "serial://";
    std::string udp = "udp://";
    std::string pipe = "pipe://";
    std::string fpga = "fpga://";
    std::string pty = "pty://";

    if (uri.compare(0, serial.length(), serial) == 0) {
        size_t begin_device = uri.find(serial) + serial.size();
        size_t begin_baud = uri.find_last_of(":") + 1;
        std::string device(
            uri.substr(begin_device, begin_baud - begin_device - 1));
        std::stringstream baudstring(uri.substr(begin_baud));
        speed_t baudrate = 0;
        baudstring >> baudrate;
        if (baudrate == 0) {
            baudrate = 921600;
            std::cout << "falling back to default baudrate of '" << baudrate
                      << "'\n";
        }
        std::cout << "opening serial '" << device << "' with " << baudrate
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
            inport = 34000;
            std::cout << "falling back to default inport of '" << inport
                      << "'\n";
        }
        if (outport == 0) {
            outport = 34001;
            std::cout << "falling back to default outport of '" << outport
                      << "'\n";
        }
        std::cout << "opening udp '" << hostname << "' with inport " << inport
                  << " and outport " << outport << "\n";
        return new NDLComBridgeUdp(bridge, hostname, inport, outport, flags);
    } else if (uri.compare(0, pipe.length(), pipe) == 0) {
        size_t begin_pipename = uri.find(pipe) + pipe.size();
        std::string pipename(uri.substr(begin_pipename));
        std::cout << "opening pipe '" << pipename << "'\n";
        return new NDLComBridgeNamedPipe(bridge, pipename, flags);
    } else if (uri.compare(0, fpga.length(), fpga) == 0) {
        size_t begin_fpganame = uri.find(fpga) + fpga.size();
        std::string fpganame(uri.substr(begin_fpganame));
        if (fpganame.empty())
            fpganame = "/dev/NDLCom";
        std::cout << "opening fpga '" << fpganame << "'\n";
        return new NDLComBridgeFpga(bridge, fpganame);
    } else if (uri.compare(0, pty.length(), pty) == 0) {
        size_t begin_ptyname = uri.find(pty) + pty.size();
        std::string ptyname(uri.substr(begin_ptyname));
        std::cout << "opening pty master '" << ptyname << "'\n";
        return new NDLComBridgePty(bridge, ptyname);
    }

    return NULL;
}
