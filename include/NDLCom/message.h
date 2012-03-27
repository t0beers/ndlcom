#ifndef _NDLCOM_MESSAGE_H_
#define _NDLCOM_MESSAGE_H_

#include "protocol/header.h"
#include <ctime>

namespace NDLCom
{
    class Message
    {
        public:
            /* when creating this object, the length of the datafield is automatically supplied in the header */
            Message();
            Message(const struct timespec& time, const ProtocolHeader& hdr, const void* decodedData);
            Message(const ProtocolHeader* hdr, const void* decodedData);
            Message(const ProtocolHeader& hdr, const void* decodedData);
            /* destructor */
            ~Message();
            /*copy constructor */
            Message(const NDLCom::Message& m);
            /* assignment operator */
            Message& operator= (const NDLCom::Message &other);

            /* datastructures */
            struct timespec mTimestamp;
            struct timespec mTimestampFromSender;
            ProtocolHeader mHdr;
            char* mpDecodedData;

            int msg_size() const
            {
                return mHdr.mDataLen+sizeof(ProtocolHeader);
            };
    };
}
#endif/*_NDLCOM_MESSAGE_H_*/
