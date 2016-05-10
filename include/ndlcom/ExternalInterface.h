#ifndef NDLCOM_EXTERNAL_INTERFACE_H
#define NDLCOM_EXTERNAL_INTERFACE_H

#include "ndlcom/Parser.h"
#include "ndlcom/list.h"

/**
 * NOTE: this header describes the C-interface of the "ExternalInterface"
 */

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * this flag will cause the added interface to be a "debug mirror". it's
 * purpose is to treat incoming messages as if they originate from the internal
 * side, for write out _all_ messages passing through the bridge, additionally
 * to the normal "routing".
 */
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR 0x01
/** the default value for new interfaces: do nothing special */
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT 0x00

/**
 * Callback function to "write" escaped data from the bridge to somewhere.
 *
 * Note that this function will be in the critical path of the
 * "ndlcomBridgeProcess()" function. It should complete fast and is never
 * allowed to block!
 */
typedef void (*NDLComExternalInterfaceWriteEscapedBytes)(void *context,
                                                         const void *buf,
                                                         const size_t count);
/**
 * Callback function to "read" escaped data from somewhere into the bridge.
 *
 * Note that this function will be in the critical path of the
 * "ndlcomBridgeProcess()" function. It should complete fast and is never
 * allowed to block!
 */
typedef size_t (*NDLComExternalInterfaceReadEscapedBytes)(void *context,
                                                          void *buf,
                                                          const size_t count);

/**
 * struct to describe an ExternalInterface
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
 * @brief initialization of "struct NDLComExternalInterface" and filling in default
 *
 * @param externalInterface pointer to the struct to initialize
 * @param write function pointer to the write function
 * @param read function pointer to the write function
 * @param flags flags to be used during initialization
 * @param context additional pointer to store "private" information for context
 *        during calling the callback
 */
void
ndlcomExternalInterfaceInit(struct NDLComExternalInterface *externalInterface,
                            NDLComExternalInterfaceWriteEscapedBytes write,
                            NDLComExternalInterfaceReadEscapedBytes read,
                            const uint8_t flags, void *context);

/**
 * obtain nice information from an interface. not sure if this function is
 * overwrapping the actual structs, as it just accesses information in the
 * "struct NDLComParser, part of the struct given.
 */
uint32_t ndlcomExternalInterfaceGetCrcFails(
    const struct NDLComExternalInterface *externalInterface);

/**
 * influence flags defined for the interface at runtime.
 *
 * changing the flags during runtime _should_ be possible.
 */
void ndlcomExternalInterfaceSetFlags(
    struct NDLComExternalInterface *externalInterface,
    const uint8_t flags);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_EXTERNAL_INTERFACE_H*/
