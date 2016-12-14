#ifndef NDLCOM_INTERNAL_HANDLER_HPP
#define NDLCOM_INTERNAL_HANDLER_HPP

#include "ndlcom/HandlerBase.hpp"
#include "ndlcom/NodeHandler.h"
#include "ndlcom/BridgeHandler.h"
#include "Node.h"
#include "Bridge.h"

#include <iostream>

namespace ndlcom {

/**
 * these are needed when directly implementing a c-wrapper on top of the
 * interface
 */
typedef class HandlerBase<struct NDLComNode, struct NDLComNodeHandler>
    NodeHandlerBase;
typedef class HandlerBase<struct NDLComBridge, struct NDLComBridgeHandler>
    BridgeHandlerBase;

/**
 * @brief Provide c++ callback with same interface as wrapped c classes
 *
 * Cannot be used to wrap things that already implement/use a c callback
 *
 * Adds the wrapper-callback and its implementation. Written as a template
 * class to provide the same implementation for the two handlers used
 * internally: NodeHandler and BridgeHandler.
 *
 * When implementing the derived class, be sure to use the static function
 * "handleWrapper()" when registering at the caller and the "this" pointer as
 * context.
 */
template <class Caller, class Handler>
class InternalHandler : public HandlerBase<Caller, Handler> {
  public:
    template <typename... Args>
    InternalHandler(Args &&... args)
        : HandlerBase<Caller, Handler>(std::forward<Args>(args)...){}

    // this "handle" could be called before the last ctor did finish!
    virtual void handle(const struct NDLComHeader *header, const void *payload,
                        const void *origin) = 0;

  protected:
    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin) {
        class InternalHandler *self =
            static_cast<class InternalHandler *>(context);
        self->handle(header, payload, origin);
    }
};

/**
 * to be called from within the bridge
 *
 * provides c++ interface and uses the c-infrastructure in the background to
 * trigger calling of the virtual "handle()" function of the base-class
 *
 * specializes for the BridgeHandler
 *
 * provides registration of the internal bridge-handler at the bridge and a
 * sendRaw-function for convenience
 */
class BridgeHandler
    : public InternalHandler<struct NDLComBridge, struct NDLComBridgeHandler> {
  public:
    template <typename... Args>
    BridgeHandler(struct NDLComBridge &caller, Args &&... args)
        : InternalHandler(caller, internal, std::forward<Args>(args)...) {
        /* static "handleWrapper" as callback and "this" as context */
        ndlcomBridgeHandlerInit(&internal, handleWrapper,
                                NDLCOM_BRIDGE_HANDLER_FLAGS_DEFAULT, this);
    }
    void send(const struct NDLComHeader *header, const void *payload) {
        ndlcomBridgeSendRaw(&caller, header, payload);
    }
    void registerHandler() override final {
        ndlcomBridgeRegisterBridgeHandler(&caller, &internal);
    }
    void deregisterHandler() override final {
        ndlcomBridgeDeregisterBridgeHandler(&caller, &internal);
    }

  private:
    /* kept private! */
    struct NDLComBridgeHandler internal;
};

/**
 * @brief Same as the BridgeHandler, for NodeHandler
 */
class NodeHandler
    : public InternalHandler<struct NDLComNode, struct NDLComNodeHandler> {
  public:
    template <typename... Args>
    NodeHandler(struct NDLComNode &caller, Args &&... args)
        : InternalHandler(caller, internal, std::forward<Args>(args)...) {
        ndlcomNodeHandlerInit(&internal, handleWrapper,
                              NDLCOM_NODE_HANDLER_FLAGS_DEFAULT, this);
    }
    void send(const NDLComId receiverId, const void *payload,
              const size_t length) {
        ndlcomNodeSend(&caller, receiverId, payload, length);
    }
    void registerHandler() override final {
        ndlcomNodeRegisterNodeHandler(&caller, &internal);
    }
    void deregisterHandler() override final {
        ndlcomNodeDeregisterNodeHandler(&caller, &internal);
    }
    /* does only work after finished initialization */
    NDLComId getOwnDeviceId() const {
        return handler.node->headerConfig.mOwnSenderId;
    }

  private:
    struct NDLComNodeHandler internal;
};
}

#endif /*NDLCOM_INTERNAL_HANDLER_HPP*/
