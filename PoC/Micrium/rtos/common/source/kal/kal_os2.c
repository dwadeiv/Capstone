/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Common
*
*                             (c) Copyright 2013; Silicon Laboratories Inc.
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
*                              KERNEL ABSTRACTION LAYER (KAL) - uCOS-II
*
* File : kal_os2.c
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

#if (defined(RTOS_MODULE_KERNEL_OS2_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_opt_def.h>
#include  <rtos/common/include/rtos_path.h>
#include  <rtos_cfg.h>
#include  <rtos_description.h>

#include  <rtos/common/source/kal/kal_priv.h>
#include  <rtos/common/include/lib_math.h>
#include  <rtos/common/include/lib_mem.h>

#include  <ucos_ii.h>

#include  <os_cfg.h>

#include  <rtos/common/source/common/common_priv.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                GENERAL
*********************************************************************************************************
*/

#define  KAL_CUR_TASK_GET_IX()                 ((OSTCBCur) - &(OSTCBTbl[0u]))
#define  KAL_LOCK_OWNER_IX_NONE                (OS_MAX_TASKS + OS_N_SYS_TASKS)


/*
*********************************************************************************************************
*                                               DEBUGGING
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                           (COMMON, KAL)
#define  RTOS_MODULE_CUR                        RTOS_CFG_MODULE_COMMON


/*
*********************************************************************************************************
*                                                  MON
*********************************************************************************************************
*/

#if (defined(RTOS_MODULE_USB_HOST_AVAIL) || \
     defined(RTOS_MODULE_FS_AVAIL))
#define  KAL_MON_NEEDED                         DEF_ENABLED
#else
#define  KAL_MON_NEEDED                         DEF_DISABLED
#endif

#ifndef  KAL_MON_EN
#define  KAL_MON_EN                            (KAL_MON_NEEDED && OS_EVENT_EN)
#endif

#define  KAL_MON_EVENT_TYPE                    (OS_EVENT_TYPE_FLAG + 1u)


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

KAL_CPP_EXT  const  KAL_TASK_HANDLE      KAL_TaskHandleNull    = KAL_OBJ_HANDLE_NULL;
KAL_CPP_EXT  const  KAL_LOCK_HANDLE      KAL_LockHandleNull    = KAL_OBJ_HANDLE_NULL;
KAL_CPP_EXT  const  KAL_SEM_HANDLE       KAL_SemHandleNull     = KAL_OBJ_HANDLE_NULL;
KAL_CPP_EXT  const  KAL_TMR_HANDLE       KAL_TmrHandleNull     = KAL_OBJ_HANDLE_NULL;
KAL_CPP_EXT  const  KAL_Q_HANDLE         KAL_QHandleNull       = KAL_OBJ_HANDLE_NULL;
KAL_CPP_EXT  const  KAL_MON_HANDLE       KAL_MonHandleNull     = KAL_OBJ_HANDLE_NULL;
KAL_CPP_EXT  const  KAL_TASK_REG_HANDLE  KAL_TaskRegHandleNull = KAL_OBJ_HANDLE_NULL;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* ------------------ KAL TASK TYPE ------------------- */
typedef  struct  kal_task {
           CPU_STK     *StkBasePtr;                             /* Task stack base ptr.                                 */
           CPU_INT32U   StkSizeElements;                        /* Task stack size (in CPU_STK elements).               */

           CPU_INT08U   Prio;                                   /* Task prio.                                           */
    const  CPU_CHAR    *NamePtr;                                /* Task name string.                                    */
} KAL_TASK;

                                                                /* ------------------- KAL LOCK TYPE ------------------ */
typedef  struct  kal_lock {
    OS_EVENT    *SemEventPtr;                                   /* Pointer to an OS_EVENT sem.                          */
    CPU_INT08U   OptFlags;                                      /* Opt flags passed at creation.                        */

    CPU_INT08U   OwnerIx;                                       /* Ix of curr lock owner. Used only when re-entrant.    */
    CPU_INT08U   NestingCtr;                                    /* Curr cnt of nesting calls to acquire re-entrant lock.*/
} KAL_LOCK;

                                                                /* ------------------- KAL TMR TYPE ------------------- */
#if (OS_TMR_EN == DEF_ENABLED)
typedef  struct  kal_tmr {
    OS_TMR   *TmrPtr;                                           /* Ptr to an OS-II tmr obj.                             */

    void    (*CallbackFnct)(void  *p_arg);                      /* Tmr registered callback fnct.                        */
    void     *CallbackArg;                                      /* Arg to pass to callback fnct.                        */
} KAL_TMR;
#endif

                                                                /* ---------------- KAL TASK REG TYPE ----------------- */
#if (OS_TASK_REG_TBL_SIZE > 0u)
typedef  struct  kal_task_reg_data {
    CPU_INT08U  Id;                                             /* Id of the task reg.                                  */
} KAL_TASK_REG_DATA;
#endif

                                                                /* ------------------- KAL MON TYPE ------------------- */
#if (KAL_MON_EN == DEF_ENABLED)
typedef  struct  kal_mon_data {
    void          *EvalDataPtr;
    KAL_MON_RES  (*OnEvalCallback) (void  *p_mon_data,
                                    void  *p_eval_op_data,
                                    void  *p_scan_op_data);
    INT8U                  Ix;
    struct  kal_mon_data  *NextPtr;
} KAL_MON_DATA;


typedef  struct  kal_mon {
    void          *MonDataPtr;
    OS_EVENT       MonEvent;

    KAL_MON_DATA  *PendListHeadPtr;
} KAL_MON;

#endif


                                                                /* -------------- KAL INTERNAL DATA TYPE -------------- */
typedef  struct  kal_data {
    MEM_SEG       *MemSegPtr;                                   /* Mem Seg to alloc from.                               */

#if (OS_SEM_EN == DEF_ENABLED)
    MEM_DYN_POOL   LockPool;                                    /* Dyn mem pool used to alloc locks.                    */
#endif

#if (OS_TMR_EN == DEF_ENABLED)
    MEM_DYN_POOL   TmrPool;                                     /* Dyn mem pool used to alloc tmrs.                     */
#endif

#if (OS_TASK_REG_TBL_SIZE > 0u)
    MEM_DYN_POOL   TaskRegPool;                                 /* Dyn mem pool used to alloc task regs.                */
#endif

#if (KAL_MON_EN == DEF_ENABLED)
    MEM_DYN_POOL   MonPool;                                     /* Dyn mem pool used to alloc mons.                     */
#endif
} KAL_DATA;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  KAL_DATA  *KAL_DataPtr = DEF_NULL;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (OS_TMR_EN == DEF_ENABLED)
static  void           KAL_TmrFnctWrapper(void        *p_tmr_os,
                                          void        *p_arg);
#endif

static  KAL_TICK       KAL_msToTicks     (CPU_INT32U   ms);

#if (RTOS_ERR_CFG_LEGACY_EN == DEF_ENABLED)
static  RTOS_ERR       KAL_ErrConvert    (CPU_INT08U   err_os);
#else
static  RTOS_ERR_CODE  KAL_ErrConvert    (CPU_INT08U   err_os);
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
*                                       KAL CORE API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              KAL_Init()
*
* Description : Initialize the Kernel Abstraction Layer.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_BLK_ALLOC_CALLBACK
*                           RTOS_ERR_SEG_OVF
*
* Return(s)   : none.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  KAL_Init (RTOS_ERR  *p_err)
{
    MEM_SEG  *p_mem_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);


    p_mem_seg = Common_MemSegPtrGet();

    KAL_DataPtr = (KAL_DATA *)Mem_SegAlloc("KAL internal data",
                                           p_mem_seg,
                                           sizeof(KAL_DATA),
                                           p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }


#if (OS_SEM_EN == DEF_ENABLED)
    Mem_DynPoolCreate("KAL lock pool",
                      &KAL_DataPtr->LockPool,
                       p_mem_seg,
                       sizeof(KAL_LOCK),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
#endif

#if (OS_TMR_EN == DEF_ENABLED)
    Mem_DynPoolCreate("KAL tmr pool",
                      &KAL_DataPtr->TmrPool,
                       p_mem_seg,
                       sizeof(KAL_TMR),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
#endif

#if (OS_TASK_REG_TBL_SIZE > 0u)
    Mem_DynPoolCreate("KAL task reg pool",
                      &KAL_DataPtr->TaskRegPool,
                       p_mem_seg,
                       sizeof(KAL_TASK_REG_DATA),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
#endif

#if (KAL_MON_EN == DEF_ENABLED)
    Mem_DynPoolCreate("KAL mon pool",
                      &KAL_DataPtr->MonPool,
                       p_mem_seg,
                       sizeof(KAL_MON),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
#endif

    LOG_VRB(("KAL_Init successul."));

    return;
}


/*
*********************************************************************************************************
*                                          KAL_FeatureQuery()
*
* Description : Check if specified feature is available.
*
* Argument(s) : feature     Feature to query.
*
*               opt         Option associated with the feature requested.
*
* Return(s)   : DEF_YES, if feature is available.
*
*               DEF_NO,  otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  KAL_FeatureQuery (KAL_FEATURE  feature,
                               KAL_OPT      opt)
{
    CPU_BOOLEAN  is_en;


    is_en = DEF_NO;

    switch (feature) {
        case KAL_FEATURE_TASK_CREATE:                           /* ---------------------- TASKS ----------------------- */
             is_en = DEF_YES;
             break;


        case KAL_FEATURE_TASK_DEL:
             #if (OS_TASK_DEL_EN == DEF_ENABLED)
                 is_en = DEF_YES;
             #endif
             break;


#if (OS_SEM_EN == DEF_ENABLED)                                  /* ------------------- LOCKS & SEMS ------------------- */
        case KAL_FEATURE_LOCK_CREATE:
             if (DEF_BIT_IS_CLR(opt, KAL_OPT_CREATE_REENTRANT) == DEF_YES) {
                                                                /* Re-entrant locks not supported in OS-II right now.   */
                 is_en = DEF_YES;
             }
             break;


        case KAL_FEATURE_LOCK_RELEASE:
        case KAL_FEATURE_SEM_CREATE:
             is_en = DEF_YES;
             break;


        case KAL_FEATURE_LOCK_ACQUIRE:
             if (DEF_BIT_IS_CLR(opt, KAL_OPT_PEND_NON_BLOCKING) == DEF_YES) {
                 is_en = DEF_YES;
             } else {
                 #if (OS_SEM_ACCEPT_EN == DEF_ENABLED)
                     is_en = DEF_YES;                       /* Non-blocking supported only if OSSemAccept() is en.  */
                 #endif
             }
             break;


        case KAL_FEATURE_SEM_PEND:
             if (DEF_BIT_IS_CLR(opt, KAL_OPT_PEND_NON_BLOCKING) == DEF_YES) {
                 is_en = DEF_YES;
             } else {
                 #if (OS_SEM_ACCEPT_EN == DEF_ENABLED)
                     is_en = DEF_YES;                           /* Non-blocking supported only if OSSemAccept() is en.  */
                 #endif
             }
             break;


        case KAL_FEATURE_SEM_POST:
             is_en = DEF_YES;
             break;


        case KAL_FEATURE_SEM_ABORT:
             #if (OS_SEM_PEND_ABORT_EN == DEF_ENABLED)
                 is_en = DEF_YES;
             #endif
             break;


        case KAL_FEATURE_SEM_SET:
             #if (OS_SEM_SET_EN == DEF_ENABLED)
                 is_en = DEF_YES;
             #endif
             break;


#if (OS_SEM_DEL_EN == DEF_ENABLED)
        case KAL_FEATURE_LOCK_DEL:
             is_en = DEF_YES;
             break;


        case KAL_FEATURE_SEM_DEL:
             is_en = DEF_YES;
             break;
#endif
#endif /* OS_SEM_EN */


        case KAL_FEATURE_TMR:                                   /* ----------------------- TMRS ----------------------- */
             #if (OS_TMR_EN == DEF_ENABLED)
                 is_en = DEF_YES;
             #endif
             break;


#if (OS_Q_EN == DEF_ENABLED)                                    /* ---------------------- QUEUES ---------------------- */
        case KAL_FEATURE_Q_CREATE:
             #if ((OS_Q_POST_EN     == DEF_ENABLED) || \
                  (OS_Q_POST_OPT_EN == DEF_ENABLED))
                 is_en = DEF_YES;                               /* Qs and at least one of the Q post fnct are avail.    */
             #endif
             break;


        case KAL_FEATURE_Q_PEND:
             if (DEF_BIT_IS_CLR(opt, KAL_OPT_PEND_NON_BLOCKING) == DEF_YES) {
                 is_en = DEF_YES;
             } else {
                 #if (OS_Q_ACCEPT_EN == DEF_ENABLED)
                     is_en = DEF_YES;                           /* Non-blocking supported only if OSQAccept() is en.    */
                 #endif
             }
             break;


        case KAL_FEATURE_Q_POST:
             #if ((OS_Q_POST_OPT_EN == DEF_ENABLED) || \
                  (OS_Q_POST_EN == DEF_ENABLED))
                 is_en = DEF_YES;
             #endif
             break;
#endif  /* OS_Q_EN */


        case KAL_FEATURE_DLY:                                   /* ----------------------- DLYS ----------------------- */
             if (DEF_BIT_IS_CLR(opt, KAL_OPT_DLY_PERIODIC) == DEF_YES) {
                 is_en = DEF_YES;
             }
             break;


        case KAL_FEATURE_TASK_REG:                              /* ------------------- TASK STORAGE ------------------- */
             #if (OS_TASK_REG_TBL_SIZE > 0u)
                 is_en = DEF_YES;
             #endif
             break;


        case KAL_FEATURE_TICK_GET:                              /* ------------------- TICK CTR INFO ------------------ */
             #if (OS_TIME_GET_SET_EN == DEF_ENABLED)
                 is_en = DEF_YES;
             #endif
             break;


        case KAL_FEATURE_MON:                                   /* ------------------------ MON ----------------------- */
        case KAL_FEATURE_MON_DEL:
             #if (KAL_MON_EN == DEF_ENABLED)
                 is_en = DEF_YES;
             #endif
             break;


        case KAL_FEATURE_CPU_USAGE_GET:                         /* ------------------ CPU USAGE INFO ------------------ */
             #if (OS_TASK_STAT_EN > 0u)
                is_en = DEF_YES;
             #endif
             break;


        default:
             break;
    }

    return (is_en);
}


/*
*********************************************************************************************************
*                                         TASK API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            KAL_TaskAlloc()
*
* Description : Allocate a task object and its stack.
*
* Argument(s) : p_name      Pointer to name of the task.
*
*               p_stk_base  Pointer to start of task stack. If NULL, the stack will be allocated from
*                           the KAL memory segment.
*
*               stk_size    Size (in CPU_STK elements) of the task stack.
*
*               p_cfg       Pointer to KAL task configuration structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_SEG_OVF
*
* Return(s)   : Allocated task's handle.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_TASK_HANDLE  KAL_TaskAlloc (const  CPU_CHAR          *p_name,
                                       CPU_STK           *p_stk_base,
                                       CPU_SIZE_T         stk_size,
                                       KAL_TASK_EXT_CFG  *p_cfg,
                                       RTOS_ERR          *p_err)
{
    KAL_TASK_HANDLE   handle           = KAL_TaskHandleNull;
    KAL_TASK         *p_task;
    CPU_ADDR          stk_addr;
    CPU_ADDR          stk_addr_aligned;
    CPU_SIZE_T        actual_stk_size;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

                                                                /* Make sure no unsupported cfg recv.                   */
    RTOS_ASSERT_DBG_ERR_SET((p_cfg == DEF_NULL), *p_err, RTOS_ERR_NOT_SUPPORTED, handle);

    if (p_stk_base == DEF_NULL) {                               /* Must alloc task stk on mem seg.                      */
        stk_addr_aligned = (CPU_ADDR)Mem_SegAllocExt("KAL task stk",
                                                      KAL_DataPtr->MemSegPtr,
                                                     (stk_size * sizeof(CPU_STK)),
                                                      CPU_CFG_STK_ALIGN_BYTES,
                                                      DEF_NULL,
                                                      p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            LOG_DBG(("KAL_TaskAlloc failed to allocate task stack."));
            return (handle);
        }
        actual_stk_size = stk_size;
    } else {
                                                                /* Align stk ptr, if needed.                            */
        stk_addr         = (CPU_ADDR)p_stk_base;
        stk_addr_aligned =  MATH_ROUND_INC_UP_PWR2(stk_addr, CPU_CFG_STK_ALIGN_BYTES);
        actual_stk_size  = (((stk_size * sizeof(CPU_STK)) - (stk_addr_aligned - stk_addr)) / sizeof(CPU_STK));
    }

    p_task = (KAL_TASK *)Mem_SegAlloc("KAL task",
                                       KAL_DataPtr->MemSegPtr,
                                       sizeof(KAL_TASK),
                                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        LOG_DBG(("KAL_TaskAlloc failed to allocate KAL_TASK data object."));
        return (handle);
    }

    p_task->StkBasePtr      = (CPU_STK *)stk_addr_aligned;
    p_task->StkSizeElements =  actual_stk_size;
    p_task->Prio            =  0u;
    p_task->NamePtr         =  p_name;
    handle.TaskObjPtr       = (void *)p_task;

    LOG_VRB(("KAL_TaskAlloc call successful."));

    return (handle);
}


/*
*********************************************************************************************************
*                                           KAL_TaskCreate()
*
* Description : Create and start a task.
*
* Argument(s) : task_handle     Handle of the task to create.
*
*               p_fnct          Pointer to task function.
*
*               p_task_arg      Pointer to argument that will be passed to task function (can be DEF_NULL).
*
*               prio            Task priority.
*
*               p_cfg           Pointer to KAL task configuration structure.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_ARG
*                                   RTOS_ERR_ISR
*                                   RTOS_ERR_OS
*
* Return(s)   : none.
*
* Note(s)     : (1) The task must be allocated prior to this call using KAL_TaskAlloc().
*********************************************************************************************************
*/

void  KAL_TaskCreate (KAL_TASK_HANDLE     task_handle,
                      void              (*p_fnct)(void  *p_arg),
                      void               *p_task_arg,
                      KAL_TASK_PRIO       prio,
                      KAL_TASK_EXT_CFG   *p_cfg,
                      RTOS_ERR           *p_err)
{
    KAL_TASK      *p_task;
    CPU_STK_SIZE   stk_size_words;
    CPU_INT32U     stk_top_offset;
    CPU_INT08U     err_os;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((p_fnct != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_TASK_HANDLE_IS_NULL(task_handle) != DEF_YES), *p_err, RTOS_ERR_NULL_PTR, ;);

                                                                /* Make sure no unsupported cfg recv.                   */
    RTOS_ASSERT_DBG_ERR_SET((p_cfg == DEF_NULL), *p_err, RTOS_ERR_NOT_SUPPORTED, ;);

    p_task         = (KAL_TASK *)task_handle.TaskObjPtr;
    stk_size_words =  p_task->StkSizeElements;

    #if (OS_STK_GROWTH == DEF_ENABLED)                          /* Set stk top offset, based on OS_STK_GROWTH cfg.      */
        stk_top_offset = stk_size_words - 1u;
    #else
        stk_top_offset = 0u;
    #endif

    #if (OS_TASK_CREATE_EXT_EN == DEF_ENABLED)
    {
        CPU_INT32U  stk_bottom_offset;


        #if (OS_STK_GROWTH == DEF_ENABLED)                      /* Set stk bottom offset, based on OS_STK_GROWTH cfg.   */
            stk_bottom_offset = 0u;
        #else
            stk_bottom_offset = stk_size_words - 1u;
        #endif

        err_os = OSTaskCreateExt(p_fnct,
                                 p_task_arg,
                                &p_task->StkBasePtr[stk_top_offset],
                                 prio,
                                 prio,
                                &p_task->StkBasePtr[stk_bottom_offset],
                                 stk_size_words,
                                 DEF_NULL,
                                 OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK);
    }
    #else
        err_os = OSTaskCreate(p_fnct,
                              p_task_arg,
                             &p_task->StkBasePtr[stk_top_offset],
                              prio);
    #endif
    if (err_os != OS_ERR_NONE) {
        LOG_DBG(("Call to OSTaskCreate/Ext failed with err: ", (u)err_os));
        RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
        return;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    LOG_DBG(("Successfully created task ", (s)p_task->NamePtr, " with priority ", (u)prio));

    p_task->Prio = prio;

    #if (OS_TASK_NAME_EN == DEF_ENABLED)                        /* Set task name if names en.                           */
        if (p_task->NamePtr != DEF_NULL) {                      /* Do not try to set name if name is NULL.              */
            OSTaskNameSet(prio, (INT8U *)p_task->NamePtr, &err_os);
            if (err_os != OS_ERR_NONE) {
                LOG_DBG(("Call to OSTaskNameSet failed with err: ", (u)err_os));
                RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            }
        }
    #endif

    return;
}


/*
*********************************************************************************************************
*                                           KAL_TaskPrioSet()
*
* Description : Change the priority of a task.
*
* Argument(s) : task_handle     Handle of the task to change the priority.
*
*               prio            Task priority.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_OS_ILLEGAL_RUN_TIME
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  KAL_TaskPrioSet (KAL_TASK_HANDLE   task_handle,
                       KAL_TASK_PRIO     prio,
                       RTOS_ERR         *p_err)
{
    RTOS_ASSERT_DBG((KAL_TASK_HANDLE_IS_NULL(task_handle) != DEF_YES), RTOS_ERR_NULL_PTR, ;);

    #if (OS_TASK_CHANGE_PRIO_EN == DEF_ENABLED)
    {
        KAL_TASK    *p_task;
        CPU_INT08U   err_os;


        p_task = (KAL_TASK *)task_handle.TaskObjPtr;

        err_os = OSTaskChangePrio(p_task->Prio, prio);

        RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));

        return;
    }
    #else

        RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, ;);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_TaskDel()
*
* Description : Delete a task.
*
* Argument(s) : task_handle     Handle of the task to delete.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_TaskDel (KAL_TASK_HANDLE  task_handle)
{
    RTOS_ASSERT_DBG((KAL_TASK_HANDLE_IS_NULL(task_handle) != DEF_YES), RTOS_ERR_NULL_PTR, ;);

    #if (OS_TASK_DEL_EN == DEF_ENABLED)
    {
        KAL_TASK    *p_task;
        CPU_INT08U   err_os;


        p_task = (KAL_TASK *)task_handle.TaskObjPtr;

        err_os =  OSTaskDel(p_task->Prio);

        RTOS_ASSERT_CRITICAL((err_os == OS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

        return;
    }
    #else

        RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, ;);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                          KAL_TaskUUID_Get()
*
* Description : Get unique universal identificator (UUID) for current task.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : (1) This function can only be used to obtain the UUID of the task that is currently
*                   running.
*
*               (2) This function can be used to obtain the UUID of a task that has not been created by
*                   KAL.
*********************************************************************************************************
*/

KAL_TASK_UUID  KAL_TaskUUID_Get (void)
{
    KAL_TASK_UUID  task_uid;


    task_uid = (KAL_TASK_UUID)OSTCBCur;

    return (task_uid);
}


/*
*********************************************************************************************************
*                                         LOCK API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           KAL_LockCreate()
*
* Description : Create a lock, which is unlocked by default.
*
* Argument(s) : p_name  Pointer to name of the lock.
*
*               p_cfg   Pointer to KAL lock configuration structure.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_NOT_AVAIL
*                           RTOS_ERR_POOL_FULL
*                           RTOS_ERR_POOL_EMPTY
*                           RTOS_ERR_OS
*                           RTOS_ERR_BLK_ALLOC_CALLBACK
*                           RTOS_ERR_SEG_OVF
*
* Return(s)   : Created lock handle.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_LOCK_HANDLE  KAL_LockCreate (const  CPU_CHAR          *p_name,
                                        KAL_LOCK_EXT_CFG  *p_cfg,
                                        RTOS_ERR          *p_err)
{
    KAL_LOCK_HANDLE  handle = KAL_LockHandleNull;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

    if (p_cfg != DEF_NULL) {
        RTOS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(p_cfg->Opt, ~(KAL_OPT_CREATE_NONE | KAL_OPT_CREATE_REENTRANT)) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, handle);
    }

    #if (OS_SEM_EN == DEF_ENABLED)
    {
        KAL_LOCK  *p_kal_lock;
        OS_EVENT  *p_sem;


        p_kal_lock = (KAL_LOCK *)Mem_DynPoolBlkGet(&KAL_DataPtr->LockPool,
                                                    p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return (handle);
        }

        p_kal_lock->OptFlags = DEF_BIT_NONE;

        if ((p_cfg                                                != DEF_NULL) &&
            (DEF_BIT_IS_SET(p_cfg->Opt, KAL_OPT_CREATE_REENTRANT) == DEF_YES)) {
                                                                /* Created lock is re-entrant.                          */
            DEF_BIT_SET(p_kal_lock->OptFlags, KAL_OPT_CREATE_REENTRANT);
            p_kal_lock->OwnerIx    = KAL_LOCK_OWNER_IX_NONE;
            p_kal_lock->NestingCtr = 0u;
        }

        p_sem = OSSemCreate(1u);
        if (p_sem == DEF_NULL) {
            LOG_DBG(("Call to OSSemCreate failed."));

            Mem_DynPoolBlkFree(       &KAL_DataPtr->LockPool,   /* Free rsrc to pool.                                   */
                               (void *)p_sem,
                                       p_err);
            RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, handle);

            RTOS_ERR_SET(*p_err, RTOS_ERR_OS);
            return (handle);
        }

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        LOG_VRB(("KAL_LockCreate call successful."));

        #if (OS_EVENT_NAME_EN == DEF_ENABLED)                   /* Set name only if cfg is en.                          */
            if (p_name != DEF_NULL) {
                CPU_INT08U  err_os;


                OSEventNameSet(         p_sem,
                               (INT8U *)p_name,
                                       &err_os);
                RTOS_ASSERT_DBG((err_os == OS_ERR_NONE), RTOS_ERR_ASSERT_DBG_FAIL, handle);
            }
        #else
            PP_UNUSED_PARAM(p_name);
        #endif

        p_kal_lock->SemEventPtr =  p_sem;
        handle.LockObjPtr       = (void *)p_kal_lock;

        return (handle);
    }
    #else
        PP_UNUSED_PARAM(p_name);
        PP_UNUSED_PARAM(p_cfg);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (handle);
    #endif
}


/*
*********************************************************************************************************
*                                           KAL_LockAcquire()
*
* Description : Acquire a lock.
*
* Argument(s) : lock_handle     Handle of the lock to acquire.
*
*               opt             Options available:
*                                   KAL_OPT_PEND_NONE:          block until timeout expires or lock is available.
*                                   KAL_OPT_PEND_BLOCKING:      block until timeout expires or lock is available.
*                                   KAL_OPT_PEND_NON_BLOCKING:  return immediately even if lock is not available.
*
*               timeout_ms      Timeout, in milliseconds. A value of 0 will never timeout.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_NULL_PTR
*                                   RTOS_ERR_INVALID_ARG
*                                   RTOS_ERR_TIMEOUT
*                                   RTOS_ERR_ABORT
*                                   RTOS_ERR_ISR
*                                   RTOS_ERR_OS
*                                   RTOS_ERR_WOULD_BLOCK
*                                   RTOS_ERR_WOULD_OVF
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_LockAcquire (KAL_LOCK_HANDLE   lock_handle,
                       KAL_OPT           opt,
                       CPU_INT32U        timeout_ms,
                       RTOS_ERR         *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_LOCK_HANDLE_IS_NULL(lock_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(opt, ~(KAL_OPT_PEND_NONE | KAL_OPT_PEND_NON_BLOCKING)) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, ;);

    #if (OS_SEM_EN == DEF_ENABLED)
    {
        KAL_LOCK    *p_kal_lock;
        OS_EVENT    *p_sem;
        CPU_INT32U   timeout_ticks;
        CPU_INT08U   err_os;
        CPU_SR_ALLOC();


        p_kal_lock = (KAL_LOCK *)lock_handle.LockObjPtr;
        p_sem      =  p_kal_lock->SemEventPtr;

        if (timeout_ms != KAL_TIMEOUT_INFINITE) {
            timeout_ticks = KAL_msToTicks(timeout_ms);
        } else {
            timeout_ticks = 0u;
        }

        if (DEF_BIT_IS_SET(p_kal_lock->OptFlags, KAL_OPT_CREATE_REENTRANT) == DEF_YES) {
            CPU_CRITICAL_ENTER();
            if (p_kal_lock->OwnerIx == KAL_CUR_TASK_GET_IX()) {
                p_kal_lock->NestingCtr++;
                if (p_kal_lock->NestingCtr != 0u) {
                    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
                } else {
                    RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_OVF);
                    p_kal_lock->NestingCtr--;
                }
                CPU_CRITICAL_EXIT();
                return;
            }
            CPU_CRITICAL_EXIT();
        }

        if (DEF_BIT_IS_CLR(opt, KAL_OPT_PEND_NON_BLOCKING) == DEF_YES) {
            OSSemPend(p_sem,
                      timeout_ticks,
                     &err_os);
            if (err_os == OS_ERR_NONE) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
            } else {
                RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
                LOG_DBG(("Call to OSSemPend failed with err: ", (u)err_os));
                return;
            }
        } else {
            #if (OS_SEM_ACCEPT_EN == DEF_ENABLED)               /* If OSSemAccept() is en.                              */
                CPU_INT16U  ret_val;


                ret_val = OSSemAccept(p_sem);
                if (ret_val != 0u) {
                    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
                } else {
                    RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_BLOCK);
                    return;
                }
            #else                                               /* If OSSemAccept() is dis, cannot exec operation.      */
                RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);
                return;
            #endif
        }

        if (DEF_BIT_IS_SET(p_kal_lock->OptFlags, KAL_OPT_CREATE_REENTRANT) == DEF_YES) {
            CPU_CRITICAL_ENTER();
                                                                /* If lock is re-entrant, set ix to cur.                */
            p_kal_lock->OwnerIx    = KAL_CUR_TASK_GET_IX();
            p_kal_lock->NestingCtr = 0u;
            CPU_CRITICAL_EXIT();
        }

        return;
    }
    #else
        PP_UNUSED_PARAM(lock_handle);
        PP_UNUSED_PARAM(opt);
        PP_UNUSED_PARAM(timeout_ms);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                           KAL_LockRelease()
*
* Description : Release a lock.
*
* Argument(s) : lock_handle     Handle of the lock to release.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_NULL_PTR
*                                   RTOS_ERR_INVALID_ARG
*                                   RTOS_ERR_WOULD_OVF
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_LockRelease (KAL_LOCK_HANDLE   lock_handle,
                       RTOS_ERR         *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_LOCK_HANDLE_IS_NULL(lock_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    #if (OS_SEM_EN == DEF_ENABLED)
    {
        KAL_LOCK    *p_kal_lock;
        OS_EVENT    *p_sem;
        CPU_INT08U   err_os;
        CPU_SR_ALLOC();


        p_kal_lock = (KAL_LOCK *)lock_handle.LockObjPtr;
        p_sem      =  p_kal_lock->SemEventPtr;

        if (DEF_BIT_IS_SET(p_kal_lock->OptFlags, KAL_OPT_CREATE_REENTRANT) == DEF_YES) {
                                                                /* Re-entrant lock.                                     */
            CPU_CRITICAL_ENTER();
            if (p_kal_lock->NestingCtr != 0u) {
                p_kal_lock->NestingCtr--;
                CPU_CRITICAL_EXIT();
                RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
                LOG_VRB(("KAL_LockRelease call successful."));
                return;
            }
                                                                /* If lock is re-entrant and is not nested, set ...     */
            p_kal_lock->OwnerIx = KAL_LOCK_OWNER_IX_NONE;       /* ... OwnerIx to 'none'.                               */
            CPU_CRITICAL_EXIT();
        }

        err_os = OSSemPost(p_sem);
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSSemPost failed with err: ", (u)err_os));
        }

        return;
    }
    #else
        PP_UNUSED_PARAM(lock_handle);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_LockDel()
*
* Description : Delete a lock.
*
* Argument(s) : lock_handle     Handle of the lock to delete.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_LockDel (KAL_LOCK_HANDLE  lock_handle)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */

    RTOS_ASSERT_DBG((KAL_LOCK_HANDLE_IS_NULL(lock_handle) != DEF_YES), RTOS_ERR_NULL_PTR, ;);

    #if ((OS_SEM_EN     == DEF_ENABLED) && \
         (OS_SEM_DEL_EN == DEF_ENABLED))                        /* Sems and sems del are avail.                         */
    {
        KAL_LOCK    *p_kal_lock;
        CPU_INT08U   err_os;
        RTOS_ERR     err_lib;
        CPU_SR_ALLOC();


        p_kal_lock = (KAL_LOCK *)lock_handle.LockObjPtr;

        if (DEF_BIT_IS_SET(p_kal_lock->OptFlags, KAL_OPT_CREATE_REENTRANT) == DEF_YES) {
            CPU_CRITICAL_ENTER();
            p_kal_lock->OwnerIx    = KAL_LOCK_OWNER_IX_NONE;    /* If lock is re-entrant, set OwnerIx to 'none'.        */
            p_kal_lock->NestingCtr = 0u;
            CPU_CRITICAL_EXIT();
        }
        (void)OSSemDel(p_kal_lock->SemEventPtr,
                       OS_DEL_ALWAYS,
                      &err_os);
        RTOS_ASSERT_CRITICAL((err_os == OS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

        Mem_DynPoolBlkFree(       &KAL_DataPtr->LockPool,       /* Free rsrc to pool.                                   */
                           (void *)p_kal_lock,
                                  &err_lib);
        RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err_lib) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

        return;
    }
    #else                                                       /* Sems or sems del is not avail.                       */
        PP_UNUSED_PARAM(lock_handle);

         RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, ;);

         return;
    #endif
}


/*
*********************************************************************************************************
*                                          SEM API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            KAL_SemCreate()
*
* Description : Create a semaphore, with a count of 0.
*
* Argument(s) : p_name  Pointer to name of the semaphore.
*
*               p_cfg   Pointer to KAL semaphore configuration structure.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_NOT_AVAIL
*                           RTOS_ERR_OS
*
* Return(s)   : Created semaphore's handle.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_SEM_HANDLE  KAL_SemCreate (const  CPU_CHAR         *p_name,
                                      KAL_SEM_EXT_CFG  *p_cfg,
                                      RTOS_ERR         *p_err)
{
    KAL_SEM_HANDLE  handle = KAL_SemHandleNull;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

                                                                /* Make sure no unsupported cfg recv.                   */
    RTOS_ASSERT_DBG_ERR_SET((p_cfg == DEF_NULL), *p_err, RTOS_ERR_NOT_SUPPORTED, handle);

    #if (OS_SEM_EN == DEF_ENABLED)
    {
        OS_EVENT  *p_sem;


        p_sem = OSSemCreate(0u);
        if (p_sem == DEF_NULL) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_OS);
            LOG_DBG(("Call to OSSemCreate failed."));
            return (handle);
        }

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        LOG_VRB(("KAL_SemCreate call successful."));

        #if (OS_EVENT_NAME_EN == DEF_ENABLED)
            if (p_name != DEF_NULL) {
                CPU_INT08U  err_os;


                OSEventNameSet(         p_sem,
                               (INT8U *)p_name,
                                       &err_os);
                RTOS_ASSERT_DBG((err_os == OS_ERR_NONE), RTOS_ERR_ASSERT_DBG_FAIL, handle);
            }
        #else
            PP_UNUSED_PARAM(p_name);
        #endif

        handle.SemObjPtr = (void *)p_sem;

        return (handle);
    }
    #else
        PP_UNUSED_PARAM(p_name);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (handle);
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_SemPend()
*
* Description : Pend on a semaphore.
*
* Argument(s) : sem_handle  Handle of the semaphore to pend on.
*
*               opt         Options available:
*
*               timeout_ms  Timeout, in milliseconds. A value of 0 will never timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NULL_PTR
*                               RTOS_ERR_INVALID_ARG
*                               RTOS_ERR_ABORT
*                               RTOS_ERR_TIMEOUT
*                               RTOS_ERR_ISR
*                               RTOS_ERR_WOULD_BLOCK
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_SemPend (KAL_SEM_HANDLE   sem_handle,
                   KAL_OPT          opt,
                   CPU_INT32U       timeout_ms,
                   RTOS_ERR        *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_SEM_HANDLE_IS_NULL(sem_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(opt, ~(KAL_OPT_PEND_NONE | KAL_OPT_PEND_NON_BLOCKING)) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, ;);

    #if (OS_SEM_EN == DEF_ENABLED)
    {
        OS_EVENT    *p_sem;
        CPU_INT32U   timeout_ticks;


        p_sem = (OS_EVENT *)sem_handle.SemObjPtr;

        if (timeout_ms != KAL_TIMEOUT_INFINITE) {
            timeout_ticks = KAL_msToTicks(timeout_ms);
        } else {
            timeout_ticks = 0u;
        }

        if (DEF_BIT_IS_CLR(opt, KAL_OPT_PEND_NON_BLOCKING) == DEF_YES) {
            CPU_INT08U  err_os;


            OSSemPend(p_sem,
                      timeout_ticks,
                     &err_os);
            if (err_os == OS_ERR_NONE) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
            } else {
                RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            }
        } else {
            #if (OS_SEM_ACCEPT_EN == DEF_ENABLED)               /* If OSSemAccept() is en.                              */
            {
                CPU_INT16U  ret_val;


                ret_val = OSSemAccept(p_sem);
                if (ret_val == 0u) {
                    RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_BLOCK);
                }
            }
            #else                                               /* If OSSemAccept() is dis, cannot exec operation.      */
                RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);
            #endif
        }

        return;
    }
    #else
        PP_UNUSED_PARAM(sem_handle);
        PP_UNUSED_PARAM(opt);
        PP_UNUSED_PARAM(timeout_ms);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_SemPost()
*
* Description : Post a semaphore.
*
* Argument(s) : sem_handle  Handle of the semaphore to post.
*
*               opt         Options available:
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NULL_PTR
*                               RTOS_ERR_INVALID_ARG
*                               RTOS_ERR_WOULD_OVF
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_SemPost (KAL_SEM_HANDLE   sem_handle,
                   KAL_OPT          opt,
                   RTOS_ERR        *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_SEM_HANDLE_IS_NULL(sem_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((opt == KAL_OPT_POST_NONE), *p_err, RTOS_ERR_INVALID_ARG, ;);

    #if (OS_SEM_EN == DEF_ENABLED)
    {
        OS_EVENT    *p_sem;
        CPU_INT08U   err_os;


        p_sem  = (OS_EVENT *)sem_handle.SemObjPtr;

        err_os =  OSSemPost(p_sem);
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSSemPost failed with err: ", (u)err_os));
        }

        return;
    }
    #else
        PP_UNUSED_PARAM(sem_handle);
        PP_UNUSED_PARAM(opt);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                          KAL_SemPendAbort()
*
* Description : Abort given semaphore and resume all the tasks pending on it.
*
* Argument(s) : sem_handle  Handle of the sempahore to abort.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NULL_PTR
*                               RTOS_ERR_INVALID_ARG
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_SemPendAbort (KAL_SEM_HANDLE   sem_handle,
                        RTOS_ERR        *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_SEM_HANDLE_IS_NULL(sem_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    #if ((OS_SEM_EN            == DEF_ENABLED) && \
         (OS_SEM_PEND_ABORT_EN == DEF_ENABLED))                 /* Sems and sems pend abort are avail.                  */
    {
        OS_EVENT    *p_sem;
        CPU_INT08U   err_os;


        p_sem = (OS_EVENT *)sem_handle.SemObjPtr;

        (void)OSSemPendAbort(p_sem,
                             OS_PEND_OPT_BROADCAST,
                            &err_os);
        if ((err_os == OS_ERR_NONE) ||
            (err_os == OS_ERR_PEND_ABORT)) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSSemPendAbort failed with err: ", (u)err_os));
        }

        return;
    }
    #else                                                       /* Sems or sems pend abort is not avail.                */
        PP_UNUSED_PARAM(sem_handle);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_SemSet()
*
* Description : Set value of semaphore.
*
* Argument(s) : sem_handle  Handle of the semaphore to set.
*
*               cnt         Count value to set semaphore.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NULL_PTR
*                               RTOS_ERR_INVALID_ARG
*                               RTOS_ERR_OS_TASK_WAITING
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_SemSet (KAL_SEM_HANDLE   sem_handle,
                  CPU_INT08U       cnt,
                  RTOS_ERR        *p_err)
{
                                                                /* ------------------ VALIDATE ARGS ------------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_SEM_HANDLE_IS_NULL(sem_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    #if ((OS_SEM_EN     == DEF_ENABLED) && \
         (OS_SEM_SET_EN == DEF_ENABLED))                        /* Sems and sems set are avail.                         */
    {
        CPU_INT08U  err_os;


        OSSemSet((OS_EVENT *)sem_handle.SemObjPtr,
                             cnt,
                            &err_os);
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSSemSet failed with err: ", (u)err_os));
        }

        return;
    }
    #else                                                       /* Sems or sems set is not avail.                       */
        PP_UNUSED_PARAM(sem_handle);
        PP_UNUSED_PARAM(cnt);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_SemDel()
*
* Description : Delete a semaphore.
*
* Argument(s) : sem_handle  Handle of the semaphore to delete.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_SemDel (KAL_SEM_HANDLE  sem_handle)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */

    RTOS_ASSERT_DBG((KAL_SEM_HANDLE_IS_NULL(sem_handle) == DEF_NO), RTOS_ERR_NULL_PTR, ;);

    #if ((OS_SEM_EN     == DEF_ENABLED) && \
         (OS_SEM_DEL_EN == DEF_ENABLED))                        /* Sems and sems del are avail.                         */
    {
        CPU_INT08U  err_os;


        (void)OSSemDel((OS_EVENT *)sem_handle.SemObjPtr,
                                   OS_DEL_ALWAYS,
                                  &err_os);
        RTOS_ASSERT_CRITICAL((err_os == OS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

        return;
    }
    #else                                                       /* Sems or sems del is not avail.                       */
        PP_UNUSED_PARAM(sem_handle);


        RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, ;);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                          TMR API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            KAL_TmrCreate()
*
* Description : Create a single-shot timer.
*
* Argument(s) : p_name          Pointer to name of the timer.
*
*               p_callback      Pointer to the callback function that will be called on completion of timer.
*
*               p_callback_arg  Argument passed to callback function.
*
*               interval_ms     If timer is 'one-shot', delay  used by the timer, in milliseconds.
*                               If timer is 'periodic', period used by the timer, in milliseconds.
*
*               p_cfg           Pointer to KAL timer configuration structure.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_POOL_FULL
*                                   RTOS_ERR_POOL_EMPTY
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_SEG_OVF
*                                   RTOS_ERR_INVALID_ARG
*                                   RTOS_ERR_ISR
*                                   RTOS_ERR_NO_MORE_RSRC
*                                   RTOS_ERR_OS
*
* Return(s)   : Created timer handle.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_TMR_HANDLE  KAL_TmrCreate (const  CPU_CHAR          *p_name,
                                      void             (*p_callback)(void  *p_arg),
                                      void              *p_callback_arg,
                                      CPU_INT32U         interval_ms,
                                      KAL_TMR_EXT_CFG   *p_cfg,
                                      RTOS_ERR          *p_err)
{
    KAL_TMR_HANDLE  handle = KAL_TmrHandleNull;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

                                                                /* Validate callback fnct ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_callback != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, handle);

                                                                /* Validate time.                                       */
    RTOS_ASSERT_DBG_ERR_SET((interval_ms != 0u), *p_err, RTOS_ERR_INVALID_ARG, handle);

    if (p_cfg != DEF_NULL) {
        RTOS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(p_cfg->Opt, ~(KAL_OPT_TMR_NONE | KAL_OPT_TMR_PERIODIC)) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, handle);
    }

    #if (OS_TMR_EN == DEF_ENABLED)
    {
        KAL_TMR     *p_tmr;
        CPU_INT32U   tmr_dly;
        CPU_INT32U   tmr_period;
        CPU_INT08U   opt_os;
        CPU_INT08U   err_os;


        p_tmr = (KAL_TMR *)Mem_DynPoolBlkGet(&KAL_DataPtr->TmrPool,
                                              p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return (handle);
        }

        p_tmr->CallbackFnct = p_callback;
        p_tmr->CallbackArg  = p_callback_arg;
                                                                /* Tmr is 'periodic'.                                   */
        if ((p_cfg                                            != DEF_NULL) &&
            (DEF_BIT_IS_SET(p_cfg->Opt, KAL_OPT_TMR_PERIODIC) == DEF_YES)) {
            opt_os     = OS_TMR_OPT_PERIODIC;
            tmr_dly    = 0u;
            tmr_period = KAL_msToTicks(interval_ms);
        } else {
            opt_os     = OS_TMR_OPT_ONE_SHOT;                   /* Tmr is 'one-shot'.                                   */
            tmr_dly    = KAL_msToTicks(interval_ms);
            tmr_period = 0u;
        }

        p_tmr->TmrPtr = OSTmrCreate(              tmr_dly,      /* Create tmr obj.                                      */
                                                  tmr_period,
                                                  opt_os,
                                                  KAL_TmrFnctWrapper,
                                    (void       *)p_tmr,
                                    (CPU_INT08U *)p_name,
                                                 &err_os);
        if (err_os == OS_ERR_NONE) {
            handle.TmrObjPtr = p_tmr;
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
            LOG_VRB(("KAL_TmrCreate call successful."));
        } else {
            LOG_DBG(("Call to OSTmrCreate failed with err: ", (u)err_os));

            Mem_DynPoolBlkFree(&KAL_DataPtr->TmrPool,
                                p_tmr,
                                p_err);
            RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, handle);
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
        }

        return (handle);
    }
    #else
        PP_UNUSED_PARAM(p_name);
        PP_UNUSED_PARAM(p_callback);
        PP_UNUSED_PARAM(p_callback_arg);
        PP_UNUSED_PARAM(interval_ms);
        PP_UNUSED_PARAM(p_cfg);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (handle);
    #endif
}


/*
*********************************************************************************************************
*                                            KAL_TmrStart()
*
* Description : Start timer.
*
* Argument(s) : tmr_handle  Handle of timer to start.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_ISR
*                               RTOS_ERR_INVALID_ARG
*                               RTOS_ERR_OS
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_TmrStart (KAL_TMR_HANDLE   tmr_handle,
                    RTOS_ERR        *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_TMR_HANDLE_IS_NULL(tmr_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    #if (OS_TMR_EN == DEF_ENABLED)
    {
        KAL_TMR     *p_tmr;
        CPU_INT08U   err_os;


        p_tmr = (KAL_TMR *)tmr_handle.TmrObjPtr;

        (void)OSTmrStart(p_tmr->TmrPtr,
                        &err_os);
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSTmrStart failed with err: ", (u)err_os));
        }

        return;
    }
    #else
        PP_UNUSED_PARAM(tmr_handle);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                           Q API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             KAL_QCreate()
*
* Description : Create an empty queue.
*
* Argument(s) : p_name          Pointer to name of the queue.
*
*               max_msg_qty     Maximum number of message contained in the queue.
*
*               p_cfg           Pointer to KAL queue configuration structure.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_OS
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : Created queue handle.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_Q_HANDLE  KAL_QCreate (const  CPU_CHAR       *p_name,
                                  KAL_MSG_QTY     max_msg_qty,
                                  KAL_Q_EXT_CFG  *p_cfg,
                                  RTOS_ERR       *p_err)
{
    KAL_Q_HANDLE  handle = KAL_QHandleNull;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

                                                                /* Make sure no unsupported cfg recv.                   */
    RTOS_ASSERT_DBG_ERR_SET((p_cfg == DEF_NULL), *p_err, RTOS_ERR_NOT_SUPPORTED, handle);

    #if ((OS_Q_EN          == DEF_ENABLED) && \
        ((OS_Q_POST_EN     == DEF_ENABLED) || \
         (OS_Q_POST_OPT_EN == DEF_ENABLED)))                    /* Qs and at least one of the Q post fnct are avail.    */
    {
        void      **p_q_start;
        OS_EVENT   *p_q_event;


        p_q_start = (void **)Mem_SegAlloc("KAL Q",
                                           KAL_DataPtr->MemSegPtr,
                                           sizeof(void *) * max_msg_qty,
                                           p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            LOG_DBG(("KAL_QCreate: failed to allocate memory for q."));
            return (handle);
        }

        p_q_event = OSQCreate(p_q_start,
                              max_msg_qty);
        if (p_q_event == DEF_NULL) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_OS);
            LOG_DBG(("Call to OSQCreate failed."));
            return (handle);
        }

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        LOG_VRB(("KAL_QCreate call successful."));

        #if (OS_EVENT_NAME_EN == DEF_ENABLED)
            if (p_name != DEF_NULL) {
                CPU_INT08U  err_os;


                OSEventNameSet(              p_q_event,
                               (CPU_INT08U *)p_name,
                                            &err_os);
                RTOS_ASSERT_DBG((err_os == OS_ERR_NONE), RTOS_ERR_ASSERT_DBG_FAIL, handle);
            }
        #else
            PP_UNUSED_PARAM(p_name);
        #endif

        handle.QObjPtr = (void *)p_q_event;

        return (handle);
    }
    #else                                                       /* Qs or Q post are not avail.                          */
        PP_UNUSED_PARAM(p_name);
        PP_UNUSED_PARAM(max_msg_qty);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (handle);
    #endif
}


/*
*********************************************************************************************************
*                                              KAL_QPend()
*
* Description : Pend/get first message of queue.
*
* Argument(s) : q_handle    Handle of the queue to pend on.
*
*               opt         Options available:
*                               KAL_OPT_PEND_NONE:          block until timeout expires or message is available.
*                               KAL_OPT_PEND_BLOCKING:      block until timeout expires or message is available.
*                               KAL_OPT_PEND_NON_BLOCKING:  return immediately with or without message.
*
*               timeout_ms  Timeout, in milliseconds. A value of 0 will never timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NULL_PTR
*                               RTOS_ERR_INVALID_ARG
*                               RTOS_ERR_TIMEOUT
*                               RTOS_ERR_ISR
*                               RTOS_ERR_OS
*                               RTOS_ERR_WOULD_BLOCK
*
* Return(s)   : Pointer to message obtained, if any, if no error.
*
*               Null pointer,                        otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  *KAL_QPend (KAL_Q_HANDLE   q_handle,
                  KAL_OPT        opt,
                  CPU_INT32U     timeout_ms,
                  RTOS_ERR      *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

    RTOS_ASSERT_DBG_ERR_SET((KAL_Q_HANDLE_IS_NULL(q_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);

    RTOS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(opt, ~(KAL_OPT_PEND_NONE | KAL_OPT_PEND_NON_BLOCKING)) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, DEF_NULL);

    #if ((OS_Q_EN          == DEF_ENABLED) && \
        ((OS_Q_POST_EN     == DEF_ENABLED) || \
         (OS_Q_POST_OPT_EN == DEF_ENABLED)))                    /* Qs and at least one of the Q post fnct are avail.    */
    {
        OS_EVENT    *p_q_event;
        void        *p_msg;
        CPU_INT08U   err_os;


        p_q_event = (OS_EVENT *)q_handle.QObjPtr;

        if (DEF_BIT_IS_CLR(opt, KAL_OPT_PEND_NON_BLOCKING) == DEF_YES) {
            CPU_INT32U  timeout_ticks;


                                                                /* Blocking call.                                       */
            if (timeout_ms != KAL_TIMEOUT_INFINITE) {
                timeout_ticks = KAL_msToTicks(timeout_ms);
            } else {
                timeout_ticks = 0u;
            }

            p_msg = OSQPend(p_q_event,
                            timeout_ticks,
                           &err_os);
        } else {                                                /* Non-blocking call.                                   */
            #if (OS_Q_ACCEPT_EN == DEF_ENABLED)
                p_msg = OSQAccept(p_q_event,
                                 &err_os);
            #else
                RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);
                return (DEF_NULL);
            #endif
        }
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSQPend failed with err: ", (u)err_os));
        }

        return (p_msg);
    }
    #else                                                       /* Qs or Q post are not avail.                          */
        PP_UNUSED_PARAM(q_handle);
        PP_UNUSED_PARAM(opt);
        PP_UNUSED_PARAM(timeout_ms);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (DEF_NULL);
    #endif
}


/*
*********************************************************************************************************
*                                              KAL_QPost()
*
* Description : Post message on queue.
*
* Argument(s) : q_handle    Handle of the queue on which to post message.
*
*               p_msg       Pointer to message to post.
*
*               opt         Options available:
*                                   KAL_OPT_POST_NONE:     wake only the highest priority task pending on queue.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NULL_PTR
*                               RTOS_ERR_INVALID_ARG
*                               RTOS_ERR_NO_MORE_RSRC
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_QPost (KAL_Q_HANDLE   q_handle,
                 void          *p_msg,
                 KAL_OPT        opt,
                 RTOS_ERR      *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_Q_HANDLE_IS_NULL(q_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((opt == KAL_OPT_POST_NONE), *p_err, RTOS_ERR_INVALID_ARG, ;);

    #if ((OS_Q_EN          == DEF_ENABLED) && \
        ((OS_Q_POST_EN     == DEF_ENABLED) || \
         (OS_Q_POST_OPT_EN == DEF_ENABLED)))                    /* Qs and at least one of the Q post fnct are avail.    */
    {
        OS_EVENT    *p_q_event;
        CPU_INT08U   err_os;


        p_q_event = (OS_EVENT *)q_handle.QObjPtr;

        #if (OS_Q_POST_OPT_EN == DEF_ENABLED)
            err_os = OSQPostOpt(p_q_event,
                                p_msg,
                                OS_POST_OPT_NONE);
        #elif (OS_Q_POST_EN == DEF_ENABLED)
            err_os = OSQPost(p_q_event,
                             p_msg);
        #endif
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSQPost failed with err: ", (u)err_os));
        }

        return;
    }
    #else                                                       /* Qs or Q post are not avail.                          */
        PP_UNUSED_PARAM(q_handle);
        PP_UNUSED_PARAM(p_msg);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                          MON API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            KAL_MonCreate()
*
* Description : Create a monitor.
*
* Argument(s) : p_name      Pointer to name of the monitor.
*
*               p_mon_obj   Pointer to monitor object, if any. If DEF_NULL, monitor object will be
*                           allocated from monitor pool by KAL.
*
*               p_mon_data  Pointer to data that will be passed to callback functions.
*
*               p_cfg       Pointer to KAL monitor configuration structure. Reserved for future use, must
*                           be DEF_NULL.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_POOL_EMPTY
*                               RTOS_ERR_BLK_ALLOC_CALLBACK
*                               RTOS_ERR_SEG_OVF
*
* Return(s)   : Created monitor handle.
*
* Note(s)     : (1) The initialization of the monitor module only needs to be done once, when creating
*                   the first monitor.
*********************************************************************************************************
*/

KAL_MON_HANDLE  KAL_MonCreate (const CPU_CHAR         *p_name,
                                     void             *p_mon_obj,
                                     void             *p_mon_data,
                                     KAL_MON_EXT_CFG  *p_cfg,
                                     RTOS_ERR         *p_err)
{
    KAL_MON_HANDLE  handle = KAL_MonHandleNull;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

                                                                /* Make sure no unsupported cfg recv.                   */
    RTOS_ASSERT_DBG_ERR_SET((p_cfg == DEF_NULL), *p_err, RTOS_ERR_NOT_SUPPORTED, handle);

    #if (KAL_MON_EN == DEF_ENABLED)
    {
        KAL_MON   *p_kal_mon;
        OS_EVENT  *p_event;


        if (p_mon_obj != DEF_NULL) {
            p_kal_mon = (KAL_MON *)p_mon_obj;
        } else {
            p_kal_mon = (KAL_MON *)Mem_DynPoolBlkGet(&KAL_DataPtr->MonPool,
                                                      p_err);
            if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                LOG_DBG(("KAL_MonCreate: failed to allocate mon data."));
                return (handle);
            }
        }
        p_event = &p_kal_mon->MonEvent;

        p_event->OSEventType = KAL_MON_EVENT_TYPE;
        p_event->OSEventPtr  = DEF_NULL;                        /* Unlink from ECB free list.                           */
        #if (OS_EVENT_NAME_EN == DEF_ENABLED)
            p_event->OSEventName = (INT8U *)p_name;
        #else
            PP_UNUSED_PARAM(p_name);
        #endif
        OS_EventWaitListInit(p_event);                          /* Init to 'nobody waiting' on event.                   */

        p_kal_mon->MonDataPtr      = p_mon_data;
        p_kal_mon->PendListHeadPtr = DEF_NULL;

        handle.MonObjPtr = (void *)p_kal_mon;

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        LOG_VRB(("KAL_MonCreate call successful."));

        return (handle);
    }
    #else
        PP_UNUSED_PARAM(p_name);
        PP_UNUSED_PARAM(p_mon_obj);
        PP_UNUSED_PARAM(p_mon_data);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (handle);
    #endif
}


/*
*********************************************************************************************************
*                                              KAL_MonOp()
*
* Description : Execute an operation on a monitor.
*
* Argument(s) : mon_handle      Handle of the monitor on which operation is executed.
*
*               p_op_data       Pointer to arg that will be passed to callback functions.
*
*               on_enter_fnct   Function that will be called on entering the monitor.
*
*               on_eval_fnct    Function that will be called when evaluating if this task can run.
*
*               opt             Options for call.
*                                   KAL_OPT_MON_NONE            No option.
*                                   KAL_OPT_MON_NO_SCHED        Scheduler will not be called.
*
*               timeout_ms      Timeout, in milliseconds. A value of KAL_TIMEOUT_INFINITE will never timeout.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_OS
*                                   RTOS_ERR_ABORT
*                                   RTOS_ERR_TIMEOUT
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_MonOp (KAL_MON_HANDLE    mon_handle,
                 void             *p_op_data,
                 KAL_MON_RES     (*on_enter_fnct)(void  *p_mon_data, void  *p_op_data),
                 KAL_MON_RES     (*on_eval_fnct) (void  *p_mon_data, void  *p_eval_op_data, void  *p_scan_op_data),
                 KAL_OPT           opt,
                 KAL_TICK          timeout_ms,
                 RTOS_ERR         *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_MON_HANDLE_IS_NULL(mon_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((DEF_BIT_IS_SET_ANY(opt, ~(KAL_OPT_MON_NONE | KAL_OPT_MON_NO_SCHED)) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, ;);

    if (OSLockNesting > 0u) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_OS);                      /* Can't call this when sched is locked.                */
        return;
    }

    #if (KAL_MON_EN == DEF_ENABLED)
    {
        KAL_MON_RES    op_res;
        KAL_MON_RES    mon_res;
        KAL_MON       *p_kal_mon      = (KAL_MON *)mon_handle.MonObjPtr;
        OS_EVENT      *p_mon_event    = &p_kal_mon->MonEvent;
        KAL_MON_DATA   mon_local_data;
        KAL_MON_DATA  *p_cur_data;
        CPU_BOOLEAN    sched          =  DEF_NO;
        INT32U         timeout_ticks;
        INT8U          pend_status;
        CPU_SR_ALLOC();


        if (timeout_ms == KAL_TIMEOUT_INFINITE) {
            timeout_ticks = 0u;
        } else {
            timeout_ticks = KAL_msToTicks(timeout_ms);
        }

        OS_ENTER_CRITICAL();

        if (on_enter_fnct != DEF_NULL) {
            op_res = on_enter_fnct(p_kal_mon->MonDataPtr, p_op_data);
        } else {
            op_res = KAL_MON_RES_BLOCK | KAL_MON_RES_STOP_EVAL;
        }

        if (DEF_BIT_IS_SET(op_res, KAL_MON_RES_BLOCK) == DEF_YES) {
                                                                /* Block task pending on monitor.                       */

            OSTCBCur->OSTCBStat     |= OS_STAT_MUTEX;           /* Resource not available, pend on it.                  */
            OSTCBCur->OSTCBStatPend  = OS_STAT_PEND_OK;
            OSTCBCur->OSTCBDly       = timeout_ticks;           /* Store pend timeout in TCB.                           */
            OS_EventTaskWait(p_mon_event);                      /* Suspend task until event or timeout occurs.          */
            sched = DEF_YES;


            mon_local_data.OnEvalCallback = on_eval_fnct;
            mon_local_data.EvalDataPtr    = p_op_data;
            mon_local_data.Ix             = KAL_CUR_TASK_GET_IX();
            mon_local_data.NextPtr        = DEF_NULL;

            if (p_kal_mon->PendListHeadPtr != DEF_NULL) {
                p_cur_data = p_kal_mon->PendListHeadPtr;


                while (p_cur_data->NextPtr != DEF_NULL) {
                    p_cur_data = p_cur_data->NextPtr;
                }
                p_cur_data->NextPtr = &mon_local_data;

            } else {
                p_kal_mon->PendListHeadPtr = &mon_local_data;
            }
        }

        if (DEF_BIT_IS_CLR(op_res, KAL_MON_RES_STOP_EVAL) == DEF_YES) {
            INT8U          highest_prio;
            INT8U          eval_prio;
            KAL_MON_DATA  *p_eval_data;
            OS_TCB        *p_tcb;


            p_cur_data = p_kal_mon->PendListHeadPtr;

            if (p_cur_data != DEF_NULL) {
                eval_prio = 0u;

                while (eval_prio < OS_LOWEST_PRIO) {
                    p_cur_data   =  p_kal_mon->PendListHeadPtr;
                    p_tcb        = &OSTCBTbl[p_cur_data->Ix];
                    highest_prio =  OS_LOWEST_PRIO;
                    p_eval_data  =  DEF_NULL;
                    while (p_cur_data != DEF_NULL) {

                        if ((p_tcb->OSTCBPrio < highest_prio) &&
                            (p_tcb->OSTCBPrio > eval_prio)) {
                            highest_prio = p_tcb->OSTCBPrio;
                            p_eval_data  = p_cur_data;
                        }
                        p_cur_data = p_cur_data->NextPtr;
                    }
                    eval_prio = highest_prio;
                    if (eval_prio == OS_LOWEST_PRIO) {
                        break;
                    }

                    if (p_eval_data->OnEvalCallback != DEF_NULL) {
                        mon_res = p_eval_data->OnEvalCallback(p_kal_mon->MonDataPtr, p_eval_data->EvalDataPtr, p_op_data);
                    } else {
                        mon_res = KAL_MON_RES_STOP_EVAL;
                    }

                    if (DEF_BIT_IS_CLR(mon_res, KAL_MON_RES_BLOCK) == DEF_YES) {
                                                            /* Point to eval task's OS_TCB.                         */
                        p_tcb           = &OSTCBTbl[p_eval_data->Ix];
                        p_tcb->OSTCBDly =  0u;              /* Prevent OSTimeTick() from readying task.             */

                                                            /* Clr bit associated with event type.                  */
                        p_tcb->OSTCBStat &= (INT8U)~OS_STAT_MUTEX;
                                                            /* Set pend status of post or abort.                    */
                        p_tcb->OSTCBStatPend = OS_STAT_PEND_OK;
                                                            /* See if task is ready (could be susp'd).              */
                        if ((p_tcb->OSTCBStat & OS_STAT_SUSPEND) == OS_STAT_RDY) {
                                                            /* Put task in the ready to run list.                   */
                            OSRdyGrp                |= p_tcb->OSTCBBitY;
                            OSRdyTbl[p_tcb->OSTCBY] |= p_tcb->OSTCBBitX;
                        }

                                                            /* Remove this task from event wait list.               */
                        OS_EventTaskRemove(p_tcb, p_mon_event);

                        if (DEF_BIT_IS_CLR(opt, KAL_OPT_MON_NO_SCHED) == DEF_YES) {
                            sched = DEF_YES;
                        }
                    }

                    if (DEF_BIT_IS_SET(mon_res, KAL_MON_RES_STOP_EVAL) == DEF_YES) {
                        goto exit_pend_eval;
                    }
                }
            }
        }

exit_pend_eval:
        OS_EXIT_CRITICAL();

        if (sched == DEF_YES) {
            OS_Sched();
        }

        if (DEF_BIT_IS_SET(op_res, KAL_MON_RES_BLOCK) == DEF_YES) {

            OS_ENTER_CRITICAL();
            pend_status = OSTCBCur->OSTCBStatPend;

            if (pend_status == OS_STAT_PEND_TO) {               /* We didn't get semaphore within timeout.              */
                 OS_EventTaskRemove(OSTCBCur, p_mon_event);
            }

            OSTCBCur->OSTCBStat     = OS_STAT_RDY;              /* Set task status to ready.                            */
            OSTCBCur->OSTCBStatPend = OS_STAT_PEND_OK;          /* Clr pend status.                                     */
            OSTCBCur->OSTCBEventPtr = DEF_NULL;                 /* Clr event ptrs.                                      */

                                                                /* Remove local data from pend list.                    */
            if (p_kal_mon->PendListHeadPtr != &mon_local_data) {
                p_cur_data = p_kal_mon->PendListHeadPtr;

                while (p_cur_data->NextPtr != &mon_local_data) {
                    p_cur_data = p_cur_data->NextPtr;
                }
                p_cur_data->NextPtr = p_cur_data->NextPtr->NextPtr;

            } else {
                p_kal_mon->PendListHeadPtr = p_kal_mon->PendListHeadPtr->NextPtr;
            }

            OS_EXIT_CRITICAL();

            switch (pend_status) {                              /* See if we timed-out or aborted.                      */
                case OS_STAT_PEND_OK:                           /* We got the monitor.                                  */
                     RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
                     break;

                case OS_STAT_PEND_ABORT:                        /* Indicate that we aborted.                            */
                     RTOS_ERR_SET(*p_err, RTOS_ERR_ABORT);
                     LOG_DBG(("Call to OSMonOp failed with abort err."));
                     break;

                case OS_STAT_PEND_TO:                           /* Indicate that we didn't get semaphore within timeout.*/
                     RTOS_ERR_SET(*p_err, RTOS_ERR_TIMEOUT);
                     LOG_DBG(("Call to OSMonOp failed with timeout err."));
                     break;

                default:
                     RTOS_ERR_SET(*p_err, RTOS_ERR_OS);
                     LOG_DBG(("Call to OSMonOp failed with unknown err."));
                     break;
            }
        } else {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        }
    }
    #else
        PP_UNUSED_PARAM(mon_handle);
        PP_UNUSED_PARAM(p_op_data);
        PP_UNUSED_PARAM(on_enter_fnct);
        PP_UNUSED_PARAM(on_eval_fnct);
        PP_UNUSED_PARAM(opt);
        PP_UNUSED_PARAM(timeout_ms);


        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                             KAL_MonDel()
*
* Description : Delete a monitor.
*
* Argument(s) : mon_handle  Handle of the monitor to delete.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_MonDel (KAL_MON_HANDLE  mon_handle)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */

    RTOS_ASSERT_DBG((KAL_MON_HANDLE_IS_NULL(mon_handle) == DEF_NO), RTOS_ERR_NULL_PTR, ;);

    #if (KAL_MON_EN == DEF_ENABLED)
    {
        KAL_MON       *p_kal_mon      = (KAL_MON *)mon_handle.MonObjPtr;
        OS_EVENT      *p_mon_event    = &p_kal_mon->MonEvent;
        CPU_BOOLEAN    tasks_waiting;
        RTOS_ERR       err_lib;

        CPU_SR_ALLOC();


        OS_ENTER_CRITICAL();
        if (p_mon_event->OSEventGrp != 0u) {                    /* See if any tasks waiting on monitor.                 */
            tasks_waiting = OS_TRUE;
        } else {
            tasks_waiting = OS_FALSE;
        }

        while (p_mon_event->OSEventGrp != 0u) {                 /* Ready ALL tasks waiting for monitor.                 */
            (void)OS_EventTaskRdy(p_mon_event, DEF_NULL, OS_STAT_MUTEX, OS_STAT_PEND_ABORT);
        }

        p_mon_event->OSEventType = OS_EVENT_TYPE_UNUSED;
        p_mon_event->OSEventPtr  = OSEventFreeList;             /* Return Event Control Block to free list.             */
        p_mon_event->OSEventCnt  = 0u;
        OS_EXIT_CRITICAL();
        if (tasks_waiting == DEF_TRUE) {                        /* Reschedule only if task(s) were waiting.             */
            OS_Sched();                                         /* Find highest prio task ready to run.                 */
        }

        Mem_DynPoolBlkFree(&KAL_DataPtr->MonPool,
                            mon_handle.MonObjPtr,
                           &err_lib);
        RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err_lib) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
        return;
    }
    #else
        PP_UNUSED_PARAM(mon_handle);


        RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, ;);
    #endif
}


/*
*********************************************************************************************************
*                                          DLY API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               KAL_Dly()
*
* Description : Delay current task (in milliseconds).
*
* Argument(s) : dly_ms  Delay value, in milliseconds.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_Dly (CPU_INT32U  dly_ms)
{
    CPU_INT32U  dly_ticks;


    if (dly_ms != 0u) {
        dly_ticks = KAL_msToTicks(dly_ms);
    } else {
        dly_ticks = 0u;
    }

    OSTimeDly(dly_ticks);

    return;
}


/*
*********************************************************************************************************
*                                             KAL_DlyTick()
*
* Description : Delay current task (in ticks).
*
* Argument(s) : dly_ticks   Delay value, in ticks.
*
*               opt         Options available:
*                               KAL_OPT_DLY_NONE:       apply a 'normal' delay.
*                               KAL_OPT_DLY:            apply a 'normal' delay.
*                               KAL_OPT_DLY_PERIODIC:   apply a periodic delay.
*
* Return(s)   : none.
*
* Note(s)     : (1) KAL_OPT_DLY_PERIODIC is not supported. Using KAL_OPT_DLY instead.
*********************************************************************************************************
*/

void  KAL_DlyTick (KAL_TICK  dly_ticks,
                   KAL_OPT   opt)
{
    PP_UNUSED_PARAM(opt);

    OSTimeDly(dly_ticks);

    return;
}


/*
*********************************************************************************************************
*                                       TASK REG API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          KAL_TaskRegCreate()
*
* Description : Create a task register.
*
* Argument(s) : p_cfg   Pointer to KAL task register configuration structure.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_NOT_AVAIL
*                           RTOS_ERR_NO_MORE_RSRC
*                           RTOS_ERR_POOL_FULL
*                           RTOS_ERR_POOL_EMPTY
*                           RTOS_ERR_BLK_ALLOC_CALLBACK
*                           RTOS_ERR_SEG_OVF
*
* Return(s)   : Created task register's handle.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_TASK_REG_HANDLE  KAL_TaskRegCreate (KAL_TASK_REG_EXT_CFG  *p_cfg,
                                        RTOS_ERR              *p_err)
{
    KAL_TASK_REG_HANDLE  handle = KAL_TaskRegHandleNull;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, handle);

                                                                /* Make sure no unsupported cfg recv.                   */
    RTOS_ASSERT_DBG_ERR_SET((p_cfg == DEF_NULL), *p_err, RTOS_ERR_NOT_SUPPORTED, handle);

    #if (OS_TASK_REG_TBL_SIZE > 0u)
    {
        KAL_TASK_REG_DATA  *p_task_reg;
        CPU_INT08U          err_os;


        p_task_reg = (KAL_TASK_REG_DATA *)Mem_DynPoolBlkGet(&KAL_DataPtr->TaskRegPool,
                                                             p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return (handle);
        }

        p_task_reg->Id = OSTaskRegGetID(&err_os);
        if (err_os == OS_ERR_NONE) {
            handle.TaskRegObjPtr = (void *)p_task_reg;
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
            LOG_VRB(("KAL_TaskRegCreate call successful."));
        } else {
            LOG_DBG(("Call to OSTaskRegGetID failed with err: ", (u)err_os));

                                                                /* Free rsrc to pool.                                   */
            Mem_DynPoolBlkFree(       &KAL_DataPtr->TaskRegPool,
                               (void *)p_task_reg,
                                       p_err);
            RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, handle);
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
        }

        return (handle);
    }
    #else
        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (handle);
    #endif
}


/*
*********************************************************************************************************
*                                           KAL_TaskRegGet()
*
* Description : Get value from a task register.
*
* Argument(s) : task_handle         Handle of the task associated to the task register. Current task is used if NULL.
*
*               task_reg_handle     Handle of the task register to read.
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       RTOS_ERR_NONE
*                                       RTOS_ERR_NOT_AVAIL
*                                       RTOS_ERR_INVALID_ARG
*
* Return(s)   : Value read from the task register, if no error.
*               0,                                 otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_TASK_REG  KAL_TaskRegGet (KAL_TASK_HANDLE       task_handle,
                              KAL_TASK_REG_HANDLE   task_reg_handle,
                              RTOS_ERR             *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

    RTOS_ASSERT_DBG_ERR_SET((KAL_TASK_REG_HANDLE_IS_NULL(task_reg_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, 0u);

    #if (OS_TASK_REG_TBL_SIZE > 0u)
    {
        KAL_TASK_REG_DATA  *p_task_reg;
        CPU_INT08U          task_prio;
        KAL_TASK_REG        ret_val;
        CPU_INT08U          err_os;


        p_task_reg = (KAL_TASK_REG_DATA *)task_reg_handle.TaskRegObjPtr;

        if (KAL_TASK_HANDLE_IS_NULL(task_handle) == DEF_YES) {
            task_prio = OS_PRIO_SELF;                           /* Use cur task if no task handle is provided.          */
        } else {
                                                                /* Get prio from task handle provided.                  */
            task_prio = ((KAL_TASK *)task_handle.TaskObjPtr)->Prio;
        }

        ret_val = OSTaskRegGet(task_prio,
                               p_task_reg->Id,
                              &err_os);
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSTaskRegGet failed with err: ", (u)err_os));
        }

        return (ret_val);
    }
    #else
        PP_UNUSED_PARAM(task_handle);
        PP_UNUSED_PARAM(task_reg_handle);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (0u);
    #endif
}


/*
*********************************************************************************************************
*                                           KAL_TaskRegSet()
*
* Description : Set value of task register.
*
* Argument(s) : task_handle         Handle of the task associated to the task register. Current task is used if NULL.
*
*               task_reg_handle     Handle of the task register to write to.
*
*               val                 Value to write in the task register.
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       RTOS_ERR_NONE
*                                       RTOS_ERR_NOT_AVAIL
*                                       RTOS_ERR_INVALID_ARG
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  KAL_TaskRegSet (KAL_TASK_HANDLE       task_handle,
                      KAL_TASK_REG_HANDLE   task_reg_handle,
                      KAL_TASK_REG          val,
                      RTOS_ERR             *p_err)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ASSERT_DBG_ERR_SET((KAL_TASK_REG_HANDLE_IS_NULL(task_reg_handle) == DEF_NO), *p_err, RTOS_ERR_NULL_PTR, ;);

    #if (OS_TASK_REG_TBL_SIZE > 0u)
    {
        KAL_TASK_REG_DATA  *p_task_reg;
        CPU_INT08U          task_prio;
        CPU_INT08U          err_os;


        p_task_reg = (KAL_TASK_REG_DATA *)task_reg_handle.TaskRegObjPtr;

        if (KAL_TASK_HANDLE_IS_NULL(task_handle) == DEF_YES) {
            task_prio = OS_PRIO_SELF;                           /* Use cur task if no task handle is provided.          */
        } else {
                                                                /* Get prio from task handle provided.                  */
            task_prio = ((KAL_TASK *)task_handle.TaskObjPtr)->Prio;
        }

        OSTaskRegSet(task_prio,
                     p_task_reg->Id,
                     val,
                    &err_os);
        if (err_os == OS_ERR_NONE) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        } else {
            RTOS_ERR_SET(*p_err, KAL_ErrConvert(err_os));
            LOG_DBG(("Call to OSTaskRegSet failed with err: ", (u)err_os));
        }

        return;
    }
    #else
        PP_UNUSED_PARAM(task_handle);
        PP_UNUSED_PARAM(task_reg_handle);
        PP_UNUSED_PARAM(val);

        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return;
    #endif
}


/*
*********************************************************************************************************
*                                         TICK API FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             KAL_TickGet()
*
* Description : Get value of OS' tick counter.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_NOT_AVAIL
*
* Return(s)   : OS tick counter's value.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_TICK  KAL_TickGet (RTOS_ERR  *p_err)
{
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

    #if (OS_TIME_GET_SET_EN == DEF_ENABLED)
    {
        KAL_TICK  tick_cnt;


        tick_cnt = OSTimeGet();

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

        return (tick_cnt);
    }
    #else
        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);

        return (0u);
    #endif
}


/*
*********************************************************************************************************
*                                           KAL_TickRateGet()
*
* Description : Get value of OS' tick rate, in Hz.
*
* Argument(s) : none.
*
* Return(s)   : OS tick rate's value.
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_TICK_RATE_HZ  KAL_TickRateGet (void)
{
    return (OS_TICKS_PER_SEC);
}


/*
*********************************************************************************************************
*                                        CPU USAGE API FUNCTION
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          KAL_CPU_UsageGet()
*
* Description : Get value of the CPU Usage.
*
* Argument(s) : none.
*
* Return(s)   : CPU usage's percentage represented as an integer from 0 (0%) to 10000 (100%).
*
* Note(s)     : none.
*********************************************************************************************************
*/

KAL_CPU_USAGE  KAL_CPU_UsageGet (void)
{
                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    #if (OS_TASK_STAT_EN > 0u)
    {
        KAL_CPU_USAGE  cpu_usage;


        cpu_usage  = (KAL_CPU_USAGE) OSCPUUsage;
        cpu_usage *= 100u;

        return (cpu_usage);
    }
    #else
        RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, 0u);

        return (0u);
    #endif
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         KAL_TmrFnctWrapper()
*
* Description : Wrapper function for timer callback.
*
* Argument(s) : p_tmr_os    Pointer to OS timer object.
*
*               p_arg       Pointer to KAL timer object.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if (OS_TMR_EN == DEF_ENABLED)
static  void  KAL_TmrFnctWrapper (void  *p_tmr_os,
                                  void  *p_arg)
{
    KAL_TMR  *p_tmr;


    PP_UNUSED_PARAM(p_tmr_os);

    p_tmr = (KAL_TMR *)p_arg;
    p_tmr->CallbackFnct(p_tmr->CallbackArg);
}
#endif


/*
*********************************************************************************************************
*                                            KAL_msToTicks()
*
* Description : Convert milliseconds value in tick value.
*
* Argument(s) : ms  Millisecond value to convert.
*
* Return(s)   : Number of ticks corresponding to the millisecond value, rounded up, if needed.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  KAL_TICK  KAL_msToTicks (CPU_INT32U  ms)
{
           KAL_TICK    ticks;
#if  ((OS_TICKS_PER_SEC          >= 1000u) && \
      (OS_TICKS_PER_SEC %  1000u ==    0u))
    const  CPU_INT08U  mult = OS_TICKS_PER_SEC / 1000u;
#endif


    #if  ((OS_TICKS_PER_SEC          >= 1000u) && \
          (OS_TICKS_PER_SEC %  1000u ==    0u))                 /* Optimize calc if possible for often used vals.       */
        ticks =    ms * mult;
    #elif (OS_TICKS_PER_SEC ==  100u)
        ticks =  ((ms +  9u) /  10u);
    #elif (OS_TICKS_PER_SEC ==   10u)
        ticks =  ((ms + 99u) / 100u);
    #else                                                       /* General formula.                                     */
        ticks = (((ms * OS_TICKS_PER_SEC)  + 1000u - 1u) / 1000u);
    #endif

    return (ticks);
}


/*
*********************************************************************************************************
*                                           KAL_ErrConvert()
*
* Description : Convert OS errors in KAL errors.
*
* Argument(s) : err_os  Error value used by the OS.
*
* Return(s)   : KAL error.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if (RTOS_ERR_CFG_LEGACY_EN == DEF_ENABLED)
static  RTOS_ERR       KAL_ErrConvert (CPU_INT08U  err_os)
#else
static  RTOS_ERR_CODE  KAL_ErrConvert (CPU_INT08U  err_os)
#endif
{
#if (RTOS_ERR_CFG_LEGACY_EN == DEF_ENABLED)
    RTOS_ERR       err_rtos;
#else
    RTOS_ERR_CODE  err_rtos;
#endif

    switch (err_os) {
        case OS_ERR_NONE:
             err_rtos = RTOS_ERR_NONE;
             break;


        case OS_ERR_TASK_WAITING:
             err_rtos = RTOS_ERR_OS_TASK_WAITING;
             break;


        case OS_ERR_EVENT_TYPE:
        case OS_ERR_INVALID_OPT:
        case OS_ERR_ID_INVALID:
        case OS_ERR_PNAME_NULL:
        case OS_ERR_PRIO_EXIST:
        case OS_ERR_PRIO_INVALID:
        case OS_ERR_TASK_DEL:
        case OS_ERR_TASK_DEL_IDLE:
        case OS_ERR_TASK_NOT_EXIST:
        case OS_ERR_TMR_INACTIVE:
        case OS_ERR_TMR_INVALID_DLY:
        case OS_ERR_TMR_INVALID_TYPE:
        case OS_ERR_TMR_INVALID:
             err_rtos = RTOS_ERR_INVALID_ARG;
             break;


        case OS_ERR_PEVENT_NULL:
             err_rtos = RTOS_ERR_NULL_PTR;
             break;


        case OS_ERR_PEND_LOCKED:
        case OS_ERR_TMR_INVALID_PERIOD:
        case OS_ERR_TMR_INVALID_OPT:
        case OS_ERR_TMR_INVALID_STATE:
             err_rtos = RTOS_ERR_OS;
             break;


        case OS_ERR_TIMEOUT:
             err_rtos = RTOS_ERR_TIMEOUT;
             break;


        case OS_ERR_PEND_ABORT:
             err_rtos = RTOS_ERR_ABORT;
             break;


        case OS_ERR_Q_EMPTY:
             err_rtos = RTOS_ERR_WOULD_BLOCK;
             break;


        case OS_ERR_PEND_ISR:
        case OS_ERR_DEL_ISR:
        case OS_ERR_NAME_SET_ISR:
        case OS_ERR_TASK_CREATE_ISR:
        case OS_ERR_TASK_DEL_ISR:
        case OS_ERR_TMR_ISR:
             err_rtos = RTOS_ERR_ISR;
             break;


        case OS_ERR_SEM_OVF:
             err_rtos = RTOS_ERR_WOULD_OVF;
             break;


        case OS_ERR_Q_FULL:
        case OS_ERR_NO_MORE_ID_AVAIL:
        case OS_ERR_TMR_NON_AVAIL:
             err_rtos = RTOS_ERR_NO_MORE_RSRC;
             break;


        default:
             err_rtos = RTOS_ERR_OS;
             break;
    }

    return (err_rtos);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* (defined(RTOS_MODULE_KERNEL_OS2_AVAIL)) */
