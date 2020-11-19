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
*                                                802x
*
* File : net_if_802x_priv.h
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

#ifndef  _NET_IF_802x_PRIV_H_
#define  _NET_IF_802x_PRIV_H_

#include  "../../include/net_cfg_net.h"

#ifdef   NET_IF_802x_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include "../../include/net_if.h"
#include "../../include/net_type.h"

#include  "net_def_priv.h"
#include  "net_buf_priv.h"
#include  "net_ctr_priv.h"

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  NETWORK INTERFACE / 802x DEFINES
*********************************************************************************************************
*/

#define  NET_IF_802x_ADDR_SIZE                        NET_IF_802x_HW_ADDR_LEN   /* Size of 48-bit Ether MAC addr (in octets).   */
#define  NET_IF_ETHER_ADDR_SIZE                       NET_IF_802x_ADDR_SIZE     /* Req'd for backwards-compatibility.   */


/*
*********************************************************************************************************
*                         802x SIZE & MAXIMUM TRANSMISSION UNIT (MTU) DEFINES
*
* Note(s) : (1) (a) RFC #894, Section 'Frame Format' & RFC #1042, Section 'Frame Format and MAC Level Issues :
*                   For IEEE 802.3' specify the following range on Ethernet & IEEE 802 frame sizes :
*
*                   (1) Minimum frame size =   64 octets
*                   (2) Maximum frame size = 1518 octets
*
*               (b) Since the 4-octet CRC trailer included in the specified minimum & maximum frame sizes is
*                   NOT necessarily included or handled by the network protocol suite, the minimum & maximum
*                   frame sizes for receive & transmit packets is adjusted by the CRC size.
*********************************************************************************************************
*/

#define  NET_IF_802x_FRAME_CRC_SIZE                        4
#define  NET_IF_ETHER_FRAME_CRC_SIZE                     NET_IF_802x_FRAME_CRC_SIZE /* Req'd for backwards-compatibility.   */

#define  NET_IF_802x_FRAME_MIN_CRC_SIZE                   64    /* See Note #1a1.                                           */
#define  NET_IF_802x_FRAME_MIN_SIZE                     (NET_IF_802x_FRAME_MIN_CRC_SIZE - NET_IF_802x_FRAME_CRC_SIZE)
#define  NET_IF_ETHER_FRAME_MIN_SIZE                     NET_IF_802x_FRAME_MIN_SIZE

#define  NET_IF_802x_FRAME_MAX_CRC_SIZE                 1518    /* See Note #1a2.                                           */

                                                                /* Must keep for backyard compatibility.                    */
#define  NET_IF_ETHER_FRAME_MAX_CRC_SIZE                 NET_IF_802x_FRAME_MAX_CRC_SIZE
#define  NET_IF_ETHER_FRAME_MAX_SIZE                    (NET_IF_802x_FRAME_MAX_CRC_SIZE - NET_IF_802x_FRAME_CRC_SIZE)

#define  NET_IF_802x_FRAME_MAX_SIZE                     (NET_IF_802x_FRAME_MAX_CRC_SIZE - NET_IF_802x_FRAME_CRC_SIZE)


#define  NET_IF_MTU_ETHER                               (NET_IF_802x_FRAME_MAX_CRC_SIZE - NET_IF_802x_FRAME_CRC_SIZE - NET_IF_HDR_SIZE_ETHER)
#define  NET_IF_MTU_IEEE_802                            (NET_IF_802x_FRAME_MAX_CRC_SIZE - NET_IF_802x_FRAME_CRC_SIZE - NET_IF_HDR_SIZE_IEEE_802)

#define  NET_IF_IEEE_802_SNAP_CODE_SIZE                  3u     /*  3-octet SNAP org code         (see Note #1).        */

#define  NET_IF_802x_BUF_SIZE_MIN                       (NET_IF_ETHER_FRAME_MIN_SIZE    + NET_BUF_DATA_SIZE_MIN      - NET_BUF_DATA_PROTOCOL_HDR_SIZE_MIN)


#define  NET_IF_802x_BUF_RX_LEN_MIN                      NET_IF_ETHER_FRAME_MAX_SIZE
#define  NET_IF_802x_BUF_TX_LEN_MIN                      NET_IF_ETHER_FRAME_MIN_SIZE


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                             NETWORK INTERFACE HEADER / FRAME DATA TYPES
*********************************************************************************************************
*/

                                                                /* ----------------- NET IF 802x HDR ------------------ */
typedef  struct  net_if_hdr_802x {
    CPU_INT08U  AddrDest[NET_IF_802x_ADDR_SIZE];                /*       802x dest  addr.                               */
    CPU_INT08U  AddrSrc[NET_IF_802x_ADDR_SIZE];                 /*       802x src   addr.                               */
    CPU_INT16U  FrameType_Len;                                  /* Demux 802x frame type vs. IEEE 802.3 frame len.      */
} NET_IF_HDR_802x;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                                        /* -------- INIT FNCTS -------- */
void         NetIF_802x_Init                     (RTOS_ERR               *p_err);

                                                                                        /* --------- RX FNCTS --------- */
void         NetIF_802x_Rx                       (NET_IF                 *p_if,
                                                  NET_BUF                *p_buf,
                                                  NET_CTR_IF_802x_STATS  *p_ctrs_stat,
                                                  NET_CTR_IF_802x_ERRS   *p_ctrs_err,
                                                  RTOS_ERR               *p_err);

                                                                                        /* --------- TX FNCTS --------- */
void         NetIF_802x_Tx                       (NET_IF                 *p_if,
                                                  NET_BUF                *p_buf,
                                                  NET_CTR_IF_802x_STATS  *p_ctrs_stat,
                                                  NET_CTR_IF_802x_ERRS   *p_ctrs_err,
                                                  RTOS_ERR               *p_err);

                                                                                        /* -------- MGMT FNCTS -------- */
void         NetIF_802x_AddrHW_Get               (NET_IF                 *p_if,
                                                  CPU_INT08U             *p_addr_hw,
                                                  CPU_INT08U             *p_addr_len,
                                                  RTOS_ERR               *p_err);

void         NetIF_802x_AddrHW_Set               (NET_IF                 *p_if,
                                                  CPU_INT08U             *p_addr_hw,
                                                  CPU_INT08U              addr_len,
                                                  RTOS_ERR               *p_err);

CPU_BOOLEAN  NetIF_802x_AddrHW_IsValid           (NET_IF                 *p_if,
                                                  CPU_INT08U             *p_addr_hw);

void         NetIF_802x_AddrMulticastAdd         (NET_IF                 *p_if,
                                                  CPU_INT08U             *p_addr_protocol,
                                                  CPU_INT08U              addr_protocol_len,
                                                  NET_PROTOCOL_TYPE       addr_protocol_type,
                                                  RTOS_ERR               *p_err);

void         NetIF_802x_AddrMulticastRemove      (NET_IF                 *p_if,
                                                  CPU_INT08U             *p_addr_protocol,
                                                  CPU_INT08U              addr_protocol_len,
                                                  NET_PROTOCOL_TYPE       addr_protocol_type,
                                                  RTOS_ERR               *p_err);

void         NetIF_802x_AddrMulticastProtocolToHW(NET_IF                 *p_if,
                                                  CPU_INT08U             *p_addr_protocol,
                                                  CPU_INT08U              addr_protocol_len,
                                                  NET_PROTOCOL_TYPE       addr_protocol_type,
                                                  CPU_INT08U             *p_addr_hw,
                                                  CPU_INT08U             *p_addr_hw_len,
                                                  RTOS_ERR               *p_err);

void         NetIF_802x_BufPoolCfgValidate       (NET_IF                 *p_if,
                                                  RTOS_ERR               *p_err);

void         NetIF_802x_MTU_Set                  (NET_IF                 *p_if,
                                                  NET_MTU                 mtu,
                                                  RTOS_ERR               *p_err);

CPU_INT16U   NetIF_802x_GetPktSizeHdr            (NET_IF                 *p_if);

CPU_INT16U   NetIF_802x_GetPktSizeMin            (NET_IF                 *p_if);

CPU_INT16U   NetIF_802x_GetPktSizeMax            (NET_IF                 *p_if);

CPU_BOOLEAN  NetIF_802x_PktSizeIsValid           (CPU_INT16U              size);

void         NetIF_802x_ISR_Handler              (NET_IF                 *p_if,
                                                  NET_DEV_ISR_TYPE        type,
                                                  RTOS_ERR               *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_IF_802x_MODULE_EN */
#endif  /* _NET_IF_802x_PRIV_H_  */

