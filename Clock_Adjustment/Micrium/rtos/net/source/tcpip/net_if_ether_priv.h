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
*                                       NETWORK INTERFACE LAYER
*                                              ETHERNET
*
* File : net_if_ether_priv.h
*********************************************************************************************************
* Note(s) : (1) Implements code common to the following network interface layers :
*
*               (a) Ethernet as described in RFC # 894
*               (b) IEEE 802 as described in RFC #1042
*
*           (2) Ethernet implementation conforms to RFC #1122, Section 2.3.3, bullets (a) & (b), but
*               does NOT implement bullet (c) :
*
*               RFC #1122                  LINK LAYER                  October 1989
*
*               2.3.3  ETHERNET (RFC-894) and IEEE 802 (RFC-1042) ENCAPSULATION
*
*                      Every Internet host connected to a 10Mbps Ethernet cable :
*
*                      (a) MUST be able to send and receive packets using RFC-894 encapsulation;
*
*                      (b) SHOULD be able to receive RFC-1042 packets, intermixed with RFC-894 packets; and
*
*                      (c) MAY be able to send packets using RFC-1042 encapsulation.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_IF_ETHER_PRIV_H_
#define  _NET_IF_ETHER_PRIV_H_

#include  "../../include/net_cfg_net.h"

#ifdef  NET_IF_ETHER_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "net_if_802x_priv.h"

#include  <rtos/net/include/net_if_ether.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_IF_ETHER_BUF_RX_LEN_MIN     NET_IF_802x_BUF_RX_LEN_MIN
#define  NET_IF_ETHER_BUF_TX_LEN_MIN     NET_IF_802x_BUF_TX_LEN_MIN



/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void  NetIF_Ether_Init (RTOS_ERR                *p_err);


void  NetIF_Ether_Reg (CPU_CHAR              *p_name,
                       NET_IF_ETHER_HW_INFO  *p_ctrlr_info,
                       RTOS_ERR              *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_IF_ETHER_MODULE_EN */
#endif  /* _NET_IF_ETHER_PRIV_H_  */

