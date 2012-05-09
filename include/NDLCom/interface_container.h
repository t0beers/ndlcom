/**
 * @file include/NDLCom/interface_container.h
 * @author Martin Zenzes
 * @date 2011, 09-05-2012
 */
#ifndef _NDLCOM_INTERFACE_CONTAINER_H_
#define _NDLCOM_INTERFACE_CONTAINER_H_

#include <QObject>
#include <QMap>
#include <QIcon>
#include <QList>

#include "NDLCom/representation_mapper.h"
#include "NDLCom/interface.h"

class QIcon;
class QAction;
class QTimer;

namespace NDLCom
{

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
        /** FIXME to be removed. the name of this function is missleading/inconsistent... use the
         * alternative "getInstance()" */
        static NDLCom::InterfaceContainer* getInterfaceContainer();
        /** users can ask here for the pointer of this module */
        static NDLCom::InterfaceContainer* getInstance();

        /* will return the names of all known interfaces types, which are active in the moment */
        QStringList getInterfaceTypes();
        /** will return a list of all existing interfaces of a specific type */
        QList<Interface*> getActiveInterfaces(QString interfaceType);
        /** will return the number of _all_ active interfaces */
        int getNumberOfActiveInterfaces();

        /** removing all active connection in one click! */
        QAction* actionDisconnectAll;

        /** action which is checkable. if enabled, all received messages will be echoed on all other
         * interfaces. attention: this is merely experimental and dangerous */
        QAction* actionBridging;

    public slots:
        /** finished and valid telegram to be sent away into the world */
        void txMessage(const NDLCom::Message&);

    signals:
        /** current status (overall transferred data and current rates) of all interfaces, formatted
         * in one string  */
        void transferRate(const QString);

        /** this signal will print a string holding rx/tx bytes and current rates, summed over all
         * interfaces each, given in kB/s and suitable for the istruct plotter */
        void status(QString, double);

        /**
         * this signal emits the connection state to higher level instances, e.g. the control
         * of the processing flow of the application (e.g. starting/stopping of keep alive messaging
         * TODO use number of active interfaces */
        void connectionStatusChanged(bool _isConnected);

	protected:
        /**
         * @brief standard qt constructor...
         *
         * @param parent normally "this" of the main-window or something...
         */
        InterfaceContainer(QObject* parent = NULL);
        /**
         * @brief Destructor of InterfaceContainer class. Doing cleanup here
         */
        virtual ~InterfaceContainer();

    private:

        /** this time is used to generate the status strings and statistics */
        QTimer* mpStatisticsTimer;

        /** this map contains the mapping of a "interfaceType" name to the list containing all
         * interfaces of this type */
        QMap<QString, QList<Interface*>* > activeInterfaces;

        /** a map holding some statistical data. this map is _not_ cleared by disconnected
         * interfaces! */
        std::map<Interface*, Interface::Statistics> mStatistics;

        /** static pointer to the singleton instance */
        static InterfaceContainer* spInstance;

    private slots:

        void slot_rxMessage(const NDLCom::Message&);
        void slot_rxRate(double);
        void slot_txRate(double);
        void slot_rxBytes(double);
        void slot_txBytes(double);

        void connected();
        void disconnected();
        void updateIcons();

        void on_mpStatisticsTimer_timeout();
        void on_actionDisconnectAll_triggered();
    };
};

#endif/*_NDLCOM_INTERFACE_CONTAINER_H_*/
