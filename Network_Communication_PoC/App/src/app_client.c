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
*                                             APP CLIENT
*
* File : app_client.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <stdint.h>
#include  <stdio.h>
#include  <string.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/common.h>
#include  <rtos/kernel/include/os.h>

#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/toolchains.h>

#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/net_if.h>
#include  <rtos/net/include/net_ip.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/net_app.h>
#include  <rtos/net/include/net_ascii.h>

#include  "app_tcp.h"
#include  "bspconfig.h"
#include  "em_cmu.h"
#include  "em_device.h"
#include  "em_gpio.h"
#include  "gpiointerrupt.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  APP_CLIENT_TCP_SERVER_ADDR         "10.1.10.9"
#define  APP_CLIENT_TCP_SERVER_PORT             10001u
#define  APP_CLIENT_TX_BUF_SIZE                    16u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/
                                                                /* Client Task Stack.                                 */
static  CPU_STK              AppClient_TaskStk[APP_CLIENT_TASK_STK_SIZE];
                                                                /* Client Task TCB.                                   */
static  OS_TCB               AppClient_TaskTCB;

static  APP_CLIENT_STATUS_t  AppClient_Status = APP_CLIENT_STATUS_NONE;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void         AppClient_GPIOInit         (void);

static  void         AppClient_Task             (void               *p_arg);

static  void         AppClient_GPIOCallback     (uint8_t             pin);

static  void         AppClient_LinkStateStatus  (NET_IF_NBR          if_nbr,
                                                 NET_IF_LINK_STATE   link_state);

static  CPU_BOOLEAN  AppClient_Tx               (NET_SOCK_ID         sock,
		                                         NET_SOCK_ADDR      *p_server_sock_addr,
												 void               *p_data,
												 NET_SOCK_DATA_SIZE  len);

static  NET_SOCK_ID  AppClient_NetInit          (NET_SOCK_ADDR      *server_sock_addr);



/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         AppClientInit()
*
* Description : Initialize the client task
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  AppClientInit (NET_IF_NBR if_nbr)
{
    RTOS_ERR  err;


    AppClient_Status = APP_CLIENT_STATUS_INIT;

    NetIF_LinkStateSubscribe ( if_nbr,                          /* Subscribe to the link state status                   */
                               AppClient_LinkStateStatus,
                              &err);

    OSTaskCreate(&AppClient_TaskTCB,                            /* Create the Client Task.                              */
                 "Client Task",
                  AppClient_Task,
                  DEF_NULL,
                  APP_CLIENT_TASK_PRIO,
                 &AppClient_TaskStk[0],
                 (APP_CLIENT_TASK_STK_SIZE / 10u),
                  APP_CLIENT_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}


/*
*********************************************************************************************************
*                                          AppClientGetStatus()
*
* Description : Gets the current status of the client
*
* Argument(s) : None.
*
* Return(s)   : The client's status
*
* Notes       : None.
*********************************************************************************************************
*/

APP_CLIENT_STATUS_t  AppClientGetStatus (void)
{
    return AppClient_Status;
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
*                                          AppClient_Task()
*
* Description : The client task that connects to the server
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  AppClient_Task (void  *p_arg)
{
    RTOS_ERR              err;
    NET_SOCK_ADDR         server_sock_addr;
    NET_SOCK_ID           sock;
    APP_TCP_MSG_t         tcp_msg = {0};
    CPU_BOOLEAN           status;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */
    AppClient_GPIOInit();                                       /* Initialize the GPIO pins for the button interrupt    */

    while (DEF_ON) {

        AppClient_Status = APP_CLIENT_STATUS_CONNECTING;        /* Initialize the client                                */
        status           = DEF_OK;

        sock = AppClient_NetInit(&server_sock_addr);            /* Initialize the network connection                    */

        if(sock > 0)
        {
            AppClient_Status = APP_CLIENT_STATUS_CONNECTED;     /* Set the status to connected and flush the queue      */
            OSTaskQFlush(&AppClient_TaskTCB,
                         &err);

            while(status == DEF_OK)
            {
                                                                /* Wait for a message to send                           */
                tcp_msg.val = (APP_TCP_ID_t) OSTaskQPend(                1000,
                                                                         OS_OPT_PEND_BLOCKING,
                                                         (OS_MSG_SIZE*)&(tcp_msg.id),
                                                                         0,
                                                                        &err);

                if(err.Code == RTOS_ERR_NONE)                   /* If we received data, send it to the server           */
                {
                    if(tcp_msg.id == APP_TCP_ID_LINKDOWN)       /* If we get a linkdown message, close the socket       */
                    {
                        NetSock_Close(sock, &err);
                        status = DEF_FAIL;
                    }

                    else                                        /* Otherwise send the message to the server             */
                    {
                        status = AppClient_Tx(        sock,
                                                     &server_sock_addr,
                                              (void*)&tcp_msg,
                                                     sizeof(tcp_msg));
                    }
                }
            }
        }

        AppClient_Status = APP_CLIENT_STATUS_DISCONNECTED;

        OSTimeDlyHMSM( 0, 0, 1, 0,
                       OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}


/*
*********************************************************************************************************
*                                          AppClient_GPIOInit()
*
* Description : Initialize the GPIO pins
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  AppClient_GPIOInit (void)
{
                                                                /* Enable GPIO clock and GPIO interrupt dispatcher      */
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIOINT_Init();
                                                                /* Config PB0 and PB1 as inputs and enable interrupts   */
    GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 0);
    GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, 0);

                                                                /* Register callbacks before enabling interrupts        */
    GPIOINT_CallbackRegister(BSP_GPIO_PB0_PIN, AppClient_GPIOCallback);
    GPIOINT_CallbackRegister(BSP_GPIO_PB1_PIN, AppClient_GPIOCallback);

                                                                /* Enable interrupts                                    */
    GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, DEF_FALSE, DEF_TRUE, DEF_TRUE);
    GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, DEF_FALSE, DEF_TRUE, DEF_TRUE);


}


/*
*********************************************************************************************************
*                                          AppClient_Init()
*
* Description : Initialize the
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  NET_SOCK_ID  AppClient_NetInit (NET_SOCK_ADDR *server_sock_addr)
{
    NET_IPv4_ADDR             server_addr;
    NET_SOCK_PROTOCOL_FAMILY  protocol_family;
    NET_SOCK_ADDR_FAMILY      sock_addr_family;
    NET_IP_ADDR_LEN           ip_addr_len;
    NET_SOCK_ID               sock;
    RTOS_ERR                  err;


                                                                /* ---------------- CONVERT IP ADDRESS ---------------- */
    NetASCII_Str_to_IP( APP_CLIENT_TCP_SERVER_ADDR,
                       &server_addr,
                        sizeof(server_addr),
                       &err);
    if (err.Code != RTOS_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------------ SET IP FAMILY ------------------- */
    sock_addr_family = NET_SOCK_ADDR_FAMILY_IP_V4;
    protocol_family  = NET_SOCK_PROTOCOL_FAMILY_IP_V4;
    ip_addr_len      = NET_IPv4_ADDR_SIZE;

                                                                /* ------------------- OPEN SOCKET -------------------- */
    sock = NetSock_Open( protocol_family,
                         NET_SOCK_TYPE_STREAM,
                         NET_SOCK_PROTOCOL_TCP,
                        &err);
    if (err.Code != RTOS_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ CONFIGURE SOCKET'S ADDRESS ------------ */
    NetApp_SetSockAddr(         server_sock_addr,
                                sock_addr_family,
                                APP_CLIENT_TCP_SERVER_PORT,
                       (void *)&server_addr,
                                ip_addr_len,
                               &err);
    if (err.Code != RTOS_ERR_NONE) {
        NetSock_Close(sock, &err);
        return (DEF_FAIL);
    }

                                                                /* ------------------ CONNECT SOCKET ------------------ */
    NetSock_Conn(sock, server_sock_addr, sizeof(server_sock_addr), &err);
    if (err.Code != RTOS_ERR_NONE) {
        NetSock_Close(sock, &err);
        return (DEF_FAIL);
    }
    ETH->NETWORKCTRL |= ETH_NETWORKCTRL_PTPUNICASTEN;                                      /**< Enable detection of unicast PTP unicast frames. */
   	ETH->NETWORKCTRL |= ETH_NETWORKCTRL_ONESTEPSYNCMODE;
    ETH->IENS |= ETH_IENS_PTPDLYREQFRMRX;

    return sock;

}


/*
*********************************************************************************************************
*                                          AppClient_Tx()
*
* Description :
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  AppClient_Tx (NET_SOCK_ID sock, NET_SOCK_ADDR *p_server_sock_addr, void* p_data, NET_SOCK_DATA_SIZE len)
{

    NET_SOCK_DATA_SIZE        tx_size;
    NET_SOCK_DATA_SIZE        tx_rem;
    RTOS_ERR                  err;


    tx_rem = len;

    do {
        tx_size = NetSock_TxDataTo(                  sock,
                                   (void           *)p_data,
                                                     len,
                                                     NET_SOCK_FLAG_NONE,
                                   (NET_SOCK_ADDR *) p_server_sock_addr,
                                                     NET_SOCK_ADDR_SIZE,
                                                    &err);
        switch (err.Code) {
            case RTOS_ERR_NONE:
                 tx_rem -= tx_size;
                 p_data   = (CPU_INT08U *)(p_data + tx_size);
                 break;

            case RTOS_ERR_POOL_EMPTY:                               /* Transitory Errors                                    */
            case RTOS_ERR_TIMEOUT:
            case RTOS_ERR_WOULD_BLOCK:
            case RTOS_ERR_NET_ADDR_UNRESOLVED:
            case RTOS_ERR_NET_IF_LINK_DOWN:
                 OSTimeDlyHMSM(0, 0, 0, 5, OS_OPT_TIME_DLY, &err);
                 break;

            default:
                 NetSock_Close(sock, &err);
                 return (DEF_FAIL);
        }
    } while (tx_rem > 0);

    return (DEF_TRUE);
}


/*
*********************************************************************************************************
*                                          AppClient_GPIOCallback()
*
* Description : GPIO Interrupt Callback Function
*
* Argument(s) : pin     The pin the interrupt occurred on
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static void AppClient_GPIOCallback(uint8_t pin)
{
    RTOS_ERR err;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();                                       /* Make the kernel aware of the start of the interrupt  */
    OSIntEnter();
    CPU_CRITICAL_EXIT();

    if (pin == BSP_GPIO_PB0_PIN) {                              /* Act on interrupts                                    */
        OSTaskQPost(             &AppClient_TaskTCB,
                    (void*)       0x01,
                    (OS_MSG_SIZE) APP_TCP_ID_BTN0,
                                  OS_OPT_POST_FIFO,
                                 &err);
    }

    if (pin == BSP_GPIO_PB1_PIN) {
        OSTaskQPost(             &AppClient_TaskTCB,
                    (void*)       0x01,
                    (OS_MSG_SIZE) APP_TCP_ID_BTN1,
                                  OS_OPT_POST_FIFO,
                                 &err);
    }

    OSIntExit();                                                /* Make the kernel aware of the end of the interrupt    */
}


/*
*********************************************************************************************************
*                                          AppClient_LinkStateStatus()
*
* Description : Handle a linkdown event
*
* Argument(s) : if_nbr      The interface number that has gone down
*
*               link_state  The state of the link
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/
static  void  AppClient_LinkStateStatus(NET_IF_NBR        if_nbr,
                                        NET_IF_LINK_STATE link_state)
{
    RTOS_ERR err;


    if(link_state == NET_IF_LINK_DOWN)
    {
        OSTaskQPost(             &AppClient_TaskTCB,
                    (void*)       0x01,
                    (OS_MSG_SIZE) APP_TCP_ID_LINKDOWN,
                                  OS_OPT_POST_LIFO,
                                 &err);
    }
}

