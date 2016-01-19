#ifndef INTERNALHANDLER_HPP
#define INTERNALHANDLER_HPP

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"
#include "ndlcom/InternalHandlerBase.hpp"

#include <bitset>

namespace ndlcom {

/**
 * @brief uses callback to print out information header of EVERY observed packet
 *
 */
class BridgePrintAll : public BridgeHandler {
  public:
    BridgePrintAll(struct NDLComBridge &_bridge, std::ostream &_out);
    void handle(const struct NDLComHeader *header, const void *payload);

  private:
    std::ostream &out;
};

class NodeHandler {
  public:
    NodeHandler(struct NDLComNode &_node);
    virtual ~NodeHandler();

    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload);

    virtual void handle(const struct NDLComHeader *header,
                        const void *payload) = 0;

    void send(const NDLComId destination, const void *payload,
              const size_t length);

  protected:
    struct NDLComNode &node;

  private:
    struct NDLComInternalHandler internal;
};

class NodePrintOwnId : public NodeHandler {
  public:
    NodePrintOwnId(struct NDLComNode &_node, std::ostream &_out);
    void handle(const struct NDLComHeader *header, const void *payload);

  private:
    std::ostream &out;
};

class BridgePrintMissEvents : public BridgeHandler {
  public:
    BridgePrintMissEvents(struct NDLComBridge &_bridge, std::ostream &_out);
    void handle(const struct NDLComHeader *header, const void *payload);
    void resetMissEvents();

  private:
    std::ostream &out;
    /* keep this matrix for miss-events between all possible combinations of
     * known deviceIds */
    unsigned int numberOfPacketMissEvents[NDLCOM_MAX_NUMBER_OF_DEVICES]
                                         [NDLCOM_MAX_NUMBER_OF_DEVICES];
    /* remember the expected PacketCounter for each deviceId, based on the
     * last-seen counter incremented by 1 */
    NDLComCounter expectedNextPacketCounter[NDLCOM_MAX_NUMBER_OF_DEVICES]
                                           [NDLCOM_MAX_NUMBER_OF_DEVICES];
    /* for the size of the bitset: for each combination of sender+receive we
     * have to remeber if we saw it before. */
    std::bitset<NDLCOM_MAX_NUMBER_OF_DEVICES << 8> alreadySeen;
};
}

#endif /*INTERNALHANDLER_HPP*/
