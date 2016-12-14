#ifndef NDLCOM_NODE_HPP
#define NDLCOM_NODE_HPP

#include "ndlcom/Node.h"
#include "ndlcom/InternalHandler.hpp"

#include <memory>
#include <vector>

namespace ndlcom {

/**
 * @brief c++ wrapper for NDLComNode
 *
 * Sadly, the class-structure (even in c) does not reflect what is going on.  A
 * Node is really an encapsulated NDLComBridgeHandler with added stuff going
 * on. But we cannot use the BridgeHandlerBase class, as it contains an
 * additional copy of the NDLComBridgeHandler struct.
 *
 * So then this is its own thing.
 *
 * Can create objects derived from ndlcom::NodeHandlerBase, takes ownership.
 *
 * Derives from the "BridgeHandlerWrapper" to be able to reuse the
 * c-implementation. therefore, this c++ object does not do much, see
 * NDLComNode.
 */
class Node : public BridgeHandlerBase {
  public:
    Node(struct NDLComBridge &bridge, NDLComId ownDeviceId);
    ~Node();

    /**
     * factory function. call like so:
     *
     *    std::shared_ptr<class ndlcom::NodePrintOwnId> p2 =
     *        node->createNodeHandler<ndlcom::NodePrintOwnId>();
     *
     * keeps internal copy of the returned shared_ptr to prevent early delete.
     */
    template <class T, class... A>
    std::shared_ptr<T> createNodeHandler(A... args) {
        static_assert(
            std::is_base_of<ndlcom::NodeHandlerBase, T>(),
            "can only create classes derived from ndlcom::NodeHandlerBase");
        std::shared_ptr<T> ret = std::make_shared<T>(node, args...);
        // this factory function shall be the only way to create these objects,
        // to make sure that the "registerHandler" function is always called
        ret->registerHandler();
        allHandler.push_back(ret);
        return ret;
    }

    void printStatus(std::ostream &out);

    NDLComId getOwnDeviceId() const;

    /**
     * guess what this does...
     */
    void send(const NDLComId receiverId, const void *payload,
              const size_t payloadSize);

    // no-ops in the moment...
    void registerHandler() override;
    void deregisterHandler() override;

  private:
    /**
     * harhar, private! please use the factor-functions to obtain new objects.
     * this will take care of valid creation and deletion
     */
    struct NDLComNode node;

    /* needed to keep the shared_ptr in scope until our ctor is called */
    std::vector<std::shared_ptr<ndlcom::NodeHandlerBase> > allHandler;
};
}

#endif /*NDLCOM_NODE_HPP*/
