/**
 * @file test/testEncoder.cpp
 * @date 2012
 */
#include "ndlcom/Protocol.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string.h>
#include <cassert>
#include <sstream>
#include <limits>
#include <stdlib.h>

/* some values */
const std::string comment("--");
const std::string sender("S");
const std::string receiver("R");
const std::string frameCounter("F");
const std::string length("L");
const std::string dataline("D");
const std::string finished("N");

/* where we'll work on */
NDLComHeader header;
unsigned char payload[255];

/**
 * reading "stimuli" commands from stdin to output encoded ndlcom-packets in decimal byte-form
 *
 *
 */

template<typename T>
bool fillContent(std::istringstream& iss, T* tgt)
{
    if (!std::numeric_limits<T>::is_integer) {
        std::cerr << "not an int\n";
    }

    /* this helps catching invalid inputs, like negative numbers of too-big ones */
    int temp;
    bool success = iss >> temp;
    if (success)
    {
        if (temp > std::numeric_limits<T>::max()) {
            std::cerr << "too big: " << temp << "\n";
            exit(EXIT_FAILURE);
        }
        else if (temp < std::numeric_limits<T>::min()) {
            std::cerr << "too small: " << temp << "\n";
            exit(EXIT_FAILURE);
        }
        else {
            *tgt = (T)temp;
        }
    }

    /* this function can indeed return false: on parsing the last of a data-line, where no number is
     * detected anymore. this condition is used further down in the loop to abort the looking for
     * further data-numbers */

    return success;
}


/* encoding it and couting as decimal-numbers */
void outputPacket(const NDLComHeader &hdr, unsigned char* data)
{
    unsigned char encoded[1024];
    size_t sz = ndlcomEncode(encoded, sizeof(encoded), &hdr, data);

    /* writing the encoded content to stdout, where the last entry has a newline instead of a space
     * appended */
    for (unsigned int i=0;i<sz-1;i++) {
        std::cout << (unsigned int)(encoded[i]) << " ";
    }
    std::cout << (unsigned int)(encoded[sz-1]) << "\n";
}

void cleanPacket() {
    memset(&header, 0, sizeof(header));
    memset(&payload, 0, sizeof(payload));
}


int main(int argc, char const *argv[])
{
    cleanPacket();

    /* maybe do argument parsing... */

    /* now reading lines of data from std-in. we expect a "stimuli.in" file in a certain format
     * - lines starting with a double-dash are considered comments
     * - lines beginning with S, R, F, D, L and N are valuable instructions
     */
    char ch[1024];
    /* reading linewise from stdin until eof */
    while (std::cin.getline(ch, sizeof(ch)))
    {
        std::string line(ch);

        /* now splitting the line at the spaces to go though the tokens one by one: */
        std::istringstream iss(line);

        /* somehow change the state of the iss... no care... */
        std::string sub;
        iss >> sub;

        if (line.find(comment) == 0) {
            /* std::cerr << "skipping because of comment at beginning of line\n"; */
            continue;
        }
        else if (line.find(comment) != std::string::npos) {
            /* std::cerr << "skipping because of comment in the middle of the line\n"; */
            continue;
        }
        else if (line.find(sender) == 0) {
            fillContent(iss, &header.mSenderId);
            /* std::cerr << "got sender " << (int)header.mSenderId << "\n"; */
            continue;
        }
        else if (line.find(receiver) == 0) {
            fillContent(iss, &header.mReceiverId);
            /* std::cerr << "got receiver " << (int)header.mReceiverId << "\n"; */
            continue;
        }
        else if (line.find(frameCounter) == 0) {
            fillContent(iss, &header.mCounter);
            /* std::cerr << "got counter " << (int)header.mCounter << "\n"; */
            continue;
        }
        else if (line.find(length) == 0) {
            /* ATTENTION* setting the length manually will be overwritten by the number of numbers
             * in a preceeding data-line */
            fillContent(iss, &header.mDataLen);
            /* std::cerr << "got length " << (int)header.mCounter << "\n"; */
            continue;
        }
        else if (line.find(dataline) == 0) {
            size_t off = 0;
            while (iss) {
                if (!fillContent(iss, payload+off))
                    continue;

                /* std::cerr << "got data " << (int)*(payload+off) << " with off " << off << "\n"; */
                off++;
            }

            header.mDataLen = off;
            continue;
        }
        else if (line.find(finished) == 0)
        {
            /* std::cerr << "finished a packet\n"; */
            outputPacket(header, payload);

            /* and reset everything to start again from scratch */
            cleanPacket();
        }
        else {
            /* std::cerr << "unkown content\n"; */
        }
    }
}
