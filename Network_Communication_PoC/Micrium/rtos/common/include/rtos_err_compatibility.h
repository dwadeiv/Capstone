/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Common
*
*                             (c) Copyright 2017; Silicon Laboratories Inc.
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
*                                      RTOS ERROR (COMPATIBILITY)
*
* File : rtos_err_compatibility.h
*********************************************************************************************************
* Note(s) : (1) All these types and error codes are deprecated and will be removed in a future version.
*               Please use the codes from RTOS_ERR instead, located in common_err.h.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _RTOS_ERR_COMPATIBILITY_H_
#define  _RTOS_ERR_COMPATIBILITY_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/include/rtos_path.h>
#include  <rtos_description.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            CPU ERROR TYPES
*********************************************************************************************************
*/

typedef  RTOS_ERR  CPU_ERR;

#define  CPU_ERR_NONE               RTOS_ERR_NONE
#define  CPU_ERR_NULL_PTR           RTOS_ERR_NULL_PTR

#define  CPU_ERR_NAME_SIZE          RTOS_ERR_INVALID_ARG

#define  CPU_ERR_TS_FREQ_INVALID    RTOS_ERR_INVALID_CFG


/*
*********************************************************************************************************
*                                           OS-3 ERROR TYPES
*********************************************************************************************************
*/

#if (defined(RTOS_MODULE_KERNEL_OS3_AVAIL) || \
     defined(RTOS_MODULE_KERNEL_AVAIL))
typedef  RTOS_ERR  OS_ERR;

#define  OS_ERR_NONE                        RTOS_ERR_NONE

#define  OS_ERR_A                           RTOS_ERR_FAIL
#define  OS_ERR_ACCEPT_ISR                  RTOS_ERR_ISR

#define  OS_ERR_B                           RTOS_ERR_FAIL

#define  OS_ERR_C                           RTOS_ERR_FAIL
#define  OS_ERR_CREATE_ISR                  RTOS_ERR_ISR

#define  OS_ERR_D                           RTOS_ERR_FAIL
#define  OS_ERR_DEL_ISR                     RTOS_ERR_ISR

#define  OS_ERR_E                           RTOS_ERR_FAIL

#define  OS_ERR_F                           RTOS_ERR_FAIL
#define  OS_ERR_FATAL_RETURN                RTOS_ERR_OS

#define  OS_ERR_FLAG_GRP_DEPLETED           RTOS_ERR_NO_MORE_RSRC
#define  OS_ERR_FLAG_NOT_RDY                RTOS_ERR_NOT_READY
#define  OS_ERR_FLAG_PEND_OPT               RTOS_ERR_INVALID_ARG
#define  OS_ERR_FLUSH_ISR                   RTOS_ERR_ISR

#define  OS_ERR_G                           RTOS_ERR_FAIL

#define  OS_ERR_H                           RTOS_ERR_FAIL

#define  OS_ERR_I                           RTOS_ERR_FAIL
#define  OS_ERR_ILLEGAL_CREATE_RUN_TIME     RTOS_ERR_OS_ILLEGAL_RUN_TIME
#define  OS_ERR_INT_Q                       RTOS_ERR_NULL_PTR
#define  OS_ERR_INT_Q_FULL                  RTOS_ERR_WOULD_OVF
#define  OS_ERR_INT_Q_SIZE                  RTOS_ERR_INVALID_CFG
#define  OS_ERR_INT_Q_STK_INVALID           RTOS_ERR_INVALID_CFG
#define  OS_ERR_INT_Q_STK_SIZE_INVALID      RTOS_ERR_INVALID_CFG
#define  OS_ERR_ILLEGAL_DEL_RUN_TIME        RTOS_ERR_OS_ILLEGAL_RUN_TIME

#define  OS_ERR_J                           RTOS_ERR_FAIL

#define  OS_ERR_K                           RTOS_ERR_FAIL

#define  OS_ERR_L                           RTOS_ERR_FAIL
#define  OS_ERR_LOCK_NESTING_OVF            RTOS_ERR_WOULD_OVF

#define  OS_ERR_M                           RTOS_ERR_FAIL

#define  OS_ERR_MEM_CREATE_ISR              RTOS_ERR_ISR
#define  OS_ERR_MEM_FULL                    RTOS_ERR_POOL_FULL
#define  OS_ERR_MEM_INVALID_P_ADDR          RTOS_ERR_NULL_PTR
#define  OS_ERR_MEM_INVALID_BLKS            RTOS_ERR_INVALID_ARG
#define  OS_ERR_MEM_INVALID_PART            RTOS_ERR_INVALID_ARG
#define  OS_ERR_MEM_INVALID_P_BLK           RTOS_ERR_NULL_PTR
#define  OS_ERR_MEM_INVALID_P_MEM           RTOS_ERR_NULL_PTR
#define  OS_ERR_MEM_INVALID_P_DATA          RTOS_ERR_NULL_PTR
#define  OS_ERR_MEM_INVALID_SIZE            RTOS_ERR_INVALID_ARG
#define  OS_ERR_MEM_NO_FREE_BLKS            RTOS_ERR_POOL_EMPTY

#define  OS_ERR_MSG_POOL_EMPTY              RTOS_ERR_NO_MORE_RSRC
#define  OS_ERR_MSG_POOL_NULL_PTR           RTOS_ERR_INVALID_CFG

#define  OS_ERR_MUTEX_NOT_OWNER             RTOS_ERR_OWNERSHIP
#define  OS_ERR_MUTEX_OWNER                 RTOS_ERR_IS_OWNER
#define  OS_ERR_MUTEX_NESTING               RTOS_ERR_IS_OWNER
#define  OS_ERR_MUTEX_OVF                   RTOS_ERR_WOULD_OVF

#define  OS_ERR_N                           RTOS_ERR_FAIL
#define  OS_ERR_NAME                        RTOS_ERR_INVALID_ARG
#define  OS_ERR_NO_MORE_ID_AVAIL            RTOS_ERR_NO_MORE_RSRC

#define  OS_ERR_O                           RTOS_ERR_FAIL
#define  OS_ERR_OBJ_CREATED                 RTOS_ERR_FAIL
#define  OS_ERR_OBJ_DEL                     RTOS_ERR_OS_OBJ_DEL
#define  OS_ERR_OBJ_PTR_NULL                RTOS_ERR_NULL_PTR
#define  OS_ERR_OBJ_TYPE                    RTOS_ERR_INVALID_TYPE

#define  OS_ERR_OPT_INVALID                 RTOS_ERR_INVALID_ARG

#define  OS_ERR_OS_NOT_RUNNING              RTOS_ERR_NOT_READY
#define  OS_ERR_OS_RUNNING                  RTOS_ERR_INVALID_STATE
#define  OS_ERR_OS_NOT_INIT                 RTOS_ERR_NOT_INIT
#define  OS_ERR_OS_NO_APP_TASK              RTOS_ERR_INVALID_CFG

#define  OS_ERR_P                           RTOS_ERR_FAIL
#define  OS_ERR_PEND_ABORT                  RTOS_ERR_ABORT
#define  OS_ERR_PEND_ABORT_ISR              RTOS_ERR_ISR
#define  OS_ERR_PEND_ABORT_NONE             RTOS_ERR_NONE_WAITING
#define  OS_ERR_PEND_ABORT_SELF             RTOS_ERR_INVALID_ARG
#define  OS_ERR_PEND_DEL                    RTOS_ERR_OS_OBJ_DEL
#define  OS_ERR_PEND_ISR                    RTOS_ERR_ISR
#define  OS_ERR_PEND_LOCKED                 RTOS_ERR_FAIL
#define  OS_ERR_PEND_WOULD_BLOCK            RTOS_ERR_WOULD_BLOCK

#define  OS_ERR_POST_NULL_PTR               RTOS_ERR_NULL_PTR
#define  OS_ERR_POST_ISR                    RTOS_ERR_ISR

#define  OS_ERR_PRIO_EXIST                  RTOS_ERR_ALREADY_EXISTS
#define  OS_ERR_PRIO                        RTOS_ERR_FAIL
#define  OS_ERR_PRIO_INVALID                RTOS_ERR_INVALID_ARG

#define  OS_ERR_PTR_INVALID                 RTOS_ERR_NULL_PTR

#define  OS_ERR_Q                           RTOS_ERR_FAIL
#define  OS_ERR_Q_FULL                      RTOS_ERR_WOULD_OVF
#define  OS_ERR_Q_EMPTY                     RTOS_ERR_NOT_FOUND
#define  OS_ERR_Q_MAX                       RTOS_ERR_WOULD_OVF
#define  OS_ERR_Q_SIZE                      RTOS_ERR_INVALID_ARG

#define  OS_ERR_R                           RTOS_ERR_FAIL
#define  OS_ERR_REG_ID_INVALID              RTOS_ERR_INVALID_ARG
#define  OS_ERR_ROUND_ROBIN_1               RTOS_ERR_NONE_WAITING
#define  OS_ERR_ROUND_ROBIN_DISABLED        RTOS_ERR_NOT_AVAIL

#define  OS_ERR_S                           RTOS_ERR_FAIL
#define  OS_ERR_SCHED_INVALID_TIME_SLICE    RTOS_ERR_INVALID_ARG
#define  OS_ERR_SCHED_LOCK_ISR              RTOS_ERR_ISR
#define  OS_ERR_SCHED_LOCKED                RTOS_ERR_OS_SCHED_LOCKED
#define  OS_ERR_SCHED_NOT_LOCKED            RTOS_ERR_INVALID_STATE
#define  OS_ERR_SCHED_UNLOCK_ISR            RTOS_ERR_ISR

#define  OS_ERR_SEM_OVF                     RTOS_ERR_WOULD_OVF
#define  OS_ERR_SET_ISR                     RTOS_ERR_ISR

#define  OS_ERR_STAT_RESET_ISR              RTOS_ERR_ISR
#define  OS_ERR_STAT_PRIO_INVALID           RTOS_ERR_INVALID_CFG
#define  OS_ERR_STAT_STK_INVALID            RTOS_ERR_INVALID_CFG
#define  OS_ERR_STAT_STK_SIZE_INVALID       RTOS_ERR_INVALID_CFG
#define  OS_ERR_STATE_INVALID               RTOS_ERR_INVALID_STATE
#define  OS_ERR_STATUS_INVALID              RTOS_ERR_OS
#define  OS_ERR_STK_INVALID                 RTOS_ERR_NULL_PTR
#define  OS_ERR_STK_SIZE_INVALID            RTOS_ERR_INVALID_ARG
#define  OS_ERR_STK_LIMIT_INVALID           RTOS_ERR_INVALID_ARG

#define  OS_ERR_T                           RTOS_ERR_FAIL
#define  OS_ERR_TASK_CHANGE_PRIO_ISR        RTOS_ERR_ISR
#define  OS_ERR_TASK_CREATE_ISR             RTOS_ERR_ISR
#define  OS_ERR_TASK_DEL                    RTOS_ERR_FAIL
#define  OS_ERR_TASK_DEL_IDLE               RTOS_ERR_INVALID_ARG
#define  OS_ERR_TASK_DEL_INVALID            RTOS_ERR_INVALID_ARG
#define  OS_ERR_TASK_DEL_ISR                RTOS_ERR_ISR
#define  OS_ERR_TASK_INVALID                RTOS_ERR_NULL_PTR
#define  OS_ERR_TASK_NO_MORE_TCB            RTOS_ERR_NO_MORE_RSRC
#define  OS_ERR_TASK_NOT_DLY                RTOS_ERR_INVALID_STATE
#define  OS_ERR_TASK_NOT_EXIST              RTOS_ERR_INVALID_ARG
#define  OS_ERR_TASK_NOT_SUSPENDED          RTOS_ERR_INVALID_STATE
#define  OS_ERR_TASK_OPT                    RTOS_ERR_NOT_SUPPORTED
#define  OS_ERR_TASK_RESUME_ISR             RTOS_ERR_ISR
#define  OS_ERR_TASK_RESUME_PRIO            RTOS_ERR_INVALID_ARG
#define  OS_ERR_TASK_RESUME_SELF            RTOS_ERR_INVALID_ARG
#define  OS_ERR_TASK_RUNNING                RTOS_ERR_OS
#define  OS_ERR_TASK_STK_CHK_ISR            RTOS_ERR_ISR
#define  OS_ERR_TASK_SUSPENDED              RTOS_ERR_OS_TASK_SUSPENDED
#define  OS_ERR_TASK_SUSPEND_IDLE           RTOS_ERR_OS
#define  OS_ERR_TASK_SUSPEND_INT_HANDLER    RTOS_ERR_OS
#define  OS_ERR_TASK_SUSPEND_ISR            RTOS_ERR_ISR
#define  OS_ERR_TASK_SUSPEND_PRIO           RTOS_ERR_INVALID_ARG
#define  OS_ERR_TASK_WAITING                RTOS_ERR_OS_TASK_WAITING
#define  OS_ERR_TASK_SUSPEND_CTR_OVF        RTOS_ERR_WOULD_OVF

#define  OS_ERR_TCB_INVALID                 RTOS_ERR_NULL_PTR

#define  OS_ERR_TLS_ID_INVALID              RTOS_ERR_INVALID_ARG
#define  OS_ERR_TLS_ISR                     RTOS_ERR_ISR
#define  OS_ERR_TLS_NO_MORE_AVAIL           RTOS_ERR_NO_MORE_RSRC
#define  OS_ERR_TLS_NOT_EN                  RTOS_ERR_NOT_AVAIL
#define  OS_ERR_TLS_DESTRUCT_ASSIGNED       RTOS_ERR_INVALID_ARG

#define  OS_ERR_TICK_PRIO_INVALID           RTOS_ERR_INVALID_CFG
#define  OS_ERR_TICK_STK_INVALID            RTOS_ERR_INVALID_CFG
#define  OS_ERR_TICK_STK_SIZE_INVALID       RTOS_ERR_INVALID_CFG
#define  OS_ERR_TICK_WHEEL_SIZE             RTOS_ERR_INVALID_ARG

#define  OS_ERR_TIME_DLY_ISR                RTOS_ERR_ISR
#define  OS_ERR_TIME_DLY_RESUME_ISR         RTOS_ERR_ISR
#define  OS_ERR_TIME_GET_ISR                RTOS_ERR_ISR
#define  OS_ERR_TIME_INVALID_HOURS          RTOS_ERR_INVALID_ARG
#define  OS_ERR_TIME_INVALID_MINUTES        RTOS_ERR_INVALID_ARG
#define  OS_ERR_TIME_INVALID_SECONDS        RTOS_ERR_INVALID_ARG
#define  OS_ERR_TIME_INVALID_MILLISECONDS   RTOS_ERR_INVALID_ARG
#define  OS_ERR_TIME_NOT_DLY                RTOS_ERR_FAIL
#define  OS_ERR_TIME_SET_ISR                RTOS_ERR_ISR
#define  OS_ERR_TIME_ZERO_DLY               RTOS_ERR_INVALID_ARG

#define  OS_ERR_TIMEOUT                     RTOS_ERR_TIMEOUT

#define  OS_ERR_TMR_INACTIVE                RTOS_ERR_NOT_INIT
#define  OS_ERR_TMR_INVALID_DEST            RTOS_ERR_FAIL
#define  OS_ERR_TMR_INVALID_DLY             RTOS_ERR_INVALID_ARG
#define  OS_ERR_TMR_INVALID_PERIOD          RTOS_ERR_INVALID_ARG
#define  OS_ERR_TMR_INVALID_STATE           RTOS_ERR_OS
#define  OS_ERR_TMR_INVALID                 RTOS_ERR_NULL_PTR
#define  OS_ERR_TMR_ISR                     RTOS_ERR_ISR
#define  OS_ERR_TMR_NO_CALLBACK             RTOS_ERR_INVALID_ARG
#define  OS_ERR_TMR_NON_AVAIL               RTOS_ERR_NOT_AVAIL
#define  OS_ERR_TMR_PRIO_INVALID            RTOS_ERR_INVALID_CFG
#define  OS_ERR_TMR_STK_INVALID             RTOS_ERR_INVALID_CFG
#define  OS_ERR_TMR_STK_SIZE_INVALID        RTOS_ERR_INVALID_CFG
#define  OS_ERR_TMR_STOPPED                 RTOS_ERR_INVALID_STATE
#define  OS_ERR_TMR_INVALID_CALLBACK        RTOS_ERR_NULL_PTR

#define  OS_ERR_U                           RTOS_ERR_FAIL

#define  OS_ERR_V                           RTOS_ERR_FAIL

#define  OS_ERR_W                           RTOS_ERR_FAIL

#define  OS_ERR_X                           RTOS_ERR_FAIL

#define  OS_ERR_Y                           RTOS_ERR_FAIL
#define  OS_ERR_YIELD_ISR                   RTOS_ERR_ISR

#define  OS_ERR_Z                           RTOS_ERR_FAIL
#endif


/*
*********************************************************************************************************
*                                          COMMON ERROR TYPES
*********************************************************************************************************
*/

                                                                /* -------------------- CLK ERRORS -------------------- */
typedef  RTOS_ERR  CLK_ERR;

#define  CLK_ERR_NONE           RTOS_ERR_NONE
#define  CLK_ERR_NOT_NULL_PTR   RTOS_ERR_INVALID_ARG

#define  CLK_ERR_NOT_INIT       RTOS_ERR_NOT_INIT
#define  CLK_ERR_ALLOC          RTOS_ERR_ALLOC
#define  CLK_OS_ERR_NONE        RTOS_ERR_NONE

                                                                /* -------------------- LIB ERRORS -------------------- */
typedef  RTOS_ERR  LIB_ERR;

#define  LIB_MEM_ERR_NONE                       RTOS_ERR_NONE
#define  LIB_ERR_NONE                           RTOS_ERR_NONE

#define  LIB_MEM_ERR_NULL_PTR                   RTOS_ERR_NULL_PTR

#define  LIB_MEM_ERR_INVALID_MEM_SIZE           RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_MEM_ALIGN          RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_SEG_SIZE           RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_SEG_OVERLAP        RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_SEG_EXISTS         RTOS_ERR_INVALID_ARG


#define  LIB_MEM_ERR_INVALID_BLK_NBR            RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_BLK_SIZE           RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_BLK_ALIGN          RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_BLK_ADDR           RTOS_ERR_INVALID_ARG
#define  LIB_MEM_ERR_INVALID_BLK_ADDR_IN_POOL   RTOS_ERR_ALREADY_EXISTS

#define  LIB_MEM_ERR_SEG_OVF                    RTOS_ERR_SEG_OVF
#define  LIB_MEM_ERR_POOL_FULL                  RTOS_ERR_POOL_FULL
#define  LIB_MEM_ERR_POOL_EMPTY                 RTOS_ERR_POOL_EMPTY
#define  LIB_MEM_ERR_POOL_UNLIMITED             RTOS_ERR_POOL_UNLIMITED

#define  LIB_MEM_ERR_HEAP_OVF                   RTOS_ERR_SEG_OVF

                                                                /* -------------------- KAL ERRORS -------------------- */
typedef  RTOS_ERR  KAL_ERR;

#define  KAL_ERR_NONE           RTOS_ERR_NONE

#define  KAL_ERR_NOT_AVAIL      RTOS_ERR_NOT_AVAIL
#define  KAL_ERR_UNIMPLEMENTED  RTOS_ERR_NOT_SUPPORTED
#define  KAL_ERR_INVALID_ARG    RTOS_ERR_INVALID_ARG
#define  KAL_ERR_NULL_PTR       RTOS_ERR_NULL_PTR
#define  KAL_ERR_OS             RTOS_ERR_OS

#define  KAL_ERR_ALREADY_INIT   RTOS_ERR_ALREADY_INIT
#define  KAL_ERR_MEM_ALLOC      RTOS_ERR_ALLOC
#define  KAL_ERR_POOL_INIT      RTOS_ERR_INIT
#define  KAL_ERR_CREATE         RTOS_ERR_OS
#define  KAL_ERR_TIMEOUT        RTOS_ERR_TIMEOUT
#define  KAL_ERR_ABORT          RTOS_ERR_ABORT
#define  KAL_ERR_WOULD_BLOCK    RTOS_ERR_WOULD_BLOCK
#define  KAL_ERR_ISR            RTOS_ERR_ISR
#define  KAL_ERR_OVF            RTOS_ERR_WOULD_OVF
#define  KAL_ERR_RSRC           RTOS_ERR_NO_MORE_RSRC

#define  KAL_ERR_LOCK_OWNER     RTOS_ERR_OWNERSHIP

                                                                /* ------------------- SHELL ERRORS ------------------- */
typedef  RTOS_ERR  SHELL_ERR;
#define  SHELL_ERR_NONE                         RTOS_ERR_NONE
#define  SHELL_ERR_NULL_PTR                     RTOS_ERR_NULL_PTR
#define  SHELL_ERR_MODULE_CMD_EMPTY             RTOS_ERR_CMD_EMPTY
#define  SHELL_ERR_MODULE_CMD_NAME_TOO_LONG     RTOS_ERR_INVALID_ARG
#define  SHELL_ERR_MODULE_CMD_NAME_NONE         RTOS_ERR_INVALID_ARG
#define  SHELL_ERR_MODULE_CMD_NAME_COPY         RTOS_ERR_FAIL
#define  SHELL_ERR_MODULE_CMD_NONE_AVAIL        RTOS_ERR_NO_MORE_RSRC
#define  SHELL_ERR_MODULE_CMD_ALREADY_IN        RTOS_ERR_ALREADY_EXISTS
#define  SHELL_ERR_MODULE_CMD_NOT_FOUND         RTOS_ERR_NOT_FOUND
#define  SHELL_ERR_CMD_EXEC                     RTOS_ERR_SHELL_CMD_EXEC
#define  SHELL_ERR_CMD_NOT_FOUND                RTOS_ERR_NOT_FOUND
#define  SHELL_ERR_ARG_TBL_FULL                 RTOS_ERR_NO_MORE_RSRC
#define  SHELL_ERR_CMD_SEARCH                   RTOS_ERR_FAIL
#define  SHELL_ERR_CMD_ALREADY_IN               RTOS_ERR_ALREADY_EXISTS


/*
*********************************************************************************************************
*                                           USBD ERROR TYPES
*********************************************************************************************************
*/

                                                                /* ------------------- USBD ERRORS -------------------- */
typedef  RTOS_ERR  USBD_ERR;

#define  USBD_ERR_NONE                          RTOS_ERR_NONE
#define  USBD_ERR_FAIL                          RTOS_ERR_FAIL
#define  USBD_ERR_RX                            RTOS_ERR_RX
#define  USBD_ERR_TX                            RTOS_ERR_TX
#define  USBD_ERR_ALLOC                         RTOS_ERR_ALLOC
#define  USBD_ERR_NULL_PTR                      RTOS_ERR_NULL_PTR
#define  USBD_ERR_INVALID_ARG                   RTOS_ERR_INVALID_ARG
#define  USBD_ERR_INVALID_CFG                   RTOS_ERR_INVALID_CFG
#define  USBD_ERR_INVALID_CLASS_STATE           RTOS_ERR_USB_INVALID_CLASS_STATE

#define  USBD_ERR_DEV_ALLOC                     RTOS_ERR_ALLOC
#define  USBD_ERR_DEV_INVALID_NBR               RTOS_ERR_INVALID_ARG
#define  USBD_ERR_DEV_INVALID_STATE             RTOS_ERR_USB_INVALID_DEV_STATE
#define  USBD_ERR_DEV_INVALID_SPD               RTOS_ERR_INVALID_ARG
#define  USBD_ERR_DEV_UNAVAIL_FEAT              RTOS_ERR_NOT_SUPPORTED

#define  USBD_ERR_CFG_ALLOC                     RTOS_ERR_USB_CFG_ALLOC
#define  USBD_ERR_CFG_INVALID_NBR               RTOS_ERR_INVALID_ARG
#define  USBD_ERR_CFG_INVALID_MAX_PWR           RTOS_ERR_INVALID_ARG
#define  USBD_ERR_CFG_SET_FAIL                  RTOS_ERR_FAIL

#define  USBD_ERR_IF_ALLOC                      RTOS_ERR_ALLOC
#define  USBD_ERR_IF_INVALID_NBR                RTOS_ERR_INVALID_ARG
#define  USBD_ERR_IF_ALT_ALLOC                  RTOS_ERR_USB_IF_ALT_ALLOC
#define  USBD_ERR_IF_ALT_INVALID_NBR            RTOS_ERR_INVALID_ARG
#define  USBD_ERR_IF_GRP_ALLOC                  RTOS_ERR_USB_IF_GRP_ALLOC
#define  USBD_ERR_IF_GRP_NBR_IN_USE             RTOS_ERR_ALREADY_EXISTS

#define  USBD_ERR_EP_ALLOC                      RTOS_ERR_ALLOC
#define  USBD_ERR_EP_INVALID_ADDR               RTOS_ERR_USB_INVALID_EP
#define  USBD_ERR_EP_INVALID_STATE              RTOS_ERR_USB_INVALID_EP_STATE
#define  USBD_ERR_EP_INVALID_TYPE               RTOS_ERR_USB_INVALID_EP
#define  USBD_ERR_EP_NONE_AVAIL                 RTOS_ERR_USB_EP_NONE_AVAIL
#define  USBD_ERR_EP_ABORT                      RTOS_ERR_ABORT
#define  USBD_ERR_EP_STALL                      RTOS_ERR_FAIL
#define  USBD_ERR_EP_IO_PENDING                 RTOS_ERR_NOT_READY
#define  USBD_ERR_EP_QUEUING                    RTOS_ERR_USB_EP_QUEUING

#define  USBD_ERR_OS_INIT_FAIL                  RTOS_ERR_OS
#define  USBD_ERR_OS_SIGNAL_CREATE              RTOS_ERR_OS
#define  USBD_ERR_OS_FAIL                       RTOS_ERR_OS
#define  USBD_ERR_OS_TIMEOUT                    RTOS_ERR_TIMEOUT
#define  USBD_ERR_OS_ABORT                      RTOS_ERR_ABORT
#define  USBD_ERR_OS_DEL                        RTOS_ERR_OS
#define  USBD_ERR_OS_FEAT_NOT_AVAIL             RTOS_ERR_NOT_AVAIL

#define  USBD_ERR_DRV_BUF_OVERFLOW              RTOS_ERR_WOULD_OVF
#define  USBD_ERR_DRV_INVALID_PKT               RTOS_ERR_RX

#define  USBD_ERR_CLASS_INVALID_NBR             RTOS_ERR_INVALID_ARG
#define  USBD_ERR_CLASS_XFER_IN_PROGRESS        RTOS_ERR_NOT_READY

#define  USBD_ERR_AUDIO_INSTANCE_ALLOC          RTOS_ERR_USB_CLASS_INSTANCE_ALLOC
#define  USBD_ERR_AUDIO_AS_IF_ALLOC             RTOS_ERR_ALLOC
#define  USBD_ERR_AUDIO_IT_ALLOC                RTOS_ERR_ALLOC
#define  USBD_ERR_AUDIO_OT_ALLOC                RTOS_ERR_ALLOC
#define  USBD_ERR_AUDIO_FU_ALLOC                RTOS_ERR_ALLOC
#define  USBD_ERR_AUDIO_MU_ALLOC                RTOS_ERR_ALLOC
#define  USBD_ERR_AUDIO_SU_ALLOC                RTOS_ERR_ALLOC
#define  USBD_ERR_AUDIO_REQ_INVALID_CTRL        RTOS_ERR_INVALID_ARG
#define  USBD_ERR_AUDIO_REQ_INVALID_ATTRIB      RTOS_ERR_INVALID_ARG
#define  USBD_ERR_AUDIO_REQ_INVALID_RECIPIENT   RTOS_ERR_INVALID_ARG
#define  USBD_ERR_AUDIO_REQ                     RTOS_ERR_FAIL
#define  USBD_ERR_AUDIO_INVALID_SAMPLING_FRQ    RTOS_ERR_NOT_SUPPORTED
#define  USBD_ERR_AUDIO_CODEC_INIT_FAILED       RTOS_ERR_INIT

#define  USBD_ERR_CDC_INSTANCE_ALLOC            RTOS_ERR_USB_CLASS_INSTANCE_ALLOC
#define  USBD_ERR_CDC_DATA_IF_ALLOC             RTOS_ERR_ALLOC
#define  USBD_ERR_CDC_SUBCLASS_INSTANCE_ALLOC   RTOS_ERR_USB_SUBCLASS_INSTANCE_ALLOC

#define  USBD_ERR_HID_INSTANCE_ALLOC            RTOS_ERR_USB_CLASS_INSTANCE_ALLOC
#define  USBD_ERR_HID_REPORT_INVALID            RTOS_ERR_INVALID_ARG
#define  USBD_ERR_HID_REPORT_ALLOC              RTOS_ERR_ALLOC
#define  USBD_ERR_HID_REPORT_PUSH_POP_ALLOC     RTOS_ERR_ALLOC

#define  USBD_ERR_MSC_INSTANCE_ALLOC            RTOS_ERR_USB_CLASS_INSTANCE_ALLOC
#define  USBD_ERR_MSC_INVALID_CBW               RTOS_ERR_FAIL
#define  USBD_ERR_MSC_INVALID_DIR               RTOS_ERR_FAIL
#define  USBD_ERR_MSC_MAX_LUN_EXCEED            RTOS_ERR_NO_MORE_RSRC
#define  USBD_ERR_MSC_MAX_VEN_ID_LEN_EXCEED     RTOS_ERR_INVALID_ARG
#define  USBD_ERR_MSC_MAX_PROD_ID_LEN_EXCEED    RTOS_ERR_INVALID_ARG
#define  USBD_ERR_SCSI_UNSUPPORTED_CMD          RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_LU_NOTRDY                RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_LU_NOTSUPPORTED          RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_LU_BUSY                  RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_LOG_BLOCK_ADDR           RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_MEDIUM_NOTPRESENT        RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_MEDIUM_NOT_RDY_TO_RDY    RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_MEDIUM_RDY_TO_NOT_RDY    RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_LOCK                     RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_LOCK_TIMEOUT             RTOS_ERR_FAIL
#define  USBD_ERR_SCSI_UNLOCK                   RTOS_ERR_FAIL

#define  USBD_ERR_PHDC_INSTANCE_ALLOC           RTOS_ERR_USB_CLASS_INSTANCE_ALLOC

#define  USBD_ERR_VENDOR_INSTANCE_ALLOC         RTOS_ERR_USB_CLASS_INSTANCE_ALLOC


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of rtos err compatibility module include.        */
