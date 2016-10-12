#ifndef EXTERNALINTERFACEBASE_H
#define EXTERNALINTERFACEBASE_H

#include "ndlcom/Bridge.h"

#include <iostream>

namespace ndlcom {

/**
 * @brief Virtual class to wrap "struct NDLComExternalInterface" into cpp-object
 *
 * This base class implements a tight and lean wrapper which are able to use
 * the C-based core implementation for message handling.  This is how we write
 * code -- reusable.
 *
 * Stores private reference of the "struct NDLComBridge" where this interface
 * is connected to.
 *
 * Has reference to the preferred output stream for status reports.
 * Additionally adds some statistics and convenience things like statistics, a
 * label and pausing.
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

    void setFlag(uint8_t flag, bool value);

  protected:
    struct NDLComBridge &bridge;
    struct NDLComExternalInterface external;

    /**
     * Helper function which adds up the number of incoming bytes into
     * "bytesReceived".
     *
     * @param buf pointer to the buffer
     * @param count size of the buffer
     */
    virtual void noteIncomingBytes(const void *buf, size_t count);

    /**
     * Helper function which adds up the number of outgoing bytes into
     * "bytesTransmitted".
     *
     * @param buf pointer to the buffer
     * @param count size of the buffer
     */
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
