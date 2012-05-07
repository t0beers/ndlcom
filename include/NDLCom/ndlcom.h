/**
 * @file include/NDLCom/ndlcom.h
 * @author Martin Zenzes
 * @date 2011
 */
#ifndef _NDLCOM_NDLCOM_H_
#define _NDLCOM_NDLCOM_H_

#include "NDLCom/interface_container.h"

namespace NDLCom
{
    /** FIXME to be removed. the name of this function is missleading/inconsistent... use the
     * alternative "getInstance()" */
    NDLCom::InterfaceContainer* getNDLComInstance();
    NDLCom::InterfaceContainer* getInstance();
}

#endif/*_NDLCOM_NDLCOM_H_*/

