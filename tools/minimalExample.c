/**
 * @file tools/minimalExample.c
 *
 * This file demonstrates a very minimal use-case of a "struct NDLComBridge", a
 * "struct NDLComNode" and handler functions.
 *
 * This example will only use c-language stuff
 */
#include "ndlcom/Node.h"
#include "ndlcom/Bridge.h"
#include "ndlcom/ExternalInterface.h"

/* some standard headers */
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

/* two ids: */
NDLComId idA = 82;
NDLComId idB = 83;

/* a metadata struct: */
struct MetaData {
    int a;
    int b;
};
/* and two instances of it */
struct MetaData ctxA = {1,2};
struct MetaData ctxB = {3,4};

/*
 * a dummy-read function. as this example does not implement anything useful,
 * we cannot really read bytes from our emulated interface
 *
 * normally, these are the places where data is read from a uart or written
 * into it.
 *
 * both functions should operate as fast as possible (non-blocking), they are
 * in the hotpath of the "process()" function. if buffers are full: bad luck,
 * just waste the bytes...
 */
size_t readFun(void *context, void *buf, const size_t count) {
    // so always say "no, we cannot read any bytes"
    return 0;
}
void writeFun(void *context, const void *buf, const size_t count) {
    // print what would happen
    printf("writeFun would write %lu bytes\n", count);
}

/*
 * in this example, the handler listens to messages passing through the
 * connected node. it expects a "struct MetaData" as context pointer.  another
 * possibility would be to use the "struct Node" itself. this would allow to
 * send replies directly from the "struct NodeHandler".
 */
void someHandler(void *context, const struct NDLComHeader *header,
                 const void *payload, const void *origin) {
    // obtain our context back
    struct MetaData *pCtx = (struct MetaData *)context;
    // and print it. keep in mind that nodes can see their own broadcast
    // messages...
    printf("node got message from %i: ctx.a = %i -- ctx.b = %i\n",
           (int)header->mSenderId, pCtx->a, pCtx->b);
}

// tooling function
int checkAvailableBytesOnStdin() {
    fd_set s_rd;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&s_rd);
    FD_SET(fileno(stdin), &s_rd);
    int r = select(fileno(stdin) + 1, &s_rd, NULL, NULL, &tv);
    return r;
}

int main(int argc, char *argv[]) {

    /* NDLComBridge, responsible to carry data between registered
     * "ExternalInterface" and "BridgeHandler". */
    struct NDLComBridge bridge;
    ndlcomBridgeInit(&bridge);

    /* to demonstrate the ExternalInterface: */
    struct NDLComExternalInterface inter;
    ndlcomExternalInterfaceInit(&inter, writeFun, readFun,
                                NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT, 0);
    ndlcomBridgeRegisterExternalInterface(&bridge, &inter);

    /* NDLComNode, which will filter messages for the given deviceId */
    struct NDLComNode nodeA;
    ndlcomNodeInit(&nodeA, idA);
    ndlcomNodeRegister(&nodeA, &bridge);
    /** for this specific example we'll need two: */
    struct NDLComNode nodeB;
    ndlcomNodeInit(&nodeB, idB);
    ndlcomNodeRegister(&nodeB, &bridge);

    /** both nodes will get a "NodeHandler" which is called for every message
     * directed at the nodes device id */
    struct NDLComNodeHandler nodeHandlerA;
    ndlcomNodeHandlerInit(&nodeHandlerA, someHandler,
                          NDLCOM_NODE_HANDLER_FLAGS_DEFAULT, &ctxA);
    ndlcomNodeRegisterNodeHandler(&nodeA, &nodeHandlerA);
    /** and the second one: */
    struct NDLComNodeHandler nodeHandlerB;
    ndlcomNodeHandlerInit(&nodeHandlerB, someHandler,
                          NDLCOM_NODE_HANDLER_FLAGS_DEFAULT, &ctxB);
    ndlcomNodeRegisterNodeHandler(&nodeB, &nodeHandlerB);

    printf("setup done, press any key to exit\n");

    /* runs until key is pressed */
    int cnt = 0;
    while (!checkAvailableBytesOnStdin()) {
        printf("entering loop %i\n", cnt);

        /* Repeat every 100ms, or in other words: 10Hz */
        usleep(100000);
        /* Check for new data. This will trigger the callback in the
         * ExampleHandler on a received message.  */
        ndlcomBridgeProcess(&bridge);

        /* Just to show that we can: sometimes send a message. */
        if (!((++cnt) % 5)) {

            /* sending packages without payload, just for demo */
            ndlcomNodeSend(&nodeA, NDLCOM_ADDR_BROADCAST, 0, 0);
            ndlcomNodeSend(&nodeB, idA, 0, 0);
        }
    }

    printf("cleaning up\n");

    // very important: proper cleanup!
    ndlcomNodeDeregisterNodeHandler(&nodeA, &nodeHandlerA);
    ndlcomNodeDeregisterNodeHandler(&nodeB, &nodeHandlerB);
    ndlcomNodeDeregister(&nodeA);
    ndlcomNodeDeregister(&nodeB);

    return 0;
}
