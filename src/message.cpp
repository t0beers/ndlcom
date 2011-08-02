#include "NDLCom/message.h"
#include "string.h"
#include "assert.h"

NDLCom::Message::Message(const ProtocolHeader hdr, const void* decodedData)
{
    /* header*/
    mHdr = hdr;

    /* payload */
    mpDecodedData = new char[mHdr.mDataLen];
    memcpy(mpDecodedData, decodedData, mHdr.mDataLen);

    /* timestamp */
    int r = clock_gettime(CLOCK_REALTIME, &mTimestamp);
    assert(r==0);
}

NDLCom::Message::Message(const ProtocolHeader* hdr, const void* decodedData)
{
    /* header*/
    mHdr = *hdr;

    /* payload */
    mpDecodedData = new char[mHdr.mDataLen];
    memcpy(mpDecodedData, decodedData, mHdr.mDataLen);

    /* timestamp */
    int r = clock_gettime(CLOCK_REALTIME, &mTimestamp);
    assert(r==0);
}

NDLCom::Message::~Message()
{
    delete[] mpDecodedData;
}

NDLCom::Message::Message(const ::NDLCom::Message& org)
{
    /* header*/
    mHdr = org.mHdr;

    /* payload */
    mpDecodedData = new char[mHdr.mDataLen];
    memcpy(mpDecodedData, org.mpDecodedData, mHdr.mDataLen);

    /* timestamp */
    mTimestamp = org.mTimestamp;
}

NDLCom::Message& NDLCom::Message::operator= (const ::NDLCom::Message &other)
{
    if (this != &other) // protect against invalid self-assignment
    {
        /* header */
        mHdr = other.mHdr;

        /* payload */
        delete[] mpDecodedData;
        mpDecodedData = new char[mHdr.mDataLen];
        memcpy(mpDecodedData, other.mpDecodedData, mHdr.mDataLen);

        /* timestamp */
        mTimestamp = other.mTimestamp;
    }
    // by convention, always return *this
    return *this;
}

