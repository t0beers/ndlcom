/**
 * <martin.zenzes@dfki.de> 2015
 *
 * TODO:
 * - add "debug mirror" class of ports. supposed so send out _all_ messages
 *   going through the bridge, and accept messages, using "internal" as
 *   "origin"
 * - what about security? ssh? accept all connection?
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

#define NDLCOM_BRIDGE_DEFAULT_SENDER_ID 0x02
struct NDLComBridge bridge;

bool stopMainLoop = false;

double mainLoopFrequency_hz = 100.0;

std::vector<class NDLComBridgeExternalInterface*> allInterfaces;

void help(const char *name) { std::cout << "\nhere comes dragons!\n"; }

void signal_handler(int signal) {
    stopMainLoop = true;
}

class NDLComBridgeExternalInterface* parseUriAndCreateInterface(std::string uri) {
    std::string serial = "serial://";
    std::string udp = "udp://";

    if (uri.compare(0, serial.length(), serial) == 0) {
        size_t begin_device = uri.find(serial) + serial.size();
        size_t begin_baud = uri.find_last_of(":") + 1;
        std::string device(
            uri.substr(begin_device, begin_baud - begin_device - 1));
        std::stringstream baudstring(uri.substr(begin_baud));
        speed_t baudrate;
        baudstring >> baudrate;
        std::cout << "opening '" << device << "' with " << baudrate << "baud\n";
        return new NDLComBridgeSerial(bridge, device, baudrate);
    } else if (uri.compare(0, udp.length(), udp) == 0) {
        size_t begin_hostname = uri.find(udp) + udp.size();
        size_t begin_inport = uri.find(":", begin_hostname) + 1;
        size_t begin_outport = uri.find(":", begin_inport) + 1;
        std::string hostname(
            uri.substr(begin_hostname, begin_inport - begin_hostname - 1));
        std::stringstream inportstring(
            uri.substr(begin_inport, begin_outport - begin_inport - 1));
        std::stringstream outportstring(
            uri.substr(begin_outport));
        unsigned int inport;
        unsigned int outport;
        inportstring >> inport;
        outportstring >> outport;
        std::cout << "opening '" << hostname << "' with inport " << inport
                  << " and outport " << outport << "\n";
        return new NDLComBridgeUdp(bridge, hostname, inport, outport);
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    ndlcomBridgeInit(&bridge, NDLCOM_BRIDGE_DEFAULT_SENDER_ID);

    /* option handling is based on the manpage optarg(3). */
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"uri", required_argument, 0, 'u'},
            {"ownSenderId", required_argument, 0, 'i'},
            {"frequency", required_argument, 0, 'f'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "u:i:f:h", long_options, &option_index);
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
        case 'i': {
            std::istringstream ss(optarg);
            int tempInt;
            ss >> tempInt;
            ndlcomBridgeSetOwnSenderId(&bridge, tempInt);
            break;
        }
        case 'f': {
            std::istringstream ss(optarg);
            ss >> mainLoopFrequency_hz;
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

    // initialize this object _after_ "ndlcomBridgeInit()" was called
    class NDLComBridgePrintAll printAllReceivedPackages(bridge);
    class NDLComBridgePrintOwnId printOwnPackages(bridge);

    std::signal(SIGINT, signal_handler);

    useconds_t usleep_us = round(1. / mainLoopFrequency_hz * 1000000);
    std::cout << "using update rate of " << mainLoopFrequency_hz
              << "Hz (update every " << usleep_us << "us)\n";
    printf("using senderId 0x%02x\n", bridge.headerConfig.mOwnSenderId);

    while (!stopMainLoop) {
        //printf("new loop\n");
        ndlcomBridgeProcess(&bridge);

        ndlcomBridgeSend(&bridge, 0x01, 0, 0);

        usleep(usleep_us);
    }

    // clean up our memory
    for (std::vector<class NDLComBridgeExternalInterface *>::iterator it =
             allInterfaces.begin();
         it != allInterfaces.end(); ++it) {
        delete *it;
    }
    allInterfaces.clear();

    std::cout << "quitting\n";

    exit(EXIT_SUCCESS);
}
