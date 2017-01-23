#ifndef NDLCOM_BRIDGE_HPP
#define NDLCOM_BRIDGE_HPP

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "ndlcom/Bridge.h"
#include "ndlcom/ExternalInterface.h"
#include "ndlcom/InternalHandler.hpp"
#include "ndlcom/Types.h"

namespace ndlcom {

class ExternalInterfaceBase;
class Node;

// tooling:
//
// split one "delim" separated string into several substrings
std::vector<std::string> splitStringIntoStrings(std::string s,
                                                const char delim);
// extract actual numbers (NDLComId) from a vector if strings
std::vector<NDLComId> convertStringToIds(std::vector<std::string> numbers,
                                         std::ostream &out);
// use uri-string like "3,6,14,66" to initialize routingtable of already
// registered interface.
void setRoutingByString(std::weak_ptr<class ndlcom::ExternalInterfaceBase> p,
                        std::string conn, std::ostream &out);

class Bridge {
  public:
    Bridge(std::ostream &_out = std::cerr);
    ~Bridge();

    /** printer functions. move outside of this class? */
    void printRoutingTable();
    void printStatus();

    /**
     * @brief Main entry to data processing
     *
     * blocks until all available bytes are processed. could therefore run
     * forever.
     */
    void process();

    /**
     * @brief Transmitting a new message from the bridge
     */
    void sendMessageRaw(const struct NDLComHeader *header, const void *payload);

    /**
     * @brief Parse string containing "uri", create ExternalInterfaceBase 
     *
     * This function is a factory function which reads a string and tries to
     * parse it into information about which kind of specialization for the
     * low-level "ExternalInterface" to create. Suitable for commandline
     * parsing. Knows about the following types:
     *
     * "udp://localhost:$SRCPORT:$DSTPORT (default: 34000 and 34001)
     * "fpga:///dev/NDLCom"
     * "serial:///dev/ttyUSB0:$BAUDRATE" (default: 921600)
     * "pipe:///tmp/testpipe"
     * "pty:///tmp/testpty"
     * "tcpclient://localhost:$PORT" (default: 2000)
     *
     * Every uri can have a trailing string specifying apriori information
     * concerning the NDLComRoutingTable for this ExternalInterface in the
     * format "&1,2,3".
     *
     * The actual regexes are implemented in the respective interface. The list
     * of which interfaces to parse is implemented inside this function. For
     * information about the specific behaviour of returned interface classes
     * see their respective header.
     *
     * This function obtains ownership of the returned pointers.
     *
     * @param uri string stating which kind of interface to create and return
     * @param flags settings for the low-level "struct ndlcomExternalInterface"
     *
     * @return nullptr on failure, otherwise weak_ptr of ExternalInterfaceBase
     */
    std::weak_ptr<class ndlcom::ExternalInterfaceBase>
    createInterface(std::string uri,
                    uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

    /**
     * Recursive template-based parsing of uri into provided list of types
     *
     * Decide wether to call the tail-end (below) or recurse again into this
     *function
     *
     * Due to being a child-class of the Bridge we can access private data
     *fields.
     *
     * This function is where the "regex_match" operation will be called, once
     * for each member of the "head+tail" types. Can be that the actual
     * structure is a bit messed up. Well, it works...
     */
    template <typename Head, typename... Tail> struct ExternalInterfaceCreator {
        /** access the first element of the list of types */
        using FirstOfTail =
            typename std::tuple_element<0, std::tuple<Tail...>>::type;
        /** and the factory function */
        static std::weak_ptr<class ndlcom::ExternalInterfaceBase>
        createInterfaceByMatch(class ndlcom::Bridge *bridge, std::string uri,
                               std::smatch &match) {
            // try to match the given uri to the regex of the Head, and
            // create interface if it matched:
            if (std::regex_match(uri, match, Head::uri)) {
                return ExternalInterfaceCreator<Head>::createInterfaceByMatch(
                    bridge, uri, match);
            }
            // if this didn't work _and_ we have only one type left in the Tail
            // additionally try to match this type to the uri:
            if (sizeof...(Tail) == 1) {
                if (std::regex_match(uri, match, FirstOfTail::uri)) {
                    return ExternalInterfaceCreator<
                        FirstOfTail>::createInterfaceByMatch(bridge, uri,
                                                             match);
                } else {
                    // and if this match did not succeed we are not able to
                    // parse the uri. this is the error-case...
                    return std::weak_ptr<ndlcom::ExternalInterfaceBase>();
                }
            }
            // recurse into the rest of the given types, the "Tail" of the
            // template arguments
            return ExternalInterfaceCreator<Tail...>::createInterfaceByMatch(
                bridge, uri, match);
        }
    };

    /**
     * The "tail end" of the recursive template class
     *
     * Here, the actual objects are created, registered and possibly their
     * routing tables initialized. The reference to the match-result from the
     * previous stage is put into the prepared dtor of the ExternalInterface
     * classes.
     */
    template <typename T> struct ExternalInterfaceCreator<T> {
        static std::weak_ptr<class ndlcom::ExternalInterfaceBase>
        createInterfaceByMatch(class ndlcom::Bridge *bridge, std::string uri,
                               std::smatch &match) {
            // reuse the bridges factory
            std::weak_ptr<class ExternalInterfaceBase> retval =
                bridge->createExternalInterface<T>(match);
            // routingtable intitialization: we just assume that each uri has
            // the routing-table stuff at the end, last match...
            setRoutingByString(retval, match[match.size() - 1].str(),
                               std::cerr);
            // Note that the bridge is the exclusive owner of the created
            // interface, we can only return only a weak pointer!
            return retval;
        }
    };

    /**
     * tries to match one of the given _types_ onto the uri-string given
     *
     * Will use a "Args...::uri" regex, so make sure the given interface
     * classes provide this. Then tries every to match+create a new
     * ExternalInterface for every type named in the variadic template.
     */
    template <typename... Args>
    std::weak_ptr<class ndlcom::ExternalInterfaceBase> createInterfaceByUri(
        std::string uri,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT) {
        std::smatch match;
        return ExternalInterfaceCreator<Args...>::createInterfaceByMatch(
            this, uri, match);
    }

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
    std::weak_ptr<T> createExternalInterface(A... args) {
        static_assert(std::is_base_of<class ndlcom::ExternalInterfaceBase, T>(),
                      "can only create classes derived from "
                      "ndlcom::ExternalInterfaceBase");
        std::shared_ptr<T> ret = std::make_shared<T>(bridge, args...);
        ret->registerHandler();
        externalInterfaces.push_back(ret);
        return ret;
    }

    /**
     * similar factor for the BridgeHandler
     */
    template <class T, class... A>
    std::weak_ptr<T> createBridgeHandler(A... args) {
        static_assert(
            std::is_base_of<ndlcom::BridgeHandlerBase, T>(),
            "can only create classes derived from ndlcom::BridgeHandlerBase");
        std::shared_ptr<T> ret = std::make_shared<T>(bridge, args...);
        ret->registerHandler();
        bridgeHandler.push_back(ret);
        return ret;
    }

    /**
     * @brief Construct and initialize a new Node object
     *
     * This function makes sure that there may be only one ndlcom::Node for a
     * given deviceId (technically, there may be more than on). It caches the
     * returned ndlcom::Node to create a new instance or return the pointer to
     * an existing one.
     *
     * Copy of the shared_ptr is kept inside this class.
     */
    template <class T, class... A> std::weak_ptr<T> createNode(A... args) {
        static_assert(std::is_base_of<ndlcom::Node, T>(),
                      "can only create classes derived from ndlcom::Node");
        std::shared_ptr<T> ret = std::make_shared<T>(bridge, args...);
        ret->registerHandler();
        nodes.push_back(ret);
        return ret;
    }

    /**
     * using the "erase-remove" ideom
     */
    template <class T> void destroyNode(std::weak_ptr<T> a) {
        std::shared_ptr<T> p = a.lock();
        p->deregisterHandler();
        nodes.erase(std::remove(nodes.begin(), nodes.end(), p), nodes.end());
    }
    template <class T> void destroyBridgeHandler(std::weak_ptr<T> a) {
        std::shared_ptr<T> p = a.lock();
        p->deregisterHandler();
        bridgeHandler.erase(
            std::remove(bridgeHandler.begin(), bridgeHandler.end(), p),
            bridgeHandler.end());
    }
    template <class T> void destroyExternalInterface(std::weak_ptr<T> a) {
        std::shared_ptr<T> p = a.lock();
        p->deregisterHandler();
        externalInterfaces.erase(std::remove(externalInterfaces.begin(),
                                             externalInterfaces.end(), p),
                                 externalInterfaces.end());
    }

    /**
     * @brief lookup existing interfaces for the given label
     *
     * @param name the label to lookup
     * @return either a weak pointer of zero
     */
    std::weak_ptr<ndlcom::ExternalInterfaceBase>
    getInterfaceByName(const std::string name) const;

    /**
     * @brief Create ndlcom::NodeHandler and maybe a ndlcom::Node if needed
     *
     * Will create a Node and optionally add a "printer" for its own id
     *
     * @param nodeDeviceId the deviceId to set in the node
     * @param print if set to true, an additional NodeHandler is added to Node
     * @return the actually created ndlcom::Node object
     */
    std::weak_ptr<class ndlcom::Node> enableOwnId(const NDLComId nodeDeviceId,
                                                  bool print = false);

    /**
     * create a ndlcom::BridgeHandler which prints every message. keeps
     * internal copy of shared_ptr
     */
    std::weak_ptr<class ndlcom::BridgeHandler> enablePrintAll();
    /**
     * create a ndlcom::BridgeHandler which prints missing message. keeps
     * internal copy of shared_ptr
     */
    std::weak_ptr<class ndlcom::BridgeHandler> enablePrintMiss();

    /**
     * @brief obtain the list of currently active interfaces
     *
     * NOTE: This function os _not_ here to stay, just provided for backward
     * compatibility... Using strings for idendentity checks is not safe...
     */
    std::vector<std::string> getInterfaceNames() const;

  private:
    // these datastructures are needed to be able to cleanup the created
    // classes/structs in dtor, but not earlier.
    std::vector<std::shared_ptr<class ndlcom::ExternalInterfaceBase>>
        externalInterfaces;
    // "bridgeHandler" and "nodes" have the same base-class. still kept in
    // different vectors to allow nicer lookup of existing nodes prior to
    // creating a new one
    std::vector<std::shared_ptr<ndlcom::BridgeHandler>> bridgeHandler;
    std::vector<std::shared_ptr<class ndlcom::Node>> nodes;

    /*
     * harhar. by having this struct private we can more or less be sure that
     * there are not so many additional nodes we do not know about...?
     */
    struct NDLComBridge bridge;

    std::ostream &out;
};
} // namespace ndlcom

#endif /*NDLCOM_BRIDGE_HPP*/
