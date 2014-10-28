/**
 * @file src/Crc.c
 * @date 2013
 */

#include "ndlcom/Crc.h"

inline NDLComCrc ndlcomDoCrc(const NDLComCrc currentCrc, const unsigned char *c)
{
    return currentCrc ^ *c;
}
