/**
 * @file test/encoderDecoder_pingPong.cpp
 * @date 2012
 */
#include "ndlcom_core/Protocol.h"

#include <iostream>
#include <random>
#include <chrono>

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 * @addtogroup Communication_NDLCom_Test
 * @{
 */

/**
 * small test-program to check if the current encoder is correctly working with the current decoder.
 * is also used to measure the actual timing of the two -- how long it takes to encode/decode random
 * packets on the current architecture.
 *
 * just for reference:
 * crc xor8 (SVN r7289):
 *  x86_64, 2.8GHz i7: all 100000 trials worked! encoding took 0.90748us decoding took 5.26462us
 * crc fcs16:
 *  x86_64, 2.8GHz i7: all 100000 trials worked! encoding took 1.04513us decoding took 6.0782us
 *
 */
int main(int argc, char const *argv[])
{

    char buffer[1024];
    struct ProtocolParser* parser = protocolParserCreate(buffer, sizeof(buffer));

    if (argc != 2)
    {
        std::cerr << "usage: give one argument: the number of pingPong games to play\n";
        exit(EXIT_FAILURE);
    }

    unsigned int trialsOverall = atoi(argv[1]);

    std::cerr << "creating " << trialsOverall << " completely random packets. encode "
              << "and decode them again. while doing this the timing is measured for benchmarking\n";

    /* these are used to build a "perfect" random packet */
    std::string str("some random seed");
    std::seed_seq seed1 (str.begin(),str.end());

    std::default_random_engine generator(seed1);
    std::uniform_int_distribution<ndlcomDataLen> datalenDistribution(std::numeric_limits<ndlcomDataLen>::min(),
                                                                     std::numeric_limits<ndlcomDataLen>::max());
    std::uniform_int_distribution<ndlcomId> receiverDistribution(std::numeric_limits<ndlcomId>::min(),
                                                                 std::numeric_limits<ndlcomId>::max());
    std::uniform_int_distribution<ndlcomId> senderDistribution(std::numeric_limits<ndlcomId>::min(),
                                                               std::numeric_limits<ndlcomId>::max());
    std::uniform_int_distribution<ndlcomCounter> counterDistribution(std::numeric_limits<ndlcomCounter>::min(),
                                                                     std::numeric_limits<ndlcomCounter>::max());
    std::uniform_int_distribution<uint8_t> payloadDistribution(std::numeric_limits<uint8_t>::min(),
                                                               std::numeric_limits<uint8_t>::max());

    /* used for timing measurements */
    std::chrono::high_resolution_clock clock;
    std::chrono::duration<int, std::micro> durationEncode(0);
    std::chrono::duration<int, std::micro> durationDecode(0);

    for (unsigned int trial=0;trial<trialsOverall;trial++)
    {
        ProtocolHeader hdr;
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
        char* data = new char[hdr.mDataLen];

        /* fill up our data-buffer with single "bytes" */
        for (int i=0;i<hdr.mDataLen;i++)
        {
            data[i] = payloadDistribution(generator);
        }

        char encoded[1024];

        {
            std::chrono::system_clock::time_point start = clock.now();
                protocolEncode(encoded, sizeof(encoded), &hdr, data);
            std::chrono::system_clock::time_point end = clock.now();

            durationEncode += end - start;
        }

        size_t i = 0;
        while (!protocolParserHasPacket(parser))
        {
            uint8_t byte = encoded[i++];

            {
                std::chrono::system_clock::time_point start = clock.now();
                    /* parsing deliberatively only one byte! we are measuring timing here, remember? */
                    protocolParserReceive(parser,&byte,sizeof(byte));
                std::chrono::system_clock::time_point end = clock.now();

                durationDecode += end - start;
            }

            if (i>=sizeof(encoded))
            {
                struct ProtocolParserState state;
                protocolParserGetState(parser, &state);

                std::cout << "trial " << trial << " did not work."
                          << " number of crc-errors: " << (int)state.mNumberOfCRCFails
                          << " current parser state: " << protocolParserStateName[state.mState] << "...\n";
                exit(EXIT_FAILURE);
            }
        }

        protocolParserDestroyPacket(parser);

        delete[] data;
    }

    std::cout << "all " << trialsOverall << " trials worked!"
              << " encoding took " << (double)durationEncode.count()/trialsOverall << "us"
              << " decoding took " << (double)durationDecode.count()/trialsOverall << "us"
              << "\n";

    exit(EXIT_SUCCESS);
}

/**
 * @}
 * @}
 * @}
 */
