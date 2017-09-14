#ifndef NDLCOM_HANDLER_COMMON_HPP
#define NDLCOM_HANDLER_COMMON_HPP

#include <iostream>
#include <string>

namespace ndlcom {

/**
 * @brief The interface-class for using callback structs in c++
 *
 * Used as basis for "ExternalInterface", "NodeHandler" and "BridgeHandler".
 *
 * Provides some convenience things like a label and the means to
 * register/deregister the Handler at the Caller. By having this
 * base-implementation much of the setup/teardown is done automatically in
 * base-classes.
 *
 * NOTE: It does work to wrap an existing c-object into c++ using this
 * base-class! See ...
 */
template <class Caller, class Handler> class HandlerCommon {
  public:
    HandlerCommon(Caller &_caller, Handler &_handler, std::string _label,
                  std::ostream &_out = std::cerr)
        : label(_label), out(_out), handler(_handler), caller(_caller) {}
    virtual ~HandlerCommon() {}
    /**
     * delete copy and assignment operators
     *
     * Might help to prevent weirdness from happening. By doing this in this
     * very base-class, all (at least for now) deriving Handlers won't have
     * these as well -- as long as nobody implements them explicitly.
     */
    HandlerCommon(const HandlerCommon &) = delete;
    HandlerCommon &operator=(HandlerCommon const &) = delete;
    /**
     * Set this string in ctor of deriving classes, shall not be changed during
     * runtime. Is used to display a nice name.
     */
    const std::string label;
    /**
     * Used to attach the used "Handler" to the "Caller". To prevent
     * uninitialized objects this function shall be called AFTER the ctor of the
     * c++ object finished.
     */
    virtual void registerHandler() = 0;
    /**
     * Used to remove the "Handler" from the "Caller". This function shall be
     * called BEFORE the dtor of the c++ object!
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
} // namespace ndlcom

#endif /*NDLCOM_HANDLER_COMMON_HPP*/
