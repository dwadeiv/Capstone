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
*                                      NETWORK COUNTER MANAGEMENT
*
* File : net_ctr.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_NET_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_cfg_net.h>
#include  "net_ctr_priv.h"
#include  "net_priv.h"

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_mem.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                                  (NET)
#define  RTOS_MODULE_CUR                               RTOS_CFG_MODULE_NET


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
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           STATISTIC COUNTERS
*********************************************************************************************************
*/

#if (NET_CTR_CFG_STAT_EN == DEF_ENABLED)
NET_CTR_STATS  Net_StatCtrs;
#endif


/*
*********************************************************************************************************
*                                           ERROR COUNTERS
*********************************************************************************************************
*/

#if (NET_CTR_CFG_ERR_EN  == DEF_ENABLED)
NET_CTR_ERRS   Net_ErrCtrs;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            NetCtr_Init()
*
* Description : (1) Initialize Network Counter Management Module :
*
*                   (a) Initialize network statistics counters
*                   (b) Initialize network error      counters
*
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetCtr_Init (MEM_SEG   *p_mem_seg,
                   RTOS_ERR  *p_err)
{
    PP_UNUSED_PARAM(p_mem_seg);
    PP_UNUSED_PARAM(p_err);

#if (NET_CTR_CFG_STAT_EN == DEF_ENABLED)                        /* ---------------- INIT NET STAT CTRS ---------------- */
    Mem_Clr(&Net_StatCtrs,
             sizeof(Net_StatCtrs));
#endif

#if (NET_CTR_CFG_ERR_EN  == DEF_ENABLED)                        /* ---------------- INIT NET ERR  CTRS ---------------- */
    Mem_Clr(&Net_ErrCtrs,
             sizeof(Net_ErrCtrs));
#endif

#if (NET_CTR_CFG_STAT_EN == DEF_ENABLED)
    Net_StatCtrs.IFs.IF = (NET_CTR_IF_STATS *)Mem_SegAlloc("Stat Counter table for Interfaces",
                                                            p_mem_seg,
                                                            sizeof(NET_CTR_IF_STATS) * Net_CoreDataPtr->IF_NbrTot,
                                                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
#endif

#if (NET_CTR_CFG_ERR_EN  == DEF_ENABLED)
    Net_ErrCtrs.IFs.IF = (NET_CTR_IF_ERRS *)Mem_SegAlloc("Error Counter table for Interfaces",
                                                            p_mem_seg,
                                                            sizeof(NET_CTR_IF_ERRS) * Net_CoreDataPtr->IF_NbrTot,
                                                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
#endif
}


/*
*********************************************************************************************************
*                                            NetCtr_Inc()
*
* Description : Increment a network counter.
*
* Argument(s) : pctr        Pointer to a network counter.
*
* Return(s)   : none.
*
* Note(s)     : (1) Network counter variables MUST ALWAYS be accessed exclusively in critical sections.
*
*                   See also 'net_ctr.h  NETWORK COUNTER MACRO'S  Note #1a'.
*********************************************************************************************************
*/

#ifdef  NET_CTR_MODULE_EN
void  NetCtr_Inc (NET_CTR  *p_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
  (*p_ctr)++;
    CPU_CRITICAL_EXIT();
}
#endif


/*
*********************************************************************************************************
*                                          NetCtr_IncLarge()
*
* Description : Increment a large network counter.
*
* Argument(s) : pctr_hi     Pointer to high half of a large network counter.
*
*               pctr_lo     Pointer to low  half of a large network counter.
*
* Return(s)   : none.
*
* Note(s)     : (1) Network counter variables MUST ALWAYS be accessed exclusively in critical sections.
*
*                   See also 'net_ctr.h  NETWORK COUNTER MACRO'S  Note #1b'.
*********************************************************************************************************
*/

#ifdef  NET_CTR_MODULE_EN
void  NetCtr_IncLarge (NET_CTR  *p_ctr_hi,
                       NET_CTR  *p_ctr_lo)
{
  (*p_ctr_lo)++;                                                /* Inc lo-half ctr.                                     */
    if (*p_ctr_lo == 0u) {                                      /* If  lo-half ctr ovfs, ...                            */
       (*p_ctr_hi)++;                                           /* inc hi-half ctr.                                     */
    }
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL */

