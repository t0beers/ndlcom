/**
 * @file src/representation_mapper.cpp
 * @author zenzes
 * @date 2011
 */

#include <QDebug>
#include <QMetaType>
#include "NDLCom/representation_mapper.h"
#include "NDLCom/message.h"

#include "ui_show_representations.h"

/* extra types of representation */
#include "representations/representation.h"
#include "representations/names.h"
#include "representations/id.h"
/* please keep this list alphabetically sorted */
#include "representations/accelerometer.h"
#include "representations/current_values.h"
#include "representations/debug_message.h"
#include "representations/distance_si1120.h"
#include "representations/dms_board_config.h"
#include "representations/dms_raw.h"
#include "representations/force_torque.h"
#include "representations/joint_angles.h"
#include "representations/memory.h"
#include "representations/register.h"
#include "representations/sensor_array.h"
#include "representations/telemetry_values.h"
#include "representations/temperature.h"
#include "representations/thermometer_ds18b20.h"

NDLCom::RepresentationMapper::RepresentationMapper(QWidget* parent) : QWidget(parent)
{
    /* should be moved into main gui? */
    qRegisterMetaType<ProtocolHeader>("ProtocolHeader");

    /* please keep this list alphabetically sorted */
    qRegisterMetaType<Representations::AccelerometerConfig>("Representations::AccelerometerConfig");
    qRegisterMetaType<Representations::AccelerometerData>("Representations::AccelerometerData");
    qRegisterMetaType<Representations::DMSBoardConfig>("Representations::DMSBoardConfig");
    qRegisterMetaType<Representations::DebugMessage>("Representations::DebugMessage");
    qRegisterMetaType<Representations::DistanceSI1120>("Representations::DistanceSI1120");
    qRegisterMetaType<Representations::DmsRaw>("Representations::DmsRaw");
    qRegisterMetaType<Representations::ForceTorque>("Representations::ForceTorque");
    qRegisterMetaType<Representations::JointAngles>("Representations::JointAngles");
    qRegisterMetaType<Representations::MemoryData>("Representations::MemoryData");
    qRegisterMetaType<Representations::RegisterDescriptionResponse>("Representations::RegisterDescriptionResponse>");
    qRegisterMetaType<Representations::RegisterValueResponse>("RegisterValueResponse");
    qRegisterMetaType<Representations::SensorArray_channelData>("Representations::mIdSensorArray_channelData");
    qRegisterMetaType<Representations::SensorArray_matrixData>("Representations::mIdSensorArray_matrixData");
    qRegisterMetaType<Representations::SensorArray_vectorData>("Representations::mIdSensorArray_vectorData");
    qRegisterMetaType<Representations::TelemetryValues>("Representations::TelemetryValues");
    qRegisterMetaType<Representations::Temperature>("Representations::Temperature");
    qRegisterMetaType<Representations::ThermometerDS18B20>("Representations::ThermometerDS18B20");

    connect(this, SIGNAL(rxMessage(const ::NDLCom::Message&)), this, SLOT(slot_rxMessage(const ::NDLCom::Message&)));
}

void NDLCom::RepresentationMapper::slot_rxMessage(const ::NDLCom::Message& msg)
{
    /* than, we try to get a pointer to representation-data */
    const Representations::Representation* repreData = reinterpret_cast<const Representations::Representation*>(msg.mpDecodedData);

    try
    {
        bool hasFixedSize =
            (   repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_DebugMessage
             || repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_MemoryData
             || repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_RegisterDescriptionResponse
            ) ? false : true;

        //handle packets with dynamic size
        //DebugMessage has a dynamic size and needs special treatment
        if (!hasFixedSize)
        {
            if(msg.mHdr.mDataLen < Representations::getSize(repreData->mId))
            {
                qWarning() << "Parser: packet too small.";
            }
            if(repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_DebugMessage)
            {
                const Representations::DebugMessage* msgHdr = reinterpret_cast<const Representations::DebugMessage*>(repreData);
                int structLength = sizeof(Representations::DebugMessage);
                QString messageText = QString::fromAscii((const char*)(msgHdr + structLength), msg.mHdr.mDataLen - structLength);
                const char* senderName = Representations::Names::getDeviceName(msg.mHdr.mSenderId);
                qDebug("%s (id=%d): %s", senderName, msg.mHdr.mSenderId, messageText.toAscii().data());
            }
            else if(repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_MemoryData)
            {
                //TODO: Fixed size or dynamic size?
                emit rxRepresentation(msg.mHdr, *(const Representations::MemoryData *)(repreData));
            }
            else if(repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_RegisterDescriptionResponse)
            {
                //dynamic length: the description data follows the RegisterValueResponse struct.
                int structLength = sizeof(Representations::RegisterDescriptionResponse);
                int descrLength = msg.mHdr.mDataLen - structLength;
                const Representations::RegisterDescriptionResponse r = *(Representations::RegisterDescriptionResponse*)repreData;
                const QByteArray description((char*)repreData + structLength, descrLength);
                emit rxRepresentation(msg.mHdr,r,description);
            }
        }
        else
        {
            /* check size */
            const ssize_t expectedSize = Representations::getSize(repreData->mId);
            if (expectedSize != msg.mHdr.mDataLen)
            {
                throw size_t(expectedSize);
            }
            /* and switch to the correct signal */
            /* please keep this list alphabetically sorted */
            switch(repreData->mId)
            {
                case REPRESENTATIONS_REPRESENTATION_ID_AccelerometerConfig:
                    emit rxRepresentation(msg.mHdr, *(Representations::AccelerometerConfig*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_AccelerometerData:
                    emit rxRepresentation(msg.mHdr, *(Representations::AccelerometerData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_CurrentValues:
                    emit rxRepresentation(msg.mHdr, *(Representations::CurrentValues*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_DMSBoardConfig:
                    emit rxRepresentation(msg.mHdr, *(Representations::DMSBoardConfig*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_DistanceSI1120:
                    emit rxRepresentation(msg.mHdr, *(Representations::DistanceSI1120*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_DmsRaw:
                    emit rxRepresentation(msg.mHdr, *(Representations::DmsRaw*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_ForceTorque:
                    emit rxRepresentation(msg.mHdr, *(Representations::ForceTorque*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_JointAngles:
                    emit rxRepresentation(msg.mHdr, *(Representations::JointAngles*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_MemoryData:
                    emit rxRepresentation(msg.mHdr, *(Representations::MemoryData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RegisterValueResponse:
                    emit rxRepresentation(msg.mHdr, *(Representations::RegisterValueResponse*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SensorArray_channelData:
                    emit rxRepresentation(msg.mHdr, *(Representations::SensorArray_channelData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SensorArray_matrixData:
                    emit rxRepresentation(msg.mHdr, *(Representations::SensorArray_matrixData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SensorArray_vectorData:
                    emit rxRepresentation(msg.mHdr, *(Representations::SensorArray_vectorData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_TelemetryValues:
                    emit rxRepresentation(msg.mHdr, *(Representations::TelemetryValues*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_Temperature:
                    emit rxRepresentation(msg.mHdr, *(Representations::Temperature*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_ThermometerDS18B20:
                    emit rxRepresentation(msg.mHdr, *(Representations::ThermometerDS18B20*)repreData);
                    break;
            }
        }
    }
    catch(size_t wrongLength)
    {
        qWarning() << tr("Parser: Received data with wrong "
                         "length from %1: Got %2 but expected %3.")
            .arg(Representations::Names::getDeviceName(msg.mHdr.mSenderId))
            .arg(msg.mHdr.mDataLen)
            .arg(wrongLength);
    }
}

/* constructor of little widget to show all Representation-names and Device-name with their Id ! */
NDLCom::ShowRepresentations::ShowRepresentations(QWidget* parent) : QWidget(parent)
{
    Ui::ShowRepresentations* ui = new Ui::ShowRepresentations();
    ui->setupUi(this);

    for (int id = 0;id<255;id++)
    {
        if (representationsNamesGetDeviceName(id))
        {
            QString name(representationsNamesGetDeviceName(id));
            QString str = QString("0x%1").arg(id,2,16,QChar('0'));
            ui->deviceForm->addRow(str, new QLabel(name,this));
        }
    }

    for (int id = 0;id<255;id++)
    {
        if (representationsNamesGetRepresentationName(id))
        {
            QString name(representationsNamesGetRepresentationName(id));
            QString str = QString("0x%1").arg(id,2,16,QChar('0'));
            ui->repreForm->addRow(str, new QLabel(name,this));
        }
    }
}

