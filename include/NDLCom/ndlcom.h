/**
 * @file include/NDLCom/ndlcom.h
 * @author Martin Zenzes
 * @date 2011
 */
#ifndef _NDLCOM_NDLCOM_H_
#define _NDLCOM_NDLCOM_H_

#include "NDLCom/interface_container.h"

/* if this message bites you: go away... */
#if QT_VERSION < 0x040600
#error "NDLCom and iStruct don't support Qt below 4.6"
#endif

namespace NDLCom
{
    /** FIXME to be removed. the name of this function is missleading/inconsistent... use the
     * alternative "getInstance()" */
    NDLCom::InterfaceContainer* getNDLComInstance();
    NDLCom::InterfaceContainer* getInstance();
}

#endif/*_NDLCOM_NDLCOM_H_*/

