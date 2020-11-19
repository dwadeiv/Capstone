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
*                                           FTP CLIENT MODULE
*
* File : ftp_client.h
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _FTP_CLIENT_H_
#define  _FTP_CLIENT_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>

#include  <rtos/cpu/include/cpu.h>

#include  <rtos/net/include/net_ip.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/net_cfg_net.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  FTPc_CTRL_NET_BUF_SIZE                         1024    /* Ctrl buffer size.                                    */
#define  FTPc_DTP_NET_BUF_SIZE                          1460    /* Dtp buffer size.                                     */

#define  FTPc_PORT_DFLT                                   21


/*
*********************************************************************************************************
*                                       FTP SECURE CFG DATA TYPE
*********************************************************************************************************
*/

typedef  struct  ftpc_secure_cfg {
    CPU_CHAR                    *CommonName;
    NET_SOCK_SECURE_TRUST_FNCT   TrustCallback;
} FTPc_SECURE_CFG;


/*
*********************************************************************************************************
*                                    TFTPc CONFIGURATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  ftpc_cfg {
    CPU_INT32U  CtrlConnMaxTimout_ms;
    CPU_INT32U  CtrlRxMaxTimout_ms;
    CPU_INT32U  CtrlTxMaxTimout_ms;

    CPU_INT32U  CtrlRxMaxDly_ms;

    CPU_INT32U  CtrlTxMaxRetry;
    CPU_INT32U  CtrlTxMaxDly_ms;

    CPU_INT32U  DTP_ConnMaxTimout_ms;
    CPU_INT32U  DTP_RxMaxTimout_ms;
    CPU_INT32U  DTP_TxMaxTimout_ms;

    CPU_INT32U  DTP_TxMaxRetry;
    CPU_INT32U  DTP_TxMaxDly_ms;
} FTPc_CFG;


typedef  struct  ftpc_conn {
           NET_SOCK_ID         SockID;
           NET_SOCK_ADDR       SockAddr;
           NET_IP_ADDR_FAMILY  SockAddrFamily;
#ifdef  NET_SECURE_MODULE_EN
    const  FTPc_SECURE_CFG    *SecureCfgPtr;
#endif
           CPU_INT08U          Buf[FTPc_CTRL_NET_BUF_SIZE];
    const  FTPc_CFG           *CfgPtr;
} FTPc_CONN;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

CPU_BOOLEAN  FTPc_Open    (       FTPc_CONN        *p_conn,
                           const  FTPc_CFG         *p_cfg,
                           const  FTPc_SECURE_CFG  *p_secure_cfg,
                                  CPU_CHAR         *p_host_server,
                                  NET_PORT_NBR      port_nbr,
                                  CPU_CHAR         *p_user,
                                  CPU_CHAR         *p_pass,
                                  RTOS_ERR         *p_err);

CPU_BOOLEAN  FTPc_Close   (       FTPc_CONN        *p_conn,
                                  RTOS_ERR         *p_err);

CPU_BOOLEAN  FTPc_RecvBuf (       FTPc_CONN        *p_conn,
                                  CPU_CHAR         *p_remote_file_name,
                                  CPU_INT08U       *p_buf,
                                  CPU_INT32U        buf_len,
                                  CPU_INT32U       *p_file_size,
                                  RTOS_ERR         *p_err);

CPU_BOOLEAN  FTPc_SendBuf (       FTPc_CONN        *p_conn,
                                  CPU_CHAR         *p_remote_file_name,
                                  CPU_INT08U       *p_buf,
                                  CPU_INT32U        buf_len,
                                  CPU_BOOLEAN       append,
                                  RTOS_ERR         *p_err);

CPU_BOOLEAN  FTPc_RecvFile(       FTPc_CONN        *p_conn,
                                  CPU_CHAR         *p_remote_file_name,
                                  CPU_CHAR         *p_local_file_name,
                                  RTOS_ERR         *p_err);

CPU_BOOLEAN  FTPc_SendFile(       FTPc_CONN        *p_conn,
                                  CPU_CHAR         *p_remote_file_name,
                                  CPU_CHAR         *p_local_file_name,
                                  CPU_BOOLEAN       append,
                                  RTOS_ERR         *p_net);

#ifdef __cplusplus
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef NET_TCP_MODULE_EN
#error  "NET_TCP_CFG_EN illegally #define'd in 'net_cfg.h' [MUST be  DEF_ENABLED]"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _FTP_CLIENT_H_ */
