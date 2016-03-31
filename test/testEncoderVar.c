#include "ndlcom/Encoder.h"
#include <stdio.h>

/*
 * one time test program. to be removed.
 */
int main(int argc, char *argv[]) {
    struct NDLComHeader header;
    uint8_t data1[4] = {1, 2, 3, 4};
    uint8_t data2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t data3[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    uint8_t output[NDLCOM_MAX_ENCODED_MESSAGE_SIZE];
    int i;
    size_t sz;

    header.mDataLen = sizeof(data1) + sizeof(data2) + sizeof(data3);

    sz = ndlcomEncodeVar(output, sizeof(output), &header, 3, data1,
                         sizeof(data1), data2, sizeof(data2), data3,
                         sizeof(data3));

    for (i = 0; i < sz; i++) {
        printf("0x%02x ", output[i]);
    }
    printf("\n");

    return 0;
}
