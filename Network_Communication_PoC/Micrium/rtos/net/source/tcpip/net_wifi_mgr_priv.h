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
*                                       NETWORK WIRELESS MANAGER
*
* File : net_wifi_mgr_priv.h
*********************************************************************************************************
* Note(s) : (1) The wireless hardware used with this wireless manager is assumed to embed the wireless
*               supplicant within the wireless hardware and provide common command.
*
*           (2) Interrupt support is Hardware specific, therefore the generic Wireless manager does NOT
*               support interrupts.  However, interrupt support is easily added to the generic wireless
*               manager & thus the ISR handler has been prototyped and & populated within the function
*               pointer structure for example purposes.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_WIFI_MGR_PRIV_H_
#define  _NET_WIFI_MGR_PRIV_H_

#include  <rtos/net/include/net_cfg_net.h>

#ifdef    NET_IF_WIFI_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/cpu/include/cpu.h>

#include  <rtos/common/include/lib_str.h>
#include  <rtos/common/source/KAL/kal_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

struct  net_wifi_mgr_ctx {
    CPU_BOOLEAN  WaitResp;
    CPU_INT32U   WaitRespTimeout_ms;
    CPU_BOOLEAN  MgmtCompleted;
};


typedef  struct  net_if_wifi_mgr_data {
    KAL_LOCK_HANDLE  MgrLock;
    KAL_Q_HANDLE     MgmtSignalResp;
    CPU_BOOLEAN      DevStarted;
    CPU_BOOLEAN      AP_Joined;
    CPU_BOOLEAN      AP_Created;
} NET_WIFI_MGR_DATA;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_IF_WIFI_MODULE_EN */
#endif  /* _NET_WIFI_MGR_PRIV_H_ */
