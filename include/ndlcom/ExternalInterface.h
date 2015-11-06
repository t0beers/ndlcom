#ifndef NDLCOM_EXTERNAL_INTERFACE_H
#define NDLCOM_EXTERNAL_INTERFACE_H

#include "ndlcom/Parser.h"
#include "ndlcom/list.h"

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
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT 0x00

/**
 * callback function "write" of escaped data. note that this function will be
 * in the critical path of the "ndlcomBridgeProcess()" function. is should
 * complete fast and never block.
 */
typedef void (*NDLComExternalInterfaceWriteEscapedBytes)(void *context,
                                                         const void *buf,
                                                         const size_t count);
/**
 * callback function "read" of escaped data. note that this function will be
 * in the critical path of the "ndlcomBridgeProcess()" function. is should
 * complete fast and never block.
 */
typedef size_t (*NDLComExternalInterfaceReadEscapedBytes)(void *context,
                                                          void *buf,
                                                          const size_t count);

struct NDLComExternalInterface {
    /* the context will be provided in the read/write functions */
    void *context;
    /** influences the behaviour of the external interface */
    uint8_t flags;
    /* every interface needs its parser */
    struct NDLComParser parser;
    NDLComExternalInterfaceReadEscapedBytes read;
    NDLComExternalInterfaceWriteEscapedBytes write;
    /** stored  inside a doubly linked list as part of a "NDLComBridge" */
    struct list_head list;
};

/**
 * @brief
 * @param external
 * @param write
 * @param read
 * @param context
 */
void
ndlcomExternalInterfaceInit(struct NDLComExternalInterface *externalInterface,
                            NDLComExternalInterfaceWriteEscapedBytes write,
                            NDLComExternalInterfaceReadEscapedBytes read,
                            const uint8_t flags, void *context);

uint32_t ndlcomExternalInterfaceGetCrcFails(
    const struct NDLComExternalInterface *externalInterface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_EXTERNAL_INTERFACE_H*/
