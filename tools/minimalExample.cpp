/**
 * @file tools/minimalExample.cpp
 *
 * This file demonstrates a very minimal use-case of a ndlcom::Bridge, a
 * ndlcom::Node and some convenience classes. It opens an udp-interface,
 * listens to messages of a certain pattern and regularly transmits an empty
 * dummy message to another device.
 *
 * No command line parsing, no fancy implementations. This file only covers the
 * bare minimum needed.
 */
#include "ndlcom/Bridge.hpp"
#include "ndlcom/Node.hpp"
#include "ndlcom/NodeHandler.hpp"
/* provides the base-class for the ExampleHandler */
#include "ndlcom/InternalHandler.hpp"
/* and the base-class for another interface */
#include "ndlcom/ExternalInterfaceBase.hpp"

/* some standard headers */
#include <unistd.h>
#include <csignal>

bool stopMainLoop = false;
void signal_handler(int signal) { stopMainLoop = true; }

/* The actual NDLComBridge, responsible to carry data between any registered
 * "ndlcom::ExternalInterface", "ndlcom::BridgeHandler" and possibly
 * "ndlcom::NodeHandler" */
class ndlcom::Bridge bridge;

/* Just an example */
uint8_t ownDeviceId = 82;
/* This will be used to filter messages in the "handle()" function of the
 * receiver class */
uint8_t someFirstByteToListen = 0;
/* string for a convenience helper function. is used to create a local
 * udp-interface. listens on port 34000 and writes to port 34001. you can
 * connect with any ndlcom-client to this port */
std::string connectionUdp = "udp://localhost:34000:34001";
/* additionally, a pseudo terminal: */
std::string connectionPty = "pty://ptytest";

namespace example {
/**
 * Example of a subclassed NodeHandlerBase
 *
 * Listening to the deviceId of the corresponding NDLComNode and printing
 * message-information if a certain first-byte is seen in a message.
 *
 * using the "final" keyword is not strictly needed, but good practice to
 * prevent bugs from unwanted inheritance
 */
class ExampleHandler final : public ndlcom::NodeHandler {
  private:
    /**
     * This byte will be compared to the first byte of every observed
     * message. If they match, information is printed. */
    uint8_t firstByteToListen;

  public:
    ExampleHandler(struct NDLComNode &node, uint8_t _firstByteToListen)
        : NodeHandler(node, "ExampleHandler"),
          firstByteToListen(_firstByteToListen) {}

    /* This function will be called when the NDLComNode received a message
     * directed at its own deviceId. */
    void handle(const struct NDLComHeader *header, const void *payload,
                const struct NDLComExternalInterface *origin) {

        /* at first do some printing */
        printf("message from 0x%02x with %i bytes",
               header->mSenderId, (int)header->mDataLen);
        if (origin) {
            printf(" from %s\n",
                   bridge.getInterfaceByOrigin(origin).lock()->label.c_str());
        } else {
            printf(" from internal\n");
        }
        /* NOTE: The "payload" pointer points to a data-array with the size
         * given in header->mDataLen. */
        if (header->mDataLen > 0) {
            /* Casting of the payload-pointer for easier access. */
            const char *r = (const char *)payload;
            /* For this specific example: Check the first byte of the for a
             * certain byte, ignore all other packets.  */
            if (r[0] == firstByteToListen) {
                printf("specially marked message with first byte: 0x%02x\n",
                       (int)firstByteToListen);
            }
        }
    };
};
}

/* the actual main */
int main(int argc, char *argv[]) {

    /* And a ndlcom::Node, which will register itself as a "BridgeHandler" at
     * the bridge to provide handling for node-specific messages: Will filter
     * messages by the given deviceId (and broadcasts). */
    std::weak_ptr<class ndlcom::Node> node =
        bridge.createNode<class ndlcom::Node>(ownDeviceId);

    /* The actual message handler. Will register itself on the given NDLComNode
     * during
     * initialization in the base-constructor and listens to messages which have
     * the first byte as given in the ctor. */
    std::weak_ptr<class example::ExampleHandler> handler =
        node.lock()->createNodeHandler<class example::ExampleHandler>(
            someFirstByteToListen);

    /* Little helper function, which parses the given string to create and
     * register "ExternalInterface" at the given ndlcom::Bridge. Status messages
     * and errors will be printed to std::cerr. A pointer to the new interface
     * is returned, but as no removing of registered interfaces is needed the
     * pointer is not stored. */
    bridge.createInterface(connectionUdp);
    /* also add the second interface. note that the "createInterface()" returns
     * a shared pointer, but keeps an internaly copy to prevent it going out of
     * scope. to close an interface use "bridge.destroyExternalInterface". */
    {
        std::weak_ptr<class ndlcom::ExternalInterfaceBase> inter =
            bridge.createInterface(connectionPty);
    }

    std::signal(SIGINT, signal_handler);

    bridge.printStatus();

    int cnt = 0;
    /* Runs forever, quit with ctrl-c */
    while (!stopMainLoop) {
        cnt++;
        printf("entering loop %i\n", cnt);

        /* Repeat every 100ms, or in other words: 10Hz */
        usleep(100000);
        /* Check for new data. This will trigger the callback in the
         * ExampleHandler on a received message.  */
        bridge.process();

        /* Just to show that we can: sometimes send a message. */
        if (!((cnt) % 10)) {

            /* This will be our toy-message: Just a bunch of zeros. */
            char array[5] = {0};
            /* And send it away. */
            struct NDLComHeader hdr = {NDLCOM_ADDR_BROADCAST, ownDeviceId, 0,
                                       sizeof(array)};
            bridge.sendMessageRaw(&hdr, &array);
            printf("bridge sent raw broadcast message\n");
        }

        if (!((cnt) % 5)) {
            node.lock()->send(ownDeviceId, 0, 0);
            /* the handler has a send-method as well: */
            //handler->send(ownDeviceId, 0, 0);

            printf("node sent message at myself\n");
        }
    }

    return 0;
}
