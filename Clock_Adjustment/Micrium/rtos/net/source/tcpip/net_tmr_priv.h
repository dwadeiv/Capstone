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
*                                       NETWORK TIMER MANAGEMENT
*
* File : net_tmr_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_TMR_PRIV_H_
#define  _NET_TMR_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "net_priv.h"
#include  "../../include/net_cfg_net.h"
#include  "net_type_priv.h"
#include  "net_stat_priv.h"

#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/source/Collections/slist_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     NETWORK TIMER FLAG DEFINES
*********************************************************************************************************
*/

                                                                /* ------------------ NET TMR FLAGS ------------------- */
#define  NET_TMR_FLAG_NONE                        DEF_BIT_NONE
#define  NET_TMR_FLAG_USED                        DEF_BIT_00    /* Tmr cur used; i.e. NOT in free tmr pool.             */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  NETWORK TIMER QUANTITY DATA TYPE
*
* Note(s) : (1) NET_TMR_NBR_MAX  SHOULD be #define'd based on 'NET_TMR_QTY' data type declared.
*********************************************************************************************************
*/

typedef  CPU_INT16U  NET_TMR_QTY;                               /* Defines max qty of net tmrs to support.              */

#define  NET_TMR_NBR_MIN                                  0
#define  NET_TMR_NBR_MAX                DEF_INT_16U_MAX_VAL     /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                    NETWORK TIMER FLAGS DATA TYPE
*********************************************************************************************************
*/

typedef  struct  net_tmr_flags {
    CPU_INT08U  IsPeriodic  : 1;
} NET_TMR_FLAGS;


/*
*********************************************************************************************************
*                                NETWORK TIMER OPTION FLAGS DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_INT08U  NET_TMR_OPT;

#define  NET_TMR_OPT_NONE                   DEF_BIT_NONE
#define  NET_TMR_OPT_ONE_SHOT               NET_TMR_OPT_NONE    /* One-shot tmr,   callback called only once.           */
#define  NET_TMR_OPT_PERIODIC               DEF_BIT_00          /* Periodic timer, callback called periodically.        */


/*
*********************************************************************************************************
*                                       NETWORK TIMER DATA TYPE
*
*                                    NET_TMR
*                                |-------------|
*                                | Timer Type  |
*                     Previous   |-------------|
*                      Timer <----------O      |
*                                |-------------|     Next
*                                |      O----------> Timer
*                                |-------------|                    -------------
*                                |      O-------------------------> |           |
*                                |-------------|       Object       |  Object   |
*                                |      O----------> Expiration     |   that    |
*                                |-------------|      Function      | requested |
*                                | Timer value |                    |   Timer   |
*                                |-------------|                    -------------
*                                |    Delta    |
*                                |-------------|
*                                |    Flags    |
*                                |-------------|
*********************************************************************************************************
*/

                                                                /* --------------------- NET TMR ---------------------- */
typedef  struct  net_tmr  NET_TMR;

struct net_tmr {
    NET_TMR        *PrevPtr;                                    /* Ptr to PREV tmr.                                     */
    NET_TMR        *NextPtr;                                    /* Ptr to NEXT tmr.                                     */

    void           *Obj;                                        /* Ptr to obj  using TMR.                               */
    CPU_FNCT_PTR    Fnct;                                       /* Ptr to fnct used on obj when TMR expires.            */
    CPU_INT32U      Val;                                        /* Timer value (in milliseconds).                       */
    CPU_INT32U      Delta;
    CPU_INT32U      TS_ms;
    NET_TMR_FLAGS   Flags;
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void            NetTmr_Init                (MEM_SEG       *p_mem_seg,
                                            RTOS_ERR      *p_err);

void            NetTmr_Handler             (void);

NET_TMR        *NetTmr_Get                 (CPU_FNCT_PTR    fnct,
                                            void           *obj,
                                            CPU_INT32U      time_ms,
                                            NET_TMR_OPT     opt_flags,
                                            RTOS_ERR       *p_err);

void            NetTmr_Free                (NET_TMR        *p_tmr);

void            NetTmr_Set                 (NET_TMR        *p_tmr,
                                            CPU_FNCT_PTR    fnct,
                                            CPU_INT32U      time_ms);

CPU_INT32U      NetTmr_ListMinTimeoutGet   (void);

CPU_INT32U      NetTmr_TimeRemainGet       (NET_TMR        *p_tmr);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_TMR_PRIV_H_ */
