/**
 * @file NDLCom/src/interface.cpp
 * @author Martin Zenzes
 * @date 2011
 */
#include "NDLCom/interface.h"
#include "NDLCom/interface_traffic.h"

#include "ui_interface.h"

#include <QDebug>

#ifndef GUI_TIMER_INTERVAL
#   define GUI_TIMER_INTERVAL 100
#else
#   error "WTF?"
#endif

NDLCom::Interface::Interface(QWidget* parent) : QWidget(parent)
{
    /* creating all objects which will use autoconnect before "setupUi()"... */
    actionConnect = new QAction(this);
    actionConnect->setObjectName("actionConnect");
    actionConnect->setIcon(QIcon(":/NDLCom/images/connect.png"));
    actionConnect->setText("Connect");
    actionConnect->setIcon(QIcon(":/NDLCom/images/connect.png"));
    actionConnect->setStatusTip("Create a new connection");

    actionDisconnect = new QAction(this);
    actionDisconnect->setObjectName("actionDisconnect");
    actionDisconnect->setIcon(QIcon(":/NDLCom/images/disconnect.png"));
    actionDisconnect->setDisabled(true);
    actionDisconnect->setText("Disconnect");
    actionDisconnect->setIcon(QIcon(":/NDLCom/images/disconnect.png"));
    actionDisconnect->setStatusTip("Disconnect existing Connection");

    actionPauseResume = new QAction(this);
    actionPauseResume->setObjectName("actionPauseResume");
    actionPauseResume->setCheckable(true);
    actionPauseResume->setText("Pause");
    actionPauseResume->setIcon(QIcon(":/NDLCom/images/pause.png"));
    actionPauseResume->setStatusTip("Pause Datatraffic");
    actionPauseResume->setDisabled(true);
    mPaused = false;

    actionShowTraffic = new QAction(this);
    actionShowTraffic->setObjectName("actionShowTraffic");
    actionShowTraffic->setCheckable(true);
    actionShowTraffic->setIcon(QIcon(":/NDLCom/images/serial_traffic.png"));
    actionShowTraffic->setStatusTip("Show current datatraffic");
    mpTraffic = NULL;

    mpTimer = new QTimer(this);
    mpTimer->setObjectName("mpTimer");
    mpTimer->setInterval(GUI_TIMER_INTERVAL);

    mpUi = new Ui::Interface;
    mpUi->setupUi(this);

    /* for calculating current transfer-rates */
    mRxBytes = 0;
    mTxBytes = 0;
    mRxBytes_last = 0;
    mTxBytes_last = 0;
    mRxRate_last = 0;
    mTxRate_last = 0;

    /* what we wanna do on a successfull connect */
    connect(this, SIGNAL(connected()), mpTimer, SLOT(start()));
    connect(this, SIGNAL(connected()), this, SLOT(slot_connected()));
    /* what we wanna do after disconnection occured */
    connect(this, SIGNAL(disconnected()), mpTimer, SLOT(stop()));
    connect(this, SIGNAL(disconnected()), this, SLOT(slot_disconnected()));
    /* what we wanna do on pause/resume */
    connect(this, SIGNAL(resumed()), mpTimer, SLOT(start()));
    connect(this, SIGNAL(paused()), mpTimer, SLOT(stop()));
    /* setting the current transfer-rate directly to the appropriate QLabel */
    connect(this, SIGNAL(transferRate(QString)), mpUi->status, SLOT(setText(QString)));

    /* now, that setupUi() was executed, we can populate the QToolButton with our QAction */
    mpUi->close->setDefaultAction(actionDisconnect);
    mpUi->pause->setDefaultAction(actionPauseResume);
    mpUi->info->setDefaultAction(actionShowTraffic);

    /* set this to zero to avoid overwriting it by accident */
    mpTrafficWindow = NULL;
}

NDLCom::Interface::~Interface()
{
    if (mpTrafficWindow)
    {
        qDeleteAll(mpTrafficWindow->children());
        delete mpTrafficWindow;
        mpTrafficWindow = NULL;
    }
}

/* little hack for niceness! */
QString NDLCom::Interface::sizeToString(int size)
{
    int bytes = 1;
    int kbytes = 1024*bytes;
    int mbytes = 1024*kbytes;
    int gbytes = 1024*mbytes;

    if (size > gbytes)
        return QString::number((double)(size)/gbytes,'f',2)+QString(" GiB");
    else if (size > mbytes)
        return QString::number((double)(size)/mbytes,'f',2)+QString(" MiB");
    else if (size > kbytes)
        return QString::number((double)(size)/kbytes,'f',2)+QString(" KiB");
    else
        return QString::number(size)+QString(" B");
}

/* calculating and updating current transfer-rates */
void NDLCom::Interface::on_mpTimer_timeout()
{
    /* this is some sort of low-pass filter, to allow smoother values */
    mRxRate_last = (mRxRate_last+(mRxBytes-mRxBytes_last)/(double)GUI_TIMER_INTERVAL*1000.0)/2.0;
    mTxRate_last = (mTxRate_last+(mTxBytes-mTxBytes_last)/(double)GUI_TIMER_INTERVAL*1000.0)/2.0;

    /* stand back, pure inefficiency!!! */
    QString string = QString("Rx: ")
                   + sizeToString(mRxBytes)
                   + QString(", ")
                   + sizeToString(mRxRate_last)
                   + QString("/s -- Tx: ")
                   + sizeToString(mTxBytes)
                   + QString(", ")
                   + sizeToString(mTxRate_last)
                   + QString("/s");

    mRxBytes_last = mRxBytes;
    mTxBytes_last = mTxBytes;

    emit transferRate(string);

    emit rxRate(mRxRate_last);
    emit txRate(mTxRate_last);
    emit rxBytes(mRxBytes);
    emit txBytes(mTxBytes);
}

/* append current interface-type, like "/dev/ttyUSB0" to all QAction */
void NDLCom::Interface::setStatusString(const QString& interfaceType)
{
    mpUi->type->setText(interfaceType);

    actionConnect->setText("Connect "+interfaceType);
    actionConnect->setStatusTip("Create a new connection via "+interfaceType);

    actionDisconnect->setText("Disconnect "+interfaceType);
    actionDisconnect->setStatusTip("Disconnect "+interfaceType);

    if (actionPauseResume->isChecked())
        actionPauseResume->setStatusTip("Resume "+interfaceType);
    else
        actionPauseResume->setStatusTip("Pause "+interfaceType);

    if (mpTrafficWindow)
        mpTrafficWindow->setWindowTitle("raw traffic of "+mpUi->type->text());
}

/* showing an extra Window with current Raw-Traffic */
void NDLCom::Interface::on_actionShowTraffic_toggled(bool checked)
{
    if (checked)
    {
        if (!mpTrafficWindow)
        {
            mpTrafficWindow = new QWidget();
            mpTrafficWindow->setWindowTitle("raw traffic of "+mpUi->type->text());
            mpTrafficWindow->resize(400,400);
            QVBoxLayout* layout = new QVBoxLayout();
            mpTrafficWindow->setLayout(layout);
            InterfaceTraffic* wid = new InterfaceTraffic(mpTrafficWindow);
            layout->addWidget(wid);
            connect(this, SIGNAL(rxRaw(const QByteArray&)), wid, SLOT(rxTraffic(const QByteArray&)));
            connect(this, SIGNAL(txRaw(const QByteArray&)), wid, SLOT(txTraffic(const QByteArray&)));
            mpTrafficWindow->show();
            mpTrafficWindow->activateWindow();
        }
    }
    else
    {
        if (mpTrafficWindow)
        {
            qDeleteAll(mpTrafficWindow->children());
            delete mpTrafficWindow;
            mpTrafficWindow = NULL;
        }
    }
}

/* handle pause-resume. and handle it only here in this class! */
void NDLCom::Interface::on_actionPauseResume_toggled(bool state)
{
    mPaused = state;

    if (state)
    {
        actionPauseResume->setText("Resume");
        actionPauseResume->setIcon(QIcon(":/NDLCom/images/resume.png"));
        actionPauseResume->setStatusTip("Resume Datatraffic");

        emit transferRate("paused");
        emit paused();
    }
    else
    {
        actionPauseResume->setText("Pause");
        actionPauseResume->setIcon(QIcon(":/NDLCom/images/pause.png"));
        actionPauseResume->setStatusTip("Pause Datatraffic");
        emit resumed();
    }
}

/* handle connection-lost */
void NDLCom::Interface::slot_disconnected()
{
    setStatusString("not connected");

    /* zeroing the stats */
    mRxBytes = 0;
    mTxBytes = 0;
    mRxBytes_last = 0;
    mTxBytes_last = 0;
    mRxRate_last = 0;
    mTxRate_last = 0;

    actionPauseResume->setDisabled(true);
    actionDisconnect->setDisabled(true);

    actionConnect->setText("Connect");
    actionConnect->setStatusTip("Create a new connection");

    actionDisconnect->setText("Disconnect");
    actionDisconnect->setStatusTip("Disconnect existing Connection");
}

/* handle successfull connection */
void NDLCom::Interface::slot_connected()
{
    actionPauseResume->setEnabled(true);
    actionDisconnect->setEnabled(true);
}

