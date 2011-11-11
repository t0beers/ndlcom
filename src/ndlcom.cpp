#include "NDLCom/ndlcom.h"
#include "NDLCom/interface_container.h"

#include <QDebug>

NDLCom::InterfaceContainer* NDLCom::getNDLComInstance()
{
    NDLCom::InterfaceContainer* retval = NDLCom::InterfaceContainer::getInterfaceContainer();

    if (!retval)
    {
        qWarning() << "NDLCom: could not find signal emitting object. this is nearly fatal...?";
        return NULL;
    }
    return retval;
}

