/**
 *
 * read hex-encoded bytes-stream from stdin and print ndlcom-properties
 *
 * print ndlcom-packages in a "standardized" form to stdout. packet-misses and
 * "arbritrary" filtering would be nice
 * 
 * TODO: implement signal handling for clean exiting when issuing ctrl-c...
 *
 * <martin.zenzes@dfki.de> 2015
 *
 */
#include <getopt.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <errno.h>

#include "ndlcom/Types.h"
#include "ndlcom/Parser.h"

void help(const char *name) { std::cout << "\nhere comes dragons!\n"; }

size_t readBytesBlocking(void *buf, size_t count) {
    size_t readSoFar = 0;
    unsigned int byte = 0;
    int amount = -1;
    while (readSoFar < count) {
        // TODO: receving a signal does not cause "fscanf" to return with EINTR...?
        int scanRet = fscanf(stdin, " 0x%02x%n", &byte, &amount);

        if (scanRet == EOF) {
            std::cerr << "EOF?\n";
            // no more data...?
            break;
        } else if (scanRet == 0) {
            // could not read anything... discard some bytes. TODO: this should
            // be doable more efficiently...
            char t[2];
            // should not return EOF, "fscanf()" should have handled this
            std::cerr << "throw away?\n";
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

#include <time.h>
#include <string.h>
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

int main(int argc, char *argv[]) {

    /* option handling is based on the manpage optarg(3). */
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"noheader", no_argument, 0, 'H'},
            {"print-payload", no_argument, 0, 'P'},
            {"notimestamp", no_argument, 0, 'S'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        c = getopt_long(argc, argv, "HPSh", long_options, &option_index);
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
    do {
        bytesProcessed = 0;
        bytesRead = readBytesBlocking(buffer, sizeof(buffer));
        do {
            bytesProcessed += ndlcomParserReceive(
                &parser, buffer + bytesProcessed, bytesRead - bytesProcessed);
            if (ndlcomParserHasPacket(&parser)) {
                printPackage(ndlcomParserGetHeader(&parser),
                             ndlcomParserGetPacket(&parser));

                ndlcomParserDestroyPacket(&parser);
            }

        } while (bytesRead != bytesProcessed);

    } while (true);

    exit(EXIT_SUCCESS);
}
