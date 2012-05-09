#include "NDLCom/serialcom.h"

#include "serialcom/serialcom.h"
#include "serialcom/serialcom_connect_dialog.h"
#include "protocol/protocol.h"

#include <QAction>
#include <QTimer>
#include <QDebug>

/* using namespace NDLCom is not possible because lowlevel object UdpCom has the same name... */

#define INTERFACETYPE "Serial"

bool NDLCom::Serialcom::popupConnectDialogAndTryToConnect()
{
    ::Serialcom::SerialcomConnectDialog connectDialog;

    if (connectDialog.exec()!=QDialog::Accepted)
        return false;

    portName = connectDialog.getPortName();
    baudRate = connectDialog.getBaudRate();
    parity = connectDialog.getParity();

    actionConnect->activate(QAction::Trigger);

    return true;
}

NDLCom::Serialcom::Serialcom(QObject* parent): Interface(parent)
{
    mInterfaceType = INTERFACETYPE;
    mpPorthandler = NULL;

    mpProtocolBuffer = new char[65535];
    mpProtocolParser = protocolParserCreate(mpProtocolBuffer, 65535);
}

NDLCom::Serialcom::~Serialcom()
{
    if (mpPorthandler)
    {
        delete mpPorthandler;
        mpPorthandler = NULL;
    }

    delete[] mpProtocolBuffer;
}

void NDLCom::Serialcom::on_actionConnect_triggered()
{
    if (mpPorthandler)
    {
        qWarning() << "NDLCom::Serialcom::on_actionConnect_triggered() tried to connect, altough I'm connected";
        return;
    }

    mpPorthandler = new ::Serialcom::Serialcom(this);

    if (mpPorthandler->connect(portName.toAscii().constData(),baudRate,parity))
    {
        connect(mpPorthandler,SIGNAL(lostConnection()), this, SLOT(on_actionDisconnect_triggered()));
        connect(mpPorthandler,SIGNAL(newData(const QByteArray&)), this, SLOT(receivedData( const QByteArray&)));
        connect(mpPorthandler,SIGNAL(dataTransmitted(const QByteArray&)), this, SLOT(updateTxBytes(const QByteArray&)));

        actionDisconnect->setText("Disconnect "+portName);

        setDeviceName("Serialport "+portName);

        emit connected();
    }
}

void NDLCom::Serialcom::on_actionDisconnect_triggered()
{
    if (!mpPorthandler)
    {
        qWarning() << "NDLCom::Serialcom::on_actionDisconnect_triggered() tried to disconnect, altough I'm not connected";
        return;
    }

    portName = "";
    mpPorthandler->disconnect();
    delete mpPorthandler;
    mpPorthandler = NULL;

    emit disconnected();
}

void NDLCom::Serialcom::txMessage(const NDLCom::Message& msg)
{
    if (mPaused)
        return;

    char buffer[1024];
    int len = protocolEncode(buffer, 1024, &msg.mHdr, (const void *)msg.mpDecodedData);
    mpPorthandler->send(QByteArray::fromRawData(buffer, len));
}

void NDLCom::Serialcom::updateTxBytes(const QByteArray& data)
{
	/* update bytes going out */
    mTxBytes += data.length();
	/* and notify other object that we actually sent some data */
    emit txRaw(data);
}

void NDLCom::Serialcom::receivedData(const QByteArray& encodedData)
{
    if (mPaused)
        return;

    int datalen = encodedData.size();
    const void* pData = (const void*)(encodedData.constData());

    int parsed = 0;
    while (parsed < datalen)
    {
        int r = protocolParserReceive(mpProtocolParser, (uint8_t*)pData+parsed, datalen-parsed);
        if (r == -1)
        {
            qDebug() << "NDLCom::Serialcom::receivedData() parser returned -1...";
            return;
        }
        parsed += r;

        if (protocolParserHasPacket(mpProtocolParser))
        {
            const void* data = protocolParserGetPacket(mpProtocolParser);
            const ProtocolHeader* hdr = protocolParserGetHeader(mpProtocolParser);
            Message msg(hdr, data);

            mRxBytes += msg.msg_size();

            emit rxMessage(msg);

            protocolParserDestroyPacket(mpProtocolParser);
        }
    }

    /* also, we wanna have somethin like raw-traffic displays... */
    emit rxRaw(encodedData);
}
