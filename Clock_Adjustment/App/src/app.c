/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing terms:
*   This file is provided as an example on how to use Micrium products. It has not necessarily been
*   tested under every possible condition and is only offered as a reference, without any guarantee.
*
*   Please feel free to use any application code labeled as 'EXAMPLE CODE' in your application products.
*   Example code may be used as is, in whole or in part, or may be used as a reference only. This file
*   can be modified as required.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: http://www.micrium.com
*
*   Please help us continue to provide the Embedded community with the finest software available.
*
*   Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                                APP
*
* File : app.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <stdio.h>

#include  <bsp.h>
#include  <bsp_os.h>
#include  <bsp_cpu.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/common.h>
#include  <rtos/kernel/include/os.h>

#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/toolchains.h>

#include  "ex_description.h"
#include  "common/common/ex_common.h"
#include  "net/ex_network_init.h"
#include  "net/core_init/ex_net_core_init.h"
#include  "retargetserial.h"
#include  "em_cmu.h"
#include  "em_emu.h"
#include  "em_chip.h"
#include  "app_tcp.h"
#include  "app_graphics.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  APP_START_TASK_PRIO                        21u
#define  APP_START_TASK_STK_SIZE                   512u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Start Task Stack.                                    */
static  CPU_STK  AppStartTaskStk[APP_START_TASK_STK_SIZE];
                                                                /* Start Task TCB.                                      */
static  OS_TCB   AppStartTaskTCB;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  AppStartTask (void  *p_arg);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C applications. It is assumed that your code will
*               call main() once you have performed all necessary initialization.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

int  main (void)
{
    RTOS_ERR  err;


    BSP_SystemInit();                                           /* Initialize System.                                   */
    BSP_CPUInit();                                              /* Initialize CPU.                                      */
    RETARGET_SerialInit();
    RETARGET_SerialCrLf(1);

    OS_TRACE_INIT();
    OSInit(&err);                                               /* Initialize the Kernel.                               */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

    OSTaskCreate(&AppStartTaskTCB,                              /* Create the Start Task.                               */
                 "App Start Task",
                  AppStartTask,
                  DEF_NULL,
                  APP_START_TASK_PRIO,
                 &AppStartTaskStk[0],
                 (APP_START_TASK_STK_SIZE / 10u),
                  APP_START_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


    OSStart(&err);                                              /* Start the kernel.                                    */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);


    return (1);
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
*                                          AppStartTask()
*
* Description : This is the task that will be called by the Startup when all services are initializes
*               successfully.
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  AppStartTask (void  *p_arg)
{
    RTOS_ERR    err;
    NET_IF_NBR  if_nbr;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */


    BSP_TickInit();                                             /* Initialize Kernel tick source.                       */


#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
    OSStatTaskCPUUsageInit(&err);                               /* Initialize CPU Usage.                                */
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();                                /* Initialize interrupts disabled measurement.          */
#endif

    Ex_CommonInit();                                            /* Call common module initialization example.           */

    BSP_OS_Init();                                              /* Initialize the BSP. It is expected that the BSP ...  */
                                                                /* ... will register all the hardware controller to ... */
                                                                /* ... the platform manager at this moment.             */

    // Micrium OS Network Lab 1: Step 1-3 //
    /* Set the static IP address for init */
//    Ex_NetIF_CfgDflt_Ether.IPv4.Static.Addr = "10.1.10.10";
    Ex_NetIF_CfgDflt_Ether.IPv4.Static.Addr = "10.1.10.9";
    Ex_NetworkInit(); /* Call Network module initialization example. */
    Ex_Net_CoreStartIF(); /* Call Network interface start example. */

    AppGraphicsInit();                                          /* Initialize the graphics task                         */

    if_nbr = NetIF_NbrGetFromName("eth0");                      /* Get the interface number                             */

    // Micrium OS Network Lab 1: Step 1-4 //
//    AppClientInit(if_nbr); 										/* Initialize the client task							*/
    AppServerInit(if_nbr); 										/* Initialize the client task							*/


    while (DEF_ON) {
                                                                /* Delay Start Task execution for                       */
        OSTimeDly(1000,                                         /*   1000 OS Ticks                                      */
                  OS_OPT_TIME_DLY,                              /*   from now.                                          */
                  &err);
                                                                /*   Check error code.                                  */
        APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), ;);
    }
}



