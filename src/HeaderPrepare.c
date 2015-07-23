/**
 * @file src/HeaderPrepare.c
 * @date 2012
 */

#include "ndlcom/HeaderPrepare.h"

/**
 * @brief The NDLCom-protocol defines the packet-counter variable.
 *
 * it has to be incremented for each packet transmitted to a received node -
 * regardless of the payload used in the packet. Therefore, a globally unique
 * (static) table is used to fill the correct packet counter. It is essentially
 * an array with one entry for each node, storing the last used packet counter.
 *
 * @see include/ndlcom/HeaderPrepare.h
 */
struct NDLComHeaderConfig ndlcomHeaderConfigDefault = {{0}, 0};
