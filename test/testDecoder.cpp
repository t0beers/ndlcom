/**
 * @file test/testDecoder.cpp
 * @date 2012
 */
#include "ndlcom_core/Protocol.h"

#include <iostream>
#include <cassert>
#include <sstream>

/* global stuff */
char buffer[1024];
struct NDLComParser* parser = ndlcomParserCreate(buffer, sizeof(buffer));

/**
 * a comment describing functionality of this program. for the actual data-format of the output: see
 * comments in the decodedPacket.dat file.
 */

/* decoding the byte stream and printing decoded packets in the "vhdl" convention */
void addByte(const unsigned char byte)
{
    unsigned int rcvd = ndlcomParserReceive(parser,&byte, 1);
    assert(rcvd == 1);

    if (ndlcomParserHasPacket(parser)) {
        const NDLComHeader *hdr = ndlcomParserGetHeader(parser);
        const char* data = (const char*)ndlcomParserGetPacket(parser);
        std::cout << "S " << (int)hdr->mSenderId << "\n";
        std::cout << "R " << (int)hdr->mReceiverId << "\n";
        std::cout << "F " << (int)hdr->mCounter << "\n";
        std::cout << "L " << (int)hdr->mDataLen << "\n";
        /* printing content of data-array as well, where only the last entry is followed by a
         * newline */
        std::cout << "D ";
        for (int i=0;i<hdr->mDataLen-1;i++) {
            std::cout << (int)(data[i]) << " ";
        }
        std::cout << (int)(data[hdr->mDataLen-1]) << "\n";

        /* by convention a packet is deemed finished by a "N" as a line */
        std::cout << "N\n";

        ndlcomParserDestroyPacket(parser);
    }
}

int main(int argc, char const *argv[])
{
    parser = ndlcomParserCreate(buffer, sizeof(buffer));

    /* maybe do argument parsing... */

    char ch[1024];
    /* reading linewise from stdin until eof. each line is hopefully one packet */
    while (std::cin.getline(ch, sizeof(ch)))
    {
        std::string line(ch);
        std::istringstream iss(line);

        while (iss) {
            unsigned int temp;
            bool success = iss >> temp;
            /* there is no success on the last call in the line (newline?) and on non-digits */
            if (success)
                addByte(temp);
        }
    }
}
