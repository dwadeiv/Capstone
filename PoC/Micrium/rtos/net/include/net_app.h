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
*                         NETWORK APPLICATION PROGRAMMING INTERFACE (API) LAYER
*
* File : net_app.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_APP_H_
#define  _NET_APP_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/net_ip.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  net_app_sock_secure_cfg {
    CPU_CHAR                    *CommonName;
    NET_SOCK_SECURE_TRUST_FNCT   TrustCallback;
    CPU_CHAR                    *CertPtr;
    CPU_INT32U                   CertSize;
    NET_SOCK_SECURE_CERT_KEY_FMT CertFmt;
    CPU_BOOLEAN                  CertChain;
    CPU_CHAR                    *KeyPtr;
    CPU_INT32U                   KeySize;
} NET_APP_SOCK_SECURE_CFG;


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


                                                                /* -------------------- SOCK FNCTS -------------------- */
NET_SOCK_ID         NetApp_SockOpen                     (NET_SOCK_PROTOCOL_FAMILY   protocol_family,
                                                         NET_SOCK_TYPE              sock_type,
                                                         NET_SOCK_PROTOCOL          protocol,
                                                         CPU_INT16U                 retry_max,
                                                         CPU_INT32U                 time_dly_ms,
                                                         RTOS_ERR                  *p_err);

CPU_BOOLEAN         NetApp_SockClose                    (NET_SOCK_ID                sock_id,
                                                         CPU_INT32U                 timeout_ms,
                                                         RTOS_ERR                  *p_err);


CPU_BOOLEAN         NetApp_SockBind                     (NET_SOCK_ID                sock_id,
                                                         NET_SOCK_ADDR             *p_addr_local,
                                                         NET_SOCK_ADDR_LEN          addr_len,
                                                         CPU_INT16U                 retry_max,
                                                         CPU_INT32U                 time_dly_ms,
                                                         RTOS_ERR                  *p_err);

CPU_BOOLEAN         NetApp_SockConn                     (NET_SOCK_ID                sock_id,
                                                         NET_SOCK_ADDR             *p_addr_remote,
                                                         NET_SOCK_ADDR_LEN          addr_len,
                                                         CPU_INT16U                 retry_max,
                                                         CPU_INT32U                 timeout_ms,
                                                         CPU_INT32U                 time_dly_ms,
                                                         RTOS_ERR                  *p_err);


CPU_BOOLEAN         NetApp_SockListen                   (NET_SOCK_ID                sock_id,
                                                         NET_SOCK_Q_SIZE            sock_q_size,
                                                         RTOS_ERR                  *p_err);

NET_SOCK_ID         NetApp_SockAccept                   (NET_SOCK_ID                sock_id,
                                                         NET_SOCK_ADDR             *p_addr_remote,
                                                         NET_SOCK_ADDR_LEN         *p_addr_len,
                                                         CPU_INT16U                 retry_max,
                                                         CPU_INT32U                 timeout_ms,
                                                         CPU_INT32U                 time_dly_ms,
                                                         RTOS_ERR                  *p_err);


CPU_INT16U          NetApp_SockRx                       (NET_SOCK_ID               sock_id,
                                                         void                     *p_data_buf,
                                                         CPU_INT16U                data_buf_len,
                                                         CPU_INT16U                data_rx_th,
                                                         NET_SOCK_API_FLAGS        flags,
                                                         NET_SOCK_ADDR            *p_addr_remote,
                                                         NET_SOCK_ADDR_LEN        *p_addr_len,
                                                         CPU_INT16U                retry_max,
                                                         CPU_INT32U                timeout_ms,
                                                         CPU_INT32U                time_dly_ms,
                                                         RTOS_ERR                 *p_err);

CPU_INT16U          NetApp_SockTx                       (NET_SOCK_ID               sock_id,
                                                         void                     *p_data,
                                                         CPU_INT16U                data_len,
                                                         NET_SOCK_API_FLAGS        flags,
                                                         NET_SOCK_ADDR            *p_addr_remote,
                                                         NET_SOCK_ADDR_LEN         addr_len,
                                                         CPU_INT16U                retry_max,
                                                         CPU_INT32U                timeout_ms,
                                                         CPU_INT32U                time_dly_ms,
                                                         RTOS_ERR                 *p_err);


void                NetApp_SetSockAddr                  (NET_SOCK_ADDR            *p_sock_addr,
                                                         NET_SOCK_ADDR_FAMILY      addr_family,
                                                         NET_PORT_NBR              port_nbr,
                                                         CPU_INT08U               *p_addr,
                                                         NET_IP_ADDR_LEN           addr_len,
                                                         RTOS_ERR                 *p_err);

                                                                /* ---------- ADVANCED STREAM OPEN FUNCTION ----------- */
NET_IP_ADDR_FAMILY  NetApp_ClientStreamOpenByHostname   (NET_SOCK_ID              *p_sock_id,
                                                         CPU_CHAR                 *p_host_server,
                                                         NET_PORT_NBR              port_nbr,
                                                         NET_SOCK_ADDR            *p_sock_addr,
                                                         NET_APP_SOCK_SECURE_CFG  *p_secure_cfg,
                                                         CPU_INT32U                req_timeout_ms,
                                                         RTOS_ERR                 *p_err);

NET_IP_ADDR_FAMILY  NetApp_ClientDatagramOpenByHostname (NET_SOCK_ID              *p_sock_id,
                                                         CPU_CHAR                 *p_remote_host_name,
                                                         NET_PORT_NBR              remote_port_nbr,
                                                         NET_IP_ADDR_FAMILY        ip_family,
                                                         NET_SOCK_ADDR            *p_sock_addr,
                                                         CPU_BOOLEAN              *p_is_hostname,
                                                         RTOS_ERR                 *p_err);

NET_SOCK_ID         NetApp_ClientStreamOpen             (CPU_INT08U               *p_addr,
                                                         NET_IP_ADDR_FAMILY        addr_family,
                                                         NET_PORT_NBR              remote_port_nbr,
                                                         NET_SOCK_ADDR            *p_sock_addr,
                                                         NET_APP_SOCK_SECURE_CFG  *p_secure_cfg,
                                                         CPU_INT32U                req_timeout_ms,
                                                         RTOS_ERR                 *p_err);


NET_SOCK_ID         NetApp_ClientDatagramOpen           (CPU_INT08U               *p_addr,
                                                         NET_IP_ADDR_FAMILY        addr_family,
                                                         NET_PORT_NBR              remote_port_nbr,
                                                         NET_SOCK_ADDR            *p_sock_addr,
                                                         RTOS_ERR                 *p_err);


                                                                /* ------------------ TIME DLY FNCTS ------------------ */
void                NetApp_TimeDly_ms                   (CPU_INT32U                 time_dly_ms,
                                                         RTOS_ERR                  *p_err);


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

#endif  /* _NET_APP_H_ */

