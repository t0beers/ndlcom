#ifndef EXTERNALINTERFACEBASE_H
#define EXTERNALINTERFACEBASE_H

#include "ndlcom/Bridge.h"

#include <iostream>

namespace ndlcom {

/**
 * @brief virtual class to wrap "struct NDLComExternalInterface" into cpp-object
 *
 * use this base class to implement tight and lean wrappers which are able to
 * be used with the rest of the code. although i doubt that this will will be
 * done ever, even once. but alas, this is how we write code -- reusable.
 *
 * stores private reference of the "struct NDLComBridge" where this interface
 * is connected to.
 *
 * has protected reference to the preferred output stream.
 */
class ExternalInterfaceBase {
  public:
    ExternalInterfaceBase(
        struct NDLComBridge &_bridge, std::ostream &out = std::cerr,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    virtual ~ExternalInterfaceBase(){}

    static void writeWrapper(void *context, const void *buf,
                             const size_t count);
    static size_t readWrapper(void *context, void *buf, const size_t count);

    virtual void writeEscapedBytes(const void *buf, size_t count) = 0;
    virtual size_t readEscapedBytes(void *buf, size_t count) = 0;

    std::string label;
    bool paused;
    unsigned long bytesTransmitted;
    unsigned long bytesReceived;

  protected:
    struct NDLComBridge &bridge;
    struct NDLComExternalInterface external;

    virtual void noteIncomingBytes(const void *buf, size_t count);
    virtual void noteOutgoingBytes(const void *buf, size_t count);

    std::ostream &out;

    /**
     * a common error-reporting function, which shall be used to report
     * non-recoverable errors. the default implementation will "throw" a
     * properly formatted string.
     *
     * @param error Description of the error which occured
     * @param file the filename, preferably from the __FILE__ macro
     * @param line the linenumber, preferably from the __LINE__ macro
     *
     * shall not return?
     */
    virtual void reportRuntimeError(const std::string &error, const std::string &file,
                                    const int &line) const;
};
}

#endif /*EXTERNALINTERFACEBASE_H*/
