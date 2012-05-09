/**
 * @file NDLCom/include/NDLCom/udpcom.h
 * @brief Qt wrapper (child of QObject) around lib/udpcom.
 * Project: iStruct
 * @date 07/2011
 * @author Armin, Martin Zenzes
 */

#ifndef _NDLCOM_UDPCOM_H_
#define _NDLCOM_UDPCOM_H_

#include "NDLCom/interface.h"

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 * @addtogroup Communication_NDLCom_UDPcom UDPCom
 * @{
 */

namespace UdpCom
{
    class UdpCom;
};

namespace NDLCom
{
    /**
     * @brief widget to use lib/udpcom from qt.
     */
    class UdpCom: public Interface
    {
        Q_OBJECT

    public:
        UdpCom(QObject *parent = NULL);
        virtual ~UdpCom();

        /* these three things denote a connection */
        QString addressString;
        int sendPort;
        int recvPort;

        /* asks the user to enter some settings before connection */
        bool popupConnectDialogAndTryToConnect();

    private slots:
        void txMessage(const NDLCom::Message&);
        void dataFromReceiveThread(const QByteArray&);

        void on_actionConnect_triggered();
        void on_actionDisconnect_triggered();

    private:
        class ReceiveThread;
        ::UdpCom::UdpCom* mpTransmitSocket;
        ReceiveThread* mpReceiveThread;
        friend class ReceiveThread;
    };


}; //of namespace

/**
 * @}
 * @}
 * @}
 */
#endif/*_NDLCOM_UDPCOM_H_*/
