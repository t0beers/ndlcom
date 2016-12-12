#ifndef NDLCOM_BRIDGE_HPP
#define NDLCOM_BRIDGE_HPP

#include "ndlcom/Node.hpp"

#include "ndlcom/ExternalInterface.hpp"
#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/NodeHandler.hpp"

#include <sstream>
#include <vector>
#include <map>
#include <memory>

namespace ndlcom {

class Bridge final {
  public:
    Bridge();
    ~Bridge();

    /** printer functions. move outside of this class? */
    void printRoutingTable(std::ostream &out = std::cerr);
    void printStatus(std::ostream &out = std::cerr);

    /**
     * @brief Main entry to data processing
     * @return The time it took to process
     */
    struct timespec process();

    /**
     * @brief Transmitting a new message from the bridge
     */
    void sendMessageRaw(const struct NDLComHeader *header, const void *payload);

    /**
     * using the "parseUri" string-based functions
     */
    std::shared_ptr<class ndlcom::ExternalInterfaceBase>
    createInterface(std::string uri,
                    uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

    /**
     * a generic "factory" function for ExternalInterfaces
     *
     * we keep an additional list to prevent the shared_ptr goes out of scope
     * and gets deleted.
     *
     * call like so:
     *
     *    std::shared_ptr<ndlcom::MyExternalInterface> p2 =
     *        bridge.createExternalInterface<ndlcom::MyExternalInterface();
     */
    template <class T, class... A>
    std::shared_ptr<T> createExternalInterface(A... args) {
        std::shared_ptr<T> p = std::make_shared<T>(bridge, args...);
        externalInterfaces.push_back(p);
        return p;
    }

    /**
     * similar factor for the BridgeHandler
     */
    template <class T, class... A>
    std::shared_ptr<T> createBridgeHandler(A... args) {
        std::shared_ptr<T> p = std::make_shared<T>(bridge, args...);
        bridgeHandler.push_back(p);
        return p;
    }

    /*
     * this class makes sure that there may be only one node for a given
     * id... am not sure if this makes sense...
     *
     * copy of the shared_ptr is kept inside this class
     */
    std::shared_ptr<class ndlcom::Node> createNode(const NDLComId nodeDeviceId);

    /**
     * will create a node which is listening to the given id, and maybe prints
     * received messages.
     *
     * @param nodeDeviceId the deviceId to set in the node
     * @param print if set to true, an additional NodeHandler is added to the Node
     * @return the actually created ndlcom::Node object
     */
    std::shared_ptr<class ndlcom::Node> enableOwnId(const NDLComId nodeDeviceId,
                                                    bool print = false);

    /**
     * create a ndlcom::BridgeHandler which prints every message. keeps
     * internal copy of shared_ptr
     */
    std::shared_ptr<class ndlcom::BridgeHandlerBase> enablePrintAll();
    /**
     * create a ndlcom::BridgeHandler which prints missing message. keeps
     * internal copy of shared_ptr
     */
    std::shared_ptr<class ndlcom::BridgeHandlerBase> enablePrintMiss();

  private:


    // these datastructures are needed to be able to cleanup the created
    // classes/structs in dtor, but not earlier.
    std::vector<std::shared_ptr<class ndlcom::ExternalInterfaceBase> >
        externalInterfaces;
    std::vector<std::shared_ptr<class ndlcom::BridgeHandlerBase> >
        bridgeHandler;
    std::vector<std::shared_ptr<class ndlcom::Node> > nodes;

    // harhar. by having this struct private we can more or less be sure that
    // there are not so many additional nodes we do not know about...?
    struct NDLComBridge bridge;
};
}

#endif /*NDLCOM_BRIDGE_HPP*/
