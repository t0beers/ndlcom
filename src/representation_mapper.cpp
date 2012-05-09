/**
 * @file src/representation_mapper.cpp
 * @author zenzes
 * @date 2011
 */

#include <QDebug>
#include <QMetaType>
#include "NDLCom/representation_mapper.h"
#include "NDLCom/message.h"

#include <cassert>
#include <math.h>

/* extra types of representation */
#include "representations/representation.h"
#include "representations/names.h"
#include "representations/id.h"
/* please keep this list alphabetically sorted */
#include "representations/acceleration.h"
#include "representations/angles.h"
#include "representations/ankle_joint_telemetry.h"
#include "representations/bldc_joint_telemetry.h"
#include "representations/debug_message.h"
#include "representations/distance_si1120.h"
#include "representations/dms_board_config.h"
#include "representations/dms_raw.h"
#include "representations/force_torque.h"
#include "representations/gripperstate.h"
#include "representations/imu_data.h"
#include "representations/Isp.h"
#include "representations/joint_angles.h"
#include "representations/memory.h"
#include "representations/ping.h"
#include "representations/register.h"
#include "representations/relay_board_msg_design.h"
#include "representations/scope.h"
#include "representations/sensor_array.h"
#include "representations/spine_telemetry.h"
#include "representations/support_polygon.h"
#include "representations/fpga_debug_message.h"
#include "representations/temperature.h"
#include "representations/leg_angles.h"
#include "representations/thermometer_ds18b20.h"
#include "representations/timestamp.h"
#include "representations/valve_control_board.h"
#include "representations/cam_tcm8230md.h"

NDLCom::RepresentationMapper::RepresentationMapper(QWidget* parent) : QWidget(parent)
{
    /* should be moved into main gui? */
    qRegisterMetaType<ProtocolHeader>("ProtocolHeader");

    /* please keep this list alphabetically sorted */
    qRegisterMetaType<Representations::Acceleration>("Representations::Acceleration");
    qRegisterMetaType<Representations::AnkleJointTelemetryMessage>("Representations::AnkleJointTelemetryMessage");
    qRegisterMetaType<Representations::Angles>("Representations::Angles");
    qRegisterMetaType<Representations::BLDCJointTelemetryMessage>("Representations::BLDCJointTelemetryMessage");
    qRegisterMetaType<Representations::DMSBoardConfig>("Representations::DMSBoardConfig");
    qRegisterMetaType<Representations::DebugMessage>("Representations::DebugMessage");
    qRegisterMetaType<Representations::DistanceSI1120>("Representations::DistanceSI1120");
    qRegisterMetaType<Representations::DmsRaw>("Representations::DmsRaw");
    qRegisterMetaType<Representations::ForceTorque>("Representations::ForceTorque");
    qRegisterMetaType<Representations::RepresentationsGripperState>("Representations::RepresentationsGripperState");
    qRegisterMetaType<Representations::IMUDataMessage>("Representations::IMUDataMessage");
    qRegisterMetaType<Representations::IspCommand>("Representations::IspCommand");
    qRegisterMetaType<Representations::IspData>("Representations::IspData");
    qRegisterMetaType<Representations::JointAngles>("Representations::JointAngles");
    qRegisterMetaType<Representations::MemoryData>("Representations::MemoryData");
    qRegisterMetaType<Representations::Ping>("Representations::Ping");
    qRegisterMetaType<Representations::RepresentationsPUState>("Representations::RepresentationsPUState");
    qRegisterMetaType<Representations::RegisterDescriptionResponse>("Representations::RegisterDescriptionResponse>");
    qRegisterMetaType<Representations::RegisterValueResponse>("RegisterValueResponse");
    qRegisterMetaType<Representations::RelayBoardTelemetry>("Representations::RelayBoardTelemetry");
    qRegisterMetaType<Representations::Scope_channelData>("Representations::Scope_channelData");
    qRegisterMetaType<Representations::SensorArray_calibration>("Representations::SensorArray_calibration");
    qRegisterMetaType<Representations::SensorArray_matrixData>("Representations::SensorArray_matrixData");
    qRegisterMetaType<Representations::SpineTelemetryMessage>("Representations::SpineTelemetryMessage");
    qRegisterMetaType<Representations::SupportPolygon>("Representations::SupportPolygon");
    qRegisterMetaType<Representations::FpgaDebugMessage>("Representations::FpgaDebugMessage");
    qRegisterMetaType<Representations::Temperature>("Representations::Temperature");
    qRegisterMetaType<Representations::LegAngles>("Representations::LegAngles");
    qRegisterMetaType<Representations::ThermometerDS18B20>("Representations::ThermometerDS18B20");
    qRegisterMetaType<Representations::RepresentationsValveControl>("Representations::RepresentationsValveControl");
    qRegisterMetaType<Representations::CAM_TCM8230MD>("Representations::CAM_TCM8230MD");
    qRegisterMetaType<NDLCom::Message>("NDLCom::Message");

    /* this is the Message-signal emitted by the derived class NDLCom. we connect it to here, so we
     * can map the incoming message to all type which are there */
    connect(this, SIGNAL(rxMessage(const NDLCom::Message&)), this, SLOT(slot_rxMessage(const NDLCom::Message&)));
    /* these are internal Messages generated by the Composer for example, which should be sent to
     * the Gui aswell. for testing purposes for example */
    connect(this, SIGNAL(internal_txMessage(const NDLCom::Message&)), this, SLOT(slot_rxMessage(const NDLCom::Message&)));
}

NDLCom::RepresentationMapper::~RepresentationMapper()
{
}

void NDLCom::RepresentationMapper::handleTimestampedData(const NDLCom::Message& msg)
{
    //check length
    if (msg.mHdr.mDataLen < sizeof(Representations::Timestamp))
    {
        qWarning() << "Received message too short for timestamp.";
        return;
    }

   //keep source and destination from the common header
    ProtocolHeader innerHeader(msg.mHdr);
    //data of the inner message starts after the timestamp data
    innerHeader.mDataLen = msg.mHdr.mDataLen - sizeof(Representations::Timestamp);
    NDLCom::Message innerMessage(msg.mTimestamp, innerHeader, msg.mpDecodedData + sizeof(Representations::Timestamp));

    //put timestamp from sender into message struct
    const Representations::Timestamp* pTimestamp = (const Representations::Timestamp*)msg.mpDecodedData;
    const uint64_t& micro(pTimestamp->mMicroseconds);
    innerMessage.mTimestampFromSender.tv_sec = micro / 1000000;
    innerMessage.mTimestampFromSender.tv_nsec = (micro * 1000) % 1000000000;

    //handle data in inner message
    slot_rxMessage(innerMessage);
}

void NDLCom::RepresentationMapper::slot_rxMessage(const NDLCom::Message& msg)
{
    /* than, we try to get a pointer to representation-data */
    const Representations::Representation* repreData = reinterpret_cast<const Representations::Representation*>(msg.mpDecodedData);

    try
    {
        bool hasFixedSize =
            (   repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_DebugMessage
             || repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_RegisterDescriptionResponse
             || repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_RepresentationsTimestamp
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
            else if(repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_RegisterDescriptionResponse)
            {
                //dynamic length: the description data follows the RegisterValueResponse struct.
                int structLength = sizeof(Representations::RegisterDescriptionResponse);
                int descrLength = msg.mHdr.mDataLen - structLength;
                const Representations::RegisterDescriptionResponse r = *(Representations::RegisterDescriptionResponse*)repreData;
                const QByteArray description((char*)repreData + structLength, descrLength);
                emit rxRepresentation(msg.mHdr,r,description);
            }
            else if (repreData->mId == REPRESENTATIONS_REPRESENTATION_ID_RepresentationsTimestamp)
            {
                //dynamic length: another representation follows this timestamp data
                const int secondReprLength = msg.mHdr.mDataLen - sizeof(Representations::Timestamp);
                if (secondReprLength >= 0)
                {
                    //call a function to handle this data
                    handleTimestampedData(msg);
                }
                else
                {
                    qDebug("received Timestamp message with unexpected size %d", msg.mHdr.mDataLen);
                }
            }
        }
        else
        {
            /* check size */
            const int expectedSize = Representations::getSize(repreData->mId);
            if (expectedSize == -1)
            {
                QString errormsg = QString("[%1->%2] unknown mBase->mId \"%3\"")
                    .arg(Representations::Names::getDeviceName(msg.mHdr.mSenderId))
                    .arg(Representations::Names::getDeviceName(msg.mHdr.mReceiverId))
                    .arg(repreData->mId);
                throw errormsg;
            }
            else if (expectedSize != msg.mHdr.mDataLen)
            {
                QString errormsg = QString("[%1->%2] Representation::%3 with wrong length: Got %4, expected %5.")
                    .arg(Representations::Names::getDeviceName(msg.mHdr.mSenderId))
                    .arg(Representations::Names::getDeviceName(msg.mHdr.mReceiverId))
                    .arg(Representations::Names::getRepresentationName(msg.mpDecodedData[0]))
                    .arg(msg.mHdr.mDataLen)
                    .arg(expectedSize);
                throw errormsg;
            }
            /* and switch to the correct signal */
            /* please keep this list alphabetically sorted */
            switch(repreData->mId)
            {
                case REPRESENTATIONS_REPRESENTATION_ID_Acceleration:
                    emit rxRepresentation(msg.mHdr, *(Representations::Acceleration*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_AnkleJointTelemetryMessage:
                    emit rxRepresentation(msg.mHdr, *(Representations::AnkleJointTelemetryMessage*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_Angles:
                    emit rxRepresentation(msg.mHdr, *(Representations::Angles*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_BLDCJointTelemetryMessage:
                    emit rxRepresentation(msg.mHdr, *(Representations::BLDCJointTelemetryMessage*)repreData);
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
                case REPRESENTATIONS_REPRESENTATION_ID_RepresentationsGripperState:
                    emit rxRepresentation(msg.mHdr, *(Representations::RepresentationsGripperState*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_IMUDataMessage:
                    emit rxRepresentation(msg.mHdr, *(Representations::IMUDataMessage*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_IspCommand:
                    emit rxRepresentation(msg.mHdr, *(Representations::IspCommand*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_IspData:
                    emit rxRepresentation(msg.mHdr, *(Representations::IspData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_JointAngles:
                    emit rxRepresentation(msg.mHdr, *(Representations::JointAngles*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_MemoryData:
                    emit rxRepresentation(msg.mHdr, *(Representations::MemoryData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RepresentationsPing:
                    emit rxRepresentation(msg.mHdr, *(Representations::Ping*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RepresentationsPUState:
                    emit rxRepresentation(msg.mHdr, *(Representations::RepresentationsPUState*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RegisterValueResponse:
                    emit rxRepresentation(msg.mHdr, *(Representations::RegisterValueResponse*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RelayBoardTelemetry:
                    emit rxRepresentation(msg.mHdr, *(Representations::RelayBoardTelemetry*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_Scope_channelData:
                    emit rxRepresentation(msg.mHdr, *(Representations::Scope_channelData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SensorArray_calibration:
                    emit rxRepresentation(msg.mHdr, *(Representations::SensorArray_calibration*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SensorArray_matrixData:
                    emit rxRepresentation(msg.mHdr, *(Representations::SensorArray_matrixData*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SpineTelemetryMessage:
                    emit rxRepresentation(msg.mHdr, *(Representations::SpineTelemetryMessage*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SupportPolygon:
                    emit rxRepresentation(msg.mHdr, *(Representations::SupportPolygon*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_FpgaDebugMessage:
                    emit rxRepresentation(msg.mHdr, *(Representations::FpgaDebugMessage*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_Temperature:
                    emit rxRepresentation(msg.mHdr, *(Representations::Temperature*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RepresentationsLegAngles:
                    emit rxRepresentation(msg.mHdr, *(Representations::LegAngles*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_ThermometerDS18B20:
                    emit rxRepresentation(msg.mHdr, *(Representations::ThermometerDS18B20*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_RepresentationsValveControl:
                    emit rxRepresentation(msg.mHdr, *(Representations::RepresentationsValveControl*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_CAM_TCM8230MD:
                    emit rxRepresentation(msg.mHdr, *(Representations::CAM_TCM8230MD*)repreData);
                    break;
                case REPRESENTATIONS_REPRESENTATION_ID_SheetMetalMessage:
                    emit rxRepresentation(msg.mHdr, *(Representations::SheetMetalMessage*)repreData);
                    break;
            }
        }
    }
    catch(QString errormsg)
    {
        qWarning("RepresentationMapper: %s",errormsg.toLatin1().data());
    }
}
