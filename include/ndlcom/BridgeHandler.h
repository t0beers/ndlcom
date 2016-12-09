#ifndef NDLCOM_BRIDGE_HANDLER_H
#define NDLCOM_BRIDGE_HANDLER_H

#include "ndlcom/Types.h"
#include "ndlcom/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** default, nothing special */
#define NDLCOM_BRIDGE_HANDLER_FLAGS_DEFAULT 0x00
/**
 * @brief Do not handle messages originating from NDLComBridgeHandler or
 *        NDLComNodeHandler
 *
 * If this flag is used, this NDLComBridgeHandler will not see messages sent
 * from the internal side of the NDLComBridge, eg which were created by calling
 * "ndlcomBridgeSendRaw()" or "ndlcomNodeSend()".
 *
 * This is useful to prevent loops by responding to a respond of a respond of an
 * internal message. Additionally, handlers which are not interested in these
 * message can skip testing the senderId of messages. this is actually only
 * useful for internal handlers connected directly to a bridge...
 */
#define NDLCOM_BRIDGE_HANDLER_FLAGS_NO_MESSAGES_FROM_INTERNAL 0x01

/**
 * Callback-function for internal handling of received messages. Note that you
 * have to _know_ was is passed in the "context" pointer.
 *
 * @param context Arbitrary pointer given to "ndlcomBridgeHandlerInit"
 * @param header The header of the message
 * @param payload The payload of the message. See "header->mDataLen" for the
 *        size.
 * @param origin The ExternalInterface where this message came from. Can be the
 *               "NDLComBridge" pointer if it is originating from the internal
 *               side. This might change.
 */
typedef void (*NDLComBridgeHandlerFkt)(void *context,
                                       const struct NDLComHeader *header,
                                       const void *payload, const void *origin);

struct NDLComBridgeHandler {
    /**
     * The NDLComBridge where this handler is connected to
     */
    struct NDLComBridge *bridge;
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
    /** Influences the behaviour of the BridgeHandler */
    uint8_t flags;
    /** function called to handle decoded packets for handling */
    NDLComBridgeHandlerFkt handler;
    /** doubly linked list to be part of NDLComBridge */
    struct list_head list;
};

/**
 * @brief Initialize the data structures belonging to an BridgeHandler
 *
 * @param bridgeHandler The object to initialize
 * @param handler Callback function to use
 * @param flags Initial flags
 * @param context Arbitrary pointer
 */
void ndlcomBridgeHandlerInit(struct NDLComBridgeHandler *bridgeHandler,
                             NDLComBridgeHandlerFkt handler,
                             const uint8_t flags, void *context);

/**
 * @brief Setting optional flags influencing behaviour of the BridgeHandler
 *
 * Will set the given flag pattern. Be careful not to overwrite anything.
 *
 * @param internalHandler Pointer to work on
 * @param flags The pattern to set.
 */
void ndlcomBridgeHandlerSetFlags(struct NDLComBridgeHandler *bridgeHandler,
                                 const uint8_t flags);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_BRIDGE_HANDLER_H*/
