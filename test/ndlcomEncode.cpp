//
// escape a ndlcom-header from commandline and print in hex
//
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

#include "ndlcom/Types.h"
#include "ndlcom/Encoder.h"

void help(const char *name) { std::cout << "\nhere comes dragons!\n"; }

int main(int argc, char *argv[]) {

    NDLComHeader header = {0};

    // some default-values: "Gui" to "Broadcast". can be overridden via
    // commandline
    header.mSenderId = 0x01;
    header.mReceiverId = NDLCOM_ADDR_BROADCAST;

    /* option handling is based on the manpage optarg(3). */
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"senderId",   required_argument, 0, 's'},
            {"receiverId", required_argument, 0, 'r'},
            {"counter",    required_argument, 0, 'c'},
            {"help",       no_argument,       0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "s:r:c:h", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 's': {
            std::istringstream ss(optarg);
            int temp;
            ss >> temp;
            header.mSenderId = temp;
            break;
        }
        case 'r': {
            std::istringstream ss(optarg);
            int temp;
            ss >> temp;
            header.mReceiverId = temp;
            break;
        }
        case 'c': {
            std::istringstream ss(optarg);
            int temp;
            ss >> temp;
            header.mCounter = temp;
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

    char escapedBuffer[NDLCOM_MAX_ENCODED_MESSAGE_SIZE];
    size_t size =
        ndlcomEncode(escapedBuffer, sizeof(escapedBuffer), &header, NULL);

    for (size_t i = 0; i < size; ++i) {
        std::printf("0x%02x ", (int)((uint8_t *)escapedBuffer)[i]);
    }

    return 0;
}
