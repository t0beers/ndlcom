/**
 * @file NDLCom/src/interface_container.cpp
 * @author Martin Zenzes
 * @date 2011
 */
#include "NDLCom/interface_container.h"
#include "NDLCom/communication_statistic_widget.h"
#include "NDLCom/composer.h"
#include "NDLCom/interface.h"
#include "NDLCom/message.h"
#include "NDLCom/message_traffic.h"
#include "NDLCom/serialcom.h"
#include "NDLCom/udpcom.h"

#include "representations/id.h"
#include "representations/names.h"

#include "ui_interface_container.h"

#include <QDebug>
#include <QDockWidget>
#include <QMenu>
#include <QPixmap>
#include <QPainter>
#include <QMenu>

#include <cassert>

NDLCom::InterfaceContainer* NDLCom::InterfaceContainer::spInstance = 0;

NDLCom::InterfaceContainer* NDLCom::InterfaceContainer::getInterfaceContainer()
{
    if(spInstance == 0)
        spInstance = new InterfaceContainer();

    return spInstance;
}

NDLCom::InterfaceContainer::InterfaceContainer(QWidget* parent) : RepresentationMapper(parent)
{

    /* we only want ONE single instance of this object! */
    assert(spInstance == 0);
    spInstance = this;

    /* setting up oll object for autoconnect before setupUi() */
    actionDisconnectAll = new QAction("Disconnect All",this);
    actionDisconnectAll->setObjectName("actionDisconnectAll");
    actionDisconnectAll->setIcon(QIcon(":/NDLCom/images/disconnect.png"));
    actionDisconnectAll->setShortcuts(QKeySequence::Close);
    actionDisconnectAll->setToolTip("Disconnect all exsting data connections");

    actionConnectSerial = new QAction("Connect Serial",this);
    actionConnectSerial->setObjectName("actionConnectSerial");
    actionConnectSerial->setShortcuts(QKeySequence::Open);
    actionConnectSerial->setIcon(printNumberOnIcon(":/NDLCom/images/connect.png"));

    actionConnectUdp = new QAction("Connect UDP",this);
    actionConnectUdp->setObjectName("actionConnectUdp");
    actionConnectUdp->setIcon(printNumberOnIcon(":/NDLCom/images/connect.png"));

    mpGuiTimer = new QTimer(this);
    mpGuiTimer->setObjectName("mpGuiTimer");
    mpGuiTimer->setInterval(100);/*10Hz*/
    mpGuiTimer->start();

    /* setting up Ui */
    mpUi = new Ui::InterfaceContainer();
    mpUi->setupUi(this);

    /* set up the toolButtons in our widget */
    mpUi->connectSerial->setDefaultAction(actionConnectSerial);
    mpUi->connectUdp->setDefaultAction(actionConnectUdp);
    mpUi->disconnectAll->setDefaultAction(actionDisconnectAll);

    /* to be able to provide all disconnect-actions grouped together, we ned a menu */
    mpDisconnect = new QMenu("Disconnect");
    mpDisconnect->addAction(actionDisconnectAll);

    /* for counting active interfaces */
    mRunningSerial = 0;
    mRunningUdp = 0;
}

NDLCom::InterfaceContainer::~InterfaceContainer()
{
    actionDisconnectAll->activate(QAction::Trigger);

    /* setting this back to zero again allows recreating an instance multiple times... while only
     * one at a time is alive */
    spInstance = NULL;
}

/* another sexy hack: showing the numbre of active interfaces in the actionConnect*-actions */
QIcon NDLCom::InterfaceContainer::printNumberOnIcon(QString icon, int number, QColor color)
{
    /* we don't paint zeros... */
    if (number == 0)
        return QIcon(icon);

    /* this may be unefficient, but i know it works... */
    QPixmap pix(icon);
    QPainter painter;
    painter.begin(&pix);
    painter.setPen(color);
    painter.setFont(QFont("", 14, -1, false));
    painter.drawText(QPoint(0,20), QString::number(number));
    painter.end();
    return QIcon(pix);
}

/* how to create a new serial connection */
void NDLCom::InterfaceContainer::on_actionConnectSerial_triggered()
{
    Serialcom* serialcom = new Serialcom(this);

    connect(serialcom, SIGNAL(connected()), this, SLOT(connected()));
    connect(serialcom, SIGNAL(disconnected()), this, SLOT(disconnected()));

    serialcom->actionConnect->activate(QAction::Trigger);
}

/* how to create a new udp-connection */
void NDLCom::InterfaceContainer::on_actionConnectUdp_triggered()
{
    UdpCom* udpcom = new UdpCom(this);

    connect(udpcom, SIGNAL(connected()), this, SLOT(connected()));
    connect(udpcom, SIGNAL(disconnected()), this, SLOT(disconnected()));

    udpcom->actionConnect->activate(QAction::Trigger);
}

/* how to remove _all_ connections by calling each one's disconnect action. the rest is done in the
 * disconnected() slot after the sppropriate ignal of the interfaces arrives here */
void NDLCom::InterfaceContainer::on_actionDisconnectAll_triggered()
{
    while (!runningInterfaces.isEmpty())
        runningInterfaces.first()->actionDisconnect->activate(QAction::Trigger);
}

/* what should we do on a successfull connect? */
void NDLCom::InterfaceContainer::connected()
{
    Interface* inter;

    if ((inter = qobject_cast<Interface*>(QObject::sender())))
    {
        /* show the widget in the gui, added as second last (the last is the spacer) */
        mpUi->areaGates->insertWidget(mpUi->areaGates->count()-1,inter);
        /* mark it in our list */
        if (runningInterfaces.contains(inter))
            qWarning() << "NDLCom::InterfaceContainer::connected() got a doubled connect?";
        runningInterfaces.append(inter);
        /* allow specific disconnections */
        mpDisconnect->addAction(inter->actionDisconnect);
        /* handle losing a connection... */
        connect(inter, SIGNAL(disconnected()), this, SLOT(disconnected()));

        /* we wrap all received signals by the interfaces in our private slot */
        connect(inter, SIGNAL(rxMessage(const NDLCom::Message&)), this, SLOT(slot_rxMessage(const NDLCom::Message&)));

        /* inform others about established connection */
        emit connectionStatusChanged(true);

        /* we wanna keep book */
        connect(inter, SIGNAL(rxRate(double)),  this, SLOT(slot_rxRate(double)));
        connect(inter, SIGNAL(txRate(double)),  this, SLOT(slot_txRate(double)));
        connect(inter, SIGNAL(rxBytes(double)), this, SLOT(slot_rxBytes(double)));
        connect(inter, SIGNAL(txBytes(double)), this, SLOT(slot_txBytes(double)));
    }
    else
        qWarning() << "NDLCom::InterfaceContainer::connected() someone called, though he is not an NDLCom::Interface";

    /* update the icons */
    if (qobject_cast<Serialcom*>(QObject::sender()))
        actionConnectSerial->setIcon(printNumberOnIcon(":/NDLCom/images/connect.png",
                                                       ++mRunningSerial,
                                                       QColor("#6dca00")));
    if (qobject_cast<UdpCom*>(QObject::sender()))
        actionConnectUdp->setIcon(printNumberOnIcon(":/NDLCom/images/connect.png",
                                                    ++mRunningUdp,
                                                    QColor("#6dca00")));

    actionDisconnectAll->setIcon(printNumberOnIcon(":/NDLCom/images/disconnect.png",
                                                    mRunningUdp+mRunningSerial));
}

/* what should we do after loosing a connection */
void NDLCom::InterfaceContainer::disconnected()
{
    Interface* inter;

    if ((inter = qobject_cast<Interface*>(QObject::sender())))
    {
        /* update the icons (do this before deleting, otherwise object_cast won't work) */
        if (qobject_cast<Serialcom*>(QObject::sender()))
            actionConnectSerial->setIcon(printNumberOnIcon(":/NDLCom/images/connect.png",
                        --mRunningSerial,
                        QColor("#6dca00")));

        if (qobject_cast<UdpCom*>(QObject::sender()))
            actionConnectUdp->setIcon(printNumberOnIcon(":/NDLCom/images/connect.png",
                        --mRunningUdp,
                        QColor("#6dca00")));

        delete inter;
        runningInterfaces.removeAll(inter);

        /* Inform others about closed connection */
        emit connectionStatusChanged(false);

        /* remove entry in transferrate-map for this interface (overall transferred data is kept): */
        mMapRxRate.remove(inter);
        mMapTxRate.remove(inter);
    }
    else
        qWarning() << "NDLCom::InterfaceContainer::disconnected() someone called, tough he is not an NDLCom::Interface";

    /* updating this icon in all cases */
    actionDisconnectAll->setIcon(printNumberOnIcon(":/NDLCom/images/disconnect.png",
                                                    mRunningUdp+mRunningSerial));
}

/* _all_ messages from the GUI to external devices go through this slot */
void NDLCom::InterfaceContainer::txMessage(const NDLCom::Message& msg)
{
    /* TODO choose correct interface. in the time being we send every message to every interface */
    for (int i = 0; i < runningInterfaces.size(); ++i)
    {
        emit runningInterfaces.at(i)->txMessage(msg);
    }

    /* just for other widgets who wanna see the traffic, like NDLCom::MessageTraffic */
    emit internal_txMessage(msg);
}

/* _all_ received messages go through this slot! Message which are not addressed at us will be forwarded */
void NDLCom::InterfaceContainer::slot_rxMessage(const NDLCom::Message& msg)
{
    /* in any case, we retransmit the Message. If anymone needs it */
    /* this slot can be found in RepresentationMapper */
    emit rxMessage(msg);

    /* experimental feature: forward all messages received here on the other existing interfaces...
     * this may lead to exessive flood of packages, so be carefull! to guard normal operation from
     * this, we introduced a checkbox */
    if (!mpUi->bridging->checkState())
        return;

    /* just to be sure... */
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    if (!inter)
    {
        qWarning() << "NDLCom::InterfaceContainer::slot_rxMessage() got called by an alien";
        return;
    }

    for (int i = 0; i < runningInterfaces.size(); ++i)
    {
        /* all other interfaces but this one */
        if (runningInterfaces.at(i) != inter)
            emit runningInterfaces.at(i)->txMessage(msg);
    }
}

void NDLCom::InterfaceContainer::slot_rxRate(double rate)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mMapRxRate[inter] = rate;
}

void NDLCom::InterfaceContainer::slot_txRate(double rate)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mMapTxRate[inter] = rate;
}

void NDLCom::InterfaceContainer::slot_rxBytes(double bytes)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mMapRxBytes[inter] = bytes;
}

void NDLCom::InterfaceContainer::slot_txBytes(double bytes)
{
    void* inter = qobject_cast<Interface*>(QObject::sender());
    mMapTxBytes[inter] = bytes;
}

void NDLCom::InterfaceContainer::on_mpGuiTimer_timeout()
{
    double overallRxRate;
    double overallTxRate;
    double overallRxBytes;
    double overallTxBytes;

    {
        QMapIterator<void*, double> i(mMapRxBytes);
        while (i.hasNext()) {
            i.next();
            overallRxBytes += i.value();
        }
    }

    {
        QMapIterator<void*, double> i(mMapRxRate);
        while (i.hasNext()) {
            i.next();
            overallRxRate += i.value();
        }
    }

    {
        QMapIterator<void*, double> i(mMapTxBytes);
        while (i.hasNext()) {
            i.next();
            overallTxBytes += i.value();
        }
    }

    {
        QMapIterator<void*, double> i(mMapTxRate);
        while (i.hasNext()) {
            i.next();
            overallTxRate += i.value();
        }
    }

    /* stand back, pure inefficiency!!! */
    QString string = QString("Rx: %1, %2/s -- Tx: %3, %4/s")
                    .arg(Interface::sizeToString(overallRxBytes))
                    .arg(Interface::sizeToString(overallRxRate))
                    .arg(Interface::sizeToString(overallTxBytes))
                    .arg(Interface::sizeToString(overallTxRate));

    emit transferRate(string);

    emit status("NDLCom:rxRate",overallRxRate);
    emit status("NDLCom:txRate",overallTxRate);
}
