#ifndef NDLCOM_INTERNAL_HANDLER_BASE_HPP
#define NDLCOM_INTERNAL_HANDLER_BASE_HPP

#include "ndlcom/NodeHandler.h"
#include "ndlcom/BridgeHandler.h"
#include "Node.h"
#include "Bridge.h"

#include <iostream>

namespace ndlcom {

// NOTE: is does not work to wrap an existing c-object into c++ using this
// base-class

// the interface-class...
//
// is basically the same basis for "NodeHandler" and "BridgeHandler"
//
// provides just some convenience things like the label
//
// maybe it does not need to keep a reference to "Caller"
template <class Caller, class Handler> class InternalHandlerBase {
  public:
    InternalHandlerBase(Caller &caller, Handler &handler, std::string label,
                          std::ostream &out = std::cerr)
        : handler(handler), caller(caller), label(label), out(out){};
    virtual ~InternalHandlerBase(){};
    std::string label;

  protected:
    std::ostream &out;
    Handler &handler;
    Caller &caller;
};

// typedefs to be used to shorten code
typedef class InternalHandlerBase<struct NDLComNode, struct NDLComNodeHandler>
    NodeHandlerBase;
typedef class InternalHandlerBase<struct NDLComBridge, struct NDLComBridgeHandler>
    BridgeHandlerBase;

// this class is intended to implement an actual c++ callback, cannot be used
// to wrap things that already implement/need a c callback
//
// this class adds the wrapper-callback and its implementation.  this is still
// the same for NodeHandler and BridgeHandler
template <class Caller, class Handler>
class InternalHandler : public InternalHandlerBase<Caller, Handler> {
  public:
    template <typename... Args>
    InternalHandler(Args &&... args)
        : InternalHandlerBase<Caller, Handler>(std::forward<Args>(args)...){};
    virtual ~InternalHandler(){};

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

// to be called from within the bridge
//
// specializes for the BridgeHandler
//
// provides registration of the internal bridge-handler at the bridge and a
// sendRaw-function for convenience
class BridgeHandler
    : public InternalHandler<struct NDLComBridge, struct NDLComBridgeHandler> {
  public:
    template <typename... Args>
    BridgeHandler(struct NDLComBridge &caller, Args &&... args)
        : InternalHandler(caller, internal, std::forward<Args>(args)...) {
        ndlcomBridgeHandlerInit(&internal, InternalHandler::handleWrapper,
                                NDLCOM_BRIDGE_HANDLER_FLAGS_DEFAULT, this);
        ndlcomBridgeRegisterBridgeHandler(&caller, &internal);
    }
    void send(const struct NDLComHeader *header, const void *payload) {
        ndlcomBridgeSendRaw(&caller, header, payload);
    }

  private:
    /* kept private! */
    struct NDLComBridgeHandler internal;
};

// this is for an actual c++ implementation of a handler
//
// same for NodeHandler
class NodeHandler
    : public InternalHandler<struct NDLComNode, struct NDLComNodeHandler> {
  public:
    template <typename... Args>
    NodeHandler(struct NDLComNode &caller, Args &&... args)
        : InternalHandler(caller, internal, std::forward<Args>(args)...) {
        ndlcomNodeHandlerInit(&internal, InternalHandler::handleWrapper,
                              NDLCOM_NODE_HANDLER_FLAGS_DEFAULT, this);
        ndlcomNodeRegisterNodeHandler(&caller, &internal);
    }
    void send(const NDLComId receiverId, const void *payload,
              const size_t length) {
        ndlcomNodeSend(&caller, receiverId, payload, length);
    }
    NDLComId getOwnDeviceId() const {
        return handler.node->headerConfig.mOwnSenderId;
    }

  private:
    struct NDLComNodeHandler internal;
};

// intended as a wrapper object for c things
//
// this will allow to wrap things which already use a c callback implementation
// into an interface that can be reused by the existining factory functions
//
// does only specialize the base-interface class into "Node" related functions
class NodeHandlerWrapper : public InternalHandlerBase<
                               struct NDLComNode, struct NDLComNodeHandler> {
  public:
    template <typename... Args>
    NodeHandlerWrapper(struct NDLComNode &caller,
                       struct NDLComNodeHandler &handler, Args &&... args)
        : InternalHandlerBase(caller, handler, std::forward<Args>(args)...) {}
};

// intended as a wrapper object for c things
class BridgeHandlerWrapper
    : public InternalHandlerBase<struct NDLComBridge,
                                   struct NDLComBridgeHandler> {
  public:
    template <typename... Args>
    BridgeHandlerWrapper(struct NDLComBridge &caller,
                         struct NDLComBridgeHandler &handler, Args &&... args)
        : InternalHandlerBase(caller, handler, std::forward<Args>(args)...) {}
};

}

#endif /*NDLCOM_INTERNAL_HANDLER_BASE_HPP*/
