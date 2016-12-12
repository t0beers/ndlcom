#ifndef NDLCOM_NODE_HPP
#define NDLCOM_NODE_HPP

#include "ndlcom/NodeHandlerBase.hpp"

#include <memory>
#include <vector>

namespace ndlcom {

class Node {
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
        std::shared_ptr<T> p = std::make_shared<T>(node, args...);
        allHandler.push_back(p);
        return p;
    }

    NDLComId getOwnDeviceId() const;

    /**
     * guess what this does...
     */
    void send(const NDLComId receiverId, const void *payload,
              size_t payloadSize);

    /**
     * narf... needs to be public... so that the c-code from represupport for
     * example can get the pointer... need c++ there as well...
     */
    struct NDLComNode node;

  private:
    /* needed to keep the shared_ptr in scope until our ctor is called */
    std::vector<std::shared_ptr<class ndlcom::NodeHandlerBase> > allHandler;
};
}

#endif /*NDLCOM_NODE_HPP*/
