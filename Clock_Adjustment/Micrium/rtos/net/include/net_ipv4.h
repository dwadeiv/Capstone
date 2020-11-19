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
*                                     NETWORK IP LAYER VERSION 4
*                                       (INTERNET PROTOCOL V4)
*
* File : net_ipv4.h
*********************************************************************************************************
* Note(s) : (1) Supports Internet Protocol as described in RFC #791, also known as IPv4, with the
*               following restrictions/constraints :
*
*               (a) ONLY supports a single default gateway                RFC #1122, Section 3.3.1
*                       per interface
*
*               (b) IP forwarding/routing  NOT currently supported        RFC #1122, Sections 3.3.1,
*                                                                                     3.3.4 & 3.3.5
*
*               (c) Transmit fragmentation NOT currently supported        RFC # 791, Section 2.3
*                                                                                     'Fragmentation &
*                                                                                        Reassembly'
*               (d) IP Security options    NOT           supported        RFC #1108
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_IPv4_H_
#define  _NET_IPv4_H_

#include  "net_cfg_net.h"

#ifdef   NET_IPv4_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_ip.h>
#include  <rtos/net/include/net_type.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/source/collections/slist_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  net_ipv4_addr_obj {
    NET_IPv4_ADDR          AddrHost;
    NET_IPv4_ADDR          AddrHostSubnetMask;
    NET_IPv4_ADDR          AddrHostSubnetMaskHost;
    NET_IPv4_ADDR          AddrHostSubnetNet;
    NET_IPv4_ADDR          AddrDfltGateway;
    NET_IP_ADDR_CFG_MODE   CfgMode;
    CPU_BOOLEAN            IsValid;
    SLIST_MEMBER           ListNode;
} NET_IPv4_ADDR_OBJ;


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

CPU_BOOLEAN    NetIPv4_CfgAddrAdd             (NET_IF_NBR                          if_nbr,
                                               NET_IPv4_ADDR                       addr_host,
                                               NET_IPv4_ADDR                       addr_subnet_mask,
                                               NET_IPv4_ADDR                       addr_dflt_gateway,
                                               RTOS_ERR                           *p_err);

CPU_BOOLEAN    NetIPv4_CfgAddrRemove          (NET_IF_NBR                          if_nbr,
                                               NET_IPv4_ADDR                       addr_host,
                                               RTOS_ERR                           *p_err);

CPU_BOOLEAN    NetIPv4_CfgAddrRemoveAll       (NET_IF_NBR                          if_nbr,
                                               RTOS_ERR                           *p_err);

#ifdef NET_IPv4_LINK_LOCAL_MODULE_EN
void           NetIPv4_AddrLinkLocalCfg       (NET_IF_NBR                          if_nbr,
                                               NET_IPv4_LINK_LOCAL_COMPLETE_HOOK   hook,
                                               RTOS_ERR                           *p_err);

void           NetIPv4_AddrLinkLocalCfgRemove (NET_IF_NBR                          if_nbr,
                                               RTOS_ERR                           *p_err);
#endif

CPU_BOOLEAN    NetIPv4_CfgFragReasmTimeout    (CPU_INT08U                          timeout_sec);

CPU_BOOLEAN    NetIPv4_GetAddrHost            (NET_IF_NBR                          if_nbr,
                                               NET_IPv4_ADDR                      *p_addr_tbl,
                                               NET_IP_ADDRS_QTY                   *p_addr_tbl_qty,
                                               RTOS_ERR                           *p_err);

NET_IPv4_ADDR  NetIPv4_GetAddrSrc             (NET_IPv4_ADDR                       addr_remote,
                                               RTOS_ERR                           *p_err);

NET_IPv4_ADDR  NetIPv4_GetAddrSubnetMask      (NET_IPv4_ADDR                       addr,
                                               RTOS_ERR                           *p_err);

NET_IPv4_ADDR  NetIPv4_GetAddrDfltGateway     (NET_IPv4_ADDR                       addr,
                                               RTOS_ERR                           *p_err);

CPU_BOOLEAN    NetIPv4_IsAddrClassA           (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrClassB           (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrClassC           (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrClassD           (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrThisHost         (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrLocalHost        (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrLocalLink        (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrBroadcast        (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrMulticast        (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrHost             (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrHostCfgd         (NET_IPv4_ADDR                       addr);

CPU_BOOLEAN    NetIPv4_IsAddrsCfgdOnIF        (NET_IF_NBR                          if_nbr,
                                               RTOS_ERR                           *p_err);

CPU_BOOLEAN    NetIPv4_IsValidAddrHost        (NET_IPv4_ADDR                       addr_host);

CPU_BOOLEAN    NetIPv4_IsValidAddrHostCfgd    (NET_IPv4_ADDR                       addr_host,
                                               NET_IPv4_ADDR                       addr_subnet_mask);

CPU_BOOLEAN    NetIPv4_IsValidAddrSubnetMask  (NET_IPv4_ADDR                       addr_subnet_mask);


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

#endif  /* NET_IPv4_MODULE_EN */
#endif  /* _NET_IPv4_H_       */

