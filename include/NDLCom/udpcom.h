/**
 * @file NDLCom/include/NDLCom/udpcom.h
 * @brief Qt wrapper (child of QObject) around lib/udpcom.
 * Project: iStruct
 * @date 07/2011
 * @author Armin, Martin Zenzes
 */

#ifndef _NDLCOM_UDPCOM_H_
#define _NDLCOM_UDPCOM_H_

#include <QObject>
#include <QDialog>

#include "ui_udpcom_connect_dialog.h"

#include "NDLCom/interface.h"

namespace UdpCom
{
    class UdpCom;
};

namespace NDLCom
{
    namespace Ui
    {
        class UdpComConnectDialog;
    };

    /**
     * @brief widget to use lib/udpcom from qt.
     */
    class UdpCom: public Interface
    {
        Q_OBJECT
    public:
        UdpCom(QWidget *parent=0);
        virtual ~UdpCom();

    private slots:
        void txMessage(const ::NDLCom::Message&);
        void dataFromReceiveThread(const QByteArray&);

        void on_actionConnect_triggered();
        void on_actionDisconnect_triggered();

    private:
        class ReceiveThread;
        ::UdpCom::UdpCom* mpTransmitSocket;
        ReceiveThread* mpReceiveThread;
        friend class ReceiveThread;
    };

    /**
     * @brief Connect dialog...
     */
    class UdpComConnectDialog : public QDialog
    {
        Q_OBJECT
    public:
        UdpComConnectDialog(QDialog* parent=0) : QDialog(parent)
        {
            mpUi = new Ui::UdpComConnectDialog();
            mpUi->setupUi(this);
            connect(mpUi->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
            connect(mpUi->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        };
        ~UdpComConnectDialog(){ };

        QString getHostname() { return mpUi->hostname->currentText(); }
        int getRecvport()     { return mpUi->recvport->currentText().toInt(); }
        int getSendport()     { return mpUi->sendport->currentText().toInt(); }

    private:
        Ui::UdpComConnectDialog* mpUi;
    };
};

#endif/*_NDLCOM_UDPCOM_H_*/

