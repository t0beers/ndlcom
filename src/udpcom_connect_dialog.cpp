#include "NDLCom/udpcom.h"
#include "udpcom_connect_dialog.h"
#include "ui_udpcom_connect_dialog.h"

using namespace NDLCom;

UdpComConnectDialog::UdpComConnectDialog(QDialog* parent)
    : QDialog(parent),
      mpUi(new Ui::UdpComConnectDialog())
{
    mpUi->setupUi(this);
    connect(mpUi->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(mpUi->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mpUi->saveSettings,SIGNAL(clicked()),this, SLOT(writeCommonConnectionSettings()));
    loadCommonConnectionSettings();
}

UdpComConnectDialog::~UdpComConnectDialog()
{
    delete mpUi;
}

void UdpComConnectDialog::loadCommonConnectionSettings()
{
    QSettings settings("DFKI","NDLCom");
    mpUi->hostname->addItem(settings.value("network/hostname").toString());
    mpUi->recvport->addItem(settings.value("network/recvPort").toString());
    mpUi->sendport->addItem(settings.value("network/sendPort").toString());
    mpUi->hostname->setCurrentIndex(mpUi->hostname->count()-1);
    mpUi->recvport->setCurrentIndex(mpUi->recvport->count()-1);
    mpUi->sendport->setCurrentIndex(mpUi->sendport->count()-1);
}

void UdpComConnectDialog::writeCommonConnectionSettings()
{
    QSettings networkSettings("DFKI","NDLCom");
    networkSettings.clear();
    networkSettings.setValue("network/hostname",mpUi->hostname->currentText());
    networkSettings.setValue("network/recvPort",mpUi->recvport->currentText());
    networkSettings.setValue("network/sendPort",mpUi->sendport->currentText());
    networkSettings.sync();
}


QString UdpComConnectDialog::getHostname()
{
    return mpUi->hostname->currentText();
}

int UdpComConnectDialog::getRecvport(){
    return mpUi->recvport->currentText().toInt();
}

int UdpComConnectDialog::getSendport()
{
    return mpUi->sendport->currentText().toInt();
}
