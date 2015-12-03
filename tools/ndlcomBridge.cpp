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

#include "ndlcomBridgeParseUri.hpp"
#include "ndlcomBridgeExternalInterface.hpp"
#include "ndlcom/InternalHandler.hpp"

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

void help(const char *_name) {
    std::string name(_name);
    size_t pos = name.find_last_of("/");
    std::string folder(name.substr(0, pos));
    std::string actualName(name.substr(pos + 1));

    /* clang-format off */
    fprintf(stderr,
"\n%s\n\n"
"Low-level tool to create multiple NDLCom-interfaces, connect them by routing messages as needed and listen to multiple nodeIds. Can print miss-events by observing the packet-counter of pasing messages\n"
"\n"
"Besides creating ordinary interfaces which will be used in the dynamic routing table, additional 'mirror interfaces' can requested as well. These will output a copy of _all_ passing messages and allows injecting arbritrary messages without updating the routing table\n"
"\n"
"options:\n"
"--uri\t\t-u\tInterface to create. Possible: 'fpga', 'serial', 'pty', 'pipe', 'udp'\n"
"--mirrorUri\t-m\tMirror interface to create, otherwise the same as in '--uri'\n"
"--ownDeviceId\t-i\tCreates and adds a node to the bridge listening to this deviceId\n"
"--frequency\t-f\tPolling of the main-loop in Hz\n"
"--print-all\t-A\tPrint every packet\n"
"--print-own\t-O\tPrint packets directed at the last given 'ownDeviceId'\n"
"--print-miss\t-M\tPrint miss events of packets passing thorugh the bridge\n"
"\n"
"examples:\n"
"\n"
"routing of messages from serial to udp on localhost, usable by CommonGui:\n"
"\n"
"\t%s -u udp://localhost:34001:34000 -u serial:///dev/ttyUSB0:921600\n"
"\n"
"create pseudoterminal and open this in CommonGui:\n"
"\n"
"\t%s -u pty:///tmp/symlink\n"
"\n"
"route from one hex-encoded pipe to another:\n"
"\n"
"\t%s -u pipe://pipeA -u pipe://pipeB -A\n"
"\n"
"then create random packages and write them into the first pipe:\n"
"\n"
"\twhile (true); do\n"
"\t    sleep 0.5 ; %s/ndlcomPacketProducer -H > pipeA_rx\n"
"\tdone\n"
"\n"
"then read and print the packets from the second pipe:\n"
"\n"
"\tstdbuf -i0 %s/ndlcomPacketConsumer < pipeB_tx\n"
"\n"
"NOTE: Be careful about stdio-buffering...\n",

actualName.c_str(), name.c_str(), name.c_str(), name.c_str(), folder.c_str(), folder.c_str());
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
                parseUriAndCreateInterface(bridge, optarg);
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
                    bridge, optarg,
                    NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR);
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
            std::pair<struct NDLComNode *, class NDLComNodePrintOwnId *> &
                lastEntry = allNodes.back();
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
    std::cerr << "using update rate of " << mainLoopFrequency_hz
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
    if (printAll) {
        printf("printing all messages passing through the bridge\n");
    }
    if (printMiss) {
        printf("printing miss-events for passing message streams\n");
    }

    while (!stopMainLoop) {
        // printf("new loop\n");
        ndlcomBridgeProcess(&bridge);
        /* when a Node is sending here, it will see its own message! */
        // if (!allNodes.empty())
        //    ndlcomNodeSend(allNodes.back().first, 0xff, 0, 0);

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

    std::cerr << "quitting\n";

    exit(EXIT_SUCCESS);
}
