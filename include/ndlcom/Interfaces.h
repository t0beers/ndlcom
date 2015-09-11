#ifndef NDLCOM_INTERFACES_H
#define NDLCOM_INTERFACES_H

#include "ndlcom/Parser.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * no flags in the moment
 */
#define NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT 0x00

/**
 * this flag will cause the added interface to be a "debug mirror". it's
 * purpose is to treat incoming messages as if they originate from the internal
 * side, for write out _all_ messages passing through the bridge, additionally
 * to the normal "routing".
 */
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEBUG_MIRROR 0x01
#define NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT 0x00

/**
 * callback-function for handling internal packets. note that you have to
 * _know_ was is passed in the "context" pointer.
 */
typedef void (*NDLComHandlerFkt)(void *context,
                                 const struct NDLComHeader *header,
                                 const void *payload);

struct NDLComInternalHandler {
    /**
     * three usecases for the "context" pointer:
     * - reply to a message by sending messages to the outside --
     *   bridge-pointer as context
     * - print all received messages to stdout -- no context
     * - count missEvents, do statistics... -- own "this" as context, where
     *   additional data can be stored
     *
     * not so good: you have to know what you do...
     */
    void *context;
    /** influences the behaviour of the internal handler */
    uint8_t flags;
    /** function called to handle all decoded packets */
    NDLComHandlerFkt handler;
    /** stored  inside a doubly linked list as part of a "NDLComBridge" */
    struct list_head list;
};

/**
 * @brief
 * @param internal
 * @param handler
 * @param context
 */
void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internalHandler,
                               NDLComHandlerFkt handler, const uint8_t flags,
                               void *context);

/**
 * callback function "write" of escaped data. note that this function will be
 * in the critical path of the "ndlcomBridgeProcess()" function. is should
 * complete fast and never block.
 */
typedef void (*NDLComWriteEscapedBytes)(void *context, const void *buf,
                                        const size_t count);
/**
 * callback function "read" of escaped data. note that this function will be
 * in the critical path of the "ndlcomBridgeProcess()" function. is should
 * complete fast and never block.
 */
typedef size_t (*NDLComReadEscapedBytes)(void *context, void *buf,
                                         const size_t count);

struct NDLComExternalInterface {
    /* the context will be provided in the read/write functions */
    void *context;
    /** influences the behaviour of the external interface */
    uint8_t flags;
    /* every interface needs its parser */
    struct NDLComParser parser;
    NDLComReadEscapedBytes read;
    NDLComWriteEscapedBytes write;
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
void ndlcomExternalInterfaceInit(
    struct NDLComExternalInterface *externalInterface,
    NDLComWriteEscapedBytes write, NDLComReadEscapedBytes read,
    const uint8_t flags, void *context);

uint32_t NDLComExternalInterfaceGetCrcFails(
    const NDLComExternalInterface *externalInterface);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_INTERFACES_H*/
