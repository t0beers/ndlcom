/**
 * @file NDLCom/src/interface_container_widget.cpp
 * @author Martin Zenzes
 * @date 2011
 */
#include "NDLCom/interface_container_widget.h"

#include "NDLCom/interface_container.h"
#include "NDLCom/interface.h"
#include "NDLCom/InterfaceWidget.h"
#include "NDLCom/message.h"
#include "NDLCom/serialcom.h"
#include "NDLCom/udpcom.h"
#include "NDLCom/StaticTools.h"

#include "ui_interface_container_widget.h"
#include "ui_interface_widget.h"

#include <QDebug>

using namespace NDLCom;

InterfaceContainerWidget::InterfaceContainerWidget(QWidget* parent) :
    QWidget(parent)
{

    actionConnectUdp = new QAction("Connect UDP",this);
    actionConnectUdp->setObjectName("actionConnectUdp");
    actionConnectUdp->setToolTip("Connect to an UDP port to receive data over network");

    actionConnectSerial = new QAction("Connect Serial",this);
    actionConnectSerial->setObjectName("actionConnectSerial");
    actionConnectSerial->setShortcuts(QKeySequence::Open);
    actionConnectSerial->setToolTip("Connect to a serial port");

    updateIcons();

    mpDisconnectMenu = new QMenu("Disconnect all",this);

    /* setting up Ui */
    mpUi = new Ui::InterfaceContainerWidget();
    mpUi->setupUi(this);

    /* set up the toolButtons in our widget */
    mpUi->connectSerial->setDefaultAction(actionConnectSerial);
    mpUi->connectUdp->setDefaultAction(actionConnectUdp);

    /* we will use the disconnectAll action of the low-level interface_container in the widget */
    mpUi->disconnectAll->setDefaultAction(InterfaceContainer::getInstance()->actionDisconnectAll);

    /* each time a interface connects or disconnects, we update the icons */
    connect(InterfaceContainer::getInstance(), SIGNAL(connectionStatusChanged(bool)),
            this, SLOT(updateIcons()));

    /* we tightly couple the interface's "bridging" state (which is action) with our checkbox */
    connect(InterfaceContainer::getInstance()->actionBridging, SIGNAL(toggled(bool)),
            mpUi->bridging, SLOT(setChecked(bool)));
    connect(mpUi->bridging, SIGNAL(toggled(bool)),
            InterfaceContainer::getInstance()->actionBridging, SLOT(setChecked(bool)));

    /* update our gui widget with all previously existing interfaces */
    QStringList types = InterfaceContainer::getInstance()->getInterfaceTypes();
    for (int i = 0; i < types.size(); ++i)
    {
        QList<Interface*> interfaces = InterfaceContainer::getInstance()->getActiveInterfaces(types.at(i));
        for (int j = 0; j < interfaces.size(); ++j)
        {
            addInterface(interfaces.at(j));
        }
    }
}

/* how to create a new serial connection */
void InterfaceContainerWidget::on_actionConnectSerial_triggered()
{
    Serialcom* serialcom = new Serialcom(this);

    connect(serialcom, SIGNAL(connected()), this, SLOT(connected()));
    connect(serialcom, SIGNAL(connected()), InterfaceContainer::getInstance(), SLOT(connected()));

    serialcom->popupConnectDialogAndTryToConnect();

    /* catch the case that the user declines the connection dialog using "cancel" */
    if (!serialcom->isConnected)
        delete serialcom;
}

/* how to create a new udp connection */
void InterfaceContainerWidget::on_actionConnectUdp_triggered()
{
    UdpCom* udpcom = new UdpCom(this);

    connect(udpcom, SIGNAL(connected()), this, SLOT(connected()));
    connect(udpcom, SIGNAL(connected()), InterfaceContainer::getInstance(), SLOT(connected()));

    udpcom->popupConnectDialogAndTryToConnect();

    /* catch the case that the user declines the connection dialog using "cancel" */
    if (!udpcom->isConnected)
        delete udpcom;
}

/* what should we do on a successfull connect? here, we don't distinguish between different
 * interface types anymore */
void InterfaceContainerWidget::connected()
{
    Interface* interface;

    if (!(interface = qobject_cast<Interface*>(QObject::sender())))
        return;

    addInterface(interface);
}

void InterfaceContainerWidget::addInterface(NDLCom::Interface* interface)
{

    /* create a displaying widget for this interface. it will use the base-class'es signals */
    InterfaceWidget* widget = new InterfaceWidget(this,interface);

    /* show this widget in the gui, added as second last (the last is a spacer) */
    mpUi->areaGates->insertWidget(mpUi->areaGates->count()-1,widget);

    /* save informations about the mapping "interface->widget" */
    interfaceWidgets.insert(interface,widget);

    /* allow specific disconnections, collected in our QMenu */
    mpDisconnectMenu->addAction(interface->actionDisconnect);

    /* handle losing a connection... */
    connect(interface, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

/* what should we do after loosing a connection */
void InterfaceContainerWidget::disconnected()
{
    Interface* interface;

    if (!(interface = qobject_cast<Interface*>(QObject::sender())))
        return;

    /* remove the widget from our mapping-list. by deleting it, the widget is automatically removed
     * from the layout */
    delete interfaceWidgets.take(interface);
}

/* we wanna "write" the number of active interfaces in the respective icons */
void InterfaceContainerWidget::updateIcons()
{

    int nrOfUdp = InterfaceContainer::getInstance()->getActiveInterfaces("UdpCom").size();
    actionConnectUdp->setIcon(printNumberOnIcon(QIcon(":/NDLCom/images/connect.png"),
                                                nrOfUdp,
                                                QColor("#6dca00")));

    int nrOfSerial = InterfaceContainer::getInstance()->getActiveInterfaces("Serial").size();
    actionConnectSerial->setIcon(printNumberOnIcon(QIcon(":/NDLCom/images/connect.png"),
                                                   nrOfSerial,
                                                   QColor("#6dca00")));

}
