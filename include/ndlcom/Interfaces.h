#ifndef NDLCOM_INTERFACES_H
#define NDLCOM_INTERFACES_H

#include "ndlcom/Parser.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*NDLComHandlerFkt)(void *context,
                                 const struct NDLComHeader *header,
                                 const void *payload);

struct NDLComInternalHandler {
    /**
     * three usecases for the "context" pointer:
     * - reply to a message by sending messages to the outside --
     *   bridge-pointer as context
     * - print received messages to stdout -- no context
     * - count missEvents, do statistics... -- own "this" as context
     *
     * not so godd: you have to know what you do...
     */
    void *context;
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
                               NDLComHandlerFkt handler, void *context);


typedef void (*NDLComWriteEscapedBytes)(void *context, const void *buf,
                                        const size_t count);
typedef size_t (*NDLComReadEscapedBytes)(void *context, void *buf,
                                         const size_t count);

struct NDLComExternalInterface {
    /* the context will be provided in the read/write functions */
    void *context;
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
    NDLComWriteEscapedBytes write, NDLComReadEscapedBytes read, void *context);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_INTERFACES_H*/
