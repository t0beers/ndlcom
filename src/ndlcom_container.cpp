/**
 * @file NDLCom/src/ndlcom_container.cpp
 * @brief Container widget to collect all NDLCom-Stuff
 * @author Martin Zenzes
 * @date 2011-07-28
 */

#include "NDLCom/ndlcom_container.h"
#include "NDLCom/interface_container_widget.h"
#include "NDLCom/interface_container.h"
#include "NDLCom/composer.h"
#include "NDLCom/communication_statistic_widget.h"
#include "NDLCom/representation_mapper.h"
#include "NDLCom/AvailableRepresentations.h"
#include "NDLCom/message_traffic.h"

#include <QDebug>
#include <QMainWindow>
#include <QApplication>
#include <QDockWidget>

/* Q_INIT_RESOURCE is not allowed to be used inside a namespace
 * see http://doc.trolltech.com/4.7/qdir.html#Q_INIT_RESOURCE */
inline void initMyResource() { Q_INIT_RESOURCE(NDLCom); }

NDLCom::NDLComContainer::NDLComContainer(QWidget* parent, bool showToolBar) : QWidget(parent)
{
    /* init the ressources, like icons, with this quick hack */
#ifndef SHARED_BUILD
    initMyResource();
#endif

    /* create the signal-emitting object */
    InterfaceContainer* mpInterfaceContainer = InterfaceContainer::getInstance();

    /* create all object we need for displaying */
    InterfaceContainerWidget* inter = new InterfaceContainerWidget(this);
    CommunicationStatisticWidget* comm = new CommunicationStatisticWidget(this);
    Composer* comp = new Composer(this);
    AvailableRepresentations* avail = new AvailableRepresentations(this);
    MessageTraffic* trfk = new MessageTraffic(this);

    /* we have our own menu, which can (and should) be added into the main-application */
    mpMenu = new QMenu("NDLCom",this);
    mpMenu->setObjectName("NDLCom Menu");
    mpMenu->addAction(inter->actionConnectSerial);
    mpMenu->addAction(inter->actionConnectUdp);
    mpMenu->addMenu(inter->mpDisconnectMenu);
    mpMenu->addAction(mpInterfaceContainer->actionBridging);
    mpMenu->addSeparator();

    /* so do we have our toolbar... */
    mpToolbar = new QToolBar("NDLCom",this);
    mpToolbar->setObjectName("NDLCom Toolbar");
    mpToolbar->addAction(inter->actionConnectSerial);
    mpToolbar->addAction(mpInterfaceContainer->actionDisconnectAll);

    /* to be able to add our subwidget to the main QDockWidget-area of the application */
    QMainWindow* main = NULL;
    /* and hell, this is evil... */
    foreach (QWidget* widget, QApplication::allWidgets())
        if (qobject_cast<QMainWindow*>(widget))
            main = qobject_cast<QMainWindow*>(widget);
    if (!main)
        qFatal("could not detect main-window");

    /* declare all our subwidgets */
    struct DockListEntry {QWidget* widget; const QString title; const char* name; const char* icon; };
    const DockListEntry dockList[] = {
        {inter,    "InterfaceContainerWidget", "Manages all active interface", ":/NDLCom/images/ndlcom.png"},
        {comp,     "TelegramComposer", "Composer", ":/NDLCom/images/telegram.png"},
        {avail,    "Representations Info", "Representations", ":/NDLCom/images/info.png"},
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
        if (!showToolBar)
            mpToolbar->hide();
        if (main)
            main->addDockWidget(Qt::LeftDockWidgetArea, dock);
    }

    /* showing current live-traffic in a seperate widget (Gui->world and world->Gui) */
    connect(mpInterfaceContainer, SIGNAL(internal_txMessage(const NDLCom::Message&)),
            trfk, SLOT(sentMessage(const NDLCom::Message&)));
    connect(mpInterfaceContainer, SIGNAL(rxMessage(const NDLCom::Message&)),
            trfk, SLOT(receivedMessage(const NDLCom::Message&)));

    /* the composer maybe wants to send a message to the outside world. so connecting to same slot
     * as other (external) widgets would do. */
    connect(comp, SIGNAL(txMessage(const NDLCom::Message&)),
            mpInterfaceContainer, SLOT(txMessage(const NDLCom::Message&)));

    /* allow statistics about transmitted (from inside/gui) and received (from outside) messages */
    connect(mpInterfaceContainer, SIGNAL(internal_txMessage(const NDLCom::Message&)),
            comm, SLOT(rxMessage(const NDLCom::Message&)));
    connect(mpInterfaceContainer, SIGNAL(rxMessage(const NDLCom::Message&)),
            comm, SLOT(rxMessage(const NDLCom::Message&)));

    /* gui->world: let data out. incoming data is sent from NDLCom::NDLComContainer::txMessage()
     * (here) to ndlcom directly via a function call */
    connect(mpInterfaceContainer, SIGNAL(rxMessage(const NDLCom::Message&)),
            this, SLOT(slot_rxMessage(const NDLCom::Message&)));

    /* connection of signals for RepresentationsMapper is done in it's contructor. since it the
     * father-class of NDLCom. this is a stupid solution, which hopefully will be changed in the
     * future. the nice part is, that doing this the signals seems to be emitted from the same
     * class, when viewed from outside */
}

/* Gui->world, SLOT just a receiving wrapper, sending data via function-call to ndlcom */
void NDLCom::NDLComContainer::txMessage(const NDLCom::Message& msg)
{
    emit mpInterfaceContainer->txMessage(msg);
}

/* world->Gui, private SLOT sending data to the rest of the gui */
void NDLCom::NDLComContainer::slot_rxMessage(const NDLCom::Message& msg)
{
    emit rxMessage(msg);
}
