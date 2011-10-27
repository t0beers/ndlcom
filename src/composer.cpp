/**
 * @file NDLCom/src/composer.cpp
 * @brief TelegramComposer
 * @author Martin Zenzes
 * @date 2011-07-28
 */

#include "NDLCom/composer.h"
#include "NDLCom/message.h"

#include "representations/names.h"

#include "data_line_input.h"
#include "ui_composer.h"

#include <QDebug>

NDLCom::Composer::Composer(QWidget* parent) : QWidget(parent)
{
    /* creating these object before setupUi() allows autoconnect to work */
    actionSend = new QAction("Send Message", this);
    actionSend->setObjectName("actionSend");

    mpUi = new Ui::Composer;
    mpUi->setupUi(this);

    /* some kind of handcrafted input field... */
    mpDataInput = new DataLineInput(this);
    mpUi->layout->addWidget(mpDataInput);
    connect(mpUi->dataLength, SIGNAL(valueChanged(int)), mpDataInput, SLOT(setDataLength(int)));
    mpUi->dataLength->setValue(3);

    mpUi->sendButton->setDefaultAction(actionSend);

    /* we will have to QComboBox with all known Devices, sorted by Id, to be selected as receiver/sender */
    /* skipping id 0, its not used (broadcast) */
    for (int id = 0;id<=255;id++)
    {
        if (representationsNamesGetDeviceName(id))
        {
            QString name(representationsNamesGetDeviceName(id));
            mpUi->senders->addItem(name);
            mpUi->receivers->addItem(name);
        }
    }
    mpUi->senderId->setValue(representationsNamesGetDeviceId("ControlGUI"));
    mpUi->receiverId->setValue(representationsNamesGetDeviceId("BROADCAST"));

    /* no autoSending on beginning. when enabling the checkbox, the corresponding object is created */
    mpSendThread = NULL;
}
NDLCom::Composer::~Composer()
{
    if (mpSendThread)
    {
        delete mpSendThread;
    }
}

/* sending a message */
void NDLCom::Composer::on_actionSend_triggered()
{
    if (mpUi->countUp->isChecked())
        mpUi->frameCounter->stepUp();

    struct ProtocolHeader header;
    header.mReceiverId =  mpUi->receiverId->value();
    header.mSenderId = mpUi->senderId->value();
    header.mCounter = mpUi->frameCounter->value();
    header.mDataLen = mpUi->dataLength->value();

    Message msg(header, (const void*)mpDataInput->data().constData());

    emit txMessage(msg);
}

/* switching between autosend and no autosend by creating or destroying a custom made thread */
void NDLCom::Composer::on_autoSend_toggled(bool)
{
    if (mpSendThread)
    {
        /* thread is currently running, cleanup */
        delete mpSendThread;
        mpSendThread = NULL;
    }
    else
    {
        mpSendThread = new SendThread(this, mpUi->sendRate->value());
        /* prepare signal connection */
        connect(mpSendThread, SIGNAL(timeout()), actionSend, SLOT(trigger()));
        /* and start the timer */
        mpSendThread->start();
    }
}

/* changin send-rate */
void NDLCom::Composer::on_sendRate_valueChanged(int value)
{
    if (mpSendThread)
        mpSendThread->setFrequency(value);
}

/* keeping index in QSpinBox and name in QDropBox in sync, if possible */
void NDLCom::Composer::on_receivers_currentIndexChanged(int)
{
    if (representationsNamesGetDeviceId(mpUi->receivers->currentText().toStdString().c_str()))
        mpUi->receiverId->setValue(representationsNamesGetDeviceId(mpUi->receivers->currentText().toStdString().c_str()));
}

/* keeping index in QSpinBox and name in QDropBox in sync, if possible */
void NDLCom::Composer::on_senders_currentIndexChanged(int)
{
    if (representationsNamesGetDeviceId(mpUi->senders->currentText().toStdString().c_str()))
        mpUi->senderId->setValue(representationsNamesGetDeviceId(mpUi->senders->currentText().toStdString().c_str()));
}

/* keeping index in QSpinBox and name in QDropBox in sync, if possible */
void NDLCom::Composer::on_senderId_valueChanged(int index)
{
    if (representationsNamesGetDeviceName(index))
        mpUi->senders->setCurrentIndex(mpUi->senders->findText(representationsNamesGetDeviceName(index)));
    else
        mpUi->senders->setCurrentIndex(0);
}

/* keeping index in QSpinBox and name in QDropBox in sync, if possible */
void NDLCom::Composer::on_receiverId_valueChanged(int index)
{
    if (representationsNamesGetDeviceName(index))
        mpUi->receivers->setCurrentIndex(mpUi->receivers->findText(representationsNamesGetDeviceName(index)));
    else
        mpUi->receivers->setCurrentIndex(0);
}



/**
 * a custom made QTimer replacement
 *
 * more resolution and control on linux... see headerfile for more details
 *
 */
NDLCom::SendThread::SendThread(QObject* parent, unsigned int frequency) : QThread(parent)
{
    stopRunning = false;
    mFrequency = frequency;
}
NDLCom::SendThread::~SendThread()
{
    /* stop thread and wait for it to finish */
    stopRunning = true;
    wait(500);/* timeout of 500ms */
}
void NDLCom::SendThread::setFrequency(unsigned int frequency)
{
    freqMutex.lock();
        mFrequency = frequency;
    freqMutex.unlock();
}
void NDLCom::SendThread::run()
{
    struct timespec sleepTime = {0,0};
    struct timespec timeAfter;
    struct timespec timeBefore;

    while (!stopRunning)
    {
        /* sleep for some relative time */
        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepTime, NULL);
        clock_gettime(CLOCK_MONOTONIC,&timeBefore);

        /* happy trigger! */
        emit timeout();

        /* calculate next shot */
        freqMutex.lock();
            sleepTime.tv_nsec = (1.0/(double)mFrequency)*NSEC_PER_SEC + 0.5;
            sleepTime.tv_sec = 0;
        freqMutex.unlock();

        while (sleepTime.tv_nsec >= NSEC_PER_SEC){
            sleepTime.tv_nsec -= NSEC_PER_SEC;
            sleepTime.tv_sec++;
        }

        /* subtract time which was used by "emit" and the mutex */
        clock_gettime(CLOCK_MONOTONIC,&timeAfter);

        /* remove the time passed to do our work from the sleeptime. the few calls hereafter are
         * still some kind of offset. but we can life with that */
        sleepTime.tv_nsec -= (timeAfter.tv_nsec - timeBefore.tv_nsec) + (timeAfter.tv_sec - timeBefore.tv_sec)*NSEC_PER_SEC;

        while (sleepTime.tv_nsec < 0){
            sleepTime.tv_nsec += NSEC_PER_SEC;
            sleepTime.tv_sec--;
        }

        /* this debug breaks the timing... but is nice to have if something's going awry */
        //qDebug() << "sleeping" << sleepTime.tv_sec <<"s"<< sleepTime.tv_nsec <<"ns" << "to get"<<mFrequency<<"Hz";
    }
}
