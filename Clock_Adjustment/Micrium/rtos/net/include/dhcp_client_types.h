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
*                                          DHCP CLIENT TYPES
*
* File : dhcp_client_types.h
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _DHCP_CLIENT_TYPES_H_
#define  _DHCP_CLIENT_TYPES_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_def.h>
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
*                                    DEFAULT CONFIGUATION DEFINES
*********************************************************************************************************
*/

#define  DHCPc_CFG_PORT_SERVER_DFLT                     67
#define  DHCPc_CFG_PORT_CLIENT_DFLT                     68

#define  DHCPc_CFG_TX_RETRY_NBR_DFLT                    2u
#define  DHCPc_CFG_TX_WAIT_TIMEOUT_MS_DFLT             (3u * DEF_TIME_NBR_mS_PER_SEC)


#define  DHCPc_CFG_DFLT     {                                                     \
                              .ServerPortNbr = DHCPc_CFG_PORT_SERVER_DFLT,        \
                              .ClientPortNbr = DHCPc_CFG_PORT_CLIENT_DFLT,        \
                              .TxRetryNbr    = DHCPc_CFG_TX_RETRY_NBR_DFLT,       \
                              .TxTimeout_ms  = DHCPc_CFG_TX_WAIT_TIMEOUT_MS_DFLT, \
                              .ValidateAddr  = DEF_NO,                            \
                            }

#define  DHCPc_CFG_DFLT_INIT(cfg) {                                                        \
                                    cfg.ServerPortNbr = DHCPc_CFG_PORT_SERVER_DFLT;        \
                                    cfg.ClientPortNbr = DHCPc_CFG_PORT_CLIENT_DFLT;        \
                                    cfg.TxRetryNbr    = DHCPc_CFG_TX_RETRY_NBR_DFLT;       \
                                    cfg.TxTimeout_ms  = DHCPc_CFG_TX_WAIT_TIMEOUT_MS_DFLT; \
                                    cfg.ValidateAddr  = DEF_NO;                            \
                                  }


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       RESULT STATUS DATA TYPE
*********************************************************************************************************
*/

typedef  enum  dhcpc_status {
    DHCPc_STATUS_NONE,
    DHCPc_STATUS_SUCCESS,
    DHCPc_STATUS_LINK_LOCAL,
    DHCPc_STATUS_IN_PROGRESS,
    DHCPc_STATUS_FAIL_ADDR_USED,
    DHCPc_STATUS_FAIL_OFFER_DECLINE,
    DHCPc_STATUS_FAIL_NAK_RX,
    DHCPc_STATUS_FAIL_NO_SERVER,
    DHCPc_STATUS_FAIL_ERR_FAULT
} DHCPc_STATUS;


/*
*********************************************************************************************************
*                                    HOOK FUNCTIONS DATA TYPE
*********************************************************************************************************
*/

typedef  void  (*DHCPc_ON_COMPLETE_HOOK) (NET_IF_NBR     if_nbr,
                                          DHCPc_STATUS   status,
                                          NET_IPv4_ADDR  addr,
                                          NET_IPv4_ADDR  mask,
                                          NET_IPv4_ADDR  gateway,
                                          RTOS_ERR       err);


/*
*********************************************************************************************************
*                                   RUN-TIME CONFIGURATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  dhcpc_cfg {
    CPU_INT16U                  ServerPortNbr;
    CPU_INT16U                  ClientPortNbr;

    CPU_INT08U                  TxRetryNbr;
    CPU_INT32U                  TxTimeout_ms;

    CPU_BOOLEAN                 ValidateAddr;
} DHCPc_CFG;


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

extern  const  DHCPc_CFG  DHCPc_CfgDft;

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

#endif  /* _DHCP_CLIENT_TYPES_H_ */
