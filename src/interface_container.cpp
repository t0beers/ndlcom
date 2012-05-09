/**
 * @file NDLCom/src/interface_container.cpp
 * @author Martin Zenzes
 * @date 2011, 2012
 */
#include "NDLCom/interface_container.h"

#include "NDLCom/StaticTools.h"
#include "NDLCom/CommandLineArgument.h"
#include "NDLCom/serialcom.h"

#include <QDebug>
#include <QPainter>
#include <QIcon>
#include <QTimer>
#include <QAction>

using namespace NDLCom;

/* singletons pointer */
InterfaceContainer* InterfaceContainer::spInstance = 0;

InterfaceContainer* InterfaceContainer::getInterfaceContainer()
{
#warning NDLCom::InterfaceContainer::getInterfaceContainer() deprecated, use 'NDLCom::InterfaceContainer::getInstance()' instead
    return getInstance();
}

InterfaceContainer* InterfaceContainer::getInstance()
{
    if(spInstance == 0)
        spInstance = new InterfaceContainer();

    return spInstance;
}

InterfaceContainer::InterfaceContainer(QObject* parent) :
    RepresentationMapper(parent)
{
    /* we only want ONE single instance of this object! */
    Q_ASSERT(spInstance == NULL);
    spInstance = this;

    actionDisconnectAll = new QAction("Disconnect All",this);
    actionDisconnectAll->setObjectName("actionDisconnectAll");
    actionDisconnectAll->setIcon(QIcon(":/NDLCom/images/disconnect.png"));
    actionDisconnectAll->setShortcuts(QKeySequence::Close);
    actionDisconnectAll->setToolTip("Disconnect all existing data connections");

    actionBridging = new QAction("Enable NDLCom bridging", this);
    actionBridging->setObjectName("actionBridging");
    actionBridging->setToolTip("if enabled all received messages will be echoed on all other interfaces. dangerous!");
    actionBridging->setCheckable(true);
    actionBridging->setChecked(false);

    mpStatisticsTimer = new QTimer(this);
    mpStatisticsTimer->setObjectName("mpStatisticsTimer");
    mpStatisticsTimer->setInterval(100);/*10Hz*/
    mpStatisticsTimer->start();

    QMetaObject::connectSlotsByName(this);

    connect(this, SIGNAL(connectionStatusChanged(bool)), this, SLOT(updateIcons()));

    /* we support a simple commandline argument: connecting directly! */
    QString portName = CommandLineArgument("--serialport", "", "serialport to connect to");
    int baudRate = CommandLineArgument("--baudrate", "500000", "baudrate to use").toInt();
    if (!portName.isEmpty())
    {
        Serialcom* serialcom = new Serialcom(this);
        connect(serialcom, SIGNAL(connected()), this, SLOT(connected()));
        serialcom->portName = portName;
        serialcom->baudRate = baudRate;
        serialcom->actionConnect->activate(QAction::Trigger);
    }
}

/* destructor of singleton. should never be called. however we do implement it */
InterfaceContainer::~InterfaceContainer()
{
    actionDisconnectAll->activate(QAction::Trigger);

    /* clean up list of interfaces: each map-entry is a list of pointers to interfaces... after
     * called actionDisconnectAll, this map should be empty anyway */
    QMapIterator<QString, QList<Interface* >* > i(activeInterfaces);
    while(!activeInterfaces.isEmpty())
    {
        QList<Interface* >* interfaces = activeInterfaces.take(activeInterfaces.end().key());
        while(!interfaces->isEmpty())
        {
            qWarning() << "NDLCom::InterfaceContainer::~InterfaceContainer() found non deleted interface...";
            delete interfaces->takeFirst();
        }

        delete interfaces;
    }

    qWarning() << "NDLCom::InterfaceContainer::~InterfaceContainer() called... singleton?";
    /* setting this back to zero again allows recreating an instance multiple times... while only
     * one at a time is alive */
    spInstance = NULL;
}

/* how to remove _all_ connections by calling each one's disconnect action. the rest is done in the
 * disconnected() slot after the appropriate signal of the interfaces arrives here */
void NDLCom::InterfaceContainer::on_actionDisconnectAll_triggered()
{
    QMapIterator<QString, QList<Interface* >* > i(activeInterfaces);
    while(i.hasNext())
    {
        i.next();
        QList<Interface* >* interfaces = i.value();
        for (int j=interfaces->size()-1;j>=0;j--)
            interfaces->at(j)->actionDisconnect->activate(QAction::Trigger);
    }
}

/* what should we do on a successfull connect? */
void InterfaceContainer::connected()
{
    Interface* inter;

    if (!(inter = qobject_cast<Interface*>(QObject::sender())))
        return;

    /* check if we know this type of interface already */
    if (!activeInterfaces.value(inter->mInterfaceType))
    {
        /* qDebug() << "NDLCom::InterfaceContainer::connected() learned new interface type" << inter->mInterfaceType; */
        activeInterfaces.insert(inter->mInterfaceType, new QList<Interface*>());
    }
    /* save it in our list */
    activeInterfaces.value(inter->mInterfaceType)->append(inter);

    /* handle losing a connection from this interface... */
    connect(inter, SIGNAL(disconnected()), this, SLOT(disconnected()));

    /* we wrap all received signals by the interfaces in our private slot */
    connect(inter, SIGNAL(rxMessage(const NDLCom::Message&)), this, SLOT(slot_rxMessage(const NDLCom::Message&)));

    /* inform others about established connection FIXME use number if active interfaces instead? */
    emit connectionStatusChanged(true);

    /* we wanna keep book in the statistics */
    connect(inter, SIGNAL(rxRate(double)),  this, SLOT(slot_rxRate(double)));
    connect(inter, SIGNAL(txRate(double)),  this, SLOT(slot_txRate(double)));
    connect(inter, SIGNAL(rxBytes(double)), this, SLOT(slot_rxBytes(double)));
    connect(inter, SIGNAL(txBytes(double)), this, SLOT(slot_txBytes(double)));
}

/* what should we do after loosing a connection */
void InterfaceContainer::disconnected()
{
    Interface* inter;

    /* only proceed if we have a valid caller */
    if (!(inter = qobject_cast<Interface*>(QObject::sender())))
        return;

    /* lookup this interface in our list, and remove it from the list */
    activeInterfaces.value(inter->mInterfaceType)->removeAll(inter);
    /* if this was the last interface, remove it's list aswell */
    if (activeInterfaces.value(inter->mInterfaceType)->isEmpty())
        delete activeInterfaces.take(inter->mInterfaceType);
    /* and finally delete the interface itself */
    delete inter;

    /* Inform others about closed connection */
    emit connectionStatusChanged(false);

    /* Interface statistics are currently not removed from the mInterfaceStatistics map. */
}

QList<Interface* > InterfaceContainer::getActiveInterfaces(QString type)
{
    if (activeInterfaces.value(type))
        return *activeInterfaces.value(type);
    else
        return QList<Interface*>();
}

int InterfaceContainer::getNumberOfActiveInterfaces()
{
    int retval = 0;

    QMapIterator<QString, QList<Interface* >* > i(activeInterfaces);
    while(i.hasNext())
    {
        i.next();
        retval += i.value()->size();
    }

    return retval;
}

QStringList InterfaceContainer::getInterfaceTypes()
{
    QStringList retval;

    QMapIterator<QString, QList<Interface* >* > i(activeInterfaces);
    while(i.hasNext())
    {
        i.next();
        retval << i.key();
    }

    return retval;
}

/* ---------------------------------- data handling ---------------------------------- */

/* _all_ messages from the GUI to external devices go through this slot */
void InterfaceContainer::txMessage(const NDLCom::Message& msg)
{
    /* TODO choose correct interface. */
    /* in the time being we send every message to every interface */
    QMapIterator<QString, QList<Interface* >* > i(activeInterfaces);
    while(i.hasNext())
    {
        i.next();
        for (int j=0;j<i.value()->size();++j)
        {
            emit i.value()->at(j)->txMessage(msg);
        }
    }

    /* just for other widgets who wanna see the traffic, like NDLCom::MessageTraffic */
    emit internal_txMessage(msg);
}

/* _all_ received messages go through this slot! Message which are not addressed at us will be forwarded */
void InterfaceContainer::slot_rxMessage(const NDLCom::Message& msg)
{
    /* in any case, we retransmit the Message. If anymone needs it */
    /* this slot can be found in RepresentationMapper */
    emit rxMessage(msg);

    /* experimental feature: forward all messages received here on the other existing interfaces...
     * this may lead to exessive flood of packages, so be carefull! to guard normal operation from
     * this, we introduced a checkbox */
    if (!actionBridging->isChecked())
        return;

    /* just to be sure... */
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    if (!inter)
    {
        qWarning() << "NDLCom::InterfaceContainer::slot_rxMessage() got called by an alien";
        return;
    }

    QMapIterator<QString, QList<Interface* >* > i(activeInterfaces);
    while(i.hasNext())
    {
        i.next();
        for (int j=0;j<i.value()->size();++j)
        {
            /* all other interfaces but this one */
            if (i.value()->at(j) != inter)
                emit i.value()->at(j)->txMessage(msg);
        }
    }
}

void InterfaceContainer::updateIcons()
{
    actionDisconnectAll->setIcon(printNumberOnIcon(QIcon(":/NDLCom/images/connect.png"),
                                                   getNumberOfActiveInterfaces()));
}

/* ---------------------------------- statistics ---------------------------------- */

void InterfaceContainer::slot_rxRate(double rate)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mStatistics[inter].mRxRate = rate;
}

void InterfaceContainer::slot_txRate(double rate)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mStatistics[inter].mTxRate = rate;
}

void InterfaceContainer::slot_rxBytes(double bytes)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mStatistics[inter].mRxBytes = bytes;
}

void InterfaceContainer::slot_txBytes(double bytes)
{
    Interface* inter = qobject_cast<Interface*>(QObject::sender());
    mStatistics[inter].mTxBytes = bytes;
}

void InterfaceContainer::on_mpStatisticsTimer_timeout()
{
    Interface::Statistics sum;
    sum.setZero();

    for(std::map<Interface*, Interface::Statistics>::const_iterator it(mStatistics.begin());
        it != mStatistics.end(); it++)
    {
        sum.mRxBytes += (*it).second.mRxBytes;
        sum.mTxBytes += (*it).second.mTxBytes;
        sum.mRxRate += (*it).second.mRxRate;
        sum.mTxRate += (*it).second.mTxRate;
    }

    emit transferRate(sum.toQString());

    emit status("NDLCom:rxRate",sum.mRxRate);
    emit status("NDLCom:txRate",sum.mTxRate);
}
