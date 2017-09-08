#ifndef NDLCOM_PAYLOAD_HPP
#define NDLCOM_PAYLOAD_HPP

#include "ndlcom/Types.h"

#include <chrono>
#include <vector>

/** forward decl to be able to use the origin-pointer in IncomingPayload: */
struct NDLComExternalInterface;

namespace ndlcom {

/**
 * @brief Wrapped continous memory to represent a payload of an ndlcom packet
 *
 * This is a small wrapper base-class to provide an easy contiguous region of
 * quasi-dynamically sized memory without having to write copy or assignment
 * constructors.
 *
 * Internally the data of the inherited std::vector is filled in the ctor of
 * this class when passing the void-pointer and size argument, as iterator
 * "first" and "last". This results in reserving exactly as much memory when
 * copying the bytes behind the pointer into the std::vector, not a single byte
 * more. As the payload-size shall never change after construction this is a
 * nice for-free optimization.
 *
 * Please be sure to only put "size" arguments of sensible type into the ctor,
 * the implicit cast of NDLComDataLen might suprise you. But it would happen
 * anyways, as an ndlcom-packet only carries a maximum of 255 bytes. An extra
 * size-check could be added, but meh...
 */
struct Payload : protected std::vector<uint8_t> {
    /**
     * Copies the memory region of "payload" until "payload+size" into the
     * inherited std::vector.
     *
     * Could make this ctor "explicit"...
     */
    Payload(const void *payload, NDLComDataLen size);
    /**
     * Only these "properties" of the actual std::vector interface are
     * exported, all the rest is deliberately kept hidden by using "protected"
     * when inheriting.
     */
    NDLComDataLen dataLen() const;
    /**
     * This allows to access the internal data for reading... deliberately only
     * as a "const pointer", so that nobody (tm) can change the data once it
     * was initialized.
     *
     * Implementation detail: Later deriving classes can choose to provide a
     * non-const accessor function, see represupport::TypedOutgoingPayload for
     * example.
     *
     * Internally the data is stored in std::vector<uint8_t>, but casted to a
     * void pointer by this accessor.
     */
    const void* data() const;
};

/**
 * @brief Wrapped payload+header of an ndlcom packet
 *
 * An ndlcom::RawPayload is used to represent a complete ndlcom packet with
 * full control over the appereance. This can be used either when sending or to
 * store received messages.
 *
 * Note that, normally, a message header is constructed by an ndlcom::Node
 * after consulting internal state. The normal usecase would be, after mixing
 * in some more properties, to be the base of an ndlcom::IncomingPayload, see
 * below.
 */
struct RawPayload : public ndlcom::Payload {
    RawPayload(const struct NDLComHeader *_header, const void *_payload)
        : Payload(_payload, _header->mDataLen), header(*_header) {}
    /**
     * After creation, this shall never be changed again. But when this is a
     * "const" member, there will be no default assignment operator -- which we
     * did not want to implement in the first place... what a conundrum...
     */
    struct NDLComHeader header;
};

/**
 * @brief Wrapped ndlcom packet after it was received by an ndlcom::Bridge
 *
 * It contains a NDLComHeader, as it was observed on the wire for this packet,
 * and the respective payload. Additionally, the pointer to the originating
 * NDLComExternalInterface is stored for later reference as well as the point
 * in time, when this packet was received.
 *
 * The payload is decoded and ready to be used by a consumer. An IncomingPacket
 * shall never be changed after it came to life, thus all the members are
 * const.
 */
struct IncomingPayload : public ndlcom::RawPayload {
    IncomingPayload(const struct NDLComHeader *_header, const void *_payload,
                    const struct NDLComExternalInterface *,
                    std::chrono::time_point<std::chrono::system_clock>
                        _receivedAt = std::chrono::system_clock::now());
    /**
     * The interface where this message came from.
     *
     * NOTE: This can also be nullptr, in case the message was sent by some
     * component of the ndlcom::Bridge itself.
     */
    const struct NDLComExternalInterface *origin;
    /**
     * A Timestamp
     *
     * Is it better to fill it in our ctor than using the passed-in one?
     */
    std::chrono::time_point<std::chrono::system_clock> receivedAt;
};

/**
 * @brief Wrapper for an outgoing packet, before it was actually sent
 *
 * The OutgoingPacket has an NDLComId as a destination and a number of bytes
 * intended to be used as payload.
 *
 * It has no header, as this will be created and filled by the sending
 * instance, possibly derived from ndlcom::Node.
 */
struct OutgoingPayload : public ndlcom::Payload {
    OutgoingPayload(NDLComId _destinationId, const void *_payload, size_t size);
    /**
     * Where this message shall be sent to
     */
    NDLComId destinationId;
};

/**
 * Notes on some design decisions during writing of this family of classes:
 *
 * - do not use new/delete on an internal raw pointer...  more hassle to write
 *   the ctors correctly. seems to not be noticably faster than a non-changing
 *   std::vector.
 * - smart pointers: might have the benefit to avoid unnessary copying as the
 *   data can be shared safely... if locked properly... how often does this
 *   happen? and results in more complex code. on the other hand: just wrap the
 *   resulting "Payload" object in a smart pointer and we have the same effect...
 * - wrap std::vector internally? don't need its iterators... dont need its
 *   resizing...  would not have to write the ctors... seems to be reasonable
 *   fast when not resizing... privately deriving feels even better
 * - have an internal array like "data[MAX_PKT_SZ]" and just expose the
 *   pointer... would waste some bytes but prevent dynamic allocations...
 *   would still need the ctors and all the hassle
 */

} // of namespace

#endif /*NDLCOM_PAYLOAD_HPP*/
