#include "ndlcom/InternalHandler.hpp"

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

// input/output
#include <iostream>
#include <iomanip>

using namespace ndlcom;

BridgePrintAll::BridgePrintAll(struct NDLComBridge &_bridge, std::ostream &_out)
    : BridgeHandlerBase(_bridge, _out) {}

void BridgePrintAll::handle(const struct NDLComHeader *header,
                            const void *payload) {
    out << std::string("bridge saw message from ");
    out << std::showbase << std::hex;
    out << std::setfill('0') << std::setw(4) << std::internal
        << (int)header->mSenderId;
    out << std::string(" to ");
    out << std::setfill('0') << std::setw(4) << std::internal
        << (int)header->mReceiverId;
    out << std::string(" with ");
    out << std::noshowbase << std::dec;
    out << std::setw(3) << std::setfill(' ') << std::right
        << (int)header->mDataLen;
    out << std::string(" bytes of payload\n");
}

BridgePrintMissEvents::BridgePrintMissEvents(struct NDLComBridge &_bridge,
                                             std::ostream &_out)
    : BridgeHandlerBase(_bridge, _out) {}

void BridgePrintMissEvents::handle(const struct NDLComHeader *header,
                                   const void *payload) {
    // broadcast receiver dont make sense... skip 'em
    if (header->mSenderId == NDLCOM_ADDR_BROADCAST) {
        return;
    }

    /* allow configuring smaller value for MAX_NUMBER_OF_DEVICES. is
     * always false by default and should be optimized by the compiler */
    if (header->mSenderId >= NDLCOM_MAX_NUMBER_OF_DEVICES ||
        header->mReceiverId >= NDLCOM_MAX_NUMBER_OF_DEVICES)
        return;

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
            out << std::string("miss event in message from 0x");
            out << std::showbase << std::hex;
            out << std::setfill('0') << std::setw(4) << std::internal
                << (int)header->mSenderId;
            out << std::string(" to ");
            out << std::setfill('0') << std::setw(4) << std::internal
                << (int)header->mReceiverId;
            out << std::string(" -- diff: ");
            out << std::noshowbase << std::dec;
            out << std::setw(3) << std::setfill(' ') << std::right << diff
                << "\n";
        }
    }

    // in any case we remember the next expected packet counter of this
    // connecetion
    expectedNextPacketCounter[header->mSenderId][header->mReceiverId] =
        header->mCounter + 1;
}

NodePrintOwnId::NodePrintOwnId(struct NDLComNode &_node, std::ostream &_out)
    : NodeHandlerBase(_node, _out) {}

void NodePrintOwnId::handle(const struct NDLComHeader *header,
                            const void *payload) {
    out << std::string("listener ");
    out << std::showbase << std::hex;
    out << std::setfill('0') << std::setw(4) << std::internal
        << (int)node.headerConfig.mOwnSenderId;
    out << std::string(" got message from ");
    out << std::setfill('0') << std::setw(4) << std::internal
        << (int)header->mSenderId;
    out << std::string(" with ");
    out << std::noshowbase << std::dec;
    out << std::setw(3) << std::setfill(' ') << std::right
        << (int)header->mDataLen;
    out << std::string(" bytes of payload\n");
}
