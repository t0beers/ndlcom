#include "ndlcom/Bridge.hpp"
#include "ndlcom/ExternalInterfaceParseUri.hpp"
#include "ndlcom/ExternalInterface.hpp"

#include <iomanip>

using namespace ndlcom;

struct timespec ndlcomBridgeTimeDiff(struct timespec start,
                                     struct timespec end) {
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

Bridge::Bridge() { ndlcomBridgeInit(&bridge); }

Bridge::~Bridge() {
    externalInterfaces.clear();
    bridgeHandler.clear();
    nodes.clear();
}

std::shared_ptr<class ndlcom::BridgeHandlerBase> Bridge::enablePrintAll() {
    return createBridgeHandler<ndlcom::BridgePrintAll>();
}

std::shared_ptr<class ndlcom::BridgeHandlerBase> Bridge::enablePrintMiss() {
    return createBridgeHandler<ndlcom::BridgePrintMissEvents>();
}

std::shared_ptr<class ndlcom::ExternalInterfaceBase>
Bridge::createInterface(std::string uri, uint8_t flags) {
    std::shared_ptr<class ndlcom::ExternalInterfaceBase> ret =
        ndlcom::ParseUriAndCreateExternalInterface(std::cerr, bridge, uri,
                                                   flags);
    externalInterfaces.push_back(ret);
    return ret;
}

std::shared_ptr<class ndlcom::Node>
Bridge::createNode(const NDLComId nodeDeviceId) {
    std::shared_ptr<class ndlcom::Node> retval;
    for (auto it : nodes) {
        if (it->getOwnDeviceId() == nodeDeviceId) {
            return it;
        }
    }
    retval = std::make_shared<class ndlcom::Node>(this->bridge, nodeDeviceId);
    nodes.push_back(retval);

    return retval;
}

void Bridge::printStatus(std::ostream &out) {
    for (auto it : nodes) {
        it->printStatus(out);
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
    out << "Bridge::printRoutingTable: \n";
    struct NDLComExternalInterface *externalInterface;
    if (list_empty(&bridge.externalInterfaceList)) {
        out << "No external interfaces registered. RoutingTable empty\n";
        return;
    }
    list_for_each_entry(externalInterface, &bridge.externalInterfaceList,
                        list) {

        class ndlcom::ExternalInterfaceBase *base =
            static_cast<class ndlcom::ExternalInterfaceBase *>(
                externalInterface->context);
        out << "interface '" << base->label << "':\n";

        bool printed = false;
        for (size_t deviceId = 0; deviceId < 256; ++deviceId) { // todo: sizeof
            if (bridge.routingTable.table[deviceId] == externalInterface) {
                out << deviceId << " ";
                printed = true;
            }
        }
        if (!printed) {
            out << "<none>";
        }
        out << "\n";
    }
}

std::shared_ptr<class ndlcom::Node>
Bridge::enableOwnId(const NDLComId nodeDeviceId, bool print) {
    std::shared_ptr<class ndlcom::Node> p = createNode(nodeDeviceId);
    if (print) {
        p->createNodeHandler<class ndlcom::NodeHandlerPrintOwnId>();
    }
    return p;
}

struct timespec Bridge::process() {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    ndlcomBridgeProcess(&bridge);
    clock_gettime(CLOCK_MONOTONIC, &end);

    return ndlcomBridgeTimeDiff(start, end);
}

void Bridge::sendMessageRaw(const struct NDLComHeader *header,
                            const void *payload) {
    ndlcomBridgeSendRaw(&bridge, header, payload);
}
