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

Bridge::Bridge() : printAll(NULL), printMiss(NULL) {
    ndlcomBridgeInit(&bridge);
}

Bridge::~Bridge() {
    allInterfaces.clear();
    allNodes.clear();
    // release the object before we go out-of-scope. prevents use-after-free
    printAll.reset();
    printMiss.reset();
}

std::shared_ptr<class ndlcom::BridgePrintAll> Bridge::enablePrintAll() {
    printAll = createBridgeHandler<ndlcom::BridgePrintAll>();
    return printAll;
}

std::shared_ptr<class ndlcom::BridgePrintMissEvents> Bridge::enablePrintMiss() {
    printMiss = createBridgeHandler<ndlcom::BridgePrintMissEvents>();
    return printMiss;
}

std::shared_ptr<class ndlcom::ExternalInterfaceBase>
Bridge::createInterface(std::string uri, uint8_t flags) {
    std::shared_ptr<class ndlcom::ExternalInterfaceBase> ret =
        ndlcom::ParseUriAndCreateExternalInterface(std::cerr, bridge, uri,
                                                   flags);
    allInterfaces.push_back(ret);
    return ret;
}

std::shared_ptr<class ndlcom::Node> Bridge::createNode(const NDLComId nodeDeviceId) {
    std::shared_ptr<class ndlcom::Node> p;
    auto it = allNodes.find(nodeDeviceId);
    if (it != allNodes.end()) {
        p = it->second;
    } else {
        p = std::make_shared<class ndlcom::Node>(this->bridge, nodeDeviceId);
        allNodes.insert(std::make_pair(nodeDeviceId, p));
    }

    return p;
}

void Bridge::printStatus(std::ostream &out) {

    for (auto it : allNodes) {
        out << "listening messages for receiverId 0x" << std::setfill('0')
            << std::hex << std::setw(2) << (int)it.first << std::dec << "\n";
    }
    if (printAll) {
        out << "printing all messages passing through the bridge\n";
    }
    if (printMiss) {
        out << "printing miss-events for passing message streams\n";
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

std::shared_ptr<class ndlcom::Node> Bridge::enableOwnId(const NDLComId nodeDeviceId, bool print) {

    std::shared_ptr<class ndlcom::Node> p;
    auto it = allNodes.find(nodeDeviceId);
    if (it != allNodes.end()) {
        p = it->second;
    } else {
        p = createNode(nodeDeviceId);
        allNodes.insert(std::make_pair(nodeDeviceId, p));
    }
    // sadly, there is no API in the moment to query if a given ndlcom::Node
    // owns a node of type "NodeHandlerPrintOwnId"
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
