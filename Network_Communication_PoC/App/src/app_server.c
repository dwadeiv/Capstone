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
*                                             APP_SERVER
*
* File : app_server.c
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

#include  <bsp_led.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/common.h>
#include  <rtos/kernel/include/os.h>

#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/toolchains.h>

#include  <rtos/net/include/net_if.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/net_app.h>
#include  <rtos/net/include/net_ipv4.h>
#include  <rtos/net/include/net_ipv6.h>

#include  "app_tcp.h"
#include  "em_device.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  APP_SERVER_RX_BUF_SIZE                    15u
#define  APP_SERVER_TCP_SERVER_PORT             10001u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Server Task Stack.                                 */
static  CPU_STK              AppServer_TaskStk[APP_SERVER_TASK_STK_SIZE];
                                                                /* Server Task TCB.                                   */
static  OS_TCB               AppServer_TaskTCB;

static  NET_IF_LINK_STATE    AppServer_LinkState;

static  APP_SERVER_STATUS_t  AppServer_Status = APP_SERVER_STATUS_NONE;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void         AppServer_Task             (void               *p_arg);

static  void         AppServer_Process          (APP_TCP_MSG_t      *p_msg);

static  void         AppServer_LinkStateStatus  (NET_IF_NBR          if_nbr,
                                                 NET_IF_LINK_STATE   link_state);

static  NET_SOCK_ID  AppServer_Init             (NET_SOCK_ADDR      *p_server_sock_addr_ip);

 CPU_INT32U           tcp_payload;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/
CPU_INT32U           AppGetMessageValue(){

	return   &tcp_payload;
}

/*
*********************************************************************************************************
*                                         AppServerInit()
*
* Description : Initialize the server
*
* Argument(s) : if_nbr  Network interface number
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  AppServerInit (NET_IF_NBR if_nbr)
{
    RTOS_ERR  err;


    AppServer_Status = APP_SERVER_STATUS_INIT;

    NetIF_LinkStateSubscribe ( if_nbr,                          /* Subscribe to the link state status                   */
                               AppServer_LinkStateStatus,
                              &err);

    OSTaskCreate(&AppServer_TaskTCB,                            /* Create the Client Task.                            */
                 "App Server Task",
                  AppServer_Task,
                  DEF_NULL,
                  APP_SERVER_TASK_PRIO,
                 &AppServer_TaskStk[0],
                 (APP_SERVER_TASK_STK_SIZE / 10u),
                  APP_SERVER_TASK_STK_SIZE,
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
*                                         AppServerGetStatus()
*
* Description : Returns the status of the server
*
* Argument(s) : None.
*
* Return(s)   : Server status.
*
* Note(s)     : None.
*********************************************************************************************************
*/

APP_SERVER_STATUS_t  AppServerGetStatus (void)
{
    return AppServer_Status;
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
*                                          AppServer_Task()
*
* Description : The main server task
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  AppServer_Task (void  *p_arg)
{
    NET_SOCK_ID          sock_listen;
    NET_SOCK_ID          sock_child;
    NET_SOCK_ADDR_IPv4   server_sock_addr_ip;
    NET_SOCK_ADDR_IPv4   client_sock_addr_ip;
    NET_SOCK_ADDR_LEN    client_sock_addr_ip_size;
    CPU_CHAR             rx_buf[APP_SERVER_RX_BUF_SIZE];
    CPU_BOOLEAN          fault_err;
    RTOS_ERR             err;


    fault_err = DEF_NO;

    while(DEF_TRUE)
    {                                                           /* Initialize the server                                */
        sock_listen = AppServer_Init((NET_SOCK_ADDR*) &server_sock_addr_ip);

        fault_err = DEF_NO;

        while (fault_err == DEF_NO) {
            client_sock_addr_ip_size = NET_SOCK_ADDR_IPv4_SIZE;

            AppServer_Status = APP_SERVER_STATUS_LISTENING;

                                                                /* ---------- ACCEPT NEW INCOMING CONNECTION ---------- */
            sock_child = NetSock_Accept(                  sock_listen,
                                        (NET_SOCK_ADDR *)&client_sock_addr_ip,
                                                         &client_sock_addr_ip_size,
                                                         &err);
            AppServer_LinkState = NET_IF_LINK_UP;

            switch (err.Code) {
                case RTOS_ERR_NONE:
                     do {
                         AppServer_Status = APP_SERVER_STATUS_CONNECTED;

                                                                /* ----- WAIT UNTIL RECEIVING DATA FROM A CLIENT ------ */
                         client_sock_addr_ip_size = sizeof(client_sock_addr_ip);
                         NetSock_RxDataFrom(                  sock_child,
                                                              rx_buf,
                                                              APP_SERVER_RX_BUF_SIZE,
                                                              NET_SOCK_FLAG_NONE,
                                            (NET_SOCK_ADDR *)&client_sock_addr_ip,
                                                             &client_sock_addr_ip_size,
                                                              DEF_NULL,
                                                              DEF_NULL,
                                                              DEF_NULL,
                                                             &err);
                         switch (err.Code) {
                             case RTOS_ERR_NONE:                /* Process the received data                            */
                                  AppServer_Process((APP_TCP_MSG_t*) rx_buf);
                                  break;

                             case RTOS_ERR_WOULD_BLOCK:         /* Transitory Errors                                    */
                             case RTOS_ERR_TIMEOUT:
                                  OSTimeDlyHMSM( 0, 0, 0, 5,
                                                 OS_OPT_TIME_DLY,
                                                &err);
                                  break;
                                                                /* Conn closed by peer.                                 */
                             case RTOS_ERR_NET_CONN_CLOSED_FAULT:
                             case RTOS_ERR_NET_CONN_CLOSE_RX:
                                  fault_err = DEF_YES;
                                  break;

                             default:
                                 fault_err = DEF_YES;
                                 break;
                         }
                                                                /* If the link has gone down, break out and reinit      */
                         if(AppServer_LinkState == NET_IF_LINK_DOWN) {
                             break;
                         }

                     } while (fault_err == DEF_NO);


                                                                /* ---------------- CLOSE CHILD SOCKET ---------------- */
                     NetSock_Close(sock_child, &err);
                     if (err.Code != RTOS_ERR_NONE) {
                         fault_err = DEF_YES;
                     }
                     break;


                case RTOS_ERR_WOULD_BLOCK:                      /* Transitory Errors                                    */
                case RTOS_ERR_TIMEOUT:
                case RTOS_ERR_POOL_EMPTY:
                     OSTimeDlyHMSM(0, 0, 0, 5, OS_OPT_TIME_DLY, &err);
                     break;


                default:
                     fault_err = DEF_YES;
                     break;
            }
        }
                                                                /* ------------- FATAL FAULT SOCKET ERROR ------------- */
        NetSock_Close(sock_listen, &err);                       /* This function should be reached only when a fatal ...*/
                                                                /* fault error has occurred.                            */
        AppServer_Status = APP_SERVER_STATUS_FATAL;
    }
}


/*
*********************************************************************************************************
*                                          AppServer_Init()
*
* Description : Initialize the server
*
* Argument(s) : p_server_sock_addr_ip   The server socket information structure
*
* Return(s)   : The socket id
*
* Notes       : None.
*********************************************************************************************************
*/

static  NET_SOCK_ID  AppServer_Init (NET_SOCK_ADDR* p_server_sock_addr_ip)
{
    NET_SOCK_ID  sock_listen;
    CPU_INT32U   addr_any = NET_IPv4_ADDR_ANY;
    RTOS_ERR     err;

                                                                /* ----------------- OPEN IPV4 SOCKET ----------------- */
    sock_listen = NetSock_Open( NET_SOCK_PROTOCOL_FAMILY_IP_V4,
                                NET_SOCK_TYPE_STREAM,
                                NET_SOCK_PROTOCOL_TCP,
                               &err);
    if (err.Code != RTOS_ERR_NONE) {
        return 0;
    }
                                                                /* ------------ CONFIGURE SOCKET'S ADDRESS ------------ */
    NetApp_SetSockAddr( p_server_sock_addr_ip,
                        NET_SOCK_ADDR_FAMILY_IP_V4,
                        APP_SERVER_TCP_SERVER_PORT,
                       (CPU_INT08U    *)&addr_any,
                        NET_IPv4_ADDR_SIZE,
                       &err);
    if (err.Code != RTOS_ERR_NONE) {
        NetSock_Close(sock_listen, &err);
        return 0;
    }
                                                                /* ------------------- BIND SOCKET -------------------- */
    NetSock_Bind( sock_listen,
                  p_server_sock_addr_ip,
                  NET_SOCK_ADDR_SIZE,
                 &err);
    if (err.Code != RTOS_ERR_NONE) {
        NetSock_Close(sock_listen, &err);
        return 0;
    }
                                                                /* ------------------ LISTEN SOCKET ------------------- */
    NetSock_Listen( sock_listen,
                    1,
                   &err);
    if (err.Code != RTOS_ERR_NONE) {
        NetSock_Close(sock_listen, &err);
        return 0;
    }

    return sock_listen;
}


/*
 *********************************************************************************************************
 *                                          AppServer_Process()
 *
 * Description : Process a message received from the server
 *
 * Argument(s) : p_msg   The app message received from the client
 *
 * Return(s)   : None.
 *
 * Notes       : None.
 *********************************************************************************************************
 */

static  void  AppServer_Process (APP_TCP_MSG_t *p_msg)
{
    if(p_msg->id == 0)
    {
        BSP_LED_Toggle(0);
        tcp_payload=p_msg->val;
    }

    if(p_msg->id == 1)
    {
        BSP_LED_Toggle(1);
        tcp_payload= p_msg->val;
    }
}


/*
*********************************************************************************************************
*                                          AppServer_LinkStateStatus()
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
static  void  AppServer_LinkStateStatus (NET_IF_NBR        if_nbr,
                                         NET_IF_LINK_STATE link_state)
{
    if(link_state == NET_IF_LINK_DOWN)
    {
        AppServer_LinkState = NET_IF_LINK_DOWN;
    }
}

