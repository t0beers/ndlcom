#ifndef NDLCOM_BRIDGE_HANDLER_HPP
#define NDLCOM_BRIDGE_HANDLER_HPP

#include <bitset>
#include <iostream>

#include "ndlcom/InternalHandler.hpp"
#include "ndlcom/Types.h"

namespace ndlcom {

/**
 * @brief uses callback to print out information header of EVERY observed packet
 */
class BridgePrintAll final : public BridgeHandler {
  public:
    BridgePrintAll(struct NDLComBridge &bridge, std::ostream &out = std::cerr);
    void handle(const struct NDLComHeader *header, const void *payload,
                const struct NDLComExternalInterface *origin) override;
};

/**
 * @brief printing information about missed packages
 *
 * this class hooks looks at every received pacakge and checks the packet
 * counter for a miss-event; prints a message but never sends messages itself.
 */
class BridgePrintMissEvents final : public BridgeHandler {
  public:
    BridgePrintMissEvents(struct NDLComBridge &bridge,
                          std::ostream &out = std::cerr);
    void handle(const struct NDLComHeader *header, const void *payload,
                const struct NDLComExternalInterface *origin) override;
    /** clears every internal datastructure */
    void resetMissEvents();

  private:
    /**
     * keep this matrix for miss-events between all possible combinations of
     * known deviceIds
     */
    unsigned int numberOfPacketMissEvents[NDLCOM_MAX_NUMBER_OF_DEVICES]
                                         [NDLCOM_MAX_NUMBER_OF_DEVICES];
    /**
     * remembers the expected PacketCounter for each deviceId, based on the
     * last-seen counter incremented by 1.
     */
    NDLComCounter expectedNextPacketCounter[NDLCOM_MAX_NUMBER_OF_DEVICES]
                                           [NDLCOM_MAX_NUMBER_OF_DEVICES];
    /**
     * On the calculation of the size of the bitset: for each combination of
     * sender+receive we have to remember if we saw it before.
     */
    std::bitset<NDLCOM_MAX_NUMBER_OF_DEVICES << 8> alreadySeen;
};

/**
 * @brief Provide statistics about received and transmitted bytes
 *
 * Will count how many bytes are flowing through the NDLComBridge, divided by
 * "sent internally" and "received externally", depending on the "origin" value
 * passed into the handle function. Two cases:
 *
 * - "origin" is not zero: Bytes coming from a specific
 *   NDCLComExternalInterface are  externally read and thus incoming or (in
 *   case of just routing) passing through the bridge. Will only count the
 *   decoded bytes once, as the bytes itself being written to possibly many
 *   NDLComExternalInterface can only be seen by the instances of each
 *   interface themselves.
 * - "origin" is zero: Bytes which are not accociated with a specific
 *   NDLComExternalInterface: Sent by some internal handler.
 */
class BridgeHandlerStatistics : public BridgeHandler {
  public:
    BridgeHandlerStatistics(struct NDLComBridge &_caller);
    /** to make matters simple, this class is not allowed to transmit */
    void send(const struct NDLComHeader *header, const void *payload) = delete;
    void handle(const struct NDLComHeader *header, const void *payload,
                const struct NDLComExternalInterface *origin) override;

    unsigned long currentBytesTx() const;
    unsigned long currentBytesRx() const;
    void resetBytes();

  private:
    unsigned long bytesTransmitted;
    unsigned long bytesReceived;
};
} // namespace ndlcom

#endif /*NDLCOM_BRIDGE_HANDLER_HPP*/
