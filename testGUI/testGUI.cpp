#include "testGUI.h"
#include "ui_testGUI.h"

#include "NDLCom/ndlcom_container.h"

#include <QDockWidget>
#include <QToolBar>
#include <QMenuBar>

testGUI::testGUI(QWidget *parent) : QMainWindow(parent)
{
    mpUi = new Ui::testGUI();
    mpUi->setupUi(this);

    /* carefull! in ctor of this class, QDockWidgets will be inserted into this QMainWindow! */
    NDLCom::NDLComContainer* ndlcom = new NDLCom::NDLComContainer(this);
    mpUi->menubar->addMenu(ndlcom->mpMenu);
    addToolBar(ndlcom->mpToolbar);
}

testGUI::~testGUI()
{

}
