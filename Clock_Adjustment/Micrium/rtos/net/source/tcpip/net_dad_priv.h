/*
*********************************************************************************************************
*                                              Micrium OS
*                                               Network
*
*                             (c) Copyright 2004; Silicon Laboratories Inc.
*                                        https://www.micrium.com
*
*********************************************************************************************************
* Licensing:
*           YOUR USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS OF A MICRIUM SOFTWARE LICENSE.
*   If you are not willing to accept the terms of an appropriate Micrium Software License, you must not
*   download or use this software for any reason.
*   Information about Micrium software licensing is available at https://www.micrium.com/licensing/
*   It is your obligation to select an appropriate license based on your intended use of the Micrium OS.
*   Unless you have executed a Micrium Commercial License, your use of the Micrium OS is limited to
*   evaluation, educational or personal non-commercial uses. The Micrium OS may not be redistributed or
*   disclosed to any third party without the written consent of Silicon Laboratories Inc.
*********************************************************************************************************
* Documentation:
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*********************************************************************************************************
* Technical Support:
*   Support is available for commercially licensed users of Micrium's software. For additional
*   information on support, you can contact info@micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           NETWORK DAD LAYER
*                                     (DUPLICATION ADDRESS DETECTION)
*
* File : net_dad_priv.h
*********************************************************************************************************
* Note(s) : (1) Supports Duplicate address detection as described in RFC #4862.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_DAD_PRIV_H_
#define  _NET_DAD_PRIV_H_

#include  "../../include/net_cfg_net.h"

#ifdef   NET_DAD_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "net_ipv6_priv.h"
#include  "net_priv.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          DAD DATA TYPE
*********************************************************************************************************
*/

typedef  struct  net_dad_obj  NET_DAD_OBJ;

typedef  void  (*NET_DAD_FNCT)(NET_IF_NBR                 if_nbr,
                               NET_DAD_OBJ               *p_dad_obj,
                               NET_IPv6_ADDR_CFG_STATUS   status);

struct net_dad_obj {
    NET_DAD_OBJ        *NextPtr;
    NET_IPv6_ADDR       Addr;
    KAL_SEM_HANDLE      SignalErr;
    KAL_SEM_HANDLE      SignalCompl;
    CPU_BOOLEAN         NotifyComplEn;
    NET_DAD_FNCT        Fnct;
};


/*
*********************************************************************************************************
*                                        DAD SIGNAL DATA TYPE
*********************************************************************************************************
*/

typedef enum net_dad_signal_type {
    NET_DAD_SIGNAL_TYPE_ERR,
    NET_DAD_SIGNAL_TYPE_COMPL,
} NET_DAD_SIGNAL_TYPE;


typedef  enum net_dad_status {
    NET_DAD_STATUS_NONE,
    NET_DAD_STATUS_SUCCEED,
    NET_DAD_STATUS_IN_PROGRESS,
    NET_DAD_STATUS_FAIL
} NET_DAD_STATUS;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MACRO'S
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void              NetDAD_Init                (       RTOS_ERR                *p_err);


NET_DAD_STATUS    NetDAD_Start               (       NET_IF_NBR               if_nbr,
                                                     NET_IPv6_ADDR           *p_addr,
                                                     NET_IPv6_ADDR_CFG_TYPE   addr_cfg_type,
                                                     NET_DAD_FNCT             dad_hook_fnct,
                                                     RTOS_ERR                *p_err);

void              NetDAD_Stop                (       NET_IF_NBR               if_nbr,
                                                     NET_DAD_OBJ             *p_dad_obj);

NET_DAD_OBJ      *NetDAD_ObjGet              (       RTOS_ERR                *p_err);

void              NetDAD_ObjRelease          (       NET_DAD_OBJ             *p_dad_obj);

NET_DAD_OBJ      *NetDAD_ObjSrch             (       NET_IPv6_ADDR           *p_addr);

void              NetDAD_SignalWait          (       NET_DAD_SIGNAL_TYPE      signal_type,
                                                     NET_DAD_OBJ             *p_dad_obj,
                                                     RTOS_ERR                *p_err);

void              NetDAD_Signal              (       NET_DAD_SIGNAL_TYPE      signal_type,
                                                     NET_DAD_OBJ             *p_dad_obj);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_DAD_MODULE_EN */
#endif  /* _NET_DAD_PRIV_H_  */
