#include "ndlcom/Bridge.hpp"
#include "ndlcom/ExternalInterfaceParseUri.hpp"
#include "ndlcom/ExternalInterface.hpp"
#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/NodeHandler.hpp"

#include <iomanip>

using namespace ndlcom;

Bridge::Bridge() { ndlcomBridgeInit(&bridge); }

Bridge::~Bridge() {
    externalInterfaces.clear();
    for (auto &it : bridgeHandler) {
        it->deregisterHandler();
    }
    bridgeHandler.clear();
    for (auto &it : nodes) {
        it->deregisterHandler();
    }
    nodes.clear();
}

std::shared_ptr<class ndlcom::BridgeHandler> Bridge::enablePrintAll() {
    return createBridgeHandler<class ndlcom::BridgePrintAll>();
}

std::shared_ptr<class ndlcom::BridgeHandler> Bridge::enablePrintMiss() {
    return createBridgeHandler<class ndlcom::BridgePrintMissEvents>();
}

std::shared_ptr<class ndlcom::ExternalInterfaceBase>
Bridge::createInterface(std::string uri, uint8_t flags) {
    std::shared_ptr<class ndlcom::ExternalInterfaceBase> ret(
        ndlcom::ParseUriAndCreateExternalInterface(std::cerr, bridge, uri,
                                                   flags));
    // the interface is already "registered" at the Bridge
    externalInterfaces.push_back(ret);
    return ret;
}

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

std::shared_ptr<class ndlcom::Node>
Bridge::enableOwnId(const NDLComId nodeDeviceId, bool print) {
    auto ret = createNode(nodeDeviceId);
    if (print) {
        ret->createNodeHandler<class ndlcom::NodeHandlerPrintOwnId>();
    }
    return ret;
}

void Bridge::process() { ndlcomBridgeProcess(&bridge); }

void Bridge::sendMessageRaw(const struct NDLComHeader *header,
                            const void *payload) {
    ndlcomBridgeSendRaw(&bridge, header, payload);
}
