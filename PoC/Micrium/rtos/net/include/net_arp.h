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
*                                          NETWORK ARP LAYER
*                                    (ADDRESS RESOLUTION PROTOCOL)
*
* File : net_arp.h
*********************************************************************************************************
* Note(s) : (1) Address Resolution Protocol ONLY required for network interfaces that require
*                     network-address-to-hardware-address bindings (see RFC #826 'Abstract').
*
*           (2) Supports Address Resolution Protocol as described in RFC #826 with the following
*               restrictions/constraints :
*
*               (a) ONLY supports the following hardware types :
*                     (1) 48-bit Ethernet
*
*               (b) ONLY supports the following protocol types :
*                     (1) 32-bit IP
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_ARP_H_
#define  _NET_ARP_H_

#include  <rtos/net/include/net_cfg_net.h>

#ifdef   NET_ARP_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_stat.h>
#include  <rtos/net/include/net_type.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

void               NetARP_CfgAddrFilterEn            (CPU_BOOLEAN          en);

void               NetARP_TxReqGratuitous            (NET_PROTOCOL_TYPE    protocol_type,
                                                      CPU_INT08U          *p_addr_protocol,
                                                      NET_CACHE_ADDR_LEN   addr_protocol_len,
                                                      RTOS_ERR            *p_err);

                                                                                    /* ---------- CFG FNCTS ----------- */
CPU_BOOLEAN         NetARP_CfgCacheTimeout           (CPU_INT16U           timeout_sec);

CPU_BOOLEAN         NetARP_CfgCacheTxQ_MaxTh         (NET_BUF_QTY          nbr_buf_max);

CPU_BOOLEAN         NetARP_CfgCacheAccessedTh        (CPU_INT16U           nbr_access);

CPU_BOOLEAN         NetARP_CfgPendReqTimeout         (CPU_INT08U           timeout_sec);

CPU_BOOLEAN         NetARP_CfgPendReqMaxRetries      (CPU_INT08U           max_nbr_retries);

CPU_BOOLEAN         NetARP_CfgRenewReqTimeout        (CPU_INT08U           timeout_sec);

CPU_BOOLEAN         NetARP_CfgRenewReqMaxRetries     (CPU_INT08U           max_nbr_retries);


                                                                                    /* --------- STATUS FNCTS --------- */

CPU_BOOLEAN         NetARP_IsAddrProtocolConflict    (NET_IF_NBR           if_nbr,
                                                      RTOS_ERR            *p_err);

NET_CACHE_ADDR_LEN  NetARP_CacheGetAddrHW            (NET_IF_NBR           if_nbr,
                                                      CPU_INT08U          *p_addr_hw,
                                                      NET_CACHE_ADDR_LEN   addr_hw_len_buf,
                                                      CPU_INT08U          *p_addr_protocol,
                                                      NET_CACHE_ADDR_LEN   addr_protocol_len,
                                                      RTOS_ERR            *p_err);

void                NetARP_CacheProbeAddrOnNet       (NET_PROTOCOL_TYPE    protocol_type,
                                                      CPU_INT08U          *p_addr_protocol_sender,
                                                      CPU_INT08U          *p_addr_protocol_target,
                                                      NET_CACHE_ADDR_LEN   addr_protocol_len,
                                                      RTOS_ERR            *p_err);

                                                                                    /* ---- ARP CACHE STATUS FNCTS ---- */
NET_STAT_POOL       NetARP_CachePoolStatGet          (void);

void                NetARP_CachePoolStatResetMaxUsed (void);


#ifdef __cplusplus
}
#endif

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_ARP_MODULE_EN */
#endif  /* _NET_ARP_H_       */

