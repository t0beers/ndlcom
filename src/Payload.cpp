#include "ndlcom/Payload.hpp"

using namespace ndlcom;

Payload::Payload(const void *payload, const NDLComDataLen size)
    : std::vector<uint8_t>((const uint8_t *)payload,
                           (const uint8_t *)payload + size) {}

NDLComDataLen Payload::dataLen() const { return std::vector<uint8_t>::size(); }

const void *Payload::data() const {
    return static_cast<const void *>(std::vector<uint8_t>::data());
}
void *Payload::data() {
    return static_cast<void *>(std::vector<uint8_t>::data());
}

IncomingPayload::IncomingPayload(
    const struct NDLComHeader *_header, const void *_payload,
    const struct NDLComExternalInterface *_origin,
    std::chrono::time_point<std::chrono::system_clock> _receivedAt)
    : RawPayload(_header, _payload), origin(_origin), receivedAt(_receivedAt) {}

OutgoingPayload::OutgoingPayload(NDLComId _destinationId, const void *_payload,
                                 size_t size)
    : ndlcom::Payload(_payload, size), destinationId(_destinationId) {}
