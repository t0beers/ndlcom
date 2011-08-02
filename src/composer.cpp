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

    mpSendTimer = new QTimer(this);
    mpSendTimer->setObjectName("mpSendTimer");

    mpUi = new Ui::Composer;
    mpUi->setupUi(this);

    /* some kind of handcrafted input field... */
    mpDataInput = new DataLineInput(this);
    mpUi->layout->addWidget(mpDataInput);
    connect(mpUi->dataLength, SIGNAL(valueChanged(int)), mpDataInput, SLOT(setDataLength(int)));
    mpUi->dataLength->setValue(3);

    mpUi->sendButton->setDefaultAction(actionSend);

    /* autosending by using smart autoconnect! */
    mpSendTimer->setInterval(1000);/* 1Hz */
    connect(mpSendTimer, SIGNAL(timeout()), this, SLOT(on_actionSend_triggered()));

    /* we will have to QComboBox with all known Devices, sorted by Id, to be selected as receiver/sender */
    /* skipping id 0, its not used (broadcast) */
    mpUi->senders->addItem(QString("Broadcast"));
    mpUi->receivers->addItem(QString("Broadcast"));
    for (int id = 1;id<255;id++)
    {
        if (representationsNamesGetDeviceName(id))
        {
            QString name(representationsNamesGetDeviceName(id));
            mpUi->senders->addItem(name);
            mpUi->receivers->addItem(name);
        }
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
    header.mPriority = mpUi->priority->value();
    header.mCounter = mpUi->frameCounter->value();
    header.mDataLen = mpUi->dataLength->value();

    Message msg(header, (const void*)mpDataInput->data().constData());

    emit txMessage(msg);
}

/* switching between autosend and no autosend */
void NDLCom::Composer::on_autoSend_toggled(bool state)
{
    if (state)
        mpSendTimer->start();
    else
        mpSendTimer->stop();
}

/* changin send-rate */
void NDLCom::Composer::on_sendRate_valueChanged(int value)
{
    mpSendTimer->setInterval(1./value*1000.);
    /* update our status label */
    mpUi->autoSend->setText(QString("AutoSend ")+QString::number(value)+QString(" Hz"));
}

/* keeping index in QSpinBox and name in QDropBox in sync, if possible */
void NDLCom::Composer::on_receivers_currentIndexChanged(int index)
{
    if (representationsNamesGetDeviceId(mpUi->receivers->currentText().toStdString().c_str()))
        mpUi->receiverId->setValue(representationsNamesGetDeviceId(mpUi->receivers->currentText().toStdString().c_str()));
    index++;/* suppresses "unused" warnings */
}

/* keeping index in QSpinBox and name in QDropBox in sync, if possible */
void NDLCom::Composer::on_senders_currentIndexChanged(int index)
{
    if (representationsNamesGetDeviceId(mpUi->senders->currentText().toStdString().c_str()))
        mpUi->senderId->setValue(representationsNamesGetDeviceId(mpUi->senders->currentText().toStdString().c_str()));
    index++;/* suppresses "unused" warnings */
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

