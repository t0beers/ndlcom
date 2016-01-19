#ifndef EXTERNALINTERFACEBASE_H
#define EXTERNALINTERFACEBASE_H

#include "ndlcom/Bridge.h"

namespace ndlcom {

/**
 * virtual base-class to wrap "struct NDLComExternalInterface" into a cpp-class.
 *
 * stores private reference of the "struct NDLComBridge" where this interface
 * is connected to.
 */
class ExternalInterfaceBase {
  public:
    ExternalInterfaceBase(
        struct NDLComBridge &_bridge,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    virtual ~ExternalInterfaceBase();

    static void writeWrapper(void *context, const void *buf,
                             const size_t count);
    static size_t readWrapper(void *context, void *buf, const size_t count);

    virtual void writeEscapedBytes(const void *buf, size_t count) = 0;
    virtual size_t readEscapedBytes(void *buf, size_t count) = 0;

  private:
    struct NDLComBridge &bridge;
    struct NDLComExternalInterface external;
};
}

#endif /*EXTERNALINTERFACEBASE_H*/
