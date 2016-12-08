#include "ndlcom/NodeHandlerBase.hpp"

using namespace ndlcom;

NodeHandlerBase::NodeHandlerBase(NDLComNode &_node, std::ostream &_out)
    : node(_node), out(_out) {
    ndlcomNodeHandlerInit(&internal, NodeHandlerBase::handleWrapper,
                              NDLCOM_INTERNAL_HANDLER_FLAGS_DEFAULT, this);
    ndlcomNodeRegisterNodeHandler(&node, &internal);
}

NodeHandlerBase::~NodeHandlerBase() {
    ndlcomNodeDeregisterNodeHandler(&node, &internal);
}

// static wrapper function for the C-callback
void NodeHandlerBase::handleWrapper(void *context,
                                    const struct NDLComHeader *header,
                                    const void *payload, const void *origin) {
    class NodeHandlerBase *self = static_cast<class NodeHandlerBase *>(context);
    self->handle(header, payload, origin);
}

void NodeHandlerBase::send(const NDLComId receiverId, const void *payload,
                           const size_t length) {
    ndlcomNodeSend(&node, receiverId, payload, length);
}
