#ifndef NDLCOM_HANDLER_BASE_HPP
#define NDLCOM_HANDLER_BASE_HPP

#include <iostream>

namespace ndlcom {

/**
 * @brief The interface-class for using callback structs in c++
 *
 * Is used as basis for "ExternalInterface", "NodeHandler" and "BridgeHandler".
 *
 * Provides some convenience things like a label and the means to
 * register/deregister at the Caller. Maybe it does strictly speaking not need
 * to keep a reference to "Caller", kept nevertheless.
 *
 * NOTE: Is does work to wrap an existing c-object into c++ using this
 * base-class!
 */
template <class Caller, class Handler> class HandlerBase {
  public:
    HandlerBase(Caller &caller, Handler &handler, std::string label,
                std::ostream &out = std::cerr)
        : handler(handler), caller(caller), label(label), out(out) {}
    virtual ~HandlerBase(){};
    std::string label;

    /**
     * Used to attach the used "Handler" to the "Caller". To prevent
     * uninitialized objects this function shall be called after the ctor of
     * the c++ object finished.
     */
    virtual void registerHandler() = 0;

    /**
     * Used to remove the "Handler" from the "Caller". This function can be
     * called on the dtor of the c++ object (?)
     */
    virtual void deregisterHandler() = 0;

  protected:
    /**
     * Common output stream, intended for error messages.
     */
    std::ostream &out;
    /**
     * Reference to the wrapped Handler
     */
    Handler &handler;
    /**
     * Reference to the wrapped Caller
     */
    Caller &caller;
};
}

#endif /*NDLCOM_HANDLER_BASE_HPP*/
