#include "ndlcom/Bridge.hpp"
#include "ndlcom/ExternalInterfaceParseUri.hpp"
#include "ndlcom/ExternalInterface.hpp"
#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/NodeHandler.hpp"

#include "ndlcom/list.h"

#include <iomanip>

using namespace ndlcom;

Bridge::Bridge(std::ostream &_out) : out(_out) { ndlcomBridgeInit(&bridge); }

Bridge::~Bridge() {
    // no iterators, as the call inside the loop would invalidate them
    /**
     * The dtor _has_ to explicitly destroy (to deregister the handlers in) all
     * owner+attached objects before it is finished itself. otherwise attached
     * objects may call into this bridge, use-after-free
     */
    while (!externalInterfaces.empty()) {
        destroyExternalInterface(std::weak_ptr<ndlcom::ExternalInterfaceBase>(
            externalInterfaces.front()));
    }
    // "BridgeHandler" and "Node" are essentially the same on the c level
    // (stored in "internalHandler" list), but additionally kept in two
    // distinct datastructures on the c++ level to allow easy lookup for an
    // existing Node
    while (!bridgeHandler.empty()) {
        destroyBridgeHandler(
            std::weak_ptr<ndlcom::BridgeHandler>(bridgeHandler.front()));
    }
    while (!nodes.empty()) {
        destroyNode(std::weak_ptr<ndlcom::Node>(nodes.front()));
    }
    // check that the internal c linked list is really empty
    if (!list_empty(&bridge.externalInterfaceList)) {
        throw std::runtime_error("boa");
    }
    if (!list_empty(&bridge.bridgeHandlerList)) {
        throw std::runtime_error("hossa");
    }
}

std::weak_ptr<class ndlcom::BridgeHandler> Bridge::enablePrintAll() {
    return createBridgeHandler<class ndlcom::BridgePrintAll>();
}

std::weak_ptr<class ndlcom::BridgeHandler> Bridge::enablePrintMiss() {
    return createBridgeHandler<class ndlcom::BridgePrintMissEvents>();
}

std::weak_ptr<class ndlcom::ExternalInterfaceBase>
Bridge::createInterface(std::string uri, uint8_t flags) {
    std::shared_ptr<class ndlcom::ExternalInterfaceBase> ret(
        ndlcom::ParseUriAndCreateExternalInterface(std::cerr, bridge, uri,
                                                   flags));
    // the interface is already "registered" at the Bridge, done by the parsing
    // function
    externalInterfaces.push_back(ret);
    return ret;
}

#if 0
// enforce one-node-per-id?
std::shared_ptr<class ndlcom::Node>
Bridge::createNode(const NDLComId nodeDeviceId) {
    for (auto it : nodes) {
        if (it->getOwnDeviceId() == nodeDeviceId) {
            return it;
        }
    }
    auto ret = std::make_shared<class ndlcom::Node>(this->bridge, nodeDeviceId);
    ret->registerHandler();
    nodes.push_back(ret);

    return ret;
}
#endif

void Bridge::printStatus(std::ostream &out) {
    out << "Bridge status:\n";
    for (auto it : nodes) {
        it->printStatus();
    }
    if (!bridgeHandler.empty()) {
        out << "BridgeHandler:\n";
        for (auto it : bridgeHandler) {
            out << "- " << it->label << "\n";
        }
    }
    if (!externalInterfaces.empty()) {
        out << "ExternalInterfaces:\n";
        for (auto it : externalInterfaces) {
            out << "- " << it->label << "\n";
        }
    }
}

void Bridge::printRoutingTable(std::ostream &out) {
    struct NDLComExternalInterface *externalInterface;
    if (list_empty(&bridge.externalInterfaceList)) {
        out << "printRoutingTable: No external interfaces registered. "
               "RoutingTable "
               "probably empty\n";
        return;
    } else {
        out << "printRoutingTable: \n";
    }
    /*
     * this block needs to use the c-level loop to obtain the lowest-level
     * pointer to the externalInterface struct so that we are able to compare
     * values in the NDLComRoutingTable.
     */
    list_for_each_entry(externalInterface, &bridge.externalInterfaceList,
                        list) {

        out << "interface '"
            << static_cast<const class ndlcom::ExternalInterfaceBase *>(
                   externalInterface->context)->label << "': ";

        out << std::setfill('0') << std::showbase << std::hex
            << std::setfill('0') << std::setw(4) << std::internal;
        bool printed = false;
        for (size_t deviceId = 0;
             deviceId < sizeof(bridge.routingTable.table) /
                            sizeof(bridge.routingTable.table[0]);
             ++deviceId) {
            if (bridge.routingTable.table[deviceId] == externalInterface) {
                out << deviceId << " ";
                printed = true;
            }
        }
        out << std::noshowbase << std::dec;
        if (!printed) {
            out << "<none>";
        }
        out << "\n";
    }
}

std::weak_ptr<class ndlcom::Node>
Bridge::enableOwnId(const NDLComId nodeDeviceId, bool print) {
    auto ret = createNode<class ndlcom::Node>(nodeDeviceId);
    if (print) {
        ret.lock()->createNodeHandler<class ndlcom::NodeHandlerPrintOwnId>();
    }
    return ret;
}

void Bridge::process() { ndlcomBridgeProcess(&bridge); }

void Bridge::sendMessageRaw(const struct NDLComHeader *header,
                            const void *payload) {
    ndlcomBridgeSendRaw(&bridge, header, payload);
}
