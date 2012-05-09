/**
 * @file include/NDLCom/interface_container.h
 * @author Martin Zenzes
 * @date 2011, 2012
 */
#ifndef INTERFACE_CONTAINER_WIDGET_H
#define INTERFACE_CONTAINER_WIDGET_H

#include <QWidget>
#include <QMap>
#include <QMenu>

namespace NDLCom
{

    namespace Ui
    {
        class InterfaceContainerWidget;
    };

    class Message;
    class Interface;
    class InterfaceWidget;

    /**
     * @brief a graphical representations of the underlying interface_container
     *
     * this may be instantiated inside a gui-skeleton and display the current state of the
     * interfaces
     *
     * we have our own QMenu, which offers the possibility to disconnect running/active connections.
     * also, we supply the proper actions to create new interfaces. raw telegram data (and status
     * messages) are not emitted/received from this widget, but from the "InterfaceContainer"
     * singleton.
     */
    class InterfaceContainerWidget : public QWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief standard qt constructor...
         *
         * @param parent normally "this" of the main-window or something...
         */
        InterfaceContainerWidget(QWidget* parent = NULL);
        /**
         * @brief Destructor of InterfaceContainer class. Doing cleanup here
         */
        virtual ~InterfaceContainerWidget(){};

        /** for every kind of new interface, we need one action which knows how to create it */
        QAction* actionConnectSerial;
        /** for every kind of new interface, we need one action which knows how to create it */
        QAction* actionConnectUdp;
        /** menu which collect the disconnect actions of all interfaces, aswell as the
         * "disconnectAll" of the InterfaceContainer */
        QMenu* mpDisconnectMenu;

    private:

        Ui::InterfaceContainerWidget* mpUi;

        /* simple mapping between each interface and the appropriate widget. used to clean up gui
         * during disconnects */
        QMap<Interface*,InterfaceWidget*> interfaceWidgets;

    private slots:
        void connected();
        void disconnected();

        void on_actionConnectSerial_triggered();
        void on_actionConnectUdp_triggered();

        void updateIcons();
    };
};

#endif /*INTERFACE_CONTAINER_WIDGET_H*/
