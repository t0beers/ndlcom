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
    class Acceleration;
    class CurrentValues;
    class DMSBoardConfig;
    class DebugMessage;
    class DistanceSI1120;
    class DmsRaw;
    class ForceTorque;
    class IMUDataMessage;
    class JointAngles;
    class MemoryData;
    class Ping;
    class RawData;
    class RegisterDescriptionResponse;
    class RegisterValueResponse;
    class SensorArray_channelData;
    class SensorArray_matrixData;
    class SensorArray_vectorData;
    class TelemetryValues;
    class BLDCJointTelemetryMessage;
    class Temperature;
    class TestlegAngles;
    class ThermometerDS18B20;
    class CAM_TCM8230MD;
    class SheetMetalHand;
    class RelayBoardTelemetry;
    class RelayBoardCommands;
};

class ProtocolHeader;

namespace NDLCom
{
    class Message;

    namespace Ui
    {
        class ShowRepresentations;
    };

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
        void rxRepresentation(const ProtocolHeader&, const Representations::BLDCJointTelemetryMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::CAM_TCM8230MD&);
        void rxRepresentation(const ProtocolHeader&, const Representations::CurrentValues&);
        void rxRepresentation(const ProtocolHeader&, const Representations::DMSBoardConfig&);
        void rxRepresentation(const ProtocolHeader&, const Representations::DistanceSI1120&);
        void rxRepresentation(const ProtocolHeader&, const Representations::DmsRaw&);
        void rxRepresentation(const ProtocolHeader&, const Representations::ForceTorque&);
        void rxRepresentation(const ProtocolHeader&, const Representations::IMUDataMessage&);
        void rxRepresentation(const ProtocolHeader&, const Representations::JointAngles&);
        void rxRepresentation(const ProtocolHeader&, const Representations::MemoryData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::Ping&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RegisterDescriptionResponse&, const QByteArray&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RegisterValueResponse&);
        void rxRepresentation(const ProtocolHeader&, const Representations::RelayBoardTelemetry&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SensorArray_channelData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SensorArray_matrixData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SensorArray_vectorData&);
        void rxRepresentation(const ProtocolHeader&, const Representations::SheetMetalHand&);
        void rxRepresentation(const ProtocolHeader&, const Representations::TelemetryValues&);
        void rxRepresentation(const ProtocolHeader&, const Representations::Temperature&);
        void rxRepresentation(const ProtocolHeader&, const Representations::TestlegAngles&);
        void rxRepresentation(const ProtocolHeader&, const Representations::ThermometerDS18B20&);

        /**
         *  exportString -- allowing a nice export facility to allow printing of data into files...
         *
         * @param type the representations name of the data contained in the second string
         * @param data the representations data formatted as a nice string, complete with newline
         *                and so on to be appended line by line to a logfile
         */
        void exportString(const QString type, const QString data);

    private:
        const char* exportDelimiter;
        char* pBuffer;
        QString printMessageHeader(const NDLCom::Message&);

        /** Function to extract the message after the timestamp-data and feed it to slot_rxMessage().
         * @param msg Message including the timestamp data
         */
        void handleTimestampedData(const ::NDLCom::Message& msg);

    private slots:
        void slot_rxMessage(const NDLCom::Message&);
    };

    /* just a simple Widget, which prints all devices and representation types... with their id */
    class ShowRepresentations : public QWidget
    {
        Q_OBJECT
    public:
        ShowRepresentations(QWidget* parent=0);
    };
};

#endif/*_NDLCOM_REPRESENTATION_MAPPER_H_*/
