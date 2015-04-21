/**
 * @file test/timing-decoder.cpp
 * @date 2012
 */
#include "ndlcom/Parser.h"
#include "ndlcom/Encoder.h"

#include <iostream>
#include <random>
#include <chrono>
#include <cassert>

/**
 * throwing random bytes at the parser/encoder and measuring the time it takes.
 * only intended to be run in a full-system-environment, not in microcontroller
 *
 * as a by-product a nice "fuzzy-tester" for both modules, altough they only
 * test the underlying state-machine.
 *
 * uses quite modern compiler features (c++11 yeah!), is also a small
 * (off-topic) test on what to expect in this region. so this might not compile
 * everywhere.
 */
int main(int argc, char const *argv[]) {
    int retval;
    std::random_device rd;
    std::vector<uint8_t> data;
    int numberOfCorrectPackages = 0;

    uint8_t ndlcomBuffer[1024];
    struct NDLComParser *parser =
        ndlcomParserCreate(ndlcomBuffer, sizeof(ndlcomBuffer));

    if (argc != 2) {
        std::cerr << "usage: give one argument: the number of random bytes to "
                     "throw at the parser\n";
        exit(EXIT_FAILURE);
    }

    long long int numberOfBytesToProcess = atoll(argv[1]);

    /* fill up our data-buffer with single "bytes", formed from the 4-byte
     * numbers returned by rd() */
    while ((long long int)data.size() < numberOfBytesToProcess) {
        unsigned int number = rd();
        data.push_back(number);
        data.push_back(number >> 8);
        data.push_back(number >> 16);
        data.push_back(number >> 24);
    }
    std::cout << "will test timing by throwing " << data.size()
              << "bytes of completely random data at the encoder and decoder\n";

    std::chrono::high_resolution_clock clock;
    {
        std::chrono::duration<int, std::nano> duration(0);
        long long int numberOfCalls = 0;
        std::vector<uint8_t>::const_iterator it = data.cbegin();
        while (it != data.cend()) {
            uint8_t byte = (*it);
            it++;

            std::chrono::system_clock::time_point start = clock.now();
            retval = ndlcomParserReceive(parser, &byte, sizeof(byte));
            std::chrono::system_clock::time_point end = clock.now();

            // "receive" parses byte-wise
            assert(retval == sizeof(byte));

            duration += end - start;
            numberOfCalls++;

            if (ndlcomParserHasPacket(parser)) {
                numberOfCorrectPackages++;
                ndlcomParserDestroyPacket(parser);
            }
        }
        std::cout << "decoding took "
                  << (double)duration.count() / numberOfCalls
                  << "nanoseconds per byte, detected "
                  << numberOfCorrectPackages << " correct packages\n";
    }
    {
        std::chrono::duration<int, std::nano> duration(0);
        long long int numberOfCalls = 0;
        uint8_t dataLen = 50;
        uint8_t output[1024];
        NDLComHeader hdr;
        hdr.mDataLen = dataLen;
        std::vector<uint8_t>::const_iterator it = data.cbegin();
        while (it != data.cend() - dataLen) {
            it++;

            std::chrono::system_clock::time_point start = clock.now();
            retval = ndlcomEncode(output, sizeof(output), &hdr, &*it);
            std::chrono::system_clock::time_point end = clock.now();

            // "encode" returns the encoded number of bytes. it is greater than
            // the packet-length
            assert(retval >= dataLen);

            duration += end - start;
            numberOfCalls++;
        }
        std::cout << "encoding took "
                  << (double)duration.count() / numberOfCalls
                  << "nanoseconds per " << (int)dataLen
                  << "byte-chunk of payload\n";
    }

    exit(EXIT_SUCCESS);
}
