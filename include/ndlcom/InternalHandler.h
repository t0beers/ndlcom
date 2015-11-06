#ifndef NDLCOM_INTERNAL_HANDLER_H
#define NDLCOM_INTERNAL_HANDLER_H

#include "ndlcom/Types.h"
#include "ndlcom/list.h"
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * no flags in the moment
 */
#define NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT 0x00

/**
 * callback-function for handling internal packets. note that you have to
 * _know_ was is passed in the "context" pointer.
 */
typedef void (*NDLComInternalHandlerFkt)(void *context,
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
    NDLComInternalHandlerFkt handler;
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
                               NDLComInternalHandlerFkt handler,
                               const uint8_t flags, void *context);

void
ndlcomInternalHandlerSetFlags(struct NDLComInternalHandler *internalHandler,
                              const uint8_t flags);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_INTERNAL_HANDLER_H*/
