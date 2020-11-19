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
*                                           DNS CLIENT MODULE
*
* File : dns_client.h
**********************************************************************************************************
* Note(s) : (1) This module implements a basic DNS client based on RFC #1035. It provides the
*               mechanism used to retrieve an IP address from a given host name.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _DNS_CLIENT_H_
#define  _DNS_CLIENT_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/net_ip.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/rtos_types.h>
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
*                                         DEFAULT CFG DEFINES
*********************************************************************************************************
*/

#define  DNS_CLIENT_CFG_SERVER_ADDR_DFLT                        "8.8.8.8"
#define  DNS_CLIENT_CFG_HOSTNAME_LEN_MAX_DFLT                   255u
#define  DNS_CLIENT_CFG_CACHE_ENTRIES_NBR_MAX_DFLT              LIB_MEM_BLK_QTY_UNLIMITED
#define  DNS_CLIENT_CFG_ADDR_IPv4_PER_HOST_NBR_MAX_DFLT         LIB_MEM_BLK_QTY_UNLIMITED
#define  DNS_CLIENT_CFG_ADDR_IPv6_PER_HOST_NBR_MAX_DFLT         LIB_MEM_BLK_QTY_UNLIMITED
#define  DNS_CLIENT_CFG_OP_DLY_MS_DFLT                          100u
#define  DNS_CLIENT_CFG_REQ_RETRY_NBR_MAX_DFLT                  3u
#define  DNS_CLIENT_CFG_REQ_RETRY_TIMEOUT_MS_DFLT               5000u


#define  DNSc_DFLT_CFG  { \
                            .HostNameLenMax           = DNS_CLIENT_CFG_HOSTNAME_LEN_MAX_DFLT, \
                            .CacheEntriesMaxNbr       = DNS_CLIENT_CFG_CACHE_ENTRIES_NBR_MAX_DFLT, \
                            .AddrIPv4MaxPerHost       = DNS_CLIENT_CFG_ADDR_IPv4_PER_HOST_NBR_MAX_DFLT, \
                            .AddrIPv6MaxPerHost       = DNS_CLIENT_CFG_ADDR_IPv6_PER_HOST_NBR_MAX_DFLT, \
                            .OpDly_ms                 = DNS_CLIENT_CFG_OP_DLY_MS_DFLT, \
                            .ReqRetryNbrMax           = DNS_CLIENT_CFG_REQ_RETRY_NBR_MAX_DFLT, \
                            .ReqRetryTimeout_ms       = DNS_CLIENT_CFG_REQ_RETRY_TIMEOUT_MS_DFLT, \
                            .DfltServerAddrFallbackEn = DEF_YES \
                        }


/*
*********************************************************************************************************
*                                            FLAG DEFINES
*********************************************************************************************************
*/

typedef  CPU_INT08U   DNSc_FLAGS;

#define  DNSc_FLAG_NONE              DEF_BIT_NONE
#define  DNSc_FLAG_NO_BLOCK          DEF_BIT_00
#define  DNSc_FLAG_FORCE_CACHE       DEF_BIT_01
#define  DNSc_FLAG_FORCE_RENEW       DEF_BIT_02
#define  DNSc_FLAG_FORCE_RESOLUTION  DEF_BIT_03
#define  DNSc_FLAG_IPv4_ONLY         DEF_BIT_04
#define  DNSc_FLAG_IPv6_ONLY         DEF_BIT_05


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         DNS CFG DATA TYPES
*********************************************************************************************************
*/

typedef  RTOS_TASK_CFG  DNSc_CFG_TASK;

typedef  struct DNSc_cfg {
    CPU_INT16U      HostNameLenMax;

    CPU_SIZE_T      CacheEntriesMaxNbr;

    CPU_SIZE_T      AddrIPv4MaxPerHost;
    CPU_SIZE_T      AddrIPv6MaxPerHost;

    CPU_INT08U      ReqRetryNbrMax;
    CPU_INT16U      ReqRetryTimeout_ms;

    CPU_INT08U      OpDly_ms;

    CPU_BOOLEAN     DfltServerAddrFallbackEn;
} DNSc_CFG;


/*
*********************************************************************************************************
*                                         DNS REQ CFG DATA TYPE
*********************************************************************************************************
*/

typedef  struct  DNSc_req_cfg {
    NET_IP_ADDR_OBJ  *ServerAddrPtr;
    NET_PORT_NBR      ServerPort;
    CPU_INT16U        OpDly_ms;
    CPU_INT16U        ReqTimeout_ms;
    CPU_INT08U        ReqRetry;
} DNSc_REQ_CFG;


/*
*********************************************************************************************************
*                                         DNS STATUS DATA TYPE
*********************************************************************************************************
*/

typedef  enum  DNSc_status {
    DNSc_STATUS_PENDING,
    DNSc_STATUS_RESOLVED,
    DNSc_STATUS_FAILED,
    DNSc_STATUS_UNKNOWN,
    DNSc_STATUS_NONE,
} DNSc_STATUS;


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

void         DNSc_CfgServerByStr  (CPU_CHAR          *p_server,
                                   RTOS_ERR          *p_err);

void         DNSc_CfgServerByAddr (NET_IP_ADDR_OBJ   *p_addr,
                                   RTOS_ERR          *p_err);

void         DNSc_GetServerByStr  (CPU_CHAR          *p_str,
                                   CPU_INT08U         str_len_max,
                                   RTOS_ERR          *p_err);

void         DNSc_GetServerByAddr (NET_IP_ADDR_OBJ   *p_addr,
                                   RTOS_ERR          *p_err);

DNSc_STATUS  DNSc_GetHost         (CPU_CHAR          *p_host_name,
                                   NET_IP_ADDR_OBJ   *p_addr_obj,
                                   CPU_INT08U        *p_addr_nbr,
                                   DNSc_FLAGS         flags,
                                   DNSc_REQ_CFG      *p_cfg,
                                   RTOS_ERR          *p_err);

DNSc_STATUS  DNSc_GetHostAddrs    (CPU_CHAR          *p_host_name,
                                   NET_HOST_IP_ADDR **p_addrs,
                                   CPU_INT08U        *p_addr_nbr,
                                   DNSc_FLAGS         flags,
                                   DNSc_REQ_CFG      *p_cfg,
                                   RTOS_ERR          *p_err);

void         DNSc_FreeHostAddrs   (NET_HOST_IP_ADDR  *p_addr);

void         DNSc_CacheClrAll     (RTOS_ERR          *p_err);

void         DNSc_CacheClrHost    (CPU_CHAR          *p_host_name,
                                   RTOS_ERR          *p_err);

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

#endif  /* _DNS_CLIENT_H_ */
