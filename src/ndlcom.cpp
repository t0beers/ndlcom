#include "NDLCom/ndlcom.h"
#include "NDLCom/interface_container.h"

#include <QDebug>

NDLCom::InterfaceContainer* NDLCom::getNDLComInstance()
{
    return NDLCom::InterfaceContainer::getInterfaceContainer();
}

