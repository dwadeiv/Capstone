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
*                                          SMTP CLIENT MODULE
*
* File : smtp_client.h
*********************************************************************************************************
* Note(s) : (1) This code implements a subset of the SMTP protocol (RFC 2821).  More precisely, the
*               following commands have been implemented:
*
*                 HELO
*                 AUTH (if enabled)
*                 MAIL
*                 RCPT
*                 DATA
*                 RSET
*                 NOOP
*                 QUIT
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _SMTP_CLIENT_H_
#define  _SMTP_CLIENT_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/net_app.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/lib_str.h>
#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/source/collections/slist_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  SMTPc_PORT_DFLT                                  25    /* Cfg SMTP        server IP port (see note #1).        */
#define  SMTPc_PORT_SECURE_DFLT                          465    /* Cfg SMTP secure server IP port (see Note #2).        */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


typedef  enum  smtpc_msg_param {
    SMTPc_FROM_ADDR,
    SMTPc_FROM_DISPL_NAME,
    SMTPc_TO_ADDR,
    SMTPc_CC_ADDR,
    SMTPc_BCC_ADDR,
    SMTPc_REPLY_TO_ADDR,
    SMTPc_SENDER_ADDR,
    SMTPc_ATTACHMENT,
    SMTPc_MSG_SUBJECT,
    SMTPc_MSG_BODY,
} SMTPc_MSG_PARAM;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          SMTP MSG Structure
*
* Note(s): (1) A mail object is represented in this module by the structure SMTPc_MSG.  This
*              structure contains all the necessary information to generate the mail object
*              and to send it to the SMTP server.  More specifically, it encapsulates the various
*              addresses of the sender and recipients, MIME information, the message itself, and
*              finally the eventual attachments.
*********************************************************************************************************
*/

typedef  struct  SMTPc_msg {
    CPU_CHAR               *FromAddr;                           /* "From" field     (1:1).                              */
    CPU_CHAR               *FromName;
    SLIST_MEMBER           *To_List;                             /* "To" field       (1:*).                              */

    CPU_CHAR               *ReplyTo;                            /* "Reply-to" field (0:1).                              */
    CPU_CHAR               *Sender;                             /* "Sender" field   (0:1).                              */
    SLIST_MEMBER           *CC_List;                            /* "CC" field       (0:*).                              */

    SLIST_MEMBER           *BCC_List;                           /* "BCC" field      (0:*).                              */

    CPU_CHAR               *MsgSubject;                         /* Subject of msg.                                      */
    CPU_CHAR               *MsgBody;                            /* Data of the mail obj content's body.                 */
} SMTPc_MSG;


typedef  struct  smtpc_init_cfg {
    MEM_SEG       *MemSegPtr;                                   /* Pointer to the HTTP server memory segment.           */
    CPU_INT16U     AuthBufInputLen;
    CPU_INT16U     AuthBufOutputLen;
} SMTPc_INIT_CFG;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
    extern  const  SMTPc_INIT_CFG  SMTPc_InitCfgDflt;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
void         SMTPc_ConfigureMemSeg     (MEM_SEG                  *p_mem_seg);

void         SMTPc_ConfigureAuthBufLen (CPU_INT16U                buf_len);
#endif

void         SMTPc_Init                (RTOS_ERR                 *p_err);

void         SMTPc_DfltCfgSet          (CPU_INT16U                timeout_conn_ms,
                                        CPU_INT16U                timeout_rx_ms,
                                        CPU_INT16U                timeout_tx_ms,
                                        CPU_INT16U                timeout_close_ms,
                                        RTOS_ERR                 *p_err);

SMTPc_MSG   *SMTPc_MsgAlloc            (RTOS_ERR                 *p_err);

void         SMTPc_MsgFree             (SMTPc_MSG                *p_msg,
                                        RTOS_ERR                 *p_err);

void         SMTPc_MsgClr              (SMTPc_MSG                *p_msg,
                                        RTOS_ERR                 *p_err);

void         SMTPc_MsgSetParam         (SMTPc_MSG                *p_msg,
                                        SMTPc_MSG_PARAM           param_type,
                                        void                     *p_val,
                                        RTOS_ERR                 *p_err);

void         SMTPc_SendMail            (CPU_CHAR                 *p_host_name,
                                        CPU_INT16U                port,
                                        CPU_CHAR                 *p_username,
                                        CPU_CHAR                 *p_pwd,
                                        NET_APP_SOCK_SECURE_CFG  *p_secure_cfg,
                                        SMTPc_MSG                *p_msg,
                                        RTOS_ERR                 *p_err);

                                                                /* ---------------- SMTP SESSION FNCTS ---------------- */
NET_SOCK_ID  SMTPc_Connect             (CPU_CHAR                 *p_host_name,
                                        CPU_INT16U                port,
                                        CPU_CHAR                 *p_username,
                                        CPU_CHAR                 *p_pwd,
                                        NET_APP_SOCK_SECURE_CFG  *p_secure_cfg,
                                        RTOS_ERR                 *p_err);

void         SMTPc_SendMsg             (NET_SOCK_ID               sock_id,
                                        SMTPc_MSG                *msg,
                                        RTOS_ERR                 *perr);

void         SMTPc_Disconnect          (NET_SOCK_ID               sock_id,
                                        RTOS_ERR                 *perr);

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

#endif  /* _SMTP_CLIENT_H_ */

