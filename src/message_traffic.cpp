/**
 * @file src/message_traffic.cpp
 * @author zenzes
 * @date 2011
 */

#include <QDebug>

#include "NDLCom/message_traffic.h"
#include "NDLCom/message.h"
#include "representations/representation.h"
#include "representations/names.h"
#include "protocol/protocol.h"

#include "ui_traffic.h"

#include <QDateTime>

NDLCom::MessageTraffic::MessageTraffic(QWidget* parent) : QWidget(parent)
{
    mpUi = new Ui::Traffic;
    mpUi->setupUi(this);
}

void NDLCom::MessageTraffic::sentMessage(const NDLCom::Message& msg)
{
    mpUi->plainTextEdit_Tx->appendPlainText(formatMessage(msg));
}
void NDLCom::MessageTraffic::receivedMessage(const NDLCom::Message& msg)
{
    mpUi->plainTextEdit_Rx->appendPlainText(formatMessage(msg));
}

QString NDLCom::MessageTraffic::formatMessage(const NDLCom::Message &msg)
{
    const ProtocolHeader* hdr = &msg.mHdr;
    const Representations::Representation* decodedData = reinterpret_cast<const Representations::Representation*>(msg.mpDecodedData);

    uint8_t id = 0; //ERROR
    if (hdr->mDataLen > 0)
    {
        id = decodedData->mId;
    }
    /* TODO: how to know the number of CRC-errors and the current parser state nicely? */
    QString repr = Representations::Names::getRepresentationName(id);
    if (repr.isEmpty())
        repr = QString("type 0x%1").arg(decodedData->mId,0,16);

    QString sender = Representations::Names::getDeviceName(hdr->mSenderId);
    if (sender.isEmpty())
        sender = QString("id 0x%1").arg(hdr->mSenderId,0,16);

    QString receiver = Representations::Names::getDeviceName(hdr->mReceiverId);
    if (receiver.isEmpty())
        receiver = QString("id 0x%1").arg(hdr->mReceiverId,0,16);

    QString newline;

    newline = QString("New telegram: ")
              + repr
              + QString(", sent from ")
              + sender
              + QString(" to ")
              + receiver
              + QString("\n")
              + QString(" counter: ")
              + QString::number(hdr->mCounter)
              + QString(" dataLength: ")
              + QString::number(hdr->mDataLen)
              + QString(" Bytes");

#if QT_VERSION >= 0x040700
    long int time_ms = msg.mTimestamp.tv_sec*1000.0 + msg.mTimestamp.tv_nsec/1000000.0 + 0.5;
    newline += QString(" [") + QDateTime::fromMSecsSinceEpoch(time_ms).toString("HH:mm:ss-zzz") + QString("]");
#endif

    return newline;
}

