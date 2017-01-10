#ifndef NDLCOM_INTERNAL_HANDLER_HPP
#define NDLCOM_INTERNAL_HANDLER_HPP

#include <stddef.h>
#include <algorithm>

#include "ndlcom/Bridge.h"
#include "ndlcom/HandlerCommon.hpp"
#include "ndlcom/Node.h"
#include "ndlcom/NodeHandler.h"
#include "ndlcom/Types.h"

namespace ndlcom {

/**
 * These are needed when directly implementing a c-wrapper on top of the
 * interface.
 *
 * These two are instantiation of the "InternalHandler" template for the Node
 * and Bridge case.
 */
typedef class HandlerCommon<struct NDLComNode, struct NDLComNodeHandler>
    NodeHandlerBase;
typedef class HandlerCommon<struct NDLComBridge, struct NDLComBridgeHandler>
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
 *
 * InternalHandler have to cope with decoded messages, contrary to
 * ExternalInterfaces which only see chunks of bytes.
 */
template <class Caller, class Handler>
class InternalHandler : public HandlerCommon<Caller, Handler> {
  public:
    template <typename... Args>
    InternalHandler(Args &&... args)
        : HandlerCommon<Caller, Handler>(std::forward<Args>(args)...){}

    /**
     * This function will be called by the "Caller" to handle a package
     *
     * Node: To prevent early call of this handler never call the
     * "registerHandler" of the base-class before the last ctor did finish.
     */
    virtual void handle(const struct NDLComHeader *header, const void *payload,
                        const void *origin) = 0;

  protected:
    /**
     * Internal wrapper function. This handler will be called by the c-language
     * core and dispatches to the actual c++ function belonging to this object.
     */
    static void handleWrapper(void *context, const struct NDLComHeader *header,
                              const void *payload, const void *origin) {
        class InternalHandler *self =
            static_cast<class InternalHandler *>(context);
        self->handle(header, payload, origin);
    }
};

/**
 * To be called from within the bridge
 *
 * Provides c++ interface and uses the c-infrastructure in the background to
 * trigger calling of the virtual "handle()" function of the base-class
 *
 * Specializes for the BridgeHandler
 *
 * provides registration of the internal bridge-handler at the bridge and a
 * sendRaw-function for convenience
 */
class BridgeHandler
    : public InternalHandler<struct NDLComBridge, struct NDLComBridgeHandler> {
  public:
    template <typename... Args>
    BridgeHandler(struct NDLComBridge &_caller, Args &&... args)
        : InternalHandler(_caller, internal, std::forward<Args>(args)...) {
        /* static "handleWrapper" as callback and "this" as context */
        ndlcomBridgeHandlerInit(&internal, handleWrapper,
                                NDLCOM_BRIDGE_HANDLER_FLAGS_DEFAULT, this);
    }
    void sendRaw(const struct NDLComHeader *header, const void *payload);
    /**
     * To be called after ctor
     */
    void registerHandler() final;
    /**
     * To be called before dtor
     */
    void deregisterHandler() final;

  private:
    /** Intentionally kept private! */
    struct NDLComBridgeHandler internal;
};

/**
 * @brief Same as the BridgeHandler, for NodeHandler
 */
class NodeHandler
    : public InternalHandler<struct NDLComNode, struct NDLComNodeHandler> {
  public:
    template <typename... Args>
    NodeHandler(struct NDLComNode &_caller, Args &&... args)
        : InternalHandler(_caller, internal, std::forward<Args>(args)...) {
        ndlcomNodeHandlerInit(&internal, handleWrapper,
                              NDLCOM_NODE_HANDLER_FLAGS_DEFAULT, this);
    }
    void send(const NDLComId receiverId, const void *payload,
              const size_t length);
    /**
     * To be called after ctor
     */
    void registerHandler() final;
    /**
     * To be called before dtor
     */
    void deregisterHandler() final;
    /**
     * Only works after ctor is finished
     */
    const NDLComId getOwnDeviceId() const;

  private:
    /** Intentionally kept private! */
    struct NDLComNodeHandler internal;
};
}// namespace ndlcom

#endif /*NDLCOM_INTERNAL_HANDLER_HPP*/
