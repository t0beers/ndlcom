/**
 * @file testCrc.c
 * @brief simple c-program to just test the crc-algorithm
 *
 * can be extended to add more tests, in the moment only three are used:
 * - the "standard" string 12...89
 * - one real "zero"
 * - more than one real "zero"
 *
 * @author Martin Zenzes
 * @date 2013-12-18
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "ndlcom/Types.h"
#include "ndlcom/Crc.h"

struct TestCase {
    const char* name;
    const size_t len;
    const char* string;
};

static const struct TestCase myCases[] = {
    { "ascii-ladder", 9, "123456789" },
    { "four zeros", 4, "\0\0\0\0" },
    { "one zero", 1, "\0" },
    { "", 0, 0 }
};

void printCheck(const struct TestCase* test, int xor)
{
    int i;
    NDLComCrc crc = NDLCOM_CRC_INITIAL_VALUE;
    for (i=0;i<test->len;i++)
        crc = ndlcomDoCrc(crc, (const unsigned char*)&test->string[i]);

    /* when a "0" is passed into the xor, it is basically skipped */
    crc ^= xor;

    printf("testcase \"%s\": \"%s\" with len %lu and final xor 0x%04x is: 0x%04x\n",
            test->name,
            test->string,
            test->len,
            xor,
            crc);
}

int main(int argc, char *argv[])
{
    const struct TestCase* test = myCases;
    while (*test->name)
    {
        /* xoring with 0 is a nop */
        printCheck(test, 0x0000);
        /* just to double-check this, try again with 0xffff */
        printCheck(test, 0xffff);
        test++;
    }

    return 0;
}

