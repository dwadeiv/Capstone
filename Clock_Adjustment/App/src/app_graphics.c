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
*                                             APP GRAPHICS
*
* File : app_graphics.c
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
#include  <inttypes.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/common.h>
#include  <rtos/kernel/include/os.h>

#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/include/toolchains.h>

#include  <rtos/net/include/net_if.h>
#include  <rtos/net/include/net_ipv4.h>
#include  <rtos/net/include/net_ipv6.h>

#include  "app_tcp.h"
#include  "app_graphics.h"
#include  "display.h"
#include  "dmd.h"
#include  "glib.h"
#include  "em_device.h"
#include  "micrium_logo.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  APP_GRAPHICS_GLIB_FONT_WIDTH       (glibContext.font.fontWidth + glibContext.font.charSpacing)
#define  APP_GRAPHICS_GLIB_FONT_HEIGHT      (glibContext.font.fontHeight)
#define  APP_GRAPHICS_CENTER_X              (glibContext.pDisplayGeometry->xSize / 2)
#define  APP_GRAPHICS_CENTER_Y              (glibContext.pDisplayGeometry->ySize / 2)
#define  APP_GRAPHICS_MAX_X                 (glibContext.pDisplayGeometry->xSize - 1)
#define  APP_GRAPHICS_MAX_Y                 (glibContext.pDisplayGeometry->ySize - 1)

#define  APP_GRAPHICS_MIN_X                  0u
#define  APP_GRAPHICS_MIN_Y                  0u
#define  APP_GRAPHICS_MAX_STR_LEN           48u


typedef enum
{
    APP_GRAPHICS_SCREEN_START = 0,
    APP_GRAPHICS_SCREEN_SETUP,
    APP_GRAPHICS_SCREEN_CLIENT,
    APP_GRAPHICS_SCREEN_SERVER,
} APP_GRAPHICS_SCREEN_t;

/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Graphics Task Stack.                                 */
static  CPU_STK         AppGraphics_TaskStk[APP_GRAPHICS_TASK_STK_SIZE];
                                                                /* Graphics Task TCB.                                   */
static  OS_TCB          AppGraphics_TaskTCB;

static  GLIB_Context_t  glibContext;

static  CPU_CHAR*       AppGraphics_ClientStr[MAX_APP_CLIENT_STATUS] = {"None",
																		"Initializing",
                                                                        "Connecting",
                                                                        "Connected",
                                                                        "Disconnected"};

static  CPU_CHAR*       AppGraphics_ServerStr[MAX_APP_CLIENT_STATUS] = {"None",
																		"Initializing",
                                                                        "Listening",
                                                                        "Connected",
                                                                        "Fatal"};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  AppGraphics_Task              (void  *p_arg);
static  void  AppGraphics_Init              (GLIB_Context_t *pContext);

static  void  AppGraphics_DrawLinkStatus    (void);
static  void  AppGraphics_DrawIPv4Status    (void);
static  void  AppGraphics_DrawTxRxStatus    (void);

static  void  AppGraphics_DrawStatus        (void);

static  void  AppGraphics_StartScreen       (GLIB_Context_t *pContext);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         AppGraphicsInit()
*
* Description : Initialize the graphics task
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  AppGraphicsInit (void)
{
    RTOS_ERR  err;


    OSTaskCreate(&AppGraphics_TaskTCB,                          /* Create the Graphics Task.                            */
                 "Graphics Task",
                  AppGraphics_Task,
                  DEF_NULL,
                  APP_GRAPHICS_TASK_PRIO,
                 &AppGraphics_TaskStk[0],
                 (APP_GRAPHICS_TASK_STK_SIZE / 10u),
                  APP_GRAPHICS_TASK_STK_SIZE,
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
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          AppGraphics_Task()
*
* Description : The main graphics task
*
* Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static  void  AppGraphics_Task (void  *p_arg)
{
    RTOS_ERR              err;


    PP_UNUSED_PARAM(p_arg);                                     /* Prevent compiler warning.                            */

    AppGraphics_Init(&glibContext);                             /* Initialize the screen                                */

    while (DEF_ON) {

        GLIB_clear(&glibContext);                               /* Clear the screen                                     */

        AppGraphics_StartScreen(&glibContext);

        AppGraphics_DrawLinkStatus();
        AppGraphics_DrawIPv4Status();
        AppGraphics_DrawTxRxStatus();

        AppGraphics_DrawStatus();

        DMD_updateDisplay();                                    /* Update the screen                                    */

        OSTimeDlyHMSM( 0, 0, 0, 100,
                       OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}


/*
*********************************************************************************************************
*                                          AppGraphics_Init()
*
* Description : Initalize the graphics
*
* Argument(s) : pContext:   Pointer to the glib context structure
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

void AppGraphics_Init(GLIB_Context_t *pContext)
{
    EMSTATUS status;


    status = DMD_init(0);                                   /* Initialize the DMD module for the DISPLAY device driver  */
    if (DMD_OK != status) {
      while (1) {}
    }

    status = GLIB_contextInit(&glibContext);
    if (GLIB_OK != status) {
      while (1) {}
    }
}


/*
*********************************************************************************************************
*                                          AppGraphics_StartScreen()
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

void AppGraphics_StartScreen(GLIB_Context_t *pContext)
{
	char str[12] = {0};
    GLIB_Rectangle_t rectBackground = { 0, 0, APP_GRAPHICS_MAX_X, APP_GRAPHICS_MAX_Y };


    pContext->backgroundColor = White;
    pContext->foregroundColor = White;
    GLIB_drawRectFilled(pContext, &rectBackground);

    pContext->foregroundColor = Black;
    GLIB_drawBitmap(pContext,
                    ((APP_GRAPHICS_MAX_X + 1 - MICRIUM_LOGO_BITMAP_WIDTH) / 2),
                    5,
                    MICRIUM_LOGO_BITMAP_WIDTH,
                    MICRIUM_LOGO_BITMAP_HEIGHT,
                    micrium_logoBitmap);

    if (AppClientGetStatus() != APP_CLIENT_STATUS_NONE)
    {
    	strcpy(str, "TCP Client");
    }

    else if(AppServerGetStatus() != APP_SERVER_STATUS_NONE)
    {
    	strcpy(str, "TCP Server");
    }

    GLIB_setFont(pContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);
    GLIB_drawString(pContext,
                    str,
                    strlen(str),
                    APP_GRAPHICS_CENTER_X - ((APP_GRAPHICS_GLIB_FONT_WIDTH * strlen(str)) / 2),
                    30,
                    0);
}


/*
*********************************************************************************************************
*                                       AppGraphics_DrawLinkStatus()
*
* Description : Draw the ethernet link status
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static void AppGraphics_DrawLinkStatus(void)
{
    RTOS_ERR err;
    NET_IF_LINK_STATE link;
    const char * msg;


    link = NetIF_LinkStateGet(1, &err);

    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
    {
        msg = "Link error";
    }
    else if (link == NET_IF_LINK_UP)
    {
        msg = "Link up";
    }
    else
    {
        msg = "Link down";
    }
    GLIB_drawString(&glibContext, msg, strlen(msg), 0, 50, false);
}


/*
*********************************************************************************************************
*                                     AppGraphics_DrawIPv4Status()
*
* Description : Draw the IPv4 status
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static void AppGraphics_DrawIPv4Status(void)
{
    RTOS_ERR          err;
    NET_IPv4_ADDR     addrTable[4];
    NET_IP_ADDRS_QTY  addrTableSize = 4;
    CPU_BOOLEAN       ok;
    CPU_CHAR          addrString[NET_ASCII_LEN_MAX_ADDR_IPv4];


    ok = NetIPv4_GetAddrHost(1, addrTable, &addrTableSize, &err);

    if (!ok) {
        return;
    }

    if (addrTableSize > 0)
    {
        NetASCII_IPv4_to_Str(addrTable[0], addrString, DEF_NO, &err);
        APP_RTOS_ASSERT_CRITICAL((err.Code == RTOS_ERR_NONE), ;);
        GLIB_drawString(&glibContext, addrString, strlen(addrString), 0, 60, false);
    }
}

/*
*********************************************************************************************************
*                                       AppGraphics_DrawStatus()
*
* Description : Draw the client or server status
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static void AppGraphics_DrawStatus(void)
{
	APP_CLIENT_STATUS_t client_status;
    APP_SERVER_STATUS_t server_status;
    const char * server_msg = "Server Status:";
    const char * client_msg = "Client Status:";


    server_status = AppServerGetStatus();
    client_status = AppClientGetStatus();

    if(server_status != APP_SERVER_STATUS_NONE)
    {
		GLIB_drawString(&glibContext,
						 server_msg,
						 strlen(server_msg),
						 0,
						 80,
						 false);

		GLIB_drawString(&glibContext,
						 AppGraphics_ServerStr[server_status],
						 strlen(AppGraphics_ServerStr[server_status]),
						 0,
						 90,
						 false);
    }

    else if (client_status != APP_CLIENT_STATUS_NONE)
    {
		GLIB_drawString(&glibContext,
						 client_msg,
						 strlen(client_msg),
						 0,
						 80,
						 false);

		GLIB_drawString(&glibContext,
						 AppGraphics_ClientStr[client_status],
						 strlen(AppGraphics_ClientStr[client_status]),
						 0,
						 90,
						 false);
    }
}


/*
*********************************************************************************************************
*                                       AppGraphics_DrawTxRxStatus()
*
* Description : Draw the TX/RX status
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Notes       : None.
*********************************************************************************************************
*/

static void AppGraphics_DrawTxRxStatus(void)
{
    char str[20] = {0};
    static uint32_t framesTx = 0;
    static uint32_t framesRx = 0;

    static uint32_t timestamp = 21;


    framesTx += ETH->FRAMESTXED64;
    framesTx += ETH->FRAMESTXED65;
    framesTx += ETH->FRAMESTXED128;
    framesTx += ETH->FRAMESTXED256;
    framesTx += ETH->FRAMESTXED512;
    framesTx += ETH->FRAMESTXED1024;
    framesTx += ETH->FRAMESTXED1519;

    framesRx += ETH->FRAMESRXED64;
    framesRx += ETH->FRAMESRXED65;
    framesRx += ETH->FRAMESRXED128;
    framesRx += ETH->FRAMESRXED256;
    framesRx += ETH->FRAMESRXED512;
    framesRx += ETH->FRAMESRXED1024;
    framesRx += ETH->FRAMESRXED1519;

    snprintf(str, sizeof(str), "Timestamp: %"PRIu32, timestamp);
    GLIB_drawString(&glibContext, str, strlen(str), 0, 100, false);
    snprintf(str, sizeof(str), "Frames TX: %"PRIu32, framesTx);
    GLIB_drawString(&glibContext, str, strlen(str), 0, 110, false);
    snprintf(str, sizeof(str), "Frames RX: %"PRIu32, framesRx);
    GLIB_drawString(&glibContext, str, strlen(str), 0, 120, false);
}

