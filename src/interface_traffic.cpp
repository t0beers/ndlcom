/**
 * @file src/interface_traffic.cpp
 * @author zenzes
 * @date 2011
 */

#include <QDebug>

#include "NDLCom/interface_traffic.h"

#include "ui_traffic.h"

NDLCom::InterfaceTraffic::InterfaceTraffic(QWidget* parent) : QWidget(parent)
{
    mpUi = new Ui::Traffic;
    mpUi->setupUi(this);
}

void NDLCom::InterfaceTraffic::rxTraffic(const QByteArray& data)
{
    if(isVisible())
    {
        mpUi->plainTextEdit_Rx->appendPlainText(data.toHex());
    }
}
void NDLCom::InterfaceTraffic::txTraffic(const QByteArray& data)
{
    if(isVisible())
    {
        mpUi->plainTextEdit_Tx->appendPlainText(data.toHex());
    }
}

