#ifndef NDLCOM_NODE_HANDLER_HPP
#define NDLCOM_NODE_HANDLER_HPP

#include <iostream>

#include "ndlcom/InternalHandler.hpp"

namespace ndlcom {

/**
 * @brief Prints information about messages received by the Node
 *
 * Just a simple outputting class to print a line containing information of the
 * header for every message observed.
 */
class NodeHandlerPrintOwnId final : public NodeHandler {
  public:
    NodeHandlerPrintOwnId(struct NDLComNode &node,
                          std::ostream &_out = std::cerr);
    void send(const struct NDLComHeader *header, const void *payload) = delete;
    void handle(const struct NDLComHeader *header, const void *payload,
                const struct NDLComExternalInterface *origin) override;
};

/**
 * @brief Collects statistics about data received by this node
 *
 * Only data received by this node!
 *
 * Information about data sent by this node cannot be collected centrally in a
 * reliable way due do design quirks: The handler-function of a
 * NDLComNodeHandler will only be called when the NDLComNode is actually
 * supposed to handle the message. And if someone sends to some other deviceId,
 * the message will not be seen again by the NDLComNode. Remembering the number
 * of bytes passing through the NDLComNodeSend method itself won't really work
 * as someone could still call NDLComBridgeSendRaw.  bridge.
 *
 * So this class does only provide the "transmitted bytes" information, in
 * contrast to ndlcom::BridgeHandlerStatistics.
 */
class NodeHandlerStatistics : public NodeHandler {
  public:
    NodeHandlerStatistics(struct NDLComNode &_caller);
    void send(const struct NDLComHeader *header, const void *payload) = delete;
    void handle(const struct NDLComHeader *header, const void *payload,
                const struct NDLComExternalInterface *origin) override;

    unsigned long currentBytesRx() const;
    void resetBytes();

  private:
    unsigned long bytesReceived;
};
} // namespace ndlcom

#endif /*NDLCOM_NODE_HANDLER_HPP*/
