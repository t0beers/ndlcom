/**
 * @file test/test-decoder.c
 * @date 2011
 */
#include "ndlcom_core/Protocol.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 */

/**
 * @defgroup Communication_NDLCom_Test Testing NDLCom
 * @brief Testing some difficult parsing sequences
 *
 * @{
 */

/**
 * ESC sequence as a const char* for easier test case creation.
 */
#define STR_ESC  "\x7d"
/**
 * FLAG sequence as a const char* for easier test case creation.
 */
#define STR_FLAG "\x7e"

/**
 * @brief Generic struct to contain a payload
 */
struct Data {
    int len;/**< the length of the datafiel to follow */
    const char* data;/**< data to be fed into the parser, or compared against other data */
};

/**
 * @brief Contains encoded and unencoded data which may be compared
 */
struct TestCase {
    const char* descr; /**< short test description. */
    const struct Data content; /**< content of packet (without header, without esc) */
    const struct Data encoded; /**< encoded packet (with header, with escapes, with crc) */
};

/**
 * @brief All testcases
 */
static const struct TestCase testDecoderData[] = {
    {
        "Inter-Frame-Gaps",
        {-1, ""},
        {4, STR_FLAG STR_FLAG STR_FLAG STR_FLAG}
    },
    {
        "short valid frame",
        {1, "\x01"},
        {7, STR_FLAG "\x01\x02\x01\x01" "\x01" "\x02"}
    },
    {
        "empty frame",
        {0, ""},
        {6, STR_FLAG "\x01\x02\x01\x00" "\x02"}
    },
    {
        "valid frame inside garbage",
        {1, "\x01"},
        {40, "lahsdihseriuhweruihwer" STR_FLAG " asdlkja" STR_FLAG "\x01\x02\x01\x01" "\x01" "\x02" STR_FLAG}
    },
    //end-of-list marker
    {0, {0, ""}, {0, ""}}
};


/**
 * @brief Test all cases with one parser without resets.
 * @param test Pointer to the testcases to be tested
 * @return Number of error found. So this is "false" if no error found...
 */
int testDecoder(const struct TestCase* test)
{
    int foundErrors = 0;
    struct NDLComParser* p;
    char buffer[1024];
    p = ndlcomParserCreate(buffer, sizeof(buffer));

    while (test->descr)
    {
        int parsed = 0;
        while(parsed < test->encoded.len)
        {
            int r = ndlcomParserReceive(p,
                                          test->encoded.data + parsed,
                                          test->encoded.len - parsed);
            if (r == -1)
                break;
            if (ndlcomParserHasPacket(p))
                break;
            parsed += r;
        }
        if (test->content.len != -1)
        {
            if (!ndlcomParserHasPacket(p))
            {
                struct NDLComParserState state;
                ndlcomParserGetState(p, &state);
                printf("Did not detect a valid packet:\n  %s\n  CrcFails: %i  State: %s\n",
                       test->descr,
                       state.mNumberOfCRCFails,
                       ndlcomParserStateName[state.mState]
                       );
                ++foundErrors;
            }
            else
            {
                const unsigned char* data = ndlcomParserGetPacket(p);
                const NDLComHeader* hdr = ndlcomParserGetHeader(p);
                //check length
                if (hdr->mDataLen != test->content.len)
                {
                    printf("Wrong content length:\n  %s\n",
                           test->descr);
                    ++foundErrors;
                }
                else
                {
                    //check data if length is correct
                    if (memcmp(data, test->content.data, test->content.len) != 0)
                    {
                        printf("Wrong data:\n  %s\n",
                               test->descr);
                        ++foundErrors;
                    }
                }
                ndlcomParserDestroyPacket(p);
            }
        }
        else
        {
            //an invalid packet was received
            if (ndlcomParserHasPacket(p))
            {
                printf("Failed to recognize invalid frame:\n  %s\n",
                       test->descr);
                ++foundErrors;
            }
        }

        ++test;
    };

    return foundErrors;
}

/**
 * @brief Encode data structure.
 */
int testEncoder(void)
{
    const struct {
        uint8_t id;
        int16_t rotX, rotY, rotZ;
    } __attribute((__packed__)) jointAngle = {18, 0, 126, 0};
    const NDLComHeader hdr =
    {
        1, /* mReceiverId */
        2, /* mSenderId */
        185, /* mCounter */
        sizeof(jointAngle) /* mDataLength */
    };
    const uint8_t expected[] =
        STR_FLAG
        "\x01\x02\xB9\x07"
        "\x12\x00\x00\x7D\x5E\x00\x00\x00\xD1"
        STR_FLAG;

    uint8_t buf[1024];
    int outlen = ndlcomEncode(buf, sizeof(buf),
                                &hdr, &jointAngle);

    /* compare output with predefined data */
    if (outlen != sizeof(expected) - 1)
    {
        fprintf(stderr, "\ntestEncoder(): Enexpected length\n");
        return 1;
    }
    int i;
    for (i = 0; i < outlen; ++i)
    {
        if (buf[i] != expected[i])
        {
            fprintf(stderr,
                    "\ntestEncoder(): output differs at pos %d (expected"
                    "0x%02X, encoder output was 0x%02X)\n",
                    i, expected[i], buf[i]);
            return 1;
        }
    }

    return 0;
}


/**
 * @brief Mainfunction... plain and simple
 * @param argc you guess it
 * @param argv what should i say
 * @return true if no errors found
 */
int main(int argc, char** argv)
{
    if (testEncoder())
    {
        return 1; /* error */
    }

    if (testDecoder(testDecoderData))
    {
        return 1; /* error */
    }

    return 0; /* success */
}

/**
 * @}
 */

/**
 * @}
 * @}
 */
