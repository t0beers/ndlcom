#ifndef INTERNALHANDLERBASE_H
#define INTERNALHANDLERBASE_H

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

#include <iostream>

namespace ndlcom {

/**
 * @brief virtual base class to wrap "struct NDLComInternalHandler" into object
 *
 * the "handle()" function of this object will be called for every messages
 * passing through the bridge.
 *
 * comes with a protected "out" stream, intended for outputs.
 */
class BridgeHandlerBase {
  public:
    BridgeHandlerBase(struct NDLComBridge &_bridge,
                      std::ostream &out = std::cerr);
    virtual ~BridgeHandlerBase();

    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload);

    virtual void handle(const struct NDLComHeader *header,
                        const void *payload) = 0;

  protected:
    struct NDLComBridge &bridge;
    std::ostream &out;

  private:
    struct NDLComInternalHandler internal;
};

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
                              const void *payload);

    virtual void handle(const struct NDLComHeader *header,
                        const void *payload) = 0;

    void send(const NDLComId destination, const void *payload,
              const size_t length);

  protected:
    struct NDLComNode &node;
    std::ostream &out;

  private:
    struct NDLComInternalHandler internal;
};
}

#endif /*INTERNALHANDLERBASE_H*/
