/**
 * @file src/interface_traffic.cpp
 * @author zenzes
 * @date 2011
 */

#include <QDebug>

#include "NDLCom/interface_traffic.h"

#include "ui_traffic.h"

using namespace NDLCom;

InterfaceTraffic::InterfaceTraffic(QWidget* parent) : QWidget(parent)
{
    mpUi = new Ui::Traffic;
    mpUi->setupUi(this);

    mpUi->clear->setIcon(QIcon::fromTheme("edit-clear"));
    mpUi->pause->setIcon(QIcon::fromTheme("media-playback-pause"));
    mPause = false;

    mpHighlight_Tx = new Highlighter(mpUi->plainTextEdit_Tx->document());
    mpHighlight_Rx = new Highlighter(mpUi->plainTextEdit_Rx->document());

    setToolTip(QString("red: protocol flag (0x7e, only allowd between telegrams)\n") +
               QString("blue: escaped protocol flags (0x7e -> 0x7d 0x5e)\n") +
               QString("green: escaped escaped (0x7d -> 0x7d 0x5d"));
}

void InterfaceTraffic::on_pause_clicked()
{
    if (mPause) {
        mPause=false;
        mpUi->pause->setIcon(QIcon::fromTheme("media-playback-pause"));
        mpUi->pause->setText("Pause");
    } else {
        mPause=true;
        mpUi->pause->setIcon(QIcon::fromTheme("media-playback-start"));
        mpUi->pause->setText("Resume");
    }
}

void InterfaceTraffic::rxTraffic(const QByteArray& data)
{
    if(isVisible() && !mPause)
    {
        mpUi->plainTextEdit_Rx->appendPlainText(data.toHex());
    }
}
void InterfaceTraffic::txTraffic(const QByteArray& data)
{
    if(isVisible() && !mPause)
    {
        mpUi->plainTextEdit_Tx->appendPlainText(data.toHex());
    }
}

Highlighter::Highlighter(QTextDocument* parent) :
    QSyntaxHighlighter(parent)
{
    HighlightingRule protocolFlag;
    protocolFlag.pattern = QRegExp("7e");
    protocolFlag.format.setForeground(Qt::red);
    highlightingRules.append(protocolFlag);

    /* escaped flags */
    /* 0x7e --> 0x7d 0x5e */
    HighlightingRule escapedProtocolFlag;
    escapedProtocolFlag.pattern = QRegExp("7d5e");
    escapedProtocolFlag.format.setForeground(Qt::blue);
    highlightingRules.append(escapedProtocolFlag);

    /* escaped escapes */
    /* 0x7d --> 0x7d 0x5d */
    HighlightingRule escapedEscape;
    escapedEscape.pattern = QRegExp("7d5d");
    escapedEscape.format.setForeground(Qt::green);
    highlightingRules.append(escapedEscape);

}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}
