/**
 * TODO:
 * - what about security? ssh? accept all connection?
 *
 * <martin.zenzes@dfki.de> 2015
 *
 */
#include <getopt.h>
#include <unistd.h>

#include <cstdio>
#include <sstream>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <vector>
#include <cmath>

#include "ndlcomBridgeExternalInterface.hpp"
#include "ndlcomBridgeInternalHandler.hpp"

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

struct NDLComBridge bridge;

bool stopMainLoop = false;

double mainLoopFrequency_hz = 100.0;

/* all external interfaces */
std::vector<class NDLComBridgeExternalInterface *> allInterfaces;
/* all internal "personalities", with optional printers attached */
std::vector<std::pair<struct NDLComNode *, class NDLComNodePrintOwnId *> >
    allNodes;

void signal_handler(int signal) { stopMainLoop = true; }

class NDLComBridgePrintAll *printAll = NULL;
class NDLComBridgePrintMissEvents *printMiss = NULL;

class NDLComBridgeExternalInterface *parseUriAndCreateInterface(
    std::string uri, uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT) {

    std::string serial = "serial://";
    std::string udp = "udp://";
    std::string pipe = "pipe://";
    std::string fpga = "fpga://";

    if (uri.compare(0, serial.length(), serial) == 0) {
        size_t begin_device = uri.find(serial) + serial.size();
        size_t begin_baud = uri.find_last_of(":") + 1;
        std::string device(
            uri.substr(begin_device, begin_baud - begin_device - 1));
        std::stringstream baudstring(uri.substr(begin_baud));
        speed_t baudrate;
        baudstring >> baudrate;
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
    }

    return NULL;
}

void help(const char *_name) {
    std::string name(_name);
    size_t pos = name.find_last_of("/");
    std::string folder(name.substr(0, pos));
    std::string actualName(name.substr(pos+1));

    /* clang-format off */
    printf(
"%s\n"
"\n"
"Low-level tool to create multiple NDLCom-interfaces, connect them by routing messages as needed and listen to multiple nodeIds. Can print miss-events by observing the packet-counter of pasing messages\n"
"\n"
"Besides creating ordinary interfaces which will be used in the dynamic routing table, additional 'mirror interfaces' can requested as well. These will output a copy of _all_ passing messages and allows injecting arbritrary messages without updating the routing table\n"
"\n"
"options:\n"
"--uri\t\t-u\tInterface to create. Possible: 'fpga', 'serial', 'pipe', 'udp'\n"
"--mirrorUri\t-m\tMirror interface to create, otherwise the same as in '--uri'\n"
"--ownDeviceId\t-i\tCreates and adds a node to the bridge listening to this deviceId\n"
"--frequency\t-f\tPolling of the main-loop in Hz\n"
"--print-all\t-A\tPrint every packet\n"
"--print-own\t-O\tPrint packets directed at the last given 'ownDeviceId'\n"
"--print-miss\t-A\tPrint miss events of packets passing thorugh the bridge\n"
"\n"
"examples:\n"
"\n"
"routing of messages from serial to udp on localhost, usable by CommonGui:\n"
"\n"
"\t%s --uri udp://localhost:34001:34000 --uri serial:///dev/ttyUSB0:921600\n"
"\n"
"route from one hex-encoded pipe to another:\n"
"\n"
"\t%s -u pipe://pipeA -u pipe://pipeB --print-all\n"
"\n"
"the following will create random packages:\n"
"\n"
"\twhile (true); do\n"
"\t    sleep 0.5 ; %s/ndlcomPacketProducer -H > pipeA_in\n"
"\tdone\n"
"\n"
"the following command will print the packages on the second pipe:\n"
"\n"
"\ttail -n +1 -f pipeB_out | %s/ndlcomPacketConsumer\n"
"\n"
"NOTE: Be careful about buffering... does not work as smoothly as advertised.\n",

actualName.c_str(), name.c_str(), name.c_str(), folder.c_str(), folder.c_str());
}
/* clang-format on */

int main(int argc, char *argv[]) {

    ndlcomBridgeInit(&bridge);

    /* option handling is based on the manpage optarg(3). */
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"uri", required_argument, 0, 'u'},
            {"mirrorUri", required_argument, 0, 'm'},
            {"ownDeviceId", required_argument, 0, 'i'},
            {"frequency", required_argument, 0, 'f'},
            {"print-all", no_argument, 0, 'A'},
            {"print-own", no_argument, 0, 'O'},
            {"print-miss", no_argument, 0, 'M'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "u:m:i:f:AOMh", long_options,
                        &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'u': {
            class NDLComBridgeExternalInterface *ret =
                parseUriAndCreateInterface(optarg);
            if (!ret) {
                std::cerr << "invalid uri: '" << optarg << "'\n";
                exit(EXIT_FAILURE);
            } else {
                allInterfaces.push_back(ret);
            }
            break;
        }
        case 'm': {
            class NDLComBridgeExternalInterface *ret =
                parseUriAndCreateInterface(
                    optarg, NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR);
            if (!ret) {
                std::cerr << "invalid uri: '" << optarg << "'\n";
                exit(EXIT_FAILURE);
            } else {
                allInterfaces.push_back(ret);
            }
            break;
        }
        case 'i': {
            std::istringstream ss(optarg);
            int tempId;
            ss >> tempId;
            std::pair<struct NDLComNode *, class NDLComNodePrintOwnId *> entry;
            entry.first = new struct NDLComNode;
            entry.second = NULL;
            ndlcomNodeInit(entry.first, &bridge, tempId);
            allNodes.push_back(entry);
            break;
        }
        case 'f': {
            std::istringstream ss(optarg);
            ss >> mainLoopFrequency_hz;
            break;
        }
        // these will be deleted implicitly on programm exit
        case 'A': {
            if (printAll == NULL) {
                printAll = new NDLComBridgePrintAll(bridge);
            } else {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'O': {
            /* check that we have "nodes" already, where we'll add the printer
             * to the last one */
            if (allNodes.empty()) {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            /* so get the last node added as a reference */
            std::pair<struct NDLComNode *, class NDLComNodePrintOwnId *>
                &lastEntry = allNodes.back();
            /* check that it does not have a "printOwnId" yet */
            if (lastEntry.second != NULL) {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            /* and create the own-id printer */
            lastEntry.second = new NDLComNodePrintOwnId(*lastEntry.first);
            break;
        }
        case 'M': {
            if (printMiss == NULL) {
                printMiss = new NDLComBridgePrintMissEvents(bridge);
            } else {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'h':
        case '?':
        default:
            help(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    if (optind != argc) {
        help(argv[0]);
        exit(EXIT_FAILURE);
    }

    std::signal(SIGINT, signal_handler);

    useconds_t usleep_us = round(1. / mainLoopFrequency_hz * 1000000);
    std::cout << "using update rate of " << mainLoopFrequency_hz
              << "Hz (update every " << usleep_us << "us)\n";

    for (std::vector<std::pair<struct NDLComNode *,
                               class NDLComNodePrintOwnId *> >::iterator it =
             allNodes.begin();
         it != allNodes.end(); ++it) {
        if ((*it).second != NULL) {
            printf("printing all messages for receiverId 0x%02x\n",
                   (*it).first->headerConfig.mOwnSenderId);
        } else {
            printf("silently listening to receiverId 0x%02x\n",
                   (*it).first->headerConfig.mOwnSenderId);
        }
    }

    while (!stopMainLoop) {
        // printf("new loop\n");
        ndlcomBridgeProcess(&bridge);
        // ndlcomNodeSend(allNodes.back().first, 0x09, 0, 0);

        usleep(usleep_us);
    }

    // clean up our memory
    for (std::vector<class NDLComBridgeExternalInterface *>::iterator it =
             allInterfaces.begin();
         it != allInterfaces.end(); ++it) {
        delete *it;
    }
    allInterfaces.clear();

    for (std::vector<std::pair<struct NDLComNode *,
                               class NDLComNodePrintOwnId *> >::iterator it =
             allNodes.begin();
         it != allNodes.end(); ++it) {
        ndlcomNodeDeinit((*it).first);
        if ((*it).second) {
            delete (*it).second;
        }
        delete (*it).first;
    }
    allNodes.clear();

    /* more cleaning up */
    if (printAll) {
        delete printAll;
        printAll = NULL;
    }
    if (printMiss) {
        delete printMiss;
        printMiss = NULL;
    }

    std::cout << "quitting\n";

    exit(EXIT_SUCCESS);
}
