/**
 * @file lib/NDLCom/include/NDLCom/serialcom.h
 * @brief Qt wrapper (child of QObject) around lib/serialcom.
 * Project: iStruct
 * @date 07/2011
 * @author Armin
 */

#ifndef _NDLCOM_SERIALCOM_H_
#define _NDLCOM_SERIALCOM_H_

#include "NDLCom/interface.h"

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 * @addtogroup Communication_NDLCom_Serialcom serialcom
 * @{
 */

namespace Serialcom
{
    class Serialcom;
}

struct ProtocolParser;

namespace NDLCom
{
    /**
     * @brief widget to use lib/serialcom from qt.
     */
    class Serialcom: public Interface
    {
        Q_OBJECT
    public:
        Serialcom(QObject *parent = NULL);
        virtual ~Serialcom();

        /* these three things are the settings used to connect the device */
        QString portName;
        int baudRate;
        int parity;

        /* will pop up a dialoge where the user might change the current settings */
        bool popupConnectDialogAndTryToConnect();

    private slots:
        void txMessage(const NDLCom::Message&);

        void receivedData(const QByteArray&);

        void on_actionConnect_triggered();
        void on_actionDisconnect_triggered();
        void updateTxBytes(const QByteArray&);

    private:
        ::Serialcom::Serialcom* mpPorthandler;

        ProtocolParser* mpProtocolParser;
        char* mpProtocolBuffer;
    };
};

/**
 * @}
 * @}
 * @}
 */
#endif/*_NDLCOM_SERIALCOM_H_*/

