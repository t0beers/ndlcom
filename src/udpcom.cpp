#include "udpcom/udpcom.h"
#include "NDLCom/udpcom.h"
#include "udpcom_receive_thread.h"
#include "protocol/protocol.h"
#include <QThread>
#include <QDebug>
#include <QtNetwork>
#include <QHostInfo>

#include <iostream>
#include <cassert>


NDLCom::UdpCom::UdpCom(QWidget* parent): Interface(parent)
{
    setObjectName("UdpCom");

    mpTransmitSocket = NULL;
    mpReceiveThread = NULL;

    setInterfaceType("UDP");
}

NDLCom::UdpCom::~UdpCom()
{
    if (mpReceiveThread)
    {
        mpReceiveThread->stop();
        mpReceiveThread->wait(500); //wait max 500ms until thread stops
        delete mpReceiveThread;
        mpReceiveThread = NULL;
    }
    if (mpTransmitSocket)
    {
        delete mpTransmitSocket;
        mpTransmitSocket = NULL;
    }
}

void NDLCom::UdpCom::on_actionConnect_triggered()
{
    if (mpTransmitSocket)
    {
        qWarning() << "NDLCom::UdpCom::on_actionConnect_triggered() tried to connect, altough I'm connected";
        return;
    }

    UdpComConnectDialog connectDialog;

    if (connectDialog.exec()==QDialog::Accepted)
    {
        QString hostname = connectDialog.getHostname();
        QHostAddress address(hostname);
        if (QAbstractSocket::IPv4Protocol == address.protocol())
        {
            qDebug() << "UdpCom: connecting to valid IPv4 address" << hostname;
        }
        else
        {
            /* translate the string to a ip. if a hostname was given, a ping lookup is performed.
             * if a ip was given in the string, it is returned */
            qDebug() << "UdpCom: performing lookup for" << hostname;
            hostname = QHostInfo::fromName(hostname).addresses().first().toString();
            qDebug() << "UdpCom: got address" << hostname;
        }

        int sendport = connectDialog.getSendport();

        mpTransmitSocket = new ::UdpCom::UdpCom();

        mpTransmitSocket->setTarget(hostname.toStdString().c_str(), sendport);

        mpReceiveThread = new ReceiveThread(this);
        mpReceiveThread->setRecvPort(connectDialog.getRecvport());

        /* create and start new thread, which will execute it's run() function. */
        connect(mpReceiveThread, SIGNAL(dataReceived(const QByteArray&)),
                this, SLOT(dataFromReceiveThread(const QByteArray&)));

        mpReceiveThread->start();

        setInterfaceType("UDP ["+hostname+QString(":")+QString::number(sendport)+QString("]"));

        emit connected();
    }
}

void NDLCom::UdpCom::on_actionDisconnect_triggered()
{
    if (!mpTransmitSocket || !mpReceiveThread)
    {
        qWarning() << "NDLCom::UdpCom::on_actionDisconnect_triggered() tried to disconnect, altough I'm not connected";
        return;
    }

    if (mpReceiveThread)
    {
        mpReceiveThread->stop();
        mpReceiveThread->wait(500);
        delete mpReceiveThread;
        mpReceiveThread = NULL;
    }
    if (mpTransmitSocket)
    {
        delete mpTransmitSocket;
        mpTransmitSocket = NULL;
    }

    emit disconnected();
}

void NDLCom::UdpCom::txMessage(const ::NDLCom::Message& msg)
{
    if (mPaused)
        return;

    char buffer[1024];
    int len = protocolEncodeForUDP(buffer, 1024, &msg.mHdr, (const void *)msg.mpDecodedData);
    mpTransmitSocket->write(QByteArray::fromRawData(buffer, len), len);

	/* we don't get any response from the transmit object which data was actually written, so we
	 * hope the best */
    emit txRaw(QByteArray::fromRawData(buffer, len));
    mTxBytes += len;
}

void NDLCom::UdpCom::dataFromReceiveThread(const QByteArray& encodedData)
{
    if (mPaused)
        return;

    /*
     * only parse this packet. we have to do this, because udp is paket oriented and unescaped.
     * parser would break if length is wrong, once.
     */
    char buffer[65535];
    ProtocolParser* parser = protocolParserCreate(buffer, (uint16_t)sizeof(buffer));
    protocolParserSetFlag(parser, PROTOCOL_PARSER_DISABLE_FRAMING);
    int parsed = 0;
    int notparsed = encodedData.size();
    const char* data = encodedData.data();

    while(notparsed > parsed)
    {
        int r = protocolParserReceive(parser, data, notparsed);
        if (r == -1)
        {
            qDebug() << "Parser: UdpCom_dataReceived(): protocolParserReceive returned -1...";
            return;
        }
        else
        {
            data += r;
            notparsed -= r;
        }
        
        if (protocolParserHasPacket(parser))
        {
            const void* packet = protocolParserGetPacket(parser);
            const ProtocolHeader* hdr = protocolParserGetHeader(parser);
            /* will be timestampled automatically */
            NDLCom::Message msg(hdr, packet);

            emit rxMessage(msg);

            mRxBytes += msg.msg_size();

            protocolParserDestroyPacket(parser);
        }
        else
        {
            ProtocolParserState state;
            protocolParserGetState(parser, &state);
            qDebug() << "Could not read NDLCom packet inside UDP packet."
                     << "Parser state: "
                     << protocolParserStateName[state.mState];
        }
    }

    /* also, we wanna have somethin like raw-traffic displays... */
    emit rxRaw(encodedData);
}

