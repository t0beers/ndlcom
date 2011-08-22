/**
 * @file include/NDLCom/ndlcom.h
 * @author Martin Zenzes
 * @date 2011
 */
#ifndef _NDLCOM_NDLCOM_H
#define _NDLCOM_NDLCOM_H

#include <QWidget>
#include <QToolBar>
#include <QMenu>
#include <QList>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QColor>

#include "NDLCom/representation_mapper.h"

class Serialcom;

namespace NDLCom
{
    namespace Ui
    {
        class NDLCom;
    };

    class Message;
    class Interface;
    class RepresentationMapper;

    /**
     * @brief The main class for NDLCom
     *
     * this may be instantiated inside a gui-skeleton
     *
     * ATTENTION: we try to cast the parent-pointer to QMainWindow*, and if this is successfull we
     * add some QDockWidget in there -- this is the intended use-case inside iLeggui
     *
     * we have our own QToolBar and QMenu, which both offer some functionality for the user. add
     * this widget and its menu and toolbar to your application.
     *
     * we allow conenctions to be choosen by the user, using our NDLCom::Interface class, and then
     * emit NDLCom::Message objects to all interested other widgets.
     *
     */
    class NDLCom : public RepresentationMapper
    {
        Q_OBJECT;
    public:
        /**
         * @brief standard qt constructor...
         *
         * @param parent normally "this" of the main-window or something...
         */
        NDLCom(QWidget* parent = 0);
        /**
         * @brief Destructor of NDLCom class. Doing cleanup here
         */
        virtual ~NDLCom();

        /** here, we will collect all disconnect-actions of the currently active and running interfaces */
        QMenu* mpDisconnect;
        /** for every kind of new interface, we need one action which knows how to create it */
        QAction* actionConnectSerial;
        /** for every kind of new interface, we need one action which knows how to create it */
        QAction* actionConnectUdp;
        /** removing all active connection in one click! */
        QAction* actionDisconnectAll;

        /** received and decoded a telegram, with timestamp */
        //void rxMessage(const ::NDLCom::Message&);
    public slots:
        /** finished and valid telegram to be sent away into the world */
        void txMessage(const ::NDLCom::Message&);

    signals:
        /** current status (transferred data and rates) of all interfaces */
        void transferRate(const QString);

        /** this signal will print string/datarate combinations rx/tx bytes/rate, summed over all
         * interfaces each */
        void status(QString, double);

    private:

        /** this list will contain all interfaces, which are connected right now */
        QList<Interface*> runningInterfaces;
        /** ... */
        Ui::NDLCom* mpUi;
        /** printing the number of current connections into the "new connection" icon */
        QIcon printNumberOnIcon(QString, int number = 0, QColor color = Qt::black);

        /** these two are used two counter the number of active interfaces for display in the icons */
        int mRunningUdp;
        int mRunningSerial;

        /** for displaying overall data-rate. will keep transferred bytes forever. will remove
         * current rates for disconnected devices */
        QMap<void*, double> mMapRxRate;
        QMap<void*, double> mMapTxRate;
        QMap<void*, double> mMapRxBytes;
        QMap<void*, double> mMapTxBytes;

        /* to update als summarize the received transfer rates from all interfaces this timer */
        QTimer* mpGuiTimer;

    private slots:
        void slot_rxMessage(const ::NDLCom::Message&);
        void slot_rxRate(double);
        void slot_txRate(double);
        void slot_rxBytes(double);
        void slot_txBytes(double);
        void connected();
        void disconnected();
        void on_actionConnectSerial_triggered();
        void on_actionConnectUdp_triggered();
        void on_actionDisconnectAll_triggered();
        void on_mpGuiTimer_timeout();
    };
};

#endif/*_NDLCOM_NDLCOM_H_*/

