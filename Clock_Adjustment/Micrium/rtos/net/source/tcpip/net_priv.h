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
*                                         NETWORK HEADER FILE
*
* File : net_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_PRIV_H_
#define  _NET_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        NETWORK INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "../../include/net_cfg_net.h"
#include  "net_type_priv.h"

#include  <rtos/net/source/util/net_svc_task_priv.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/include/rtos_path.h>
#include  <net_cfg.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  net_core_data {
    MEM_SEG             *CoreMemSegPtr;

    KAL_LOCK_HANDLE      GlobalLock;
    void                *GlobaLockFcntPtr;

    CPU_INT08U           IF_NbrTot;
    CPU_INT08U           IF_NbrCfgd;
    CPU_INT08U           IF_TxSuspendTimeout_ms;

    NET_SVC_TASK_HANDLE  SvcTaskHandle;
} NET_CORE_DATA;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  NET_CORE_DATA  *Net_CoreDataPtr;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void        NetCore_InitModules    (MEM_SEG       *p_mem_seg,
                                    RTOS_ERR      *p_err);

void        Net_InitCompWait       (void);                      /* Wait  until network initialization is complete.      */

void        Net_InitCompSignal     (void);                      /* Signal that network initialization is complete.      */

void        Net_GlobalLockAcquire  (void          *p_fcnt);     /* Acquire access to network protocol suite.            */

void        Net_GlobalLockRelease  (void);                      /* Release access to network protocol suite.            */

void        Net_TimeDly            (CPU_INT32U     time_dly_sec,
                                    CPU_INT32U     time_dly_us,
                                    RTOS_ERR      *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                    NETWORK CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  NET_CFG_OPTIMIZE_ASM_EN
#error  "NET_CFG_OPTIMIZE_ASM_EN        not #define'd in 'net_cfg.h'"
#error  "                         [MUST be  DEF_DISABLED]           "
#error  "                         [     ||  DEF_ENABLED ]           "

#elif  ((NET_CFG_OPTIMIZE_ASM_EN != DEF_DISABLED) && \
        (NET_CFG_OPTIMIZE_ASM_EN != DEF_ENABLED ))
#error  "NET_CFG_OPTIMIZE_ASM_EN  illegally #define'd in 'net_cfg.h'"
#error  "                         [MUST be  DEF_DISABLED]           "
#error  "                         [     ||  DEF_ENABLED ]           "
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_PRIV_H_ */

