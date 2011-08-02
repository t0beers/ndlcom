/**
 * @file lib/widgets/include/widgets/udpcom_.h
 * @brief Qt wrapper (child of QObject) around lib/udpcom.
 * Project: iStruct
 * @date 07/2011
 * @author Armin
 */

#ifndef _NDLCOM_SERIALCOM_H_
#define _NDLCOM_SERIALCOM_H_

#include <QObject>
#include <QDialog>

#include "NDLCom/interface.h"

class SerialcomPortHandler;

namespace NDLCom
{
    /**
     * @brief widget to use lib/udpcom from qt.
     */
    class Serialcom: public Interface
    {
        Q_OBJECT
    public:
        Serialcom(QWidget *parent=0);
        virtual ~Serialcom();

    private slots:
        void txMessage(const ::NDLCom::Message&);

        void receivedData(const QByteArray&);

        void on_actionConnect_triggered();
        void on_actionDisconnect_triggered();

    private:
        SerialcomPortHandler* mpPorthandler;
        QString mPortname;

        ProtocolParser* mpProtocolParser;
        char* mpProtocolBuffer;
    };
};

#endif/*_NDLCOM_SERIALCOM_H_*/

