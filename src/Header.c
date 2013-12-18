/**
 * @file src/Header.c
 * @date 2012
 */

#include "ndlcom_core/Header.h"

/**
 * @brief The ndlcom-protocol defines the packet-coutner variable.
 *
 * it has to be increased for each packet transmissed to a receivind node -- regardless of
 * the payload used in the packet. therefore, a globally unique (static) table
 * is used to fill the correct packet counter. it is essentially an array
 * with one entry for each node, storing the last used packet counter.
 *
 * @see include/ndlcom_core/Header.h
 */
NDLComHeaderConfig ndlcomHeaderConfigDefault = { 0, {0}} ;
