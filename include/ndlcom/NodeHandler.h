#ifndef NDLCOM_NODE_HANDLER_H
#define NDLCOM_NODE_HANDLER_H

#include "ndlcom/Types.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** default, nothing special */
#define NDLCOM_NODE_HANDLER_FLAGS_DEFAULT 0x00

/**
 * Callback-function for internal handling of received messages. Note that you
 * have to _know_ was is passed in the "context" pointer.
 *
 * @param context Arbitrary pointer given to "ndlcomNodeHandlerInit"
 * @param header The header of the message
 * @param payload Payload of the message. See "header->mDataLen" for the size
 * @param origin The ExternalInterface where this message came from. Can be the
 *               null if the message is originating from the internal side
 */
typedef void (*NDLComNodeHandlerFkt)(void *context,
                                     const struct NDLComHeader *header,
                                     const void *payload, const void *origin);

struct NDLComNodeHandler {
    /**
     * The NDLComNode where this handler is connected to. Keep in mind that
     * NodeHandlers can also be connected to a NDLComBridge. This case is
     * not reflected in the API. */
    struct NDLComNode *node;
    /**
     * Three use cases for the "context" pointer:
     * - reply to a message by sending messages to the outside --
     *   NDLComBridge pointer as context
     * - print all received messages to stdout -- no context
     * - count missEvents, do statistics... -- use the objects own "this" as
     *   context, where additional data can be stored
     *
     * What is not so good: you have to know what you do...
     */
    void *context;
    /** Influences the behaviour of the NodeHandler */
    uint8_t flags;
    /** function called to handle decoded packets for handling */
    NDLComNodeHandlerFkt handler;
    /** doubly linked list as part of "NDLComBridge" or "NDLComNode" */
    struct list_head list;
};

/**
 * @brief Initialize the data structures belonging to an NodeHandler
 *
 * Will not connect it to any other object, will just prepare everything.
 *
 * @param nodeHandler The object to initialize
 * @param handler Callback function to use
 * @param flags Initial flags
 * @param context Arbitrary pointer
 */
void ndlcomNodeHandlerInit(struct NDLComNodeHandler *nodeHandler,
                           NDLComNodeHandlerFkt handler, const uint8_t flags,
                           void *context);

/**
 * @brief Setting optional flags influencing behaviour of the NodeHandler
 *
 * Will set the given flag pattern. Be careful not to overwrite anything.
 *
 * @param nodeHandler Pointer to work on
 * @param flags The pattern to set.
 */
void
ndlcomNodeHandlerSetFlags(struct NDLComNodeHandler *nodeHandler,
                              const uint8_t flags);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_NODE_HANDLER_H*/
