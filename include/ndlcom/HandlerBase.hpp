#ifndef NDLCOM_HANDLER_BASE_HPP
#define NDLCOM_HANDLER_BASE_HPP

#include <iostream>

namespace ndlcom {

/**
 * NOTE: is does work to wrap an existing c-object into c++ using this
 * base-class!
 *
 * the interface-class...
 *
 * is used as basis for "ExternalInterface", "NodeHandler" and
 * "BridgeHandler"
 *
 * provides just some convenience things like the label and the means to
 * enable/disable
 *
 * maybe it does not need to keep a reference to "Caller"
 */
template <class Caller, class Handler> class HandlerBase {
  public:
    HandlerBase(Caller &caller, Handler &handler, std::string label,
                        std::ostream &out = std::cerr)
        : handler(handler), caller(caller), label(label), out(out) {}
    virtual ~HandlerBase(){};
    std::string label;

    /**
     * the two verbs which are needed...: "registering" and "deregistering"
     */
    virtual void registerHandler() = 0;
    virtual void deregisterHandler() = 0;

  protected:
    std::ostream &out;
    Handler &handler;
    Caller &caller;
};
}

#endif /*NDLCOM_HANDLER_BASE_HPP*/
