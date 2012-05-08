/**
 * @file include/NDLCom/representation_mapper.h
 * @author zenzes
 * @date 2011
 */

#ifndef _NDLCOM_REPRESENTATION_MAPPER_H_
#define _NDLCOM_REPRESENTATION_MAPPER_H_

#include <QWidget>

namespace Representations
{
    struct Representation;
    /* please keep this list alphabetically sorted */
    struct Acceleration;
    struct AnkleJointTelemetryMessage;
    struct Angles;
    struct BLDCJointTelemetryMessage;
    struct FpgaDebugMessage;
    struct CurrentValues;
    struct DMSBoardConfig;
    struct DebugMessage;
    struct DistanceSI1120;
    struct DmsRaw;
    struct ForceTorque;
    struct RepresentationsGripperState;
    struct IMUDataMessage;
    struct IspCommand;
    struct IspData;
    struct JointAngles;
    struct MemoryData;
    struct Ping;
    struct RepresentationsPUState;
    struct RawData;
    struct RegisterDescriptionResponse;
    struct RegisterValueResponse;
    struct Scope_channelData;
    struct SensorArray_calibration;
    struct SensorArray_matrixData;
    struct SpineTelemetryMessage;
    struct SupportPolygon;
    struct Temperature;
    struct LegAngles;
    struct ThermometerDS18B20;
    struct CAM_TCM8230MD;
    struct SheetMetalMessage;
    struct RelayBoardTelemetry;
    struct RepresentationsValveControl;
    struct RelayBoardCommands;
};

struct ProtocolHeader;

namespace NDLCom
{
    class Message;

    class RepresentationMapper : public QWidget
    {
        Q_OBJECT
    public:
        RepresentationMapper(QWidget* parent = 0);
        virtual ~RepresentationMapper();

    signals:
        /** this signal is emitted from the derived class (NDLCom, ndlcom.cpp), not here!!! */
        void rxMessage(const NDLCom::Message&);
        /** this signal is emitted from the derived class. it contains packages sent from
         * gui-internal, to be received again...? */
        void internal_txMessage(const NDLCom::Message&);
        /** for specialized modules: correctly detected packages. should remove some load. */
        /* please keep this list alphabetically sorted! */
        void rxRepresentation(const ProtocolHeader&, const Representations::Acceleration&);
        void rxRepresentation(const ProtocolHeader&, const Representations::AnkleJointTelemetryMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::Angles&);
        void rxRepresentation(const ProtocolHeader&, const Representations::BLDCJointTelemetryMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::FpgaDebugMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::CAM_TCM8230MD&);
        void rxRepresentation(const ProtocolHeader&, const Representations::CurrentValues&);
        void rxRepresentation(const ProtocolHeader&, const Representations::DMSBoardConfig&);
        void rxRepresentation(const ProtocolHeader&, const Representations::DistanceSI1120&);
        void rxRepresentation(const ProtocolHeader&, const Representations::DmsRaw&);
        void rxRepresentation(const ProtocolHeader&, const Representations::ForceTorque&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RepresentationsGripperState&);
        void rxRepresentation(const ProtocolHeader&, const Representations::IMUDataMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::IspCommand&);
        void rxRepresentation(const ProtocolHeader&, const Representations::IspData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::JointAngles&);
        void rxRepresentation(const ProtocolHeader&, const Representations::MemoryData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::Ping&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RepresentationsPUState&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RegisterDescriptionResponse&, const QByteArray&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RegisterValueResponse&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RelayBoardTelemetry&);
        void rxRepresentation(const ProtocolHeader&, const Representations::Scope_channelData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SensorArray_calibration&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SensorArray_matrixData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SheetMetalMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SpineTelemetryMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SupportPolygon&);
        void rxRepresentation(const ProtocolHeader&, const Representations::Temperature&);
        void rxRepresentation(const ProtocolHeader&, const Representations::LegAngles&);
        void rxRepresentation(const ProtocolHeader&, const Representations::ThermometerDS18B20&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RepresentationsValveControl&);

    private:
        /** Function to extract the message after the timestamp-data and feed it to slot_rxMessage().
         * @param msg Message including the timestamp data
         */
        void handleTimestampedData(const ::NDLCom::Message& msg);

    private slots:
        void slot_rxMessage(const NDLCom::Message&);
    };

};

#endif/*_NDLCOM_REPRESENTATION_MAPPER_H_*/
