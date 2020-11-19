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
*                                     NETWORK STATISTICS MANAGEMENT
*
* File : net_stat.c
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

#include  "net_stat_priv.h"
#include  "net_ctr_priv.h"

#include  <rtos/common/source/rtos/rtos_utils_priv.h>


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
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            NetStat_Init()
*
* Description : (1) Initialize Network Statistic Management Module :
*
*                   Module initialization NOT yet required/implemented
*
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetStat_Init (void)
{
}


/*
*********************************************************************************************************
*                                          NetStat_CtrInit()
*
* Description : Initialize a statistics counter.
*
* Argument(s) : p_stat_ctr      Pointer to a statistics counter (see Note #1).
*
* Return(s)   : none.
*
* Note(s)     : (1) Assumes 'p_stat_ctr' points to valid statistics counter (if non-NULL).
*
*               (2) Statistic counters MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_CtrInit (NET_STAT_CTR  *p_stat_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_ctr->CurCtr = 0u;
    p_stat_ctr->MaxCtr = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                          NetStat_CtrClr()
*
* Description : Clear a statistics counter.
*
* Argument(s) : p_stat_ctr      Pointer to a statistics counter (see Note #1).
*
* Return(s)   : none.
*
* Note(s)     : (1) Assumes 'p_stat_ctr' points to valid statistics counter (if non-NULL).
*
*               (2) Statistic counters MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_CtrClr (NET_STAT_CTR  *p_stat_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_ctr->CurCtr = 0u;
    p_stat_ctr->MaxCtr = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                         NetStat_CtrReset()
*
* Description : Reset a statistics counter.
*
* Argument(s) : p_stat_ctr      Pointer to a statistics counter.
*
* Return(s)   : none.
*
* Note(s)     : (1) Statistic counters MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_CtrReset (NET_STAT_CTR  *p_stat_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_ctr->CurCtr = 0u;
    p_stat_ctr->MaxCtr = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                        NetStat_CtrResetMax()
*
* Description : Reset a statistics counter's maximum number of counts.
*
*               (1) Resets maximum number of counts to the current number of counts.
*
*
* Argument(s) : p_stat_ctr      Pointer to a statistics counter.
*
* Return(s)   : none.
*
* Note(s)     : (2) Statistic counters MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_CtrResetMax (NET_STAT_CTR  *p_stat_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_ctr->MaxCtr = p_stat_ctr->CurCtr;                    /* Reset max cnts (see Note #1).                        */
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                          NetStat_CtrInc()
*
* Description : Increment a statistics counter.
*
* Argument(s) : p_stat_ctr      Pointer to a statistics counter.
*
* Return(s)   : none.
*
* Note(s)     : (1) Statistic counters MUST ALWAYS be accessed exclusively in critical sections.
*
*               (2) Statistic counter increment overflow prevented but ignored.
*
*                   See also 'NetStat_CtrDec()  Note #2'.
*********************************************************************************************************
*/

void  NetStat_CtrInc (NET_STAT_CTR  *p_stat_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_stat_ctr->CurCtr < NET_CTR_MAX) {                     /* See Note #2.                                         */
        p_stat_ctr->CurCtr++;

        if (p_stat_ctr->MaxCtr < p_stat_ctr->CurCtr) {          /* If max cnt < cur cnt, set new max cnt.               */
            p_stat_ctr->MaxCtr = p_stat_ctr->CurCtr;
        }
    }
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                          NetStat_CtrDec()
*
* Description : Decrement a statistics counter.
*
* Argument(s) : p_stat_ctr      Pointer to a statistics counter.
*
* Return(s)   : none.
*
* Note(s)     : (1) Statistic counters MUST ALWAYS be accessed exclusively in critical sections.
*
*               (2) Statistic counter decrement underflow prevented but ignored.
*
*                   See also 'NetStat_CtrInc()  Note #2'.
*********************************************************************************************************
*/

void  NetStat_CtrDec (NET_STAT_CTR  *p_stat_ctr)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_stat_ctr->CurCtr > NET_CTR_MIN) {                     /* See Note #2.                                         */
        p_stat_ctr->CurCtr--;
    }
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                         NetStat_PoolInit()
*
* Description : Initialize a statistics pool.
*
* Argument(s) : p_stat_pool     Pointer to a statistics pool (see Note #1).
*
*               nbr_avail       Total number of available statistics pool entries.
*
* Return(s)   : none.
*
* Note(s)     : (1) Assumes 'p_stat_pool' points to valid statistics pool (if non-NULL).
*
*               (2) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolInit (NET_STAT_POOL      *p_stat_pool,
                        NET_STAT_POOL_QTY   nbr_avail)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_pool->EntriesInit       = nbr_avail;                 /* Init nbr of pool  entries is also ...                */
    p_stat_pool->EntriesTot        = nbr_avail;                 /* Tot  nbr of pool  entries is also ...                */
    p_stat_pool->EntriesAvail      = nbr_avail;                 /* Init nbr of avail entries.                           */
    p_stat_pool->EntriesUsed       = 0u;
    p_stat_pool->EntriesUsedMax    = 0u;
    p_stat_pool->EntriesLostCur    = 0u;
    p_stat_pool->EntriesLostTot    = 0u;
    p_stat_pool->EntriesAllocCtr   = 0u;
    p_stat_pool->EntriesDeallocCtr = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                          NetStat_PoolClr()
*
* Description : Clear a statistics pool.
*
* Argument(s) : p_stat_pool     Pointer to a statistics pool (see Note #1).
*
* Return(s)   : none.
*
* Note(s)     : (1) Assumes 'p_stat_pool' points to valid statistics pool (if non-NULL).
*
*               (2) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolClr (NET_STAT_POOL  *p_stat_pool)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_pool->EntriesInit       = 0u;
    p_stat_pool->EntriesTot        = 0u;
    p_stat_pool->EntriesAvail      = 0u;
    p_stat_pool->EntriesUsed       = 0u;
    p_stat_pool->EntriesUsedMax    = 0u;
    p_stat_pool->EntriesLostCur    = 0u;
    p_stat_pool->EntriesLostTot    = 0u;
    p_stat_pool->EntriesAllocCtr   = 0u;
    p_stat_pool->EntriesDeallocCtr = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                         NetStat_PoolReset()
*
* Description : Reset a statistics pool.
*
*               (1) Assumes object pool is also reset; otherwise, statistics pool will NOT accurately
*                   reflect the state of the object pool.
*
*
* Argument(s) : p_stat_pool     Pointer to a statistics pool.
*
* Return(s)   : none.
*
* Note(s)     : (2) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolReset (NET_STAT_POOL  *p_stat_pool)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_pool->EntriesAvail      = p_stat_pool->EntriesTot;
    p_stat_pool->EntriesUsed       = 0u;
    p_stat_pool->EntriesUsedMax    = 0u;
    p_stat_pool->EntriesLostCur    = 0u;
    p_stat_pool->EntriesAllocCtr   = 0u;
    p_stat_pool->EntriesDeallocCtr = 0u;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                     NetStat_PoolResetUsedMax()
*
* Description : Reset a statistics pool's maximum number of entries used.
*
*               (1) Resets maximum number of entries used to the current number of entries used.
*
*
* Argument(s) : p_stat_pool     Pointer to a statistics pool.
*
* Return(s)   : none.
*
* Note(s)     : (2) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolResetUsedMax (NET_STAT_POOL  *p_stat_pool)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_stat_pool->EntriesUsedMax = p_stat_pool->EntriesUsed;     /* Reset nbr max used (see Note #1).                    */
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                     NetStat_PoolEntryUsedInc()
*
* Description : Increment a statistics pool's number of 'Used' entries.
*
* Argument(s) : p_stat_pool  Pointer to a statistics pool.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolEntryUsedInc (NET_STAT_POOL  *p_stat_pool,
                                RTOS_ERR       *p_err)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_stat_pool->EntriesAvail <= 0) {
        CPU_CRITICAL_EXIT();
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
        goto exit;
    }

                                                                /* If any stat pool entry avail,             ...    */
    p_stat_pool->EntriesAvail--;                                /* ... adj nbr of avail/used entries in pool ...    */
    p_stat_pool->EntriesUsed++;
    p_stat_pool->EntriesAllocCtr++;                             /* ... & inc tot nbr of alloc'd entries.            */
    if (p_stat_pool->EntriesUsedMax < p_stat_pool->EntriesUsed) { /* If max used < nbr used, set new max used.      */
        p_stat_pool->EntriesUsedMax = p_stat_pool->EntriesUsed;
    }
    CPU_CRITICAL_EXIT();


exit:
    return;
}


/*
*********************************************************************************************************
*                                     NetStat_PoolEntryUsedDec()
*
* Description : Decrement a statistics pool's number of 'Used' entries.
*
* Argument(s) : p_stat_pool  Pointer to a statistics pool.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolEntryUsedDec (NET_STAT_POOL  *p_stat_pool,
                                RTOS_ERR       *p_err)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_stat_pool->EntriesUsed <= 0) {
        CPU_CRITICAL_EXIT();
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_FULL);
        goto exit;
    }
                                                                /* If any stat pool entry used,              ...        */
    p_stat_pool->EntriesAvail++;                                /* ... adj nbr of avail/used entries in pool ...        */
    p_stat_pool->EntriesUsed--;
    p_stat_pool->EntriesDeallocCtr++;                           /* ... & inc tot nbr of dealloc'd entries.              */

    CPU_CRITICAL_EXIT();


exit:
    return;
}


/*
*********************************************************************************************************
*                                     NetStat_PoolEntryLostInc()
*
* Description : Increment a statistics pool's number of 'Lost' entries.
*
* Argument(s) : p_stat_pool Pointer to a statistics pool.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) Pool statistic entries MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

void  NetStat_PoolEntryLostInc (NET_STAT_POOL  *p_stat_pool,
                                RTOS_ERR       *p_err)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_stat_pool->EntriesTot <= 0) {
        CPU_CRITICAL_EXIT();
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
        goto exit;
    }

    if (p_stat_pool->EntriesUsed <= 0) {
        CPU_CRITICAL_EXIT();
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_FULL);
        goto exit;
    }
                                                                /* If   tot stat pool entries > 0  ...                  */
                                                                /* ... & any stat pool entry used, ...                  */
    p_stat_pool->EntriesUsed--;                                 /* ... adj nbr used/total/lost entries in pool.         */
    p_stat_pool->EntriesTot--;
    p_stat_pool->EntriesLostCur++;
    p_stat_pool->EntriesLostTot++;

    CPU_CRITICAL_EXIT();


exit:
    return;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL */

