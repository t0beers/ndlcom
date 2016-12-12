#ifndef NDLCOM_NODE_HANDLER_HPP
#define NDLCOM_NODE_HANDLER_HPP

#include "ndlcom/NodeHandlerBase.hpp"

namespace ndlcom {

/**
 * @brief Prints information about messages received by the Node
 *
 * Just a simple outputting class to print a line containing information of the
 * header for every message observed.
 */
class NodeHandlerPrintOwnId final : public NodeHandlerBase {
  public:
    NodeHandlerPrintOwnId(struct NDLComNode &node,
                          std::ostream &_out = std::cerr);
    void handle(const struct NDLComHeader *header, const void *payload,
                const void *origin) override;
};
}

#endif /*NDLCOM_NODE_HANDLER_HPP*/
