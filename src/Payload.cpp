#include "ndlcom/Payload.hpp"

using namespace ndlcom;

Payload::Payload(const void *payload, const size_t size)
    : std::vector<uint8_t>((const uint8_t *)payload,
                           (const uint8_t *)payload + size) {}

size_t Payload::size() const { return std::vector<uint8_t>::size(); }

uint8_t *Payload::data() { return std::vector<uint8_t>::data(); }

const uint8_t *Payload::data() const { return std::vector<uint8_t>::data(); }

IncomingPayload::IncomingPayload(const struct NDLComHeader *_header,
                                 const void *_payload,
                                 const struct NDLComExternalInterface *_origin)
    : header(*_header), payload(_payload, _header->mDataLen), origin(_origin) {}

OutgoingPayload::OutgoingPayload(const NDLComId _receiverId,
                                 const void *_payload, const size_t size)
    : receiverId(_receiverId), payload(_payload, size) {}
