/**
 * @file NDLCom/Message.h
 * @brief a cpp-class holding a "message" in ndlcom-terms
 *
 * intented to share code between armv7, stm32 and x86.
 *
 * keeps name, header, timestamps and the actual message in one. provides some operators for
 * container classes. will allocate and free data that is needed.
 *
 * timestamp handling is kinda disabled for stm32, since clock_gettime is nor provided. but the
 * get-time functions provided by stm32common can be inserted here to enable it. and since none of
 * the stm32's uses this message object in the moment...
 *
 * @author Martin Zenzes
 * @date 2013-02-20
 */
#ifndef MESSAGE_H
#define MESSAGE_H

#include "NDLCom/Types.h"
#include "NDLCom/Header.h"

#include <string>

namespace ndlcom
{
    class Message
    {
        public:
            /**
             * using the "ndlcomHeader" type, which grew organically...
             */
            Message();
            ~Message();
#ifdef linux
            Message(const struct timespec& time, const ndlcomHeader& hdr, const void* decodedData);
#endif/*linux*/
            Message(const ndlcomHeader* hdr, const void* decodedData);
            Message(const ndlcomHeader& hdr, const void* decodedData);

            /*copy constructor */
            Message(const ndlcom::Message& m);
            /* assignment operator */
            Message& operator= (const ndlcom::Message &other);

            /**
             * the actual header, to be used during sending or used while sent
             */
            ndlcomHeader mHdr;

            /**
             * string of the device which received or created this thing. can be something like
             * "/dev/ttyUSB0", "MyWidgetName" or "datafile.protolog"
             */
            std::string mOrigin;

            char* mpDecodedData;

#ifdef linux
            /**
             * the timestamp, obtained when this message was received over the wire -- or the point
             * in time where we pretend to have done so
             */
            struct timespec mTimestamp;
#else
            uint64_t mTimestamp;
#endif/*linux*/


            int msg_size() const { return mHdr.mDataLen+sizeof(ndlcomHeader); };
    };
};

#endif /*MESSAGE_H*/
