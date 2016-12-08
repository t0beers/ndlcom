#ifndef NDLCOM_NODE_HANDLER_BASE_HPP
#define NDLCOM_NODE_HANDLER_BASE_HPP

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

#include <iostream>

namespace ndlcom {

/**
 * @brief virtual base class to wrap "struct NDLComNode" into cpp object
 *
 * the "handle()" function of this object will be called for every messages
 * directed as this node's id and broadcast messages.
 *
 * comes with a protected "out" stream, intended for outputs.
 */
class NodeHandlerBase {
  public:
    NodeHandlerBase(struct NDLComNode &_node, std::ostream &out = std::cerr);
    virtual ~NodeHandlerBase();

    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin);

    virtual void handle(const struct NDLComHeader *header, const void *payload,
                        const void *origin) = 0;

    void send(const NDLComId destination, const void *payload,
              const size_t length);

  protected:
    struct NDLComNode &node;
    std::ostream &out;

  private:
    struct NDLComNodeHandler internal;
};
}

#endif /*NDLCOM_NODE_HANDLER_BASE_HPP*/
