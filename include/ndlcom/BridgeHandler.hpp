#ifndef NDLCOM_BRIDGE_HANDLER_HPP
#define NDLCOM_BRIDGE_HANDLER_HPP

#include "ndlcom/BridgeHandlerBase.hpp"

#include <bitset>

namespace ndlcom {

/**
 * @brief uses callback to print out information header of EVERY observed packet
 *
 */
class BridgePrintAll : public BridgeHandlerBase {
  public:
    BridgePrintAll(struct NDLComBridge &bridge,
                   std::ostream &_out = std::cerr);
    void handle(const struct NDLComHeader *header, const void *payload,
                const void *origin);
};

/**
 * @brief printing information about missed packages
 *
 * this class hooks looks at every received pacakge and checks the packet
 * counter for a miss-event; prints a message but never sends messages itself.
 */
class BridgePrintMissEvents : public BridgeHandlerBase {
  public:
    BridgePrintMissEvents(struct NDLComBridge &bridge,
                          std::ostream &_out = std::cerr);
    void handle(const struct NDLComHeader *header, const void *payload,
                const void *origin);
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
}

#endif /*NDLCOM_BRIDGE_HANDLER_HPP*/
