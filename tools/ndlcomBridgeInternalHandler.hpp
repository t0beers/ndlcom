#ifndef NDLCOMBRIDGEINTERNALHANDLER_H
#define NDLCOMBRIDGEINTERNALHANDLER_H

#include "ndlcom/Bridge.h"
#include "ndlcom/Interfaces.h"

class NDLComBridgeInternalHandler {
  public:
    NDLComBridgeInternalHandler(NDLComBridge& _bridge);
    ~NDLComBridgeInternalHandler();

    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload);

    virtual void handle(const struct NDLComHeader *header, const void *payload) = 0;

  protected:
    NDLComBridge& bridge;
  private:
    struct NDLComInternalHandler internal;
};

class NDLComBridgePrintAll : public NDLComBridgeInternalHandler {
  public:
    NDLComBridgePrintAll(NDLComBridge &_bridge)
        : NDLComBridgeInternalHandler(_bridge){};
    void handle(const struct NDLComHeader *header, const void *payload);
};

class NDLComBridgePrintOwnId : public NDLComBridgeInternalHandler {
  public:
    NDLComBridgePrintOwnId(NDLComBridge &_bridge)
        : NDLComBridgeInternalHandler(_bridge){};
    void handle(const struct NDLComHeader *header, const void *payload);
};

#endif /*NDLCOMBRIDGEINTERNALHANDLER_H*/
