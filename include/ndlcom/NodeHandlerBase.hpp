#ifndef NDLCOM_NODE_HANDLER_BASE_HPP
#define NDLCOM_NODE_HANDLER_BASE_HPP

#include "ndlcom/NodeHandler.h"

#include <iostream>

namespace ndlcom {

/**
 * @brief virtual base class to wrap "struct NDLComNode" into cpp object
 *
 * the "handle()" function of this object will be called for every messages
 * directed as this node's id and broadcast messages.
 *
 * comes with a protected "out" stream, intended for outputs.
 *
 * The base-class will already register at the bridge! derived classes should
 * be ready to receive data at any time.
 */
class NodeHandlerBase {
  public:
    NodeHandlerBase(struct NDLComNode &node, std::ostream &out = std::cerr);
    virtual ~NodeHandlerBase();

    void send(const NDLComId destination, const void *payload,
              const size_t length);

  protected:
    std::ostream &out;
    struct NDLComNodeHandler internal;

    virtual void handle(const struct NDLComHeader *header, const void *payload,
                        const void *origin) = 0;

  private:
    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin);
};
}

#endif /*NDLCOM_NODE_HANDLER_BASE_HPP*/
