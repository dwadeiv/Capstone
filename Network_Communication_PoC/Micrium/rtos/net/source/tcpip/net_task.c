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
*                                    NETWORK TASK MANAGEMENT
*
* File : net_task.c
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
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "net_task_priv.h"
#include  "net_priv.h"
#include  "net_if_priv.h"

#include  <rtos/net/include/net_util.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                  (NET)
#define  RTOS_MODULE_CUR               RTOS_CFG_MODULE_NET


#define  NET_TASK_NAME                "Net Task"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  KAL_SEM_HANDLE   NetTask_SignalHandle;

static  KAL_TASK_HANDLE  NetTask_Handle;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  NetTask_SignalPend (CPU_INT32U   timeout_ms,
                                  RTOS_ERR    *p_err);

static  void  NetTask_Handler    (void        *p_data);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            NetTask_Init()
*
* Description : (1) Initialize the necessary memory objects for the Network Task:
*                   (a) Task Signal semaphore.
*                   (b) Task Allocation and Creation.
*
* Argument(s) : p_task_cfg  Pointer to the Network Task Configuration Object.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetTask_Init (CPU_INT32U     task_prio,
                    CPU_STK_SIZE   stk_size_elements,
                    void          *p_stk,
                    RTOS_ERR      *p_err)
{

                                                                /* ------- CREATE NETWORK TASK SIGNAL SEMAPHORE ------- */
    NetTask_SignalHandle = KAL_SemCreate("Net Task Sem",
                                          DEF_NULL,
                                          p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

                                                                /* ---------- ALLOCATE & CREATE NETWORK TASK ---------- */
    NetTask_Handle = KAL_TaskAlloc((const  CPU_CHAR *)NET_TASK_NAME,
                                   (CPU_STK         *)p_stk,
                                                      stk_size_elements,
                                                      DEF_NULL,
                                                      p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    KAL_TaskCreate(NetTask_Handle,
                   NetTask_Handler,
                   DEF_NULL,
                   task_prio,
                   DEF_NULL,
                   p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                           NetTask_PrioSet()
*
* Description : Sets priority of the network core task.
*
* Argument(s) : prio    Priority of the network core task.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  NetTask_PrioSet (RTOS_TASK_PRIO   prio,
                       RTOS_ERR        *p_err)
{
    KAL_TASK_HANDLE  task_handle;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    task_handle = NetTask_Handle;
    CPU_CRITICAL_EXIT();

    KAL_TaskPrioSet(task_handle,
                    prio,
                    p_err);
}


/*
*********************************************************************************************************
*                                         NetTask_SignalPost()
*
* Description : Post Signal Task to wake Network Task.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) The Task Signal can be posted when:
*                   (a) a new packet is received at the interface level.
*                   (b) a new timer is added to the Network timer list.
*********************************************************************************************************
*/

void  NetTask_SignalPost (void)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemPost(NetTask_SignalHandle,
                KAL_OPT_POST_NONE,
               &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         NetTask_SignalPend()
*
* Description : Wait for Task Signal Semaphore.
*
* Argument(s) : timeout_ms  Value of the timeout to use to pend on the semaphore for a limit amount of time.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) The Task Signal can be posted when:
*                   (a) a new packet is received at the interface level.
*                   (b) a new timer is added to the Network timer list.
*********************************************************************************************************
*/

static  void  NetTask_SignalPend (CPU_INT32U   timeout_ms,
                                  RTOS_ERR    *p_err)
{

    KAL_SemPend(NetTask_SignalHandle,
                KAL_OPT_PEND_BLOCKING,
                timeout_ms,
                p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
        return;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          NetTask_Handler()
*
* Description : (1) Network Task Handler:
*                   (a) Handle received data packets from all enabled network interface(s)/device(s)
*                   (b) Handle network timers.
*
*               (2) (a) Wait for the signal to wake the task:
*                       (1) A packet receive will signaled the task from network interface(s)/device(s)
*                       (2) A new timer will also signaled the task.
*                       (3) The waiting time depends on the minimum timeout present in the Network Timer
*                           List.
*                   (b) Acquire network lock  (See Note #3)
*                   (c) Handle received packet if any
*                   (d) Handle the Network Timer List.
*                   (e) Release network lock
*
*
* Argument(s) : p_data      Pointer to task initialization data.
*
* Return(s)   : none.
*
* Note(s)     : (2) NetTask_Handler() blocked until network initialization completes.
*
*               (3) NetTask_Handler() blocks ALL other network protocol tasks by pending on & acquiring
*                   the global network lock (see 'net.h  Note #3').
*********************************************************************************************************
*/

static  void  NetTask_Handler (void  *p_data)
{
    CPU_INT32U    timeout_ms = 0;
    NET_IF_NBR    if_nbr     = NET_IF_NBR_NONE;
    RTOS_ERR      pend_err;
    RTOS_ERR      local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    PP_UNUSED_PARAM(p_data);                                    /* Prevent 'variable unused' compiler warning.          */

    while (DEF_ON) {

        RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
        RTOS_ERR_SET(pend_err, RTOS_ERR_NONE);

                                                                /* ----------- WAIT FOR SIGNAL TO WAKE TASK ----------- */
        NetTask_SignalPend(timeout_ms, &local_err);

                                                                /* ------------------ CHECK FOR RX PKT ---------------- */
        if_nbr = NetIF_RxQPend(&pend_err);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
        Net_GlobalLockAcquire((void *)NetTask_Handler);

                                                                /* -------------- HANDLE NETWORK TIMERS --------------- */
        NetTmr_Handler();

                                                                /* -------------- DEALLOCATE TX BUFFERS --------------- */
        do {
            CPU_INT08U  *p_buf_data;


            RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
            p_buf_data = NetIF_TxDeallocQPend(&local_err);
            if (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE) {
                NetIF_TxPktListDealloc(p_buf_data);
            }
        } while (RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE);

                                                                /* ------------------ HANDLE RX PKT ------------------- */
        if (RTOS_ERR_CODE_GET(pend_err) == RTOS_ERR_NONE) {
            NetIF_RxHandler(if_nbr);
        }

                                                                /* ----- SET TIMEOUT VALUE TO PEND ON TASK SIGNAL ----- */
        timeout_ms = NetTmr_ListMinTimeoutGet();
        if (timeout_ms == DEF_INT_32U_MAX_VAL) {
            timeout_ms = 0;                                     /* If no timer, set to zero to pend infinitely on sem.  */
        }

                                                                /* ----------------- RELEASE NET LOCK ----------------- */
        Net_GlobalLockRelease();
    }

}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL */

