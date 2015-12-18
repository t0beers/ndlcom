#ifndef EXTERNALINTERFACEBASE_H
#define EXTERNALINTERFACEBASE_H

#include "ndlcom/Bridge.h"

#include <string>

// for "speed_t"
#include <termios.h>
#include <iostream>
// for "struct sockaddr_in" and "socklen_t"
#include <arpa/inet.h>

/**
 * virtual base-class to wrap "struct NDLComExternalInterface" into a cpp-class
 *
 * stores private reference of the NDLComBridge this interface is connected to.
 */
class NDLComBridgeExternalInterface {
  public:
    NDLComBridgeExternalInterface(
        NDLComBridge &_bridge,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    virtual ~NDLComBridgeExternalInterface();

    static void writeWrapper(void *context, const void *buf,
                             const size_t count);
    static size_t readWrapper(void *context, void *buf, const size_t count);

    virtual void writeEscapedBytes(const void *buf, size_t count) = 0;
    virtual size_t readEscapedBytes(void *buf, size_t count) = 0;

  private:
    NDLComBridge &bridge;
    struct NDLComExternalInterface external;
};

#endif /*EXTERNALINTERFACEBASE_H*/
