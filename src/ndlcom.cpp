#include "NDLCom/ndlcom.h"
#include "NDLCom/interface_container.h"

#include <QDebug>

NDLCom::InterfaceContainer* NDLCom::getNDLComInstance()
{
#warning NDLCom::getNDLComInstance() deprecated, use 'NDLCom::getInstance()' instead
    return NDLCom::getInstance();
}

NDLCom::InterfaceContainer* NDLCom::getInstance()
{
    return NDLCom::InterfaceContainer::getInstance();
}

