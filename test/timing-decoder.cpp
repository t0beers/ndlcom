#include "NDLCom/Protocol.h"

#include <iostream>
#include <random>
#include <chrono>

int main(int argc, char const *argv[])
{
    std::random_device rd;
    std::vector<char> data;

    char buffer[1024];
    struct ProtocolParser* parser = protocolParserCreate(buffer, sizeof(buffer));

    if (argc != 2)
    {
        std::cerr << "usage: give one argument: the number of random bytes to throw at the parser\n";
        exit(EXIT_FAILURE);
    }

    unsigned int overall = atoi(argv[1]);

    /* fill up our data-buffer with single "bytes", formed from the 4-byte numbers returned by rd() */
    while (data.size() < overall)
    {
        unsigned int number = rd();
        data.push_back(number);
        data.push_back(number>>8);
        data.push_back(number>>16);
        data.push_back(number>>24);
    }
    std::cout << "will test timing by throwing " << data.size() << "bytes of completely random data at the encoder and decoder" << std::endl;

    std::chrono::high_resolution_clock clock;
    {
        std::chrono::duration<int, std::nano> duration(0);
        int calls = 0;
        std::vector<char>::const_iterator it = data.cbegin();
        while(it != data.cend())
        {
            uint8_t byte = (*it);
            it++;

            std::chrono::system_clock::time_point start = clock.now();
                protocolParserReceive(parser,&byte,sizeof(byte));
            std::chrono::system_clock::time_point end = clock.now();

            duration += end - start;
            calls++;

            if (protocolParserHasPacket(parser))
                protocolParserDestroyPacket(parser);
        }
        std::cout << "decoding took " << (double)duration.count()/calls << "nanoseconds per byte" << std::endl;
    }
    {
        std::chrono::duration<int, std::nano> duration(0);
        int calls = 0;
        int dataLen = 50;
        char output[512];
        ProtocolHeader hdr;
        hdr.mDataLen = dataLen;
        std::vector<char>::const_iterator it = data.cbegin();
        while(it != data.cend()-dataLen)
        {
            it++;

            std::chrono::system_clock::time_point start = clock.now();
                protocolEncode(output, sizeof(output), &hdr, &*it);
            std::chrono::system_clock::time_point end = clock.now();

            duration += end - start;
            calls++;
        }
        std::cout << "encoding took " << (double)duration.count()/calls << "nanoseconds per " << dataLen << "byte-chunk of payload" << std::endl;
    }

    exit(EXIT_SUCCESS);
}
