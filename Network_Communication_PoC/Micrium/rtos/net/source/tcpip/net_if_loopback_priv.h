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
*                                  NETWORK LOOPBACK INTERFACE LAYER
*
* File : net_if_loopback_priv.h
*********************************************************************************************************
* Note(s) : (1) Supports internal loopback communication.
*
*               (a) Internal loopback interface is NOT linked to, associated with, or handled by
*                   any physical network device(s) & therefore has NO physical protocol overhead.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_IF_LOOPBACK_PRIV_H_
#define  _NET_IF_LOOPBACK_PRIV_H_

#include  "../../include/net_cfg_net.h"

#ifdef   NET_IF_LOOPBACK_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_if_loopback.h>
#include  <rtos/net/source/tcpip/net_if_priv.h>
#include  <rtos/net/source/tcpip/net_if_802x_priv.h>
#include  <rtos/net/source/tcpip/net_type_priv.h>
#include  <rtos/net/source/tcpip/net_stat_priv.h>
#include  <rtos/net/source/tcpip/net_buf_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_IF_LOOPBACK_BUF_RX_LEN_MIN     NET_IF_ETHER_FRAME_MIN_SIZE
#define  NET_IF_LOOPBACK_BUF_TX_LEN_MIN     NET_IF_802x_BUF_TX_LEN_MIN


/*
*********************************************************************************************************
*                                  NETWORK INTERFACE HEADER DEFINES
*
* Note(s) : (1) The following network interface value MUST be pre-#define'd in 'net_def.h' PRIOR to
*               'net_cfg.h' so that the developer can configure the network interface for the correct
*               network interface layer values (see 'net_def.h  NETWORK INTERFACE LAYER DEFINES' &
*               'net_cfg_net.h  NETWORK INTERFACE LAYER CONFIGURATION  Note #4') :
*
*               (a) NET_IF_HDR_SIZE_LOOPBACK               0
*********************************************************************************************************
*/

#if 0
#define  NET_IF_HDR_SIZE_LOOPBACK                          0    /* See Note #1a.                                        */
#endif

#define  NET_IF_HDR_SIZE_LOOPBACK_MIN                    NET_IF_HDR_SIZE_LOOPBACK
#define  NET_IF_HDR_SIZE_LOOPBACK_MAX                    NET_IF_HDR_SIZE_LOOPBACK


/*
*********************************************************************************************************
*              NETWORK LOOPBACK INTERFACE SIZE & MAXIMUM TRANSMISSION UNIT (MTU) DEFINES
*
* Note(s) : (1) The loopback interface is NOT linked to, associated with, or handled by any physical
*               network device(s) & therefore has NO physical protocol overhead.
*
*               See also 'net_if_loopback.h  Note #1a'.
*********************************************************************************************************
*/
                                                                /* See Note #1.                                         */
#define  NET_IF_MTU_LOOPBACK                             NET_MTU_MAX_VAL

#define  NET_IF_LOOPBACK_BUF_SIZE_MIN                   (NET_IF_LOOPBACK_SIZE_MIN + NET_BUF_DATA_SIZE_MIN - NET_BUF_DATA_PROTOCOL_HDR_SIZE_MIN)


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  const  NET_IF_CFG_LOOPBACK   NetIF_Cfg_Loopback;        /*    Net loopback IF cfg.                              */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void          NetIF_Loopback_Init (MEM_SEG   *p_mem_seg,
                                   RTOS_ERR  *p_err);


                                                                /* --------------------- RX FNCTS --------------------- */
NET_BUF_SIZE  NetIF_Loopback_Rx   (NET_IF    *p_if,
                                   RTOS_ERR  *p_err);

                                                                /* --------------------- TX FNCTS --------------------- */
NET_BUF_SIZE  NetIF_Loopback_Tx   (NET_IF    *p_if,
                                   NET_BUF   *p_buf_tx,
                                   RTOS_ERR  *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_IF_LOOPBACK_MODULE_EN */
#endif  /* _NET_IF_LOOPBACK_PRIV_H_  */

