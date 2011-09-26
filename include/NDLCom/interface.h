/**
 * @file lib/NDLCom/include/NDLCom/interface.h
 * @author Martin Zenzes
 * @date 2011-07-28
 */

#ifndef _NDLCOM_INTERFACE_H_
#define _NDLCOM_INTERFACE_H_

#include "NDLCom/message.h"

#include <QWidget>
#include <QAction>
#include <QTimer>

namespace NDLCom
{
    namespace Ui
    {
        class Interface;
    };

    /* widget to show current raw-traffic. will be shown as seperate window */
    class InterfaceTraffic;

    /**
     * @brief Interface Base-Class, all neccessary "sugar" for nice NDLCom-Interfaces
     *
     * a seamless interface is provided to a NDLCom-Gui, while abstracting the underlying hardware inteface.
     *
     * a set of QAction and Signal/Slot combinations is defined and prepared. Some servicework is
     * done here, for example pausing/resuming, showing current traffic rates and raw-traffic
     */
    class Interface : public QWidget
    {
        Q_OBJECT
    public:
        Interface(QWidget* parent = 0);
        virtual ~Interface();

        /**
         * @brief will connect try to connect the underlying hardware-interface
         *
         * may pop up a QDialog to ask the user what we should do...
         *
         * should emit a "connected()" signal if succesfull.
         */
        QAction* actionConnect;
        /**
         * @brief this action shall disconnect the interface, it may delete the driver internally
         *
         * should emit "disconnected()"
         */
        QAction* actionDisconnect;

        /* little helper function */
        static QString sizeToString(int size);

        QString mInterfaceType;

    signals:
        /** emitted by inheriting classes */
        void connected();
        /** emitted by inheriting classes */
        void disconnected();
        /** emitted by this class */
        void paused();
        /** emitted by this class */
        void resumed();
        /**
         * @brief new data from outside, one complete NDLCom::Message at a time
         *
         * pure virtual... see http://stackoverflow.com/questions/2998216/does-qt-support-virtual-pure-slots
         *
         * @param msg recevied data 
         */
        void rxMessage(const ::NDLCom::Message& msg);

    public slots:
        /**
         * @brief data which should be transferred to outside, one complete NDLCom::Message at a time
         *
         * pure virtual... see http://stackoverflow.com/questions/2998216/does-qt-support-virtual-pure-slots
         *
         * @param msg to be transmitted to the outside world
         */
        virtual void txMessage(const ::NDLCom::Message& msg) = 0;

    protected:
        /** subclasses update this value as they receive raw-data, is used to display receive-rates */
        int mRxBytes;
        /** subclasses update this value as they send raw-data, is used to display send-rates */
        int mTxBytes;
        /** should be used to decide wether new data is sent/received or not. note that the basic
         * interface is still serving the hardware, but the signal/slot connection inside this class
         * is cut! */
        bool mPaused;
        /**
         * @brief may be called by a subclasses to tell this Interface it's status
         *
         * @param type something like "/dev/ttyUSB0" or "UDP 192.168.0.34:43211"
         */
        void setInterfaceType(const QString& type);

    signals:/*protected*/
		/* used to send traffic to the raw-traffic-window */
        void txRaw(const QByteArray&);
        void rxRaw(const QByteArray&);

    protected slots:
        /**
         * @brief action to be triggered for a proper connect
         *
         * pure virtual! has to be implemented by a subclass
         */
        virtual void on_actionConnect_triggered() = 0;
        /**
         * @brief action to be triggered for a proper disconnect
         *
         * pure virtual! has to be implemented by a subclass
         */
        virtual void on_actionDisconnect_triggered() = 0;

    private:
        /* used to calculate sending rates */
        int mRxBytes_last;
        int mTxBytes_last;
        double mRxRate_last;
        double mTxRate_last;
        /** @brief will add a widget showing current raw-traffic of this interface... */
        QAction* actionShowTraffic;
        /** widget which may show raw-traffic */
        InterfaceTraffic* mpTraffic;
        /**
         * @brief This action is handled exclusivly by this base-class
         *
         * sets the bool mPaused to true, if its paused, and cares to alter all other informative
         * stuff, like fresh labels, activating actions and emitting signals.
         */
        QAction* actionPauseResume;
        /** updating the Gui */
        QTimer* mpGuiTimer;
        /** used to show current raw-traffic of a Interface in a seperate window */
        QWidget* mpTrafficWindow;
        /** ... */
        Ui::Interface* mpUi;

    signals:/*private*/
        /* current data rate, transformed as a string */
        void transferRate(QString);

        /* allows some bookkeeping of data-traffic by higher-level widgets */
        void rxRate(double);
        void txRate(double);
        void rxBytes(double);
        void txBytes(double);

    private slots:
        /** updating the statusstring is done automagically... */
        void on_mpGuiTimer_timeout();
        /** when a connection was established */
        void slot_connected();
        /** when a connection was disconnected */
        void slot_disconnected();
        /** handling a request for raw-data traffic */
        void on_actionShowTraffic_toggled(bool);
        /** handling a request for pause/resume. renames QAction-tooltips, informs subclasses and displays state */
        void on_actionPauseResume_toggled(bool);
    };
};

#endif/*_NDLCOM_INTERFACE_H_*/

