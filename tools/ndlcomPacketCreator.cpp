/**
 * escape a ndlcom-header and optional payload from commandline and print
 * escaped packet in hex to stdout.
 *
 * chaining different options together allows crafting partly-random packets:
 *
 * a random header (-H) with one byte (-l 1) of random payload (-P):
 *      ./ndlcomEncode -H -l 1 -P
 *
 * by default (if not random) the payload is all zeros, receiver is "0xff"
 * (broadcast) and sender is "0x01".
 *
 * more to come?
 *
 * <martin.zenzes@dfki.de> 2015
 *
 */
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

#include "ndlcom/Types.h"
#include "ndlcom/Encoder.h"

void help(const char *name) { std::cout << "\nhere comes dragons!\n"; }

void fillRandomHeader(struct NDLComHeader *header) {

    header->mSenderId = (NDLComId)rand();
    header->mReceiverId = (NDLComId)rand();
    header->mCounter = (NDLComCounter)rand();
    header->mDataLen = (NDLComDataLen)rand();
}

void fillRandomPayload(struct NDLComHeader *header,
                       uint8_t payload[NDLCOM_MAX_PAYLOAD_SIZE]) {

    for (NDLComDataLen i = 0; i < header->mDataLen; i++) {
        payload[i] = (uint8_t)rand();
    }
}

int main(int argc, char *argv[]) {

    // prepare pseudo-randomness. since we may be running in a tight loop use
    // the current nanoseconds as seed.
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand(ts.tv_nsec);

    NDLComHeader header = {0};

    // some default-values: "Gui" to "Broadcast". can be overridden via
    // commandline
    header.mSenderId = 0x01;
    header.mReceiverId = NDLCOM_ADDR_BROADCAST;

    // prepare a hypothetical payload, all zeros. just in case.
    uint8_t payload[NDLCOM_MAX_PAYLOAD_SIZE] = {0};

    /* option handling is based on the manpage optarg(3). */
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"randomPacket",    no_argument,       0, 'R'},
            {"randomHeader",    no_argument,       0, 'H'},
            {"randomPayload",   no_argument,       0, 'P'},
            {"senderId",        required_argument, 0, 's'},
            {"receiverId",      required_argument, 0, 'r'},
            {"dataLen",         required_argument, 0, 'l'},
            {"counter",         required_argument, 0, 'c'},
            {"help",            no_argument,       0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "RHPs:r:l:c:h", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'R': {
            fillRandomHeader(&header);
            fillRandomPayload(&header, payload);
            break;
        }
        case 'H': {
            fillRandomHeader(&header);
            break;
        }
        case 'P': {
            fillRandomPayload(&header, payload);
            break;
        }
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
        case 'l': {
            std::istringstream ss(optarg);
            int temp;
            ss >> temp;
            header.mDataLen = temp;
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
        ndlcomEncode(escapedBuffer, sizeof(escapedBuffer), &header, payload);

    for (size_t i = 0; i < size; ++i) {
        std::printf("0x%02x ", (int)((uint8_t *)escapedBuffer)[i]);
    }

    return 0;
}
