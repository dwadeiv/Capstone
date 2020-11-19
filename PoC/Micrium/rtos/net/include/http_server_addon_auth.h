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
*                                   HTTP SERVER AUTHENTICATION ADD-ON
*
* File : http_server_addon_auth.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef _HTTP_SERVER_ADDON_AUTH_H_
#define _HTTP_SERVER_ADDON_AUTH_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/http_server.h>
#include  <rtos/net/include/http_server_addon_ctrl_layer.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/auth.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef enum https_auth_state {
    HTTPs_AUTH_STATE_REQ_URL,
    HTTPs_AUTH_STATE_REQ_COMPLETE
} HTTPs_AUTH_STATE;


typedef  struct  https_auth_result {
    CPU_CHAR   *RedirectPathOnValidCredPtr;
    CPU_CHAR   *RedirectPathOnInvalidCredPtr;
    CPU_CHAR   *RedirectPathOnNoCredPtr;
    CPU_CHAR   *UsernamePtr;
    CPU_CHAR   *PasswordPtr;
}  HTTPs_AUTH_RESULT;


typedef  CPU_BOOLEAN        (*HTTPs_AUTH_PARSE_LOGIN_FNCT)            (const  HTTPs_INSTANCE     *p_inst,
                                                                       const  HTTPs_CONN         *p_conn,
                                                                              HTTPs_AUTH_STATE    state,
                                                                              HTTPs_AUTH_RESULT  *p_result);

typedef  CPU_BOOLEAN        (*HTTPs_AUTH_PARSE_LOGOUT_FNCT)           (const  HTTPs_INSTANCE     *p_inst,
                                                                       const  HTTPs_CONN         *p_conn,
                                                                              HTTPs_AUTH_STATE    state);

typedef  AUTH_RIGHT         (*HTTPs_AUTH_GET_REQUIRED_RIGHT_FNCT)     (const  HTTPs_INSTANCE     *p_inst,
                                                                       const  HTTPs_CONN         *p_conn);


typedef  struct  HTTPs_Authentication_Cfg {
    HTTPs_AUTH_PARSE_LOGIN_FNCT   ParseLogin;
    HTTPs_AUTH_PARSE_LOGOUT_FNCT  ParseLogout;
} HTTPs_AUTH_CFG;


typedef  struct  HTTPs_Authorization_Cfg {
    HTTPs_AUTH_GET_REQUIRED_RIGHT_FNCT      GetRequiredRights;
} HTTPs_AUTHORIZATION_CFG;


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

extern  HTTPs_CTRL_LAYER_AUTH_HOOKS  HTTPsAuth_CookieHooksCfg;
extern  HTTPs_CTRL_LAYER_APP_HOOKS   HTTPsAuth_AppUnprotectedCookieHooksCfg;
extern  HTTPs_CTRL_LAYER_APP_HOOKS   HTTPsAuth_AppProtectedCookieHooksCfg;

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

#endif /* _HTTP_SERVER_ADDON_AUTH_H_ */
