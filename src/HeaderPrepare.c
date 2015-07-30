/**
 * @file src/HeaderPrepare.c
 * @date 2012
 */

#include "ndlcom/HeaderPrepare.h"

#include <string.h>

void ndlcomHeaderPrepareInit(struct NDLComHeaderConfig *config,
                             const NDLComId senderId) {
    config->mOwnSenderId = senderId;
    /* we became a new personality, so reset the packet counters to use for new
     * packages */
    memset(config->mCounterForReceiver, 0, sizeof(config->mCounterForReceiver));
}
