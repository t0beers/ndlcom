/**
 * TODO:
 * - what about security? ssh? accept all connection?
 *
 * <martin.zenzes@dfki.de> 2015
 *
 */
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>   /* for strerror() */

#include <cstdio>
#include <sstream>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <vector>
#include <cmath>

#include "ndlcom/ExternalInterfaceParseUri.hpp"
#include "ndlcom/ExternalInterface.hpp"

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/NodeHandler.hpp"

struct NDLComBridge bridge;

bool stopMainLoop = false;

double mainLoopFrequency_hz = 100.0;

/* all external interfaces */
std::vector<class ndlcom::ExternalInterfaceBase *> allInterfaces;
/* all internal "personalities", with optional printers attached */
std::vector<std::pair<struct NDLComNode *, class ndlcom::NodePrintOwnId *> >
    allNodes;

void signal_handler(int signal) { stopMainLoop = true; }

class ndlcom::BridgePrintAll *printAll = NULL;
class ndlcom::BridgePrintMissEvents *printMiss = NULL;

// TODO: use chrono from c++11
struct timespec diff(struct timespec start, struct timespec end) {
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

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
"Besides creating ordinary interfaces which will be used in the dynamic routing table, additional 'mirror interfaces' can requested as well. These will output a copy of _all_ passing messages and allows injecting arbritrary messages without updating the routing table. To print the content of the current routingTable press 'r' during normal operation.\n"
"\n"
"options:\n"
"--uri\t\t-u\tInterface to create. Possible: 'fpga', 'serial', 'pty', 'pipe', 'udp'\n"
"--mirrorUri\t-m\tMirror interface to create, otherwise the same as in '--uri'\n"
"--ownDeviceId\t-i\tCreates and adds a node to the bridge listening to this deviceId\n"
"--frequency\t-f\tPolling of the main-loop in Hz\n"
"--print-all\t-A\tPrint every packet\n"
"--print-own\t-O\tPrint packets directed at the last given 'ownDeviceId'\n"
"--print-miss\t-M\tPrint miss events of packets passing thorugh the bridge\n"
"--realtime\t-R\ttry to obtain realtime scheduling. needs root.\n"
"\n"
"examples:\n"
"\n"
"routing of messages from serial to udp on localhost, usable by CommonGui:\n"
"\n"
"\t%s -u udp://localhost:34000:34001 -u serial:///dev/ttyUSB0:921600\n"
"\n"
"route between kernel-module with udp-connection, print missing packages:\n"
"\n"
"\t%s -u fpga:///dev/NDLCom -u udp://localhost:34000:34001 -M\n"
"\n"
"create pseudoterminal and open this in CommonGui:\n"
"\n"
"\t%s -u pty:///tmp/symlink\n"
"\n"
"route from one hex-encoded pipe to another, print all passing packages:\n"
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

actualName.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), folder.c_str(), folder.c_str());
}
/* clang-format on */

void printRoutingTable(const struct NDLComBridge *b) {
    std::cout << "routingTable: \n";
    struct NDLComExternalInterface *externalInterface;
    if (list_empty(&b->externalInterfaceList)) {
        std::cout << "no external interfaces registered, the routingTable "
                     "should be kinda trivial...\n";
        return;
    }
    list_for_each_entry(externalInterface, &b->externalInterfaceList, list) {

        class ndlcom::ExternalInterfaceBase *base =
            static_cast<class ndlcom::ExternalInterfaceBase *>(
                externalInterface->context);
        std::cout << base->label << "\n";

        bool printed = false;
        for (size_t id = 0; id < 256; ++id) {
            if (b->routingTable.table[id] == externalInterface) {
                std::cout << id << " ";
                printed = true;
            }
        }
        if (!printed) {
            std::cout << "<none>";
        }
        std::cout << "\n";
    }
}

void handleInput() {
    fd_set s_rd;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&s_rd);
    FD_SET(fileno(stdin), &s_rd);
    int r = select(fileno(stdin) + 1, &s_rd, NULL, NULL, &tv);
    if (r != 0) {
        std::string entered;
        std::getline(std::cin, entered);
        if (entered == "r") {
            printRoutingTable(&bridge);
        }
    }
}

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
            {"realtime", no_argument, 0, 'R'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "u:m:i:f:AOMRh", long_options,
                        &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'u': {
            class ndlcom::ExternalInterfaceBase *ret =
                ndlcom::ParseUriAndCreateExternalInterface(std::cerr, bridge,
                                                           optarg);
            if (!ret) {
                std::cerr << "invalid uri: '" << optarg << "'\n";
                exit(EXIT_FAILURE);
            } else {
                allInterfaces.push_back(ret);
            }
            break;
        }
        case 'm': {
            class ndlcom::ExternalInterfaceBase *ret =
                ndlcom::ParseUriAndCreateExternalInterface(
                    std::cerr, bridge, optarg,
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
            std::pair<struct NDLComNode *, class ndlcom::NodePrintOwnId *> entry;
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
                printAll = new ndlcom::BridgePrintAll(bridge, std::cerr);
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
            std::pair<struct NDLComNode *, class ndlcom::NodePrintOwnId *> &
                lastEntry = allNodes.back();
            /* check that it does not have a "printOwnId" yet */
            if (lastEntry.second != NULL) {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            /* and create the own-id printer */
            lastEntry.second = new ndlcom::NodePrintOwnId(*lastEntry.first, std::cerr);
            break;
        }
        case 'M': {
            if (printMiss == NULL) {
                printMiss = new ndlcom::BridgePrintMissEvents(bridge, std::cerr);
            } else {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'R': {
            struct sched_param p;
            p.sched_priority = 99;
            std::cerr << "enabling SCHED_FIFO with priority "
                      << p.sched_priority << "\n";
            int r = sched_setscheduler(0, SCHED_FIFO, &p);
            if (r == -1) {
                std::cerr << "sched_setscheduler() failed: '" << strerror(errno) << "'\n";
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
                               class ndlcom::NodePrintOwnId *> >::iterator it =
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

    struct timespec last_ts, now_ts;
    clock_gettime(CLOCK_MONOTONIC, &last_ts);
    usleep(usleep_us);
    while (!stopMainLoop) {
        // printf("new loop\n");
        ndlcomBridgeProcess(&bridge);

        handleInput();

        // taking care to actually sleep the right number of microseconds as
        // specified in the commandline
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        struct timespec sinceLast_ts = diff(last_ts, now_ts);;
        last_ts = now_ts;

        // some juggling with numbers...
        int sinceLast_us = lrint(sinceLast_ts.tv_nsec / 1000.0) + sinceLast_ts.tv_sec * 1000000;
        // another uncomprehensible calculation:
        int processingDuration_us = sinceLast_us - usleep_us;
        if (processingDuration_us < 0) {
            processingDuration_us += usleep_us;
        }

        int finalSleepDuration_us = usleep_us - processingDuration_us;
        if (finalSleepDuration_us > 0) {
            usleep(usleep_us);
            /* printf("processing took %ius, will sleep for %ius\n", processingDuration_us, */
            /*        finalSleepDuration_us); */
        /* } else { */
            /* printf("processing took %ius, will not sleep\n", processingDuration_us); */
        }
    }

    // clean up our memory
    for (std::vector<class ndlcom::ExternalInterfaceBase *>::iterator it =
             allInterfaces.begin();
         it != allInterfaces.end(); ++it) {
        delete *it;
    }
    allInterfaces.clear();

    for (std::vector<std::pair<struct NDLComNode *,
                               class ndlcom::NodePrintOwnId *> >::iterator it =
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
