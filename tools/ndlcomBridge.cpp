/**
 * TODO:
 * - what about security? ssh? accept all connection?
 *
 * <martin.zenzes@dfki.de> 2015
 *
 */
#include <getopt.h>
#include <errno.h>
#include <memory>
#include <string.h> /* for strerror() */

#include <sstream>
#include <iostream>
#include <csignal>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread> /* sleep_until */

#include "ndlcom/ExternalInterface.hpp"

#include "ndlcom/Bridge.hpp"
#include "ndlcom/Node.h"

#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/NodeHandler.hpp"

class ndlcom::Bridge bridge;

bool stopMainLoop = false;

double mainLoopFrequency_hz = 1000.0;

void signal_handler(int signal) { stopMainLoop = true; }

void help(const char *_name) {
    std::string name(_name);
    size_t pos = name.find_last_of("/");
    std::string folder(name.substr(0, pos));
    std::string actualName(name.substr(pos + 1));

    /* clang-format off */
    fprintf(stderr,
"\n%s\n\n"
"Low-level tool to create multiple NDLCom-interfaces, connect them by routing messages as needed and possibly listen to multiple nodeIds. Can print miss-events by observing the packet-counter of passing messages. During runtime the mainloop will listen to 'q' (quit), 's' (status) and 'r' (print routing table). Additionally, sending a SIGINT (ctrl-c) will also close the program in a orderly way.\n"
"\n"
"Besides creating ordinary interfaces which will be used in the dynamic routing table, additional 'mirror interfaces' can requested as well. These will output a copy of _all_ passing messages and forwards incoming messages without updating the routing table.\n"
"\n"
"DeviceIds which are reachable behind an interfrace can be appended in the form '&id1,id2,id3' to the interface description. Thus it is possible to preconfigure the internal routing table in known environments\n"
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
"Specify that deviceIds 2 to 6 are reachable on localhost:\n"
"\n"
"\t%s -u \"udp://localhost:34000:34001&2,3,4,5,6\"\n"
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
,
actualName.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str());
}
/* clang-format on */

void handleInput() {
    fd_set s_rd;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&s_rd);
    FD_SET(fileno(stdin), &s_rd);
    int r = select(fileno(stdin) + 1, &s_rd, nullptr, nullptr, &tv);
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
            if (ret.expired()) {
                std::cerr << "invalid uri: '" << optarg << "'\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'm': {
            auto ret = bridge.createInterface(
                optarg, NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR);
            if (ret.expired()) {
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

    std::chrono::microseconds sleepTime(
        std::lround(1. / mainLoopFrequency_hz * 1000000L));
    std::cerr << "using update rate of " << mainLoopFrequency_hz
              << "Hz (update every " << sleepTime.count() << "us)\n\n";

    bridge.printStatus();

    std::chrono::time_point<std::chrono::high_resolution_clock> nextProcessing =
        std::chrono::system_clock::now();
    while (!stopMainLoop) {

        /* printf("new loop\n"); */
        bridge.process();

        // check for keyboard-input to create a cheap user-interface
        handleInput();

        // take care that we sleep enough, but not too long
        nextProcessing += sleepTime;
        std::this_thread::sleep_until(nextProcessing);
    }

    std::cerr << "quitting\n";

    exit(EXIT_SUCCESS);
}
