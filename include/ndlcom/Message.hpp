#ifndef NDLCOM_MESSAGE_H
#define NDLCOM_MESSAGE_H

#include "ndlcom/Types.h"
#include "ndlcom/Header.h"

#include <string>

#if defined(__cplusplus)
extern "C" {
#endif
namespace ndlcom {
class Message {
  public:
    /**
     * using the "NDLComHeader" type, which grew organically...
     */
    Message();
    ~Message();
#ifdef linux
    Message(const struct timespec &time, const NDLComHeader &hdr,
            const void *decodedData);
#endif /*linux*/
    Message(const NDLComHeader *hdr, const void *decodedData);
    Message(const NDLComHeader &hdr, const void *decodedData);

    /* copy constructor */
    Message(const ndlcom::Message &m);
    /* assignment operator */
    Message &operator=(const ndlcom::Message &other);

    /**
     * the actual header, to be used during sending or used while sent
     */
    NDLComHeader mHdr;

    /**
     * string of the device which received or created this thing. Can be
     * something like "/dev/ttyUSB0", "MyWidgetName" or "datafile.protolog"
     */
    std::string mOrigin;

    char *mpDecodedData;

#ifdef linux
    /**
     * the timestamp, obtained when this message was received over the wire --
     * or the point in time where we pretend to have done so
     */
    struct timespec mTimestamp;
#else
    uint64_t mTimestamp;
#endif /*linux*/

    int msg_size() const { return mHdr.mDataLen + sizeof(NDLComHeader); };
};
}

#if defined(__cplusplus)
}
#endif

#endif /*MESSAGE_H*/
