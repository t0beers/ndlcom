#include "NDLCom/AvailableRepresentations.h"

#include "representations/names.h"
#include "representations/representation.h"

#include "ui_AvailableRepresentations.h"

using namespace NDLCom;
using namespace Representations;

/* constructor of little widget to show all Representation-names and Device-name with their Id ! */
AvailableRepresentations::AvailableRepresentations(QWidget* parent) : QWidget(parent)
{
    Ui::AvailableRepresentations* mpUi = new Ui::AvailableRepresentations();
    mpUi->setupUi(this);

    int rowCounter = 0;
    for (int id = 0;id<=255;id++)
    {
        if (representationsNamesGetDeviceName(id))
        {
            rowCounter++;
            QString number = QString("0x%1").arg(id,2,16,QChar('0'));
            QString name(representationsNamesGetDeviceName(id));
            mpUi->deviceGrid->addWidget(new QLabel(number,this), rowCounter, 0);
            mpUi->deviceGrid->addWidget(new QLabel(name,this), rowCounter, 1);
        }
    }

    rowCounter = 0;
    for (int id = 0;id<=255;id++)
    {
        if (representationsNamesGetRepresentationName(id))
        {
            rowCounter++;
            QString number = QString("0x%1").arg(id,2,16,QChar('0'));
            QString size = QString::number(Representations::getSize(id));
            QString name(representationsNamesGetRepresentationName(id));
            mpUi->repreGrid->addWidget(new QLabel(number,this), rowCounter, 0);
            mpUi->repreGrid->addWidget(new QLabel(size,this), rowCounter, 1);
            mpUi->repreGrid->addWidget(new QLabel(name,this), rowCounter, 2);
        }
    }
}

