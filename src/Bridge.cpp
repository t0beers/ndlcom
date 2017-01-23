#include <stddef.h>
#include <iomanip>
#include <limits>

#include "ndlcom/Bridge.hpp"
#include "ndlcom/BridgeHandler.hpp"
#include "ndlcom/ExternalInterface.hpp"
#include "ndlcom/ExternalInterfaceBase.hpp"
#include "ndlcom/Node.hpp"
#include "ndlcom/NodeHandler.hpp"
#include "ndlcom/Routing.h"
#include "ndlcom/list.h"

namespace ndlcom {
class NodeHandlerPrintOwnId;
} // namespace ndlcom

using namespace ndlcom;

std::vector<std::string> ndlcom::splitStringIntoStrings(std::string s,
                                                        const char delim) {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<NDLComId>
ndlcom::convertStringToIds(std::vector<std::string> numbers,
                           std::ostream &out) {
    std::vector<NDLComId> retval;
    for (auto it : numbers) {
        int number = std::stoi(it);
        if (number == NDLCOM_ADDR_BROADCAST) {
            out << "ParseUri: will ignore broadcast\n";
            continue;
        }
        if (number < std::numeric_limits<NDLComId>::min()) {
            out << "ParseUri: too small a deviceId '" << it << "'\n";
        }
        if (number > std::numeric_limits<NDLComId>::max()) {
            out << "ParseUri: too large a deviceId '" << it << "'\n";
        }
        retval.push_back(number);
    }
    return retval;
}

void
ndlcom::setRoutingByString(std::weak_ptr<class ndlcom::ExternalInterfaceBase> p,
                           std::string conn, std::ostream &out) {

    std::vector<NDLComId> ids =
        convertStringToIds(splitStringIntoStrings(conn, ','), out);
    std::shared_ptr<class ndlcom::ExternalInterfaceBase> interface = p.lock();

    for (auto it : ids) {
        out << "ParseUri: set routingTable to use '" << interface->label
            << "' for deviceId 0x" << std::setfill('0') << std::hex
            << std::setw(2) << (int)(it) << std::dec << "\n";
        interface->setRoutingForDeviceId(it);
    }
}

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
        out << "Attention, something went wrong during teardown, "
               "c-ExternalInterfaceList is not empty\n";
    }
    if (!list_empty(&bridge.bridgeHandlerList)) {
        out << "Attention, something went wrong during teardown, "
               "c-BridgeHandlerList is not empty\n";
    }
}

std::weak_ptr<ndlcom::ExternalInterfaceBase>
Bridge::getInterfaceByName(const std::string name) const {
    for (auto it : externalInterfaces) {
        if (it->label == name) {
            return it;
        }
    }
    return std::weak_ptr<ndlcom::ExternalInterfaceBase>();
}

std::vector<std::string> Bridge::getInterfaceNames() const {
    std::vector<std::string> retval;
    for (auto it : externalInterfaces) {
        retval.push_back(it->label);
    }
    return retval;
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
        createInterfaceByUri<ExternalInterfaceSerial, ExternalInterfaceUdp,
                             ExternalInterfaceFpga, ExternalInterfacePipe,
                             ExternalInterfacePty, ExternalInterfaceTcpClient>(
            uri, flags));
    return ret;
}

void Bridge::printStatus() {
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

void Bridge::printRoutingTable() {
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
