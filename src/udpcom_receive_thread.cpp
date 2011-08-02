#include "udpcom/udpcom.h"
#include "udpcom_receive_thread.h"

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
        int r = mUdpCom.read(buffer, sizeof(buffer), 0);
        if (r == -1)
        {
            //TODO: UdpCom::readWithTimeout()
            usleep(20000);
        }
        else
        {
            std::cerr << "UdpCom: got " << r << " Bytes." << std::endl;
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

