#ifndef NDLCOM_NODE_HPP
#define NDLCOM_NODE_HPP

#include "ndlcom/Node.h"

#include <stddef.h>
#include <memory>
#include <type_traits>
#include <vector>

#include "ndlcom/InternalHandler.hpp"
#include "ndlcom/Types.h"

namespace ndlcom {

/**
 * @brief C++ wrapper for NDLComNode
 *
 * A Node is in reality an encapsulated NDLComBridgeHandler with additional
 * stuff going on. So we can use the BridgeHandlerBase class, which uses a
 * reference to the actual instance of the NDLComBridgeHandler struct inside
 * this class.
 *
 * Can create objects derived from ndlcom::NodeHandlerBase, takes ownership of
 * these.
 */
class Node : public BridgeHandlerBase {
  public:
    Node(struct NDLComBridge &bridge, const NDLComId ownDeviceId);
    /**
     * Destroys all NodeHandlerBase owned by this Node
     */
    ~Node() override;

    /**
     * @brief Create new NodeHandlerBase objects and use in this Node
     *
     * This factory function shall be the only way to create Handler objects,
     * to make sure that the "registerHandler" function is always called and
     * the resulting object is owned by this object.
     *
     * Call it like so:
     *
     *    std::shared_ptr<class ndlcom::NodePrintOwnId> p2 =
     *        node->createNodeHandler<ndlcom::NodePrintOwnId>();
     *
     * Keeps internal shared_ptr of the returned weak_ptr to make sure this
     * object has exclusive ownership.
     */
    template <class T, class... A>
    std::weak_ptr<T> createNodeHandler(A... args) {
        static_assert(
            std::is_base_of<ndlcom::NodeHandlerBase, T>(),
            "can only create classes derived from ndlcom::NodeHandlerBase");
        std::shared_ptr<T> ret = std::make_shared<T>(node, args...);
        ret->registerHandler();
        allHandler.push_back(ret);
        return ret;
    }

    /**
     * Generic "destory" function takes care to do all the right steps in order
     * to get rid of the assiciated node. Deregisters the handler from the Node
     * and removes the shared_ptr from the container.
     */
    template <class T> void destroyNodeHandler(std::weak_ptr<T> a) {
        std::shared_ptr<T> p = a.lock();
        p->deregisterHandler();
        allHandler.erase(std::remove(allHandler.begin(), allHandler.end(), p),
                         allHandler.end());
    }

    /**
     * @brief Print own label, then call "printStatus" for each owned handler
     */
    void printStatus(const std::string) const override;

    /**
     * @brief Obtain the deviceId used in the NDLComNode struct
     *
     * Please note that this function will only work after everything is
     * finished with setting up... E.g. after the ndlcomNodeInit() was called
     * in the dtor.
     */
    const NDLComId getOwnDeviceId() const;

    /**
     * @brief Changing the own personality
     *
     * Will change which packages will be seen in the NodeHandlerBase owned by
     * this class.
     *
     * @param ownDeviceId The new deviceId to use when sending/receiving
     */
    void setOwnDeviceId(const NDLComId ownDeviceId);

    /**
     * @brief Sending a new message
     *
     * @param receiverId Where to send to
     * @param payload Pointer to memory containing the payload
     * @param payloadSize Number of bytes to transmit
     */
    void send(const NDLComId receiverId, const void *payload,
              const size_t payloadSize);

    /**
     * Adds the handler-function of this Node (implemented by
     * BridgeHandlerBase) to the actual Bridge. To be called by the owner of
     * this object.
     */
    void registerHandler() override;

    /**
     * Removes the handler of this class from the owning class list of handlers.
     */
    void deregisterHandler() override;

  private:
    /**
     * harhar, private struct!
     *
     * Use the factor-functions to create new objects. This will take care of
     * valid creation and deletion. Never access this struct directly.
     */
    struct NDLComNode node;

    /**
     * This vector will keep track of all owned ndlcom::NodeHandlerBase. Keeps the
     * shared_ptr in scope until our dtor is called.
     */
    std::vector<std::shared_ptr<ndlcom::NodeHandlerBase>> allHandler;
};
} // namespace ndlcom

#endif /*NDLCOM_NODE_HPP*/
