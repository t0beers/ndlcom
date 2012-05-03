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

    mpUi->clearRx->setIcon(QIcon::fromTheme("edit-clear"));
    mpUi->clearTx->setIcon(QIcon::fromTheme("edit-clear"));
}

void NDLCom::InterfaceTraffic::rxTraffic(const QByteArray& data)
{
    if(isVisible())
    {
        QFont old = mpUi->plainTextEdit_Rx->font();
        QFont other = old;
        other.setBold(true);

        QStringList list(QString(data.toHex()).split("7e", QString::KeepEmptyParts));
        for (int i = 0;i<list.size();i++)
        {
            mpUi->plainTextEdit_Rx->setFont(other);
            mpUi->plainTextEdit_Rx->appendText(QString("7e"));
            mpUi->plainTextEdit_Rx->setFont(old);
            mpUi->plainTextEdit_Rx->appendText(list.at(i));
        }
            /* qDebug() << "list" << i << "7e" << list.at(i); */
    }
}
void NDLCom::InterfaceTraffic::txTraffic(const QByteArray& data)
{
    if(isVisible())
    {
        mpUi->plainTextEdit_Tx->appendPlainText(data.toHex());
    }
}

