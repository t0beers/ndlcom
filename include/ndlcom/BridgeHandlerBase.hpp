#ifndef NDLCOM_BRIDGE_HANDLER_BASE_HPP
#define NDLCOM_BRIDGE_HANDLER_BASE_HPP

#include "ndlcom/BridgeHandler.h"

#include <iostream>

namespace ndlcom {

/**
 * @brief virtual base class to wrap "struct NDLComBridgeHandler"
 *
 * The "handle()" function of this object will be called for every messages
 * passing through the bridge.
 *
 * Comes with a protected "out" stream, intended for outputs.
 *
 * Note that deriving classes still have to register at the Node!
 */
class BridgeHandlerBase {
  public:
    BridgeHandlerBase(struct NDLComBridge &bridge,
                      std::ostream &out = std::cerr);
    virtual ~BridgeHandlerBase();

    /**
     * To be set in deriving classes to something meaningful for a human
     */
    std::string label;

  protected:
    std::ostream &out;
    struct NDLComBridgeHandler internal;

    virtual void handle(const struct NDLComHeader *header, const void *payload,
                        const void *origin) = 0;

  private:
    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin);
};
}

#endif /*NDLCOM_BRIDGE_HANDLER_BASE_HPP*/
