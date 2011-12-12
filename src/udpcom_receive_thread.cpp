#include "udpcom/udpcom.h"
#include "udpcom_receive_thread.h"

#include <QDebug>

#include <cassert>
#include <iostream>

/**
 * @brief Thread-function of the internal ReceiveThread class.
 */
void NDLCom::UdpCom::ReceiveThread::run(void)
{
    ::UdpCom::UdpCom mUdpCom;
    int r;
    r = mUdpCom.bind("0.0.0.0", mPort);
    assert(r);
    r = mUdpCom.setBlocking(false);
    assert(r);
    mContinueLoop = true;
    while(mContinueLoop)
    {
        char buffer[65535];
        /* given timeout is 500ms */
        int r = mUdpCom.readWithTimeout(buffer, sizeof(buffer), 500);
        if (r == -1)
        {
            //TODO: UdpCom::readWithTimeout()
//            assert(errno == EAGAIN);
        }
        else
        {
            emit dataReceived(QByteArray(buffer, r));
        }
    }
}

void NDLCom::UdpCom::ReceiveThread::stop()
{
    mContinueLoop = false;
}

void NDLCom::UdpCom::ReceiveThread::setRecvPort(int port)
{
    mPort = port;
}

