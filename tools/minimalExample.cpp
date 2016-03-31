/**
 * @file tools/minimalExample.cpp
 *
 * This file demonstrates a very minimal use-case of a NDLComBridge, a
 * NDLComNode and some convenience classes. It opens an udp-interface, listens
 * to messages of a certain pattern and regularly transmits an empty dummy
 * message to another device.
 *
 * No command line parsing, not fancy implementations. This file only covers the
 * bare minimum needed.
 */

#include "ndlcom/Node.h"
#include "ndlcom/Bridge.h"
/* provides the base-class for the ExampleHandler */
#include "ndlcom/InternalHandler.hpp"
/* convenience function to create "ExternalInterface" */
#include "ndlcom/ExternalInterfaceParseUri.hpp"

/* some standard headers */
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/* just an example */
uint8_t ownDeviceId = 82;
/* this will be used to filter messages in the "handle()" function of the
 * receiver class */
uint8_t firstByteToListen = 0;
/* where messages are sent to in "sendMessage()" */
uint8_t destinationDeviceId = 1;
/* string for a convenience helper function, will create a local udp-interface.
 * listens on port 34000 and writes to port 34001. */
std::string connectionUri = "udp://localhost:34000:34001";


/* Subclassed base-handler, listening to the deviceId of the corresponding
 * NDLComNode */
class ExampleHandler : public ndlcom::NodeHandlerBase {
  public:
    ExampleHandler(struct NDLComNode &_node) : NodeHandlerBase(_node) {}

    /* This function will be called when the NDLComNode received a message
     * directed at its own deviceId. */
    void handle(const struct NDLComHeader *header, const void *payload) {
        /* Casting of the payload-pointer for easier access. */
        const char *r = (const char *)payload;
        /* For this specific example: Check the first byte of the for a certain
         * byte, ignore all other packets.  */
        if (r[0] != firstByteToListen) {
            return;
        }
        /* Just print for now. But you can do anything. Really. */
        printf("got a message from receiver 0x%02x, where the first byte is %i",
               header->mReceiverId, (int)firstByteToListen);
    };
};


/* Helper function, demonstrating how a message can be sent. */
void sendMessage(struct NDLComNode *node) {

    /* This will be our toy-message: Just a bunch of zeros. */
    char array[15];
    memset(array, 0, sizeof(array));
    /* And send it away. */
    ndlcomNodeSend(node, destinationDeviceId, &array, sizeof(array));
}


/* the actual main */
int main(int argc, char *argv[]) {

    /* The actual NDLComBridge, responsible to carry data between registered
     * "ExternalInterface" and "InternalInterface". */
    struct NDLComBridge bridge;
    ndlcomBridgeInit(&bridge);

    /* And a NDLComNode, which will register itself as an "InternalInterface"
     * in the bridge. Will handle messages for the given deviceId. */
    struct NDLComNode node;
    ndlcomNodeInit(&node, &bridge, ownDeviceId);

    /* The actual handler. Will register itself on the given NDLComNode during
     * initialization in the base-constructor. */
    class ExampleHandler handler(node);

    /* Little helper function, which parses the given string to create and
     * register "ExternalInterfaces" at the given NDLComBridge. Status messages
     * and errors will be printed to std::cerr. A pointer to the new interface
     * is returned, but as no removing of registered interfaces is needed the
     * pointer is not stored. */
    ndlcom::ParseUriAndCreateExternalInterface(std::cerr, bridge,
                                               connectionUri);

    int cnt = 0;
    /* Runs forever, quit with ctrl-c */
    while (1) {
        printf("entering new loop\n");

        /* Repeat every 100ms, or in other words: 10Hz */
        usleep(100000);
        /* Check for new data. This will trigger the callback in the */
        /* ExampleHandler on a received message. */
        ndlcomBridgeProcess(&bridge);

        /* Just to show that we can: sometimes send a message. */
        if (!((++cnt) % 20)) {
            sendMessage(&node);
            printf("did send a toy-message myself\n");
        }
    }

    return 0;
}
