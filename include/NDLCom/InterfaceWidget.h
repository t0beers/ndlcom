#ifndef INTERFACEWIDGET_H
#define INTERFACEWIDGET_H

#include <QWidget>

namespace NDLCom
{
    namespace Ui
    {
        class InterfaceWidget;
    };

    class Interface;

    /** helper widget which add some gui-functionality */
    class InterfaceWidget : public QWidget
    {
        Q_OBJECT
    public:
        InterfaceWidget(QWidget* parent = 0, Interface* interface = 0);
        virtual ~InterfaceWidget();

    public slots:
        void setDeviceName(const QString& deviceName);

    private:

        /** @brief will add a widget showing current raw-traffic of this interface... */
        QAction* actionShowTraffic;
        /** used to show current raw-traffic of a Interface in a seperate window */
        QWidget* mpTrafficWindow;
        /** ... */
        Ui::InterfaceWidget* mpUi;

        Interface* mpInterface;

    private slots:
        /** handling a request for raw-data traffic */
        void on_actionShowTraffic_toggled(bool);
    };
};

#endif /*INTERFACEWIDGET_H*/
