/**
 * @file ndlcom_core/RoutingTable.hpp
 * @date 2012
 */
#ifndef NDLCOM_CORE_ROUTINGTABLE_HPP
#define NDLCOM_CORE_ROUTINGTABLE_HPP

#include "ndlcom_core/Types.h"
#include "ndlcom_core/Header.h"
#include "ndlcom_core/DeviceIds.h"

#include <vector>
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace ndlcom
{
    /**
     * stores a global mapping of "receiverId -> vector<interface>"
     *
     * one receiver id can have more than one interface as a target, this happens with
     * broadcasts for example.
     *
     * intented use-case is using this with pointers.
     *
     * forced by the c++ standard, pointers are zero-initialized
     */
    template<class T>
    class RoutingTable
    {
        public:
            RoutingTable(){};

            /**
             * add a new "route" to the table of known connections
             */
            void noteReceiverIdOnInterface(NDLComId receiverId, const T interface)
            {
                table[receiverId].push_back(interface);
            };

            /**
             * get a (hopefully) globally unique T where the message _should_ have be sent from
             */
            T getOrigin(NDLComId senderId) const
            { 
                // lookup the sending interface (origin) in the static routing-table
                std::vector<T> interfaces = getRoutes(senderId);

                // some error handling
                if (interfaces.empty())
                {
                    std::ostringstream s;
                    s << "unknown message-origin for " << (int)senderId << " in static routing table";
                    throw std::runtime_error(s.str());
                }
                else if (interfaces.size() > 1)
                {
                    std::ostringstream s;
                    s << "not a unique origin for sender " << (int)senderId;
                    throw std::runtime_error(s.str());
                }

                // and the origin should be globally unique...?
                return interfaces.at(0);
            }

            /**
             * get a vector of interfaces which have to be used for this receiverId
             *
             * several cases are handled transparently. for example
             * broadcasts are just a large vectors for the broadcast id
             */
            std::vector<T> getRoutes(NDLComId receiverId) const
            {
                return table[receiverId];
            };

            /**
             * get a vector of interfaces where the message should be written to,
             * _excluding_ the specified message-origin
             */
            std::vector<T> getRoutes(NDLComId receiverId, const T origin) const
            {
                std::vector<T> retval = table[receiverId];

                // will throw away all routes equal to "origin" -- don't echo messages on
                // the port where they where received in the first place
                retval.erase(std::remove(retval.begin(),
                             retval.end(),
                             origin),
                       retval.end());

                return retval;
            };

            /**
             * get a vector of interfaces where the receiver is expected to listen, excluding the
             * interface where the message is expected to come from
             */
            std::vector<T> getRoutes(NDLComId receiverId, NDLComId senderId) const
            {
                return getRoutes(receiverId, getOrigin(senderId));
            }

            /** remove all routes belonging to a single receiverId
             * @return the old route, valid before the id was cleared
             */
            void clearReceiverId(NDLComId receiver)
            {
                table[receiver].clear();
            }

            /**
             * remove a interface completely from all possble routing entrys
             */
            void clearInterface(const T interface)
            {
                // "foreach"
                for (NDLComId i=0;i<NDLCOM_MAX_NUMBER_OF_DEVICES-1;i++)
                {
                    std::vector<T>& routes = table[i];

                    // will throw away all vector-elements equal to "interface"
                    routes.erase(std::remove(routes.begin(),
                                             routes.end(),
                                             interface),
                                 routes.end());
                }
            }

        private:
            /**
             * the actual "static" routing table
             */
            std::vector<T> table[NDLCOM_MAX_NUMBER_OF_DEVICES];
    };
};



#endif /*NDLCOM_CORE_ROUTINGTABLE_HPP*/
