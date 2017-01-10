/**
 * @file src/HeaderPrepare.c
 */
#include "ndlcom/HeaderPrepare.h"

#include <string.h>

void ndlcomHeaderPrepareInit(struct NDLComHeaderConfig *config,
                             const NDLComId ownSenderId) {
    config->mOwnSenderId = ownSenderId;
    /* we became a new personality, so reset the packet counters to use for new
     * packages. Someone has to reinitialize the routing table as well. */
    memset(config->mCounterForReceiver, 0,
           sizeof(config->mCounterForReceiver) /
               sizeof(config->mCounterForReceiver[0]));
}

void ndlcomHeaderPrepare(struct NDLComHeaderConfig *config,
                         struct NDLComHeader *header, const NDLComId receiverId,
                         const NDLComDataLen dataLength) {
    header->mReceiverId = receiverId;
    header->mSenderId = config->mOwnSenderId;
    /* a overflowing addition for the actual counter: */
    header->mCounter = config->mCounterForReceiver[receiverId]++;
    header->mDataLen = dataLength;
}
