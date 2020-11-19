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
*                                              APP TCP
*
* File : app_tcp.h
*********************************************************************************************************
*/

#ifndef APP_TCP_H_
#define APP_TCP_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  APP_CLIENT_TASK_PRIO                      16u
#define  APP_CLIENT_TASK_STK_SIZE                 1024u

#define  APP_SERVER_TASK_PRIO                      17u
#define  APP_SERVER_TASK_STK_SIZE                 1024u

#if APP_CLIENT_EN > 0
//#define EX_NET_CORE_IF_ETHER_IPv4_STATIC_ADDR    "10.1.10.8"
#else
//#define EX_NET_CORE_IF_ETHER_IPv4_STATIC_ADDR    "10.1.10.9"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef enum
{
    APP_TCP_ID_BTN0 = 0,
    APP_TCP_ID_BTN1,
    APP_TCP_ID_HEARTBEAT,
	APP_TCP_ID_LINKDOWN,
    MAX_APP_TCP_ID,
    APP_TCP_ID_ERR
} APP_TCP_ID_t;


typedef enum
{
    APP_SERVER_STATUS_NONE = 0,
    APP_SERVER_STATUS_INIT,
    APP_SERVER_STATUS_LISTENING,
    APP_SERVER_STATUS_CONNECTED,
    APP_SERVER_STATUS_FATAL,
    MAX_APP_SERVER_STATUS
} APP_SERVER_STATUS_t;

typedef enum
{
	APP_CLIENT_STATUS_NONE = 0,
    APP_CLIENT_STATUS_INIT,
    APP_CLIENT_STATUS_CONNECTING,
    APP_CLIENT_STATUS_CONNECTED,
    APP_CLIENT_STATUS_DISCONNECTED,
    MAX_APP_CLIENT_STATUS
} APP_CLIENT_STATUS_t;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#pragma pack(1)
typedef struct
{
    CPU_INT32U id;
    CPU_INT32U val;
} APP_TCP_MSG_t;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

APP_CLIENT_STATUS_t  AppClientGetStatus (void);

APP_SERVER_STATUS_t  AppServerGetStatus (void);

void                 AppClientInit      (NET_IF_NBR if_nbr);

void                 AppServerInit      (NET_IF_NBR if_nbr);


#endif
