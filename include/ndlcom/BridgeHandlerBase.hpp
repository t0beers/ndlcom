#ifndef NDLCOM_BRIDGE_HANDLER_BASE_HPP
#define NDLCOM_BRIDGE_HANDLER_BASE_HPP

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
                              const void *payload, const void *origin);

    virtual void handle(const struct NDLComHeader *header, const void *payload,
                        const void *origin) = 0;

  protected:
    struct NDLComBridge &bridge;
    std::ostream &out;

  private:
    struct NDLComBridgeHandler internal;
};
}

#endif /*NDLCOM_BRIDGE_HANDLER_BASE_HPP*/
