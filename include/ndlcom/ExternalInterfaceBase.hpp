#ifndef EXTERNALINTERFACEBASE_H
#define EXTERNALINTERFACEBASE_H

#include "ndlcom/HandlerCommon.hpp"
#include "ndlcom/ExternalInterface.h"

namespace ndlcom {

/**
 * Will use this typedef with a silly name
 */
typedef class HandlerCommon<struct NDLComBridge, struct NDLComExternalInterface>
    ExternalInterfaceVeryBase;

/**
 * @brief C++ wrapper for "struct NDLComExternalInterface"
 *
 * This implements a wrapper which is able to use the c-based implementation
 * for message handling.
 *
 * Relies on the references to NDLComBridge and NDLComExternalInterface stored
 * in the very base class. Handles interaction with the Bridge, wrapping
 * virtual functions around the read/write interface and provides some
 * convenience like common output stream, pauseing and statistics.
 *
 * TODO:
 * - change signature of "writeEscapedBytes" to return the number of bytes
 *   actually written so that we can count bytes which where lost due to
 *   buffer-full.
 */
class ExternalInterfaceBase : public ExternalInterfaceVeryBase {
  public:
    ExternalInterfaceBase(
        struct NDLComBridge &bridge, std::string label,
        std::ostream &out = std::cerr,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    virtual ~ExternalInterfaceBase() {}

    virtual void writeEscapedBytes(const void *buf, size_t count) = 0;
    virtual size_t readEscapedBytes(void *buf, size_t count) = 0;

    /**
     * Allows to temporarily silence this ExternalInterface. No more data will
     * be written to hardware. Note that reads are still performed to empty the
     * system buffers, but no data goes into the bridge.
     */
    bool paused;

    /**
     * Total number of raw-bytes transmitted through this interface
     */
    unsigned long bytesTransmitted;

    /**
     * Total number of raw-bytes received from this interface
     */
    unsigned long bytesReceived;

    /**
     * Allows settings flags on the interface. Not many are currently supported
     */
    void setFlag(uint8_t flag, bool value);

    /**
     * use this interface to reach deviceId
     */
    void setRoutingForDeviceId(const NDLComId deviceId);

    void registerHandler() override final;
    void deregisterHandler() override final;

  protected:
    /**
     * The wrapped C-datastructure
     */
    struct NDLComExternalInterface external;

    /**
     * Helper function which adds up the number of incoming bytes into
     * "bytesReceived".
     *
     * This function shall be called by the virtual read and write functions in
     * deriving classes.
     *
     * @param buf pointer to the buffer of incoming bytes
     * @param count size of the buffer
     */
    virtual void noteIncomingBytes(const void *buf, size_t count);

    /**
     * Helper function which adds up the number of outgoing bytes into
     * "bytesTransmitted".
     *
     * This function shall be called by the virtual read and write functions in
     * deriving classes.
     *
     * @param buf pointer to the buffer of outgoing bytes
     * @param count size of the buffer
     */
    virtual void noteOutgoingBytes(const void *buf, size_t count);

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
    virtual void reportRuntimeError(const std::string &error,
                                    const std::string &file,
                                    const int &line) const;

  private:
    /**
     * Wrapper function to bridge between C and C++ realm
     */
    static void writeWrapper(void *context, const void *buf,
                             const size_t count);
    /**
     * Wrapper function to bridge between C and C++ realm
     */
    static size_t readWrapper(void *context, void *buf, const size_t count);
};
}

#endif /*EXTERNALINTERFACEBASE_H*/
