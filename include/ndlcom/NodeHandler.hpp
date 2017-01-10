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
                const void *origin) override;
};

class NodeHandlerStatistics : public NodeHandler {
  public:
    NodeHandlerStatistics(struct NDLComNode &_caller);
    void send(const struct NDLComHeader *header, const void *payload) = delete;
    void handle(const struct NDLComHeader *header, const void *payload,
                const void *origin) override;

    unsigned long currentBytesTx() const;
    unsigned long currentBytesRx() const;
    void resetBytes();

  private:
    unsigned long bytesTransmitted;
    unsigned long bytesReceived;
};
} // namespace ndlcom

#endif /*NDLCOM_NODE_HANDLER_HPP*/
