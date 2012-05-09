/**
 * @file NDLCom/src/interface.cpp
 * @author Martin Zenzes
 * @date 2011
 */
#include "NDLCom/interface.h"
#include "NDLCom/interface_traffic.h"
#include "NDLCom/StaticTools.h"

#include <QDebug>
#include <QAction>
#include <QTimer>

/* 10Hz */
#define GUI_TIMER_INTERVAL 100

using namespace NDLCom;

Interface::Interface(QObject* parent) :
    QObject(parent)
{

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

    isConnected = false;

    mpGuiTimer = new QTimer(this);
    mpGuiTimer->setObjectName("mpGuiTimer");
    mpGuiTimer->setInterval(GUI_TIMER_INTERVAL);

    /* for calculating current transfer-rates */
    mRxBytes = 0;
    mTxBytes = 0;
    mStatistics.setZero();

    /* what we wanna do on a successfull connect */
    connect(this, SIGNAL(connected()), mpGuiTimer, SLOT(start()));
    connect(this, SIGNAL(connected()), this, SLOT(slot_connected()));
    /* what we wanna do after disconnection occured */
    connect(this, SIGNAL(disconnected()), mpGuiTimer, SLOT(stop()));
    connect(this, SIGNAL(disconnected()), this, SLOT(slot_disconnected()));
    /* what we wanna do on pause/resume */
    connect(this, SIGNAL(resumed()), mpGuiTimer, SLOT(start()));
    connect(this, SIGNAL(paused()), mpGuiTimer, SLOT(stop()));

    QMetaObject::connectSlotsByName(this);
}

/* calculating and updating current transfer-rates */
void Interface::on_mpGuiTimer_timeout()
{
    /* this is some sort of low-pass filter, to allow smoother values */
    double newRxRate = (mRxBytes-mStatistics.mRxBytes)/(double)GUI_TIMER_INTERVAL*1000.0;
    double newTxRate = (mTxBytes-mStatistics.mTxBytes)/(double)GUI_TIMER_INTERVAL*1000.0;

    /* calculate rate by comparing to byte counter from last interval */
    mStatistics.mRxRate = (mStatistics.mRxRate + newRxRate)/2.0;
    mStatistics.mTxRate = (mStatistics.mTxRate + newTxRate)/2.0;
    mStatistics.mRxBytes = mRxBytes;
    mStatistics.mTxBytes = mTxBytes;

    emit transferRate(mStatistics.toQString());

    emit rxRate(mStatistics.mRxRate);
    emit txRate(mStatistics.mTxRate);
    emit rxBytes(mStatistics.mRxBytes);
    emit txBytes(mStatistics.mTxBytes);
}

/* append current interface-type, like "/dev/ttyUSB0" to all QAction's */
void Interface::setDeviceName(const QString& deviceName)
{
    mDeviceName = deviceName;

    actionConnect->setText("Connect "+mDeviceName);
    actionConnect->setStatusTip("Create a new connection via "+mDeviceName);

    actionDisconnect->setText("Disconnect "+mDeviceName);
    actionDisconnect->setStatusTip("Disconnect "+mDeviceName);

    if (actionPauseResume->isChecked())
        actionPauseResume->setStatusTip("Resume "+mDeviceName);
    else
        actionPauseResume->setStatusTip("Pause "+mDeviceName);
}

/* handle pause-resume. and handle it only here in this class! */
void Interface::on_actionPauseResume_toggled(bool state)
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

/* handle connection-lost. although we prepare everything for a new connect, we will be deleted
 * normally */
void Interface::slot_disconnected()
{
    setDeviceName("not connected");

    /* zeroing the stats */
    mRxBytes = 0;
    mTxBytes = 0;
    mStatistics.setZero();

    actionPauseResume->setDisabled(true);
    actionDisconnect->setDisabled(true);

    actionConnect->setText("Connect");
    actionConnect->setStatusTip("Create a new connection");

    actionDisconnect->setText("Disconnect");
    actionDisconnect->setStatusTip("Disconnect existing Connection");

    if (!isConnected)
        qWarning() << "NDLCom::Interface::slot_disconnected() got a disconnect without beeing connected -- fix my code!";

    isConnected = false;
}

/* handle successfull connection */
void Interface::slot_connected()
{
    actionPauseResume->setEnabled(true);
    actionDisconnect->setEnabled(true);

    isConnected = true;
}

QString Interface::Statistics::toQString() const
{
    /* stand back, pure inefficiency!!! */
    QString string = QString("Rx: %1, %2/s -- Tx: %3, %4/s")
        .arg(sizeToString(mRxBytes))
        .arg(sizeToString(mRxRate))
        .arg(sizeToString(mTxBytes))
        .arg(sizeToString(mTxRate));
    return string;
}
