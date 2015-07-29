/**
 * @file test/encoderDecoder_pingPong.cpp
 * @date 2012
 */
#include "ndlcom/Parser.h"
#include "ndlcom/Encoder.h"

#include <iostream>
#include <sstream>
#include <random>
#include <chrono>

/**
 * small test-program to check if the current encoder is correctly working with
 * the current decoder.  is also used to measure the actual timing of the two,
 * so how long it takes to encode/decode random packets on the current
 * architecture.
 *
 * call using:
 *

   ./build/x86_64-linux-gnu/test/encoderDecoder_pingPong 100000

 *
 * just for reference:
 *
 * crc xor8 (SVN r7289):
 *  x86_64, 2.8GHz i7:
 *  all 100000 trials worked! encoding took 0.90748us decoding took 5.26462us
 * crc xor8 (spacegit eff0c8):
 *  x86_64, 2.8GHz i7:
 *  all 100000 trials worked! encoding took 1.09439us decoding took 5.59621us
 * crc fcs16:
 *  x86_64, 2.8GHz i7:
 *  all 100000 trials worked! encoding took 1.04513us decoding took 6.0782us
 *
 * a newer revision with crc16, gitlab 7a01f70:
 *  x86_64, 2.8GHz i7, gcc-4.6:
 *  all 100000 trials worked! encoding took 1.2729us decoding took 5.73017us
 *  x86_64, 2.8GHz i7, gcc-4.8:
 *  all 100000 trials worked! encoding took 0.7258us decoding took 0.06258us
 *  x86_64, 2.8GHz i7, gcc-4.9:
 *  all 100000 trials worked! encoding took 0.69116us decoding took 0.06745us
 *  x86_64, 2.8GHz i7, clang-3.7:
 *  all 100000 trials worked! encoding took 0.74204us decoding took 0.06868us
 *
 * TODO:
 * - implement a multithreaded version! worker-threads anyone?
 * - add optarg
 *   - commandline argument to set the "seed"
 *   - commandline argument to switch processing between byte-wise, random-chunk and full-chunk
 */
int main(int argc, char const *argv[]) {

    char buffer[sizeof(struct NDLComParser)];
    struct NDLComParser *parser = ndlcomParserCreate(buffer, sizeof(buffer));

    unsigned long long int trialsOverall = 100000;
    if (argc == 1) {
        std::cerr << "no argument given, assuming default value of "
                  << trialsOverall << " pingPong games to play\n";
    } else if (argc == 2) {
        std::istringstream ss(argv[1]);
        ss >> trialsOverall;
    } else {
        std::cerr << "usage: give one optional argument: the number of "
                     "pingPong games to play\n";
        exit(EXIT_FAILURE);
    }

    std::cerr << "creating " << trialsOverall
              << " completely random packets. encode and decode them again. "
                 "while doing this the timing is measured for benchmarking\n";

    /* these are used to build a "perfect" random packet */
    std::string str("some random seed");
    std::seed_seq seed1(str.begin(), str.end());

    std::default_random_engine generator(seed1);
    std::uniform_int_distribution<NDLComDataLen> datalenDistribution(
        std::numeric_limits<NDLComDataLen>::min(),
        std::numeric_limits<NDLComDataLen>::max());
    std::uniform_int_distribution<NDLComId> receiverDistribution(
        std::numeric_limits<NDLComId>::min(),
        std::numeric_limits<NDLComId>::max());
    std::uniform_int_distribution<NDLComId> senderDistribution(
        std::numeric_limits<NDLComId>::min(),
        std::numeric_limits<NDLComId>::max());
    std::uniform_int_distribution<NDLComCounter> counterDistribution(
        std::numeric_limits<NDLComCounter>::min(),
        std::numeric_limits<NDLComCounter>::max());
    std::uniform_int_distribution<uint8_t> payloadDistribution(
        std::numeric_limits<uint8_t>::min(),
        std::numeric_limits<uint8_t>::max());

    /* used for timing measurements. using 64bit to store micro-seconds gives
     * us a long time. several million years? */
    std::chrono::duration<uint64_t, std::micro> durationEncode(0);
    std::chrono::duration<uint64_t, std::micro> durationDecode(0);

    for (unsigned long long int trial = 0; trial < trialsOverall; trial++) {
        NDLComHeader hdr;
        /* generates numbers in the possible ranges */
        hdr.mReceiverId = receiverDistribution(generator);
        hdr.mSenderId = senderDistribution(generator);
        hdr.mDataLen = datalenDistribution(generator);
        hdr.mCounter = counterDistribution(generator);

        /* std::cout << "created random packet with:" */
        /*           << " receiver: " << (int)hdr.mReceiverId */
        /*           << " sender: " << (int)hdr.mSenderId */
        /*           << " dataLen: " << (int)hdr.mDataLen */
        /*           << " counter: " << (int)hdr.mCounter */
        /*           << "\n"; */

        std::random_device rd;
        char *data = new char[hdr.mDataLen];

        /* fill up our data-buffer with single "bytes" */
        for (int i = 0; i < hdr.mDataLen; i++) {
            data[i] = payloadDistribution(generator);
        }

        char encoded[NDLCOM_MAX_ENCODED_MESSAGE_SIZE];

        {
            auto start = std::chrono::high_resolution_clock::now();
            ndlcomEncode(encoded, sizeof(encoded), &hdr, data);
            auto end = std::chrono::high_resolution_clock::now();

            durationEncode +=
                std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                      start);
        }

        size_t i = 0;
        while (!ndlcomParserHasPacket(parser)) {
            uint8_t byte = encoded[i++];

            {
                auto start = std::chrono::high_resolution_clock::now();
                /* parsing deliberatively only one byte! we are measuring timing
                 * here, remember? */
                ndlcomParserReceive(parser, &byte, sizeof(byte));
                auto end = std::chrono::high_resolution_clock::now();

                durationDecode +=
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        end - start);
            }

            /* if we parsed all available bytes, but still do not found a
             * packet something is odd... */
            if (i >= sizeof(encoded)) {

                std::cout << "trial " << trial << " did not work."
                          << " number of crc-errors: "
                          << ndlcomParserGetNumberOfCRCFails(parser)
                          << " current parser state: "
                          << ndlcomParserGetState(parser) << "...\n";
                exit(EXIT_FAILURE);
            }
        }

        ndlcomParserDestroyPacket(parser);

        delete[] data;
    }

    std::cout << "all " << trialsOverall << " trials worked! encoding took "
              << (double)durationEncode.count() / trialsOverall << "us"
              << " decoding took "
              << (double)durationDecode.count() / trialsOverall << "us"
              << "\n";

    exit(EXIT_SUCCESS);
}
