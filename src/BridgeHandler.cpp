#include <iomanip>
// input/output
#include <iostream>
#include <limits>
#include <string>

#include "ndlcom/BridgeHandler.hpp"

using namespace ndlcom;

BridgePrintAll::BridgePrintAll(struct NDLComBridge &bridge, std::ostream &_out)
    : BridgeHandler(bridge, "BridgePrintAll", _out) {}

void BridgePrintAll::handle(const struct NDLComHeader *header,
                            const void *payload, const void *origin) {
    out << std::string("bridge saw message from ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mSenderId;
    out << std::string(" to ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mReceiverId;
    out << std::string(" with ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mDataLen;
    out << std::string(" bytes of payload\n");
}

BridgePrintMissEvents::BridgePrintMissEvents(struct NDLComBridge &bridge,
                                             std::ostream &_out)
    : BridgeHandler(bridge, "BridgePrintMissEvents", _out) {}

void BridgePrintMissEvents::handle(const struct NDLComHeader *header,
                                   const void *payload, const void *origin) {
    // broadcast receiver dont make sense... skip 'em
    if (header->mSenderId == NDLCOM_ADDR_BROADCAST) {
        return;
    }

    /* allow configuring smaller value for MAX_NUMBER_OF_DEVICES. is
     * always false by default and should be optimized by the compiler */
    if (NDLCOM_MAX_NUMBER_OF_DEVICES < std::numeric_limits<NDLComId>::max()) {
        if (header->mSenderId >= NDLCOM_MAX_NUMBER_OF_DEVICES ||
            header->mReceiverId >= NDLCOM_MAX_NUMBER_OF_DEVICES)
            return;
    }

    if (!alreadySeen.test(header->mSenderId << sizeof(NDLComId) * 8 |
                          header->mReceiverId)) {
        // this is the first time we see a packet for this combination of
        // sender and receiver. therefore it does not make sense to count an
        // eventually unexpected packet counter as miss-event. we did not know
        // what to expect in the first place.
        alreadySeen.set(header->mSenderId << sizeof(NDLComId) * 8 |
                        header->mReceiverId);
    } else {
        // if we saw this combination before we can go and test of the
        // packet-counter matches our expectation.
        if (expectedNextPacketCounter[header->mSenderId][header->mReceiverId] !=
            header->mCounter) {
            // calculate how many where lost (supposedly)
            const int diff =
                header->mCounter -
                expectedNextPacketCounter[header->mSenderId][header
                                                                 ->mReceiverId];
            // output the "event"
            out << std::string("miss event message from ");
            out << std::setfill(' ') << std::setw(3) << (int)header->mSenderId;
            out << std::string(" to ");
            out << std::setfill(' ') << std::setw(3) << (int)header->mReceiverId;
            out << std::string(" -- diff: ");
            out << std::setfill(' ') << std::setw(3) << diff << "\n";
        }
    }

    // in any case we remember the next expected packet counter of this
    // connecetion
    expectedNextPacketCounter[header->mSenderId][header->mReceiverId] =
        header->mCounter + 1;
}

BridgeHandlerStatistics::BridgeHandlerStatistics(struct NDLComBridge &_caller)
    : BridgeHandler(_caller, "BridgeHandlerStatistics"), bytesTransmitted(0),
      bytesReceived(0) {}

void BridgeHandlerStatistics::handle(const struct NDLComHeader *header,
                                     const void *payload, const void *origin) {
    if (origin) {
        bytesReceived += sizeof(struct NDLComHeader) + header->mDataLen;
    } else {
        bytesTransmitted += sizeof(struct NDLComHeader) + header->mDataLen;
    }
}

unsigned long BridgeHandlerStatistics::currentBytesTx() const {
    return bytesTransmitted;
}

unsigned long BridgeHandlerStatistics::currentBytesRx() const {
    return bytesReceived;
}

void BridgeHandlerStatistics::resetBytes() {
    bytesTransmitted = 0;
    bytesReceived = 0;
}
