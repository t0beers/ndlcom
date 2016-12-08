#ifndef NDLCOM_EXTERNAL_INTERFACE_H
#define NDLCOM_EXTERNAL_INTERFACE_H

#include "ndlcom/Parser.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** the default value for new interfaces: do nothing special */
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT 0x00
/**
 * @brief Make interface opaque for passing messages
 *
 * This flag will cause the added ExternalInterface to be a "debug mirror". It's
 * purpose is to treat incoming messages as if they originate from the internal
 * side, and writing out _all_ messages passing through the bridge,
 * additionally to the normal "routing". So when connecting via this "mirror",
 * another instance can act as if it is this Bridge itself.
 *
 * Messages received on this interface will not be used to update the routing table.
 */
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR 0x01

/**
 * @brief Callback to "write" escaped data from the bridge to somewhere.
 *
 * Note that this function will be in the critical path of the
 * "ndlcomBridgeProcess()" function. It should complete fast and is never
 * allowed to block!
 *
 * @param context Will contain the pointer which was passed during init
 * @param buf Buffer with data to be written to some hardware
 * @param count Size of the given buffer
 */
typedef void (*NDLComExternalInterfaceWriteEscapedBytes)(void *context,
                                                         const void *buf,
                                                         const size_t count);
/**
 * @brief Callback to "read" escaped data from somewhere into the bridge.
 *
 * Note that this function will be in the critical path of the
 * "ndlcomBridgeProcess()" function. It should complete fast and is never
 * allowed to block!
 *
 * @param context Will contain the pointer which was passed during init
 * @param buf Buffer to read into. Store fresh data from some hardware
 * @param count Size of the given buffer
 */
typedef size_t (*NDLComExternalInterfaceReadEscapedBytes)(void *context,
                                                          void *buf,
                                                          const size_t count);

/**
 * @brief Datastructure to describe an ExternalInterface
 *
 * Keeps function pointers to the read and write callbacks and stores an
 * additional "context", which can be used to store a pointer to another
 * datastructure -- a wrapping cpp class for example.
 *
 * A "flags"-field is present to enable special behaviour. And the finally the
 * mandatory NDLComParser itself, which is needed to detect packets in the
 * incoming data-stream.
 */
struct NDLComExternalInterface {
    /** The NDLComBridge where this interface is connected to.  */
    struct NDLComBridge *bridge;
    /** the context will be provided in the read/write functions */
    void *context;
    /** influences the behaviour of the external interface */
    uint8_t flags;
    /** every interface needs its parser */
    struct NDLComParser parser;
    /** callback to read data from the interface */
    NDLComExternalInterfaceReadEscapedBytes read;
    /** callback to write data into the interface */
    NDLComExternalInterfaceWriteEscapedBytes write;
    /** this struct is stored in a linked list as part "NDLComBridge" */
    struct list_head list;
};

/**
 * @brief Initialization of ExternalInterface and filling in default
 *
 * @param externalInterface pointer to the struct to initialize
 * @param write function pointer to the write function
 * @param read function pointer to the write function
 * @param flags flags to be used during initialization
 * @param context Additional pointer to store "private" information to be
 *                passed during calling the callback
 */
void
ndlcomExternalInterfaceInit(struct NDLComExternalInterface *externalInterface,
                            NDLComExternalInterfaceWriteEscapedBytes write,
                            NDLComExternalInterfaceReadEscapedBytes read,
                            const uint8_t flags, void *context);

/**
 * @brief Returns the number of mismatched CRC events
 *
 * The number of CRC fails can be an indicator for the link-quality or even a
 * wrong baudrate.
 *
 * Not sure if this function is overwrapping the actual structs, as it just
 * accesses public information in the "struct NDLComParser", part of the struct
 * given.
 */
uint32_t ndlcomExternalInterfaceGetCrcFails(
    const struct NDLComExternalInterface *externalInterface);

/**
 * @brief Influence flags defined for the interface at runtime.
 *
 * Will set the given flag pattern. Be carefull not to overwrite anything.
 *
 * @param externalInterface The pointer to work on
 * @param flags The pattern to set.
 */
void ndlcomExternalInterfaceSetFlags(
    struct NDLComExternalInterface *externalInterface,
    const uint8_t flags);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_EXTERNAL_INTERFACE_H*/
