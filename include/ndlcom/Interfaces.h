#ifndef NDLCOM_INTERFACES_H
#define NDLCOM_INTERFACES_H

#include "ndlcom/Parser.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*NDLComHandlerFkt)(void *context,
                                 const struct NDLComHeader *header,
                                 const void *payload);

struct NDLComInternalHandler {
    /**
     * usecases for the "context" pointer:
     * - reply to a message by sending messages to the outside -- bridge-pointer as context
     * - print received messages to stdout -- no context
     * - count missEvents, do statistics... -- own "this" as context
     */
    void *context;
    /** called to handle all decoded packets */
    NDLComHandlerFkt handler;
    /** poor-mans linked list: we have probably more than this single interface */
    struct NDLComInternalHandler *next;
};

/**
 * @brief
 * @param internal
 * @param handler
 * @param context
 */
void ndlcomInternalHandlerInit(struct NDLComInternalHandler *internal,
                               NDLComHandlerFkt handler, void *context);

typedef void (*NDLComWriteEscapedBytes)(void *context, const void *buf,
                                        const size_t count);
typedef size_t (*NDLComReadEscapedBytes)(void *context, void *buf,
                                         const size_t count);

struct NDLComExternalInterface {
    /* we be provided in the read/write functions */
    void *context;
    /* every interface needs its parser */
    struct NDLComParser parser;
    NDLComReadEscapedBytes read;
    NDLComWriteEscapedBytes write;
    /* linked-list... */
    struct NDLComExternalInterface *next;
};

/**
 * @brief
 * @param external
 * @param write
 * @param read
 * @param context
 */
void ndlcomExternalInterfaceInit(struct NDLComExternalInterface *external,
                                 NDLComWriteEscapedBytes write,
                                 NDLComReadEscapedBytes read, void *context);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_INTERFACES_H*/
