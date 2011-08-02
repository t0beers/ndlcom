/**
 * @file include/NDLCom/interface_traffic.h
 * @author zenzes
 * @date 2011
 */

#ifndef _NDLCOM_INTERFACE_TRAFFIC_H_
#define _NDLCOM_INTERFACE_TRAFFIC_H_

#include <QWidget>

namespace NDLCom
{
    class Message;

    namespace Ui
    {
        class Traffic;
    };

    class InterfaceTraffic : public QWidget
    {
        Q_OBJECT
    public:
        InterfaceTraffic(QWidget* parent = 0);
        virtual ~InterfaceTraffic() {};

    public slots:
        void txTraffic(const QByteArray&);
        void rxTraffic(const QByteArray&);

    private:
        Ui::Traffic* mpUi;
    };
};

#endif/*_NDLCOM_INTERFACE_TRAFFIC_H_*/
