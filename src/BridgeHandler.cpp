#include <iomanip>
// input/output
#include <iostream>
#include <limits>
#include <string>
#include <string.h> // memset

#include "ndlcom/BridgeHandler.hpp"

using namespace ndlcom;

BridgePrintAll::BridgePrintAll(struct NDLComBridge &bridge, std::ostream &_out)
    : BridgeHandler(bridge, "BridgePrintAll", _out) {}

void BridgePrintAll::handle(const struct NDLComHeader *header,
                            const void *payload,
                            const struct NDLComExternalInterface *origin) {
    out << std::string("bridge saw message from ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mSenderId;
    out << std::string(" to ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mReceiverId;
    out << std::string(" with ");
    out << std::setfill(' ') << std::setw(3) << (int)header->mDataLen;
    out << std::string(" bytes of payload\n");
}

BridgeMissEvents::BridgeMissEvents(struct NDLComBridge &bridge,
                                   std::ostream &_out)
    : BridgeHandler(bridge, "BridgeMissEvents", _out),
      numberOfPacketMissEvents{{0}}, expectedNextPacketCounter{{0}} {}

void BridgeMissEvents::handle(
    const struct NDLComHeader *header, const void *payload,
    const struct NDLComExternalInterface *origin) {
    // this does all the heavy lifting. nothing more to do!
    isMiss(header);
}

void BridgeMissEvents::printStatus(const std::string prefix) const {
    HandlerCommon::printStatus(prefix);
    // print "miss statistics"...
    for (int fromId = 0; fromId < NDLCOM_MAX_NUMBER_OF_DEVICES; fromId++) {
        for (int toId = 0; toId < NDLCOM_MAX_NUMBER_OF_DEVICES; toId++) {
            if (numberOfPacketMissEvents[fromId][toId]) {
                out << prefix << "   from " << fromId << " to " << toId << ":  "
                    << numberOfPacketMissEvents[fromId][toId]
                    << " miss events\n";
            }
        }
    }
}

void BridgeMissEvents::resetMissEvents() {
    memset(numberOfPacketMissEvents, 0, sizeof numberOfPacketMissEvents);
}

int BridgeMissEvents::isMiss(const struct NDLComHeader *header) {
    int retval = 0;
    if (header->mSenderId == NDLCOM_ADDR_BROADCAST) {
        return retval;
    }

    /* allow configuring smaller value for MAX_NUMBER_OF_DEVICES. is
     * always false by default and should be optimized by the compiler */
    if (NDLCOM_MAX_NUMBER_OF_DEVICES < std::numeric_limits<NDLComId>::max()) {
        if (header->mSenderId >= NDLCOM_MAX_NUMBER_OF_DEVICES ||
            header->mReceiverId >= NDLCOM_MAX_NUMBER_OF_DEVICES)
            return retval;
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
            retval = header->mCounter -
                     expectedNextPacketCounter[header->mSenderId]
                                              [header->mReceiverId];
            // and count!
            numberOfPacketMissEvents[header->mSenderId][header->mReceiverId]++;
        }
    }
    // in any case we remember the next expected packet counter of this
    // connecetion. for the wraparound: the counter will overflow by itself...
    // is this undefined behaviour?
    expectedNextPacketCounter[header->mSenderId][header->mReceiverId] =
        header->mCounter + 1;
    //and done
    return retval;
}

/**
 * rather simple.
 *
 * problem: the "label" of parent class is not overriden
 */
BridgePrintMissEvents::BridgePrintMissEvents(struct NDLComBridge &bridge,
                                             std::ostream &_out)
    : BridgeMissEvents(bridge, _out) {}

void BridgePrintMissEvents::handle(
    const struct NDLComHeader *header, const void *payload,
    const struct NDLComExternalInterface *origin) {
    // this does all the heavy lifting. nothing more to do!
    if (int diff = isMiss(header)) {
        // output the "event"
        out << std::string("miss event message from ");
        out << std::setfill(' ') << std::setw(3) << (int)header->mSenderId;
        out << std::string(" to ");
        out << std::setfill(' ') << std::setw(3) << (int)header->mReceiverId;
        out << std::string(" -- diff: ");
        out << std::setfill(' ') << std::setw(3) << diff << "\n";
    }
}

BridgeHandlerStatistics::BridgeHandlerStatistics(struct NDLComBridge &_caller)
    : BridgeHandler(_caller, "BridgeHandlerStatistics"), bytesTransmitted(0),
      bytesReceived(0) {}

void BridgeHandlerStatistics::handle(
    const struct NDLComHeader *header, const void *payload,
    const struct NDLComExternalInterface *origin) {
    // when "origin" is zero the message was sent internally:
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
