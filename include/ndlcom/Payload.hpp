#ifndef NDLCOM_PAYLOAD_HPP
#define NDLCOM_PAYLOAD_HPP

#include "ndlcom/Types.h"

#include <cstddef>
#include <vector>

namespace ndlcom {

/**
 * This is a small wrapper class to provide an easy contiguous region of
 * dynamically sized memory without having to write copy and assignment
 * constructors.
 */
struct Payload : private std::vector<uint8_t> {
    Payload(const void *payload, const size_t size);
    /**
     * only these three "properties" of the actual std::vector are exported,
     * all the rest is deliberately kept private:
     */
    size_t size() const;
    uint8_t *data();
    const uint8_t *data() const;
};

/**
 * This object contains a NDLComHeader, as it was observed on the wire for
 * this packet, and the respective payload. Additionally, the pointer to the
 * originating interface is stored for later reference.
 *
 * The payload is decoded and ready to be used by a consumer. An
 * IncomingPacket shall never be changed after it came to life, thus all the
 * members are const.
 *
 * TODO: Maybe an std::chrono::time_point as well?
 */
struct IncomingPayload {
    IncomingPayload(const struct NDLComHeader *_header, const void *_payload,
                    const struct NDLComExternalInterface *);
    const struct NDLComHeader header;
    const struct Payload payload;
    const struct NDLComExternalInterface* origin;
};

/**
 * The OutgoingPacket has a destinationId and a number of bytes intended to be
 * used as payload.
 */
struct OutgoingPayload {
    OutgoingPayload(const NDLComId _receiverId, const void *_payload,
                    const size_t size);
    const NDLComId receiverId;
    /**
     * modifiable, as the content of payload will be changed by
     * "represupport::send()" before actually sending.
     *
     * FIXME: this is a hard sell... where to store the "mBase.mId" to give
     * into "represupport::send()"?
     */
    struct Payload payload;
};

/**
 *
 * design decisions:
 *
 * - use new/delete on an internal raw pointer...  more hassle to write the
 *   ctors correctly. seems to not be noticably faster than a non-changing
 *   std::vector.
 * - smart pointers: might have the benefit to avoid unnessary copying as the
 *   data can be shared safely... how often does this happen? and results in
 *   more complex code. on the other hand: just wrap the resulting "packet" in
 *   a smart pointer and we have the same effect...
 * - wrap std::vector internally? don't need its iterators... dont need its
 *   resizing...  would not have to write the ctors... seems to be reasonable
 *   fast when not resizing... privately deriving feels even better
 * - have an internal array like "data[MAX_PKT_SZ]" and just expose the
 *   pointer... would waste some bytes but prevent dynamic allocations...
 *   would still need the ctors and all the hassle
 */

} // of namespace

#endif /*NDLCOM_PAYLOAD_HPP*/
