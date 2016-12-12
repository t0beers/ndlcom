/**
 * @file tools/minimalExample.cpp
 *
 * This file demonstrates a very minimal use-case of a NDLComBridge, a
 * NDLComNode and some convenience classes. It opens an udp-interface, listens
 * to messages of a certain pattern and regularly transmits an empty dummy
 * message to another device.
 *
 * No command line parsing, no fancy implementations. This file only covers the
 * bare minimum needed.
 */
#include "ndlcom/Bridge.hpp"
#include "ndlcom/Node.hpp"
/* provides the base-class for the ExampleHandler */
#include "ndlcom/NodeHandlerBase.hpp"
/* convenience function to create "ExternalInterface" */
#include "ndlcom/ExternalInterfaceParseUri.hpp"

/* some standard headers */
#include <unistd.h>
#include <csignal>

bool stopMainLoop = false;
void signal_handler(int signal) { stopMainLoop = true; }

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
class ExampleHandler final : public ndlcom::NodeHandlerBase {
  private:
    /**
     * This byte will be compared to the first byte of every observed
     * message. If they match, information is printed. */
    uint8_t firstByteToListen;

  public:
    ExampleHandler(struct NDLComNode &node, uint8_t _firstByteToListen)
        : NodeHandlerBase(node), firstByteToListen(_firstByteToListen) {
        // this label is used for proper status-displays
        label = "ExampleHandler";
        // all deriving class still have to register, after their internal data
        // was set up in the ctor
        ndlcomNodeRegisterNodeHandler(&node, &internal);
    }

    /* This function will be called when the NDLComNode received a message
     * directed at its own deviceId. */
    void handle(const struct NDLComHeader *header, const void *payload,
                const void *origin) {

        /* at first do some printing */
        printf("message from 0x%02x with %i bytes",
               header->mSenderId, (int)header->mDataLen);
        if (origin) {
            printf(" from %s\n",
                   static_cast<const class ndlcom::ExternalInterfaceBase *>(
                       origin)->label.c_str());
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

    /* The actual NDLComBridge, responsible to carry data between registered
     * "ExternalInterface" and "BridgeHandler". */
    class ndlcom::Bridge bridge;

    /* And a NDLComNode, which will register itself as an "BridgeHandler"
     * at the bridge. Will filter messages for the given deviceId. */
    std::shared_ptr<class ndlcom::Node> node = bridge.createNode(ownDeviceId);

    /* The actual message handler. Will register itself on the given NDLComNode
     * during
     * initialization in the base-constructor and listens to messages which have
     * the first byte as given in the ctor. */
    std::shared_ptr<class example::ExampleHandler> handler =
        node->createNodeHandler<class example::ExampleHandler>(
            someFirstByteToListen);

    /* Little helper function, which parses the given string to create and
     * register "ExternalInterface" at the given NDLComBridge. Status messages
     * and errors will be printed to std::cerr. A pointer to the new interface
     * is returned, but as no removing of registered interfaces is needed the
     * pointer is not stored. */
    bridge.createInterface(connectionUdp);
    /* also add the second interface. note that the "createInterface()" returns
     * a shared pointer, but keeps an internaly copy to prevent it going out of
     * scope. */
    {
        std::shared_ptr<class ndlcom::ExternalInterfaceBase> inter =
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
            node->send(ownDeviceId, 0, 0);
            /* the handler has a send-method as well: */
            //handler->send(ownDeviceId, 0, 0);

            printf("node sent message at myself\n");
        }
    }

    return 0;
}
