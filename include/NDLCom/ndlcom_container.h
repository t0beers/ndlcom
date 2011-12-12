/**
 * @file lib/NDLCom/include/NDLCom/ndlcom_container.h
 * @author Martin Zenzes
 * @date 2011-07-28
 */
#ifndef _NDLCOM_NDLCOM_CONTAINER_H_
#define _NDLCOM_NDLCOM_CONTAINER_H_

#include "NDLCom/message.h"

#include <QToolBar>
#include <QMenu>

/**
 * @addtogroup Communication
 * @{
 * @defgroup Communication_NDLCom NDLCom
 * @{
 */

namespace NDLCom
{
    class InterfaceContainer;

    /**
     * @brief This is a very cool Widget!
     *
     * It provides the NDLCom::NDLComContainer-class, which can be easily added
     * to your application! Just do like in test/testGUI.cpp
     * {{{
     *     // carefull! in ctor of this class, QDockWidgets will be inserted into this QMainWindow!
     *     NDLCom::NDLComContainer* ndlcom = new NDLCom::NDLComContainer(this);
     *     mpUi->menubar->addMenu(ndlcom->mpMenu);
     *     addToolBar(ndlcom->mpToolbar);
     * }}}
     * and this widget will add its sub-widgets into your QDockWidget area. By
     * adding the menu- and toolbar into your application, all functionality is
     * made accessible to the user. Nice!
     *
     * In the container-widget-class, new'ing of the needed sub-modules is
     * done. Connecting all internal signals and creating the QMenu and
     * QToolBar.
     *
     * The following sub-widgets are provided:
     * - Composer() -- the second generation TelegramComposer. Better than ever!
     * - CommunicationStatistics -- real-time receiving rate of messages
     * - RepresentationsInfo -- statically showing all known representation-ids
     * - MessageTraffic -- showing which messages are globally going in an out.
     * - InterfaceContainer -- of course, the interface to manage all individual connections.
     *   Allows showing raw-data-traffic of each interface.
     *
     * This whole class is based on the NDLCom::Message class. This class does
     * a timestamp when its created (during receive normally) and helps in
     * tracking the order of packages from different interfaces. It also
     * contains the ProtocolHeader of this message as well as the decoded
     * raw-data as a char-array. This data can be used by other applications.
     * Or it can be casted to a Representations::blabla class, which is done
     * in NDLCom::RepresentationsMapper.
     *
     * The NDLCom::NDLComContainer emits signals of type "const Message&"
     * (originating from NDLCom::NDLCom) as well as all currently known
     * Representations, correctly casted to the right one (originating from
     * RepresentationsMapper). It receives signals of "const Message&", which
     * are then encoded and transferred to all running interfaces.
     *
     * Each connection is represented by the same base-class
     * NDLCom::Interface, which handles all the "low-level" routine work. For
     * example calling connect/dsconnect, pause/resume, setting the correct
     * icons, showing data-rate and raw-traffic. In the moment, two kinds of
     * real interfaces are provided -- serialcom and udpcom. Of both
     * sub-types, arbitrary number of connections can be opened inside
     * NDLCom::InterfaceContainer, which then all are capable of sending and receiving
     * data. All connections can be closed on one click as well as individually,
     * via QToolBar or QMenu.
     *
     * It was tried to use QAction as often as possible, to have all the actions in a
     * unified interface. This allows easy use of QToolBar and QMenu.
     *
     * TODO:
     * - since we don't have a routing algorithm, all telegrams can be send
     *   on all interfaces! this can be disabled in the gui using the "bridging" checkbox
     *
     * FIXME:
     * - something with the NDLCom::Message() class is broken on accessing
     *   references... grep the code...
     *
     * This text was basically the used commit-message for the initial commit. for a
     * more elaborate documentation, see the other classes.
     */
    class NDLComContainer : public QWidget
    {
        Q_OBJECT
    public:
         /**
          * creates a nice collection of usefull tools, all related to ndlcom
          *
          * if second argument is false, the toolbar will not be shown, usefull if you want to build
          * your own application layout, but still want to use the wrapped creation of NDLCom-style
          * widgets in this class
          */
        NDLComContainer(QWidget* parent = NULL, bool showToolBar = true);
        virtual ~NDLComContainer() {};
        /** contains selection of sub-widgets aswell as some further actions */
        QMenu* mpMenu;
        /** contains all usable NDLCom-sub-widgets */
        QToolBar* mpToolbar;
    signals:
        /** data coming from the hardware */
        void rxMessage(const NDLCom::Message&);
        /** current datarates and amounts of all interfaces */
        void status(QString, double);
    public slots:
        /** data going to the hardware */
        void txMessage(const NDLCom::Message&);
    private:
        /** the main-widget, needed to feed data into it */
        InterfaceContainer* mpInterfaceContainer;
    private slots:
        /** used for internal receiving of signals, to be able to echo them */
        void slot_rxMessage(const NDLCom::Message&);
    };
};

/**
 * @}
 * @}
 */

#endif/*_NDLCOM_NDLCOM_CONTAINER_H_*/

