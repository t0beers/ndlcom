/**
 *
 * read hex-encoded bytes-stream from stdin and print ndlcom-properties
 *
 * print ndlcom-packages in a "standardized" form to stdout. packet-misses and
 * "arbritrary" filtering would be nice
 *
 * one possibility is to pipe the output of the "Producer" into the "Consumer",
 * to get a nice formatting:
 *      $ ndlcomPacketProducer | ndlcomPacketConsumer
 *      [2015-08-26 17:41:39.180734630] [sender: 0x01 receiver: 0xff counter:   0 length:   0]
 *
 * TODO: implement signal handling for clean exiting when issuing ctrl-c...
 *
 * <martin.zenzes@dfki.de> 2015
 *
 */
#include <time.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>

#include "ndlcom/Types.h"
#include "ndlcom/Parser.h"

size_t readBytesBlocking(void *buf, size_t count) {
    size_t readSoFar = 0;
    unsigned int byte = 0;
    int amount = -1;
    while (readSoFar < count) {
        // TODO: receving a signal does not cause "fscanf" to return with
        // EINTR...?
        int scanRet = fscanf(stdin, " 0x%02x%n", &byte, &amount);

        if (scanRet == EOF) {
            if (ferror(stdin)) {
                // this did not happened (yet), but the man page says in could.
                // handle it, just to be sure
                throw std::runtime_error(strerror(errno));
            }
            if (feof(stdin)) {
                // this should never happen! we call "select()" at an earlier
                // stage and so there should always be bytes waiting for us?
                // this happens for example when we get SIGINT, so this is a
                // mild error.  don't throw.
                exit(EXIT_FAILURE);
            }
            // no more data...?
            break;
        } else if (scanRet == 0) {
            // could not read anything... discard some bytes. TODO: this should
            // be doable more efficiently...
            char t[2];
            // should not return EOF, "fscanf()" should have handled this
            /* std::cerr << "throw away?\n"; */
            fgets(t, 2, stdin);
            /* std::cout << "found nothing, destroyed: " << t << "\n"; */
        } else if (scanRet == 1) {
            /* std::cout << "did read: 0x" << std::hex << byte << std::dec */
            /*           << " from " << amount << "bytes\n"; */
            ((uint8_t *)buf)[readSoFar++] = (uint8_t)byte;
        } else {
            throw std::runtime_error("huh? I'm lost");
        }
    }

    return readSoFar;
}

bool print_header = true;
bool print_payload = false;
bool print_timestamp = true;

void printPackage(const struct NDLComHeader *header, const void *payload) {

    if (print_timestamp) {
        char buffer[128];
        struct timespec mTimespec;
        int r = clock_gettime(CLOCK_REALTIME, &mTimespec);
        if (r != 0) {
            throw std::runtime_error(strerror(errno));
        }
        struct tm ts;
        // Format time, "yyyy-mm-dd hh:mm:ss"
        time_t time = mTimespec.tv_sec;
        ts = *localtime(&time);
        size_t pos = strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &ts);
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, ".%09lu",
                        mTimespec.tv_nsec);
        // and finally print it
        printf("[%s] ", buffer);
    }
    if (print_header) {
        printf("[sender: 0x%02x receiver: 0x%02x counter: %3i length: %3i] ",
               header->mSenderId, header->mReceiverId, header->mCounter,
               header->mDataLen);
    }
    if (print_payload) {
        printf("[");
        for (int i = 0; i < header->mDataLen; i++) {
            printf("0x%02x", ((uint8_t *)payload)[i]);
            if (i != header->mDataLen - 1) {
                printf(" ");
            }
        }
        printf("]");
    }

    printf("\n");
}

void help(const char *name) {
    /* clang-format off */
    fprintf(stderr,
"\n%s\n\n"
"print ndlcom-packages in a 'standardized' form to stdout\n"
"\n"
"options:\n"
"--noheader\t\t-H\tdon't print the header\n"
"--payload\t\t-P\tprint the payload\n"
"--notimestamp\t\t-S\tdon't print the timestamp\n"
"--filter-receiverId\t-r\tadd given Id to the filter list\n"
"--filter-senderId\t-r\tadd given Id to the filter list\n",
name);
    /* clang-format on */
}

int main(int argc, char *argv[]) {

    // these allow filtering only for wanted ids
    std::vector<NDLComId> filterSenderId;
    std::vector<NDLComId> filterReceiverId;

    /* option handling is based on the manpage optarg(3). */
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"noheader", no_argument, 0, 'H'},
            {"payload", no_argument, 0, 'P'},
            {"notimestamp", no_argument, 0, 'S'},
            {"filter-receiverId", required_argument, 0, 'r'},
            {"filter-senderId", required_argument, 0, 's'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "HPSr:s:h", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'H': {
            print_header = false;
            break;
        }
        case 'P': {
            print_payload = true;
            break;
        }
        case 'S': {
            print_timestamp = false;
            break;
        }
        case 'r': {
            std::istringstream ss(optarg);
            int temp;
            ss >> temp;
            filterReceiverId.push_back((NDLComId)temp);
            break;
        }
        case 's': {
            std::istringstream ss(optarg);
            int temp;
            ss >> temp;
            filterSenderId.push_back((NDLComId)temp);
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

    struct NDLComParser parser;
    ndlcomParserCreate(&parser, sizeof(struct NDLComParser));

    char buffer[1];
    size_t bytesProcessed = 0;
    size_t bytesRead = 0;
    fd_set fds;
    FD_ZERO(&fds);
    do {
        bytesProcessed = 0;
        bytesRead = readBytesBlocking(buffer, sizeof(buffer));
        do {
            bytesProcessed += ndlcomParserReceive(
                &parser, buffer + bytesProcessed, bytesRead - bytesProcessed);
            if (ndlcomParserHasPacket(&parser)) {
                const struct NDLComHeader *header =
                    ndlcomParserGetHeader(&parser);
                const void *payload = ndlcomParserGetPacket(&parser);

                if (!filterSenderId.empty() &&
                    std::find(filterSenderId.begin(), filterSenderId.end(),
                              (int)header->mSenderId) == filterSenderId.end())
                    goto skipPrint;
                if (!filterReceiverId.empty() &&
                    std::find(filterReceiverId.begin(), filterReceiverId.end(),
                              (int)header->mReceiverId) !=
                        filterReceiverId.end())
                    goto skipPrint;

                printPackage(header, payload);
            skipPrint:
                ndlcomParserDestroyPacket(&parser);
            }

        } while (bytesRead != bytesProcessed);

        // use select to sleep until new data is available on stdin
        FD_SET(0, &fds);
        int ret = select(1, &fds, NULL, NULL, NULL);
        if (ret == -1) {
            throw std::runtime_error(strerror(errno));
        }

    } while (true);

    exit(EXIT_SUCCESS);
}
