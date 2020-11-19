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
*                                 HTTP SERVER CONTROL LAYER ADD-ON
*
* File : http_server_addon_ctrl_layer.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef _HTTP_SERVER_ADDON_CTRL_LAYER_H_
#define _HTTP_SERVER_ADDON_CTRL_LAYER_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/http_server.h>

#include  <rtos/cpu/include/cpu.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                               CTRL LAYER AUTHENTICATION HOOKS DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_auth_hooks {
    HTTPs_INSTANCE_INIT_HOOK   OnInstanceInit;
    HTTPs_REQ_HDR_RX_HOOK      OnReqHdrRx;
    HTTPs_REQ_HOOK             OnReqAuth;
    HTTPs_RESP_HDR_TX_HOOK     OnRespHdrTx;
    HTTPs_TRANS_COMPLETE_HOOK  OnTransComplete;
    HTTPs_CONN_CLOSE_HOOK      OnConnClose;
} HTTPs_CTRL_LAYER_AUTH_HOOKS;


/*
*********************************************************************************************************
*                                CTRL LAYER APPLICATION HOOKS DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_app_hooks {
    HTTPs_INSTANCE_INIT_HOOK   OnInstanceInit;
    HTTPs_REQ_HDR_RX_HOOK      OnReqHdrRx;
    HTTPs_REQ_HOOK             OnReq;
    HTTPs_REQ_BODY_RX_HOOK     OnReqBodyRx;
    HTTPs_REQ_RDY_SIGNAL_HOOK  OnReqSignal;
    HTTPs_REQ_RDY_POLL_HOOK    OnReqPoll;
    HTTPs_RESP_HDR_TX_HOOK     OnRespHdrTx;
    HTTPs_RESP_TOKEN_HOOK      OnRespToken;
    HTTPs_RESP_CHUNK_HOOK      OnRespChunk;
    HTTPs_TRANS_COMPLETE_HOOK  OnTransComplete;
    HTTPs_ERR_HOOK             OnError;
    HTTPs_CONN_CLOSE_HOOK      OnConnClose;
} HTTPs_CTRL_LAYER_APP_HOOKS;


/*
*********************************************************************************************************
*                                CTRL LAYER AUTHENTIFIACTION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_auth_inst {
    const  HTTPs_CTRL_LAYER_AUTH_HOOKS  *HooksPtr;
           void                         *HooksCfgPtr;
} HTTPs_CTRL_LAYER_AUTH_INST;


/*
*********************************************************************************************************
*                                CTRL LAYER APPLICATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_app_inst {
    const  HTTPs_CTRL_LAYER_APP_HOOKS  *HooksPtr;
           void                        *HooksCfgPtr;
} HTTPs_CTRL_LAYER_APP_INST;


/*
*********************************************************************************************************
*                                CTRL LAYER CONFIGUATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_cfg {
    HTTPs_CTRL_LAYER_AUTH_INST    **AuthInstsPtr;
    CPU_SIZE_T                      AuthInstsNbr;
    HTTPs_CTRL_LAYER_APP_INST     **AppInstsPtr;
    CPU_SIZE_T                      AppInstsNbr;
} HTTPs_CTRL_LAYER_CFG;


/*
*********************************************************************************************************
*                               CTRL LAYER CONFIGURATION LIST DATA TYPE
*********************************************************************************************************
*/

typedef  struct  https_ctrl_layer_cfg_List {
    HTTPs_CTRL_LAYER_CFG  **CfgsPtr;
    CPU_SIZE_T              Size;
} HTTPs_CTRL_LAYER_CFG_LIST;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

extern  const  HTTPs_HOOK_CFG              HTTPsCtrlLayer_HookCfg;

extern  const  HTTPs_CTRL_LAYER_APP_HOOKS  HTTPsCtrlLayer_REST_App;

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

#endif /* _HTTP_SERVER_ADDON_CTRL_LAYER_H_ */
