/**
 * @file NDLCom/src/ndlcom_container.cpp
 * @brief Container widget to collect all NDLCom-Stuff
 * @author Martin Zenzes
 * @date 2011-07-28
 */

#include "NDLCom/ndlcom_container.h"
#include "NDLCom/ndlcom.h"
#include "NDLCom/composer.h"
#include "NDLCom/communication_statistic_widget.h"
#include "NDLCom/representation_mapper.h"
#include "NDLCom/message_traffic.h"

#include <QDebug>
#include <QMainWindow>

/* Q_INIT_RESOURCE is not allowed to be used inside a namespace
 * see http://doc.trolltech.com/4.7/qdir.html#Q_INIT_RESOURCE */
inline void initMyResource() { Q_INIT_RESOURCE(NDLCom); }

NDLCom::NDLComContainer::NDLComContainer(QWidget* parent) : QWidget(parent)
{
    /* init the ressources, like icons, with this quick hack */
    initMyResource();

    /* create all object we need */
    NDLCom* mpNdlcom = new NDLCom(this);
    CommunicationStatisticWidget* comm = new CommunicationStatisticWidget(this);
    Composer* comp = new Composer(this);
    ShowRepresentations* show = new ShowRepresentations(this);
    MessageTraffic* trfk = new MessageTraffic(this);

    /* we have our own menu, which can (and should) be added into the main-application */
    mpMenu = new QMenu("NDLCom",this);
    mpMenu->setObjectName("NDLCom Menu");
    mpMenu->addAction(mpNdlcom->actionConnectSerial);
    mpMenu->addAction(mpNdlcom->actionConnectUdp);
    mpMenu->addMenu(mpNdlcom->mpDisconnect);
    mpMenu->addSeparator();

    /* so do we have our toolbar... */
    mpToolbar = new QToolBar("NDLCom",this);
    mpToolbar->setObjectName("NDLCom Toolbar");
    mpToolbar->addAction(mpNdlcom->actionConnectSerial);
    mpToolbar->addAction(mpNdlcom->actionDisconnectAll);

    /* to be able to add our subwidget to the main QDockWidget-area of the application */
    QMainWindow* main = NULL;
    /* and hell, this is evil... */
    if (qobject_cast<QMainWindow*>(parent))
        main = qobject_cast<QMainWindow*>(parent);
    else
        qFatal("could not detect main-window");

    /* declare all our subwidgets */
    struct DockListEntry {QWidget* widget; const QString title; const char* name; const char* icon; };
    const DockListEntry dockList[] = {
        {mpNdlcom, "NDLCom", "NDLCom Connection Wizard", ":/NDLCom/images/ndlcom.png"},
        {comp,     "TelegramComposer", "Composer", ":/NDLCom/images/telegram.png"},
        {show,     "Representations Info", "Representations", ":/NDLCom/images/info.png"},
        {trfk,     "MessageTraffic", "MessageTraffic", ":/NDLCom/images/parser.png"},
        {comm,     "CommunicationStatistics", "ComStats", ":/NDLCom/images/communication_statistics.png"},
        {0, 0, 0, 0}
    };

    /* and build up the menu, toolbar and so on */
    for (const DockListEntry* d = dockList; d->widget; ++d)
    {
        QDockWidget* dock = new QDockWidget(d->title, this);
        QAction* action = dock->toggleViewAction();
        dock->setWidget(d->widget);
        dock->setObjectName(d->name);
        dock->setVisible(false);
        action->setIcon(QIcon(d->icon));
        mpMenu->addAction(action);
        mpToolbar->addAction(action);
        if (main)
            main->addDockWidget(Qt::LeftDockWidgetArea, dock);
    }

    /* showing current live-traffic in a seperate widget */
    connect(mpNdlcom, SIGNAL(signal_txMessage(const ::NDLCom::Message&)), trfk,     SLOT(sentMessage(const ::NDLCom::Message&)));
    connect(mpNdlcom, SIGNAL(rxMessage(const ::NDLCom::Message&)),        trfk,     SLOT(receivedMessage(const ::NDLCom::Message&)));
    /* the composer maybe wants to send a message to the outside world */
    connect(comp,     SIGNAL(txMessage(const ::NDLCom::Message&)),        mpNdlcom, SLOT(txMessage(const ::NDLCom::Message&)));
    /* allow statistics about received messages */
    connect(mpNdlcom, SIGNAL(rxMessage(const ::NDLCom::Message&)),        comm,     SLOT(rxMessage(const ::NDLCom::Message&)));
    /* let data out. incoming data is sent from NDLCom::NDLComContainer::txMessage() to ndlcom directly via a function call */
    connect(mpNdlcom, SIGNAL(rxMessage(const ::NDLCom::Message&)),        this,     SLOT(slot_rxMessage(const ::NDLCom::Message&)));
}

/* SLOT just a receiving wrapper, sending data via function-call to ndlcom */
void NDLCom::NDLComContainer::txMessage(const ::NDLCom::Message& msg)
{
    emit mpNdlcom->txMessage(msg);
}

/* private SLOT sending data to the rest of the gui */
void NDLCom::NDLComContainer::slot_rxMessage(const ::NDLCom::Message& msg)
{
    emit rxMessage(msg);
}
