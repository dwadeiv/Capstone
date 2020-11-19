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
*                                     DNS CLIENT REQUEST MODULE
*
* File : dns_client_req_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _DNS_CLIENT_REQ_PRIV_H_
#define  _DNS_CLIENT_REQ_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "dns_client_priv.h"

#include  "../../include/dns_client.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  enum c_req_type {
    DNSc_REQ_TYPE_IPv4,
    DNSc_REQ_TYPE_IPv6,
} DNSc_REQ_TYPE;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void         DNScReq_ServerSet (DNSc_SERVER_ADDR_TYPE   addr_type,
                                NET_IP_ADDR_OBJ        *p_addr);

void         DNScReq_ServerGet (NET_IP_ADDR_OBJ        *p_addr,
                                RTOS_ERR               *p_err);

NET_SOCK_ID  DNScReq_Init      (NET_IP_ADDR_OBJ        *p_server_addr,
                                NET_PORT_NBR            server_port,
                                RTOS_ERR               *p_err);

NET_IF_NBR   DNSc_ReqIF_Sel    (NET_IF_NBR              if_nbr_last,
                                NET_SOCK_ID             sock_id,
                                RTOS_ERR               *p_err);

void         DNSc_ReqClose     (NET_SOCK_ID             sock_id);

CPU_INT16U   DNScReq_TxReq     (CPU_CHAR               *p_host_name,
                                NET_SOCK_ID             sock_id,
                                CPU_INT16U              query_id,
                                DNSc_REQ_TYPE           req_type,
                                RTOS_ERR               *p_err);

DNSc_STATUS  DNScReq_RxResp    (DNSc_HOST_OBJ          *p_host,
                                NET_SOCK_ID             sock_id,
                                CPU_INT16U              query_id,
                                CPU_INT16U              timeout_ms,
                                RTOS_ERR               *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _DNS_CLIENT_REQ_PRIV_H_ */
