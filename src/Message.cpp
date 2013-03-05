#include "NDLCom/Message.h"

#include <string.h>
#include <cassert>

using ndlcom::Message;

Message::Message() :
    mHdr(),
    mpDecodedData(NULL),
    mTimestamp()
{
    /* by default, we name the origin "internal", which is most creator-agnostic? */
    mOrigin="internal";
}

#ifdef linux
Message::Message(const struct timespec& time, const ndlcomHeader& hdr, const void* decodedData) :
    mHdr(hdr),
    mpDecodedData(new char[mHdr.mDataLen]),
    mTimestamp(time)
{
    /* copy the payload */
    memcpy(mpDecodedData, decodedData, mHdr.mDataLen);

    /* by default, we name the origin "internal", which is most creator-agnostic? */
    mOrigin="internal";
}
#endif/*linux*/

Message::Message(const ndlcomHeader& hdr, const void* decodedData) :
    mHdr(hdr),
    mpDecodedData(new char[mHdr.mDataLen]),
    mTimestamp()
{
    /* copy the payload */
    memcpy(mpDecodedData, decodedData, mHdr.mDataLen);

    /* additionally, we'll take the current time into our data */
#ifdef linux
    int r = clock_gettime(CLOCK_REALTIME, &mTimestamp);
    assert(r==0);
#endif/*linux*/

    /* by default, we name the origin "internal", which is most creator-agnostic? */
    mOrigin="internal";
}

Message::Message(const ndlcomHeader* hdr, const void* decodedData) :
    mHdr(*hdr),
    mpDecodedData(new char[mHdr.mDataLen]),
    mTimestamp()
{
    /* copy the payload */
    memcpy(mpDecodedData, decodedData, mHdr.mDataLen);

    /* additionally, we'll take the current time into our data */
#ifdef linux
    int r = clock_gettime(CLOCK_REALTIME, &mTimestamp);
    assert(r==0);
#endif/*linux*/

    /* by default, we name the origin "internal", which is most creator-agnostic? */
    mOrigin="internal";
}

Message::~Message()
{
    delete[] mpDecodedData;
}

Message::Message(const Message& org) :
    mHdr(org.mHdr),
    mOrigin(org.mOrigin),
    mpDecodedData(new char[mHdr.mDataLen]),
    mTimestamp(org.mTimestamp)
{
    /* payload */
    memcpy(mpDecodedData, org.mpDecodedData, mHdr.mDataLen);
}

Message& Message::operator= (const Message &other)
{
    if (this != &other) // protect against invalid self-assignment
    {
        /* header */
        mHdr = other.mHdr;

        /* payload */
        delete[] mpDecodedData;
        mpDecodedData = new char[mHdr.mDataLen];
        memcpy(mpDecodedData, other.mpDecodedData, mHdr.mDataLen);

        /* the timestamps */
        mTimestamp = other.mTimestamp;

        /* the rest */
        mOrigin = other.mOrigin;
    }
    // by convention, always return *this
    return *this;
}
