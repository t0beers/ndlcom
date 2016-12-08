#ifndef NDLCOM_NODE_HANDLER_HPP
#define NDLCOM_NODE_HANDLER_HPP

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"
#include "ndlcom/NodeHandlerBase.hpp"

#include <bitset>

namespace ndlcom {

/**
 * @brief prints information about messages received by the node
 *
 * just a simple outputting class to print a line containing information of the
 * header for every message observed.
 */
class NodePrintOwnId : public NodeHandlerBase {
  public:
    NodePrintOwnId(struct NDLComNode &_node, std::ostream &_out = std::cerr);
    void handle(const struct NDLComHeader *header, const void *payload,
                const void *origin);
};
}

#endif /*NDLCOM_NODE_HANDLER_HPP*/
