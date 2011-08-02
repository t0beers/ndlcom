/**
 * @file include/NDLCom/message_traffic.h
 * @author zenzes
 * @date 2011
 */

#ifndef _NDLCOM_MESSAGE_TRAFFIC_H_
#define _NDLCOM_MESSAGE_TRAFFIC_H_

#include <QWidget>

namespace NDLCom
{
    class Message;

    namespace Ui
    {
        class Traffic;
    };

    class MessageTraffic : public QWidget
    {
        Q_OBJECT
    public:
        MessageTraffic(QWidget* parent = 0);
        virtual ~MessageTraffic() {};

    public slots:
        void sentMessage(const ::NDLCom::Message&);
        void receivedMessage(const ::NDLCom::Message&);

    private:
        Ui::Traffic* mpUi;

        QString formatMessage(const ::NDLCom::Message&);
    };
};

#endif/*_NDLCOM_MESSAGE_TRAFFIC_H_*/
