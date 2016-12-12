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
#include <memory>
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

#include "ndlcom/Bridge.hpp"
#include "ndlcom/Node.h"

#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/NodeHandler.hpp"

class ndlcom::Bridge bridge;

bool stopMainLoop = false;

double mainLoopFrequency_hz = 100.0;

void signal_handler(int signal) { stopMainLoop = true; }

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
"--print-own\t-O\tPrint packets directed at the given 'deviceId'\n"
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
            bridge.printRoutingTable();
        } else if (entered == "q") {
            stopMainLoop = true;
        } else if (entered == "s") {
            bridge.printStatus();
        }
    }
}

int main(int argc, char *argv[]) {

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
            {"print-own", required_argument, 0, 'O'},
            {"print-miss", no_argument, 0, 'M'},
            {"realtime", no_argument, 0, 'R'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "u:m:i:f:AO:MRh", long_options,
                        &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'u': {
            auto ret = bridge.createInterface(optarg);
            if (!ret) {
                std::cerr << "invalid uri: '" << optarg << "'\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'm': {
            auto ret = bridge.createInterface(
                optarg, NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR);
            if (!ret) {
                std::cerr << "invalid uri: '" << optarg << "'\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'i': {
            std::istringstream ss(optarg);
            int tempId;
            ss >> tempId;
            bridge.enableOwnId(tempId, false);
            break;
        }
        case 'f': {
            std::istringstream ss(optarg);
            ss >> mainLoopFrequency_hz;
            break;
        }
        // these will be deleted implicitly on programm exit
        case 'A': {
            bridge.enablePrintAll();
            break;
        }
        case 'O': {
            std::istringstream ss(optarg);
            int tempId;
            ss >> tempId;
            bridge.enableOwnId(tempId, true);
            break;
        }
        case 'M': {
            bridge.enablePrintMiss();
            break;
        }
        case 'R': {
            struct sched_param p;
            p.sched_priority = 99;
            std::cerr << "enabling SCHED_FIFO with priority "
                      << p.sched_priority << "\n";
            int r = sched_setscheduler(0, SCHED_FIFO, &p);
            if (r == -1) {
                std::cerr << "sched_setscheduler() failed: '" << strerror(errno)
                          << "'\n";
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

    bridge.printStatus();

    struct timespec last_ts, now_ts;
    clock_gettime(CLOCK_MONOTONIC, &last_ts);
    usleep(usleep_us);
    while (!stopMainLoop) {
        // printf("new loop\n");
        bridge.process();

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

    std::cerr << "quitting\n";

    exit(EXIT_SUCCESS);
}
