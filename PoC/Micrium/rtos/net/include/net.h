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
*                                       NETWORK CORE HEADER FILE
*
* File : net.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_H_
#define  _NET_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/dns_client.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/include/rtos_types.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     NET CORE DEFAULT CFG VALUES
*********************************************************************************************************
*/

#define  NET_CORE_TASK_CFG_STK_SIZE_ELEMENTS_DFLT                   1024u
#define  NET_CORE_TASK_CFG_STK_PTR_DFLT                             DEF_NULL

#define  NET_CORE_SVC_TASK_CFG_STK_SIZE_ELEMENTS_DFLT               512u
#define  NET_CORE_SVC_TASK_CFG_STK_PTR_DFLT                         DEF_NULL

#define  NET_CORE_MEM_SEG_PTR_DFLT                                  DEF_NULL


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  net_init_cfg {
    DNSc_CFG       DNSc_Cfg;

    CPU_STK_SIZE   CoreStkSizeElements;                         /* Size of the stk, in CPU_STK elements.                */
    void          *CoreStkPtr;                                  /* Ptr to base of the stk.                              */
    CPU_STK_SIZE   CoreSvcStkSizeElements;                      /* Size of the stk, in CPU_STK elements.                */
    void          *CoreSvcStkPtr;                               /* Ptr to base of the stk.                              */

    MEM_SEG       *MemSegPtr;                                   /* Ptr to network core mem seg.                         */
} NET_INIT_CFG;

typedef  struct  net_cfg {
    DNSc_CFG  DNSc_Cfg;
} NET_CFG;


#if 0 /* TODO_NET coming soon */
typedef  struct  net_cfg  NET_CFG;

struct  net_cfg {
    CPU_INT08U  TmrNbrMax;
    CPU_INT08U  IF_NbrMax;
    CPU_INT08U  IPv4_AddrNbrMax;
    CPU_INT08U  IPv6_AddrNbrMax;
    CPU_INT08U  IPv6_RouterNbrMax;
    CPU_INT08U  IPv6_PrefixNbrMax;
    CPU_INT08U  IPv6_DestCacheNbrMax;
    CPU_INT08U  McastHostGrpNbrMax;
};
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
extern  const  NET_INIT_CFG  Net_InitCfgDflt;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
void  Net_ConfigureCoreTaskStk     (CPU_STK_SIZE     stk_size_elements,
                                    void            *p_stk_base);

void  Net_ConfigureCoreSvcTaskStk  (CPU_STK_SIZE     stk_size_elements,
                                    void            *p_stk_base);

void  Net_ConfigureMemSeg          (MEM_SEG         *p_mem_seg);

void  Net_ConfigureDNS_Client      (DNSc_CFG        *p_cfg);
#endif

void  Net_Init                     (RTOS_ERR        *p_err);

void  Net_CoreTaskPrioSet          (RTOS_TASK_PRIO   prio,
                                    RTOS_ERR        *p_err);

void  Net_CoreSvcTaskPrioSet       (RTOS_TASK_PRIO   prio,
                                    RTOS_ERR        *p_err);

#ifdef __cplusplus
}
#endif

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_H_ */

