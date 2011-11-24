#include "NDLCom/ndlcom.h"
#include "NDLCom/interface_container.h"

#include <QDebug>

NDLCom::InterfaceContainer* NDLCom::getNDLComInstance()
{
    NDLCom::InterfaceContainer* retval = NDLCom::InterfaceContainer::getInterfaceContainer();

    if (!retval)
    {
        /* we could be nice and create the instance here... */
        return NULL;
    }
    return retval;
}

