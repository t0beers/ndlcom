#include "NDLCom/InterfaceWidget.h"
#include "NDLCom/interface.h"
#include "NDLCom/interface_traffic.h"

#include "ui_interface_widget.h"

using namespace NDLCom;

InterfaceWidget::InterfaceWidget(QWidget* parent, Interface* interface) :
    QWidget(parent),
    mpTrafficWindow(NULL),
    mpInterface(interface)
{
    actionShowTraffic = new QAction(this);
    actionShowTraffic->setObjectName("actionShowTraffic");
    actionShowTraffic->setCheckable(true);
    actionShowTraffic->setIcon(QIcon(":/NDLCom/images/serial_traffic.png"));
    actionShowTraffic->setStatusTip("Show current datatraffic");

    mpUi = new Ui::InterfaceWidget;
    mpUi->setupUi(this);

    /* now, that setupUi() was executed, we can populate the QToolButton with our QAction */
    mpUi->close->setDefaultAction(interface->actionDisconnect);
    mpUi->pause->setDefaultAction(interface->actionPauseResume);
    mpUi->info->setDefaultAction(actionShowTraffic);

    mpUi->type->setText(interface->mDeviceName);

    /* setting the current transfer-rate directly to the appropriate QLabel */
    connect(mpInterface, SIGNAL(transferRate(QString)), mpUi->status, SLOT(setText(QString)));

}

InterfaceWidget::~InterfaceWidget()
{
    /* remove this window on a disconnect */
    if (mpTrafficWindow)
    {
        mpTrafficWindow->close();
        delete mpTrafficWindow;
        mpTrafficWindow = NULL;
    }
}

/* showing an extra Window with current Raw-Traffic */
/* FIXME desotrying the window does not untoggle the action */
void InterfaceWidget::on_actionShowTraffic_toggled(bool checked)
{
    if (checked && !mpTrafficWindow)
    {
        mpTrafficWindow = new InterfaceTraffic(this);
        mpTrafficWindow->setWindowFlags(mpTrafficWindow->windowFlags() | Qt::Window);
        mpTrafficWindow->setWindowTitle("raw traffic of "+mpUi->type->text());
        mpTrafficWindow->resize(400,400);

        connect(mpInterface, SIGNAL(rxRaw(const QByteArray&)), mpTrafficWindow, SLOT(rxTraffic(const QByteArray&)));
        connect(mpInterface, SIGNAL(txRaw(const QByteArray&)), mpTrafficWindow, SLOT(txTraffic(const QByteArray&)));

        mpTrafficWindow->show();
        mpTrafficWindow->raise();
    }
    else
    {
        /* remove this window */
        if (mpTrafficWindow)
        {
            mpTrafficWindow->close();
            delete mpTrafficWindow;
            mpTrafficWindow = NULL;
        }
    }
}


void InterfaceWidget::setDeviceName(const QString& deviceName)
{
    if (mpTrafficWindow)
        mpTrafficWindow->setWindowTitle("raw traffic of "+deviceName);
    mpUi->type->setText(deviceName);
}
