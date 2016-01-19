#ifndef INTERNALHANDLERBASE_H
#define INTERNALHANDLERBASE_H

#include "ndlcom/Bridge.h"
#include "ndlcom/Node.h"

namespace ndlcom {

class BridgeHandler {
  public:
    BridgeHandler(struct NDLComBridge &_bridge,
                        uint8_t flags = NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT);
    virtual ~BridgeHandler();

    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload);

    virtual void handle(const struct NDLComHeader *header,
                        const void *payload) = 0;

  protected:
    struct NDLComBridge &bridge;

  private:
    struct NDLComInternalHandler internal;
};

}

#endif /*INTERNALHANDLERBASE_H*/
