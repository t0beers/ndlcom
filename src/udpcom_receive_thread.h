/**
 * @file lib/NDLCom/src/udpcom_receive_thread.h
 * @brief Internal class used by NDLCom::UdpCom
 * @author Armin <burchardt@uni-bremen.de>
 * @date 07/2011
 *
 * This is a separate file to let moc generate the required qt signals.
 */

#ifndef _UDPCOM_RECEIVE_THREAD_H_
#define _UDPCOM_RECEIVE_THREAD_H_

#include "NDLCom/udpcom.h"

#include <QThread>

namespace NDLCom
{
    class UdpCom::ReceiveThread: public QThread
    {
        Q_OBJECT
    public:
	ReceiveThread(NDLCom::UdpCom* wrapper): QThread(wrapper), mpWrapper(wrapper) {};
	~ReceiveThread() {};
        NDLCom::UdpCom* mpWrapper;
        void run(void);
        void stop(void);
        void setRecvPort(int);
    private:
        bool mContinueLoop;
        int mPort;
    signals:
        void dataReceived(const QByteArray&);
    };
};


#endif/*_UDPCOM_RECEIVE_THREAD_H_*/
