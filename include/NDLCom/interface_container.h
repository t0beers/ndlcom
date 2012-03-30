/**
 * @file include/NDLCom/interface_container.h
 * @author Martin Zenzes
 * @date 2011
 */
#ifndef _NDLCOM_INTERFACE_CONTAINER_H_
#define _NDLCOM_INTERFACE_CONTAINER_H_

#include <vector>
#include <QWidget>
#include <QIcon>
#include <QMap>
#include <QList>
#include <QTimer>
#include <QMenu>

#include "NDLCom/representation_mapper.h"
#include "NDLCom/interface.h"

namespace NDLCom
{
    namespace Ui
    {
        class InterfaceContainer;
    };

    class Message;
    class RepresentationMapper;

    /**
     * @brief The main class for NDLCom
     *
     * this may be instantiated inside a gui-skeleton
     *
     * we have our own QMenu, which offers the possibility to disconnect running/active connections.
     *
     * we allow connections to be choosen by the user, using our NDLCom::Interface class, and then
     * emit NDLCom::Message objects to all interested other widgets. since we inheric
     * RepresentationMapper, the (there) mapped Representations are also emitted by "us".
     *
     * we also emit some status messages showing the current rx/tx rate as a string, aswell as a
     * single signal for each interface.
     */
    class InterfaceContainer : public RepresentationMapper
    {
        Q_OBJECT

    public:
        /* users can ask here for the pointer of this module */
        static NDLCom::InterfaceContainer* getInterfaceContainer();

        /** here, we will collect all disconnect-actions of the currently active and running interfaces */
        QMenu* mpDisconnect;
        /** for every kind of new interface, we need one action which knows how to create it */
        QAction* actionConnectSerial;
        /** for every kind of new interface, we need one action which knows how to create it */
        QAction* actionConnectUdp;
        /** removing all active connection in one click! */
        QAction* actionDisconnectAll;

    public slots:
        /** finished and valid telegram to be sent away into the world */
        void txMessage(const NDLCom::Message&);

    signals:
        /** current status (transferred data and rates) of all interfaces */
        void transferRate(const QString);

        /** this signal will print string/datarate combinations rx/tx bytes/rate, summed over all
         * interfaces each */
        void status(QString, double);

        /**
         * this signal emits the connection state to higher level instances, e.g. the control
         * of the processing flow of the application (e.g. starting/stopping of keep alive messaging */
        void connectionStatusChanged(bool _isConnected);

	protected:
        /**
         * @brief standard qt constructor...
         *
         * @param parent normally "this" of the main-window or something...
         */
        InterfaceContainer(QWidget* parent = 0);
        /**
         * @brief Destructor of InterfaceContainer class. Doing cleanup here
         */
        virtual ~InterfaceContainer();

    private:

        /** this list will contain all interfaces, which are connected right now */
        QList<Interface*> runningInterfaces;

        /** ... */
        Ui::InterfaceContainer* mpUi;

        /** printing the number of current connections into the "new connection" icon */
        QIcon printNumberOnIcon(QString, int number = 0, QColor color = Qt::black);

        /** these two are used two counter the number of active interfaces for display in the icons */
        int mRunningUdp;
        int mRunningSerial;

        /** for displaying overall data-rate. will keep transferred bytes forever. will remove
         * current rates for disconnected devices */

        std::map<Interface*, Interface::Statistics> mStatistics;

        /* to update als summarize the received transfer rates from all interfaces this timer */
        QTimer* mpGuiTimer;

        /** static pointer to the only instance */
        static InterfaceContainer* spInstance;

    private slots:
        void slot_rxMessage(const NDLCom::Message&);
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

#endif/*_NDLCOM_INTERFACE_CONTAINER_H_*/
