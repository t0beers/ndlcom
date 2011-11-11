#include "testGUI.h"
#include "ui_testGUI.h"

#include "NDLCom/ndlcom_container.h"
#include "NDLCom/ndlcom.h"

#include <QDockWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QDebug>

testGUI::testGUI(QWidget *parent) : QMainWindow(parent)
{
    mpUi = new Ui::testGUI();
    mpUi->setupUi(this);

    /* carefull! in ctor of this class, QDockWidgets will be inserted into this QMainWindow! */
    NDLCom::NDLComContainer* ndlcomcontainer = new NDLCom::NDLComContainer(this);
    mpUi->menubar->addMenu(ndlcomcontainer->mpMenu);
    addToolBar(ndlcomcontainer->mpToolbar);

    /* getting pointer to signal-emitting object */
    NDLCom::InterfaceContainer* ndlcom = NDLCom::getNDLComInstance();
    if (!ndlcom)
        qCritical() << "testGUI: Can't get the pointer to the NDLCom widget.";

    QStatusBar *statusbar = new QStatusBar(this);
    setStatusBar(statusbar);
    QLabel* status = new QLabel(this);
    statusbar->addPermanentWidget(status);
    connect(ndlcom, SIGNAL(transferRate(QString)), status, SLOT(setText(QString)));
}

testGUI::~testGUI()
{

}
