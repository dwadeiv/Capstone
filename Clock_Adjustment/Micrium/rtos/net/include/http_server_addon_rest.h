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
*                                         HTTP SERVER REST ADD-ON
*
* File : http_server_addon_rest.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _HTTP_SERVER_REST_ADDON_H_
#define  _HTTP_SERVER_REST_ADDON_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/http.h>
#include  <rtos/net/include/http_server.h>

#include  <rtos/cpu/include/cpu.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define HTTPs_REST_MAX_URI_WILD_CARD           5


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          REST CONFIGURATION TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_cfg {
    const  CPU_INT32U   listID;
} HTTPs_REST_CFG;


/*
*********************************************************************************************************
*                                            REST STATE TYPE
*********************************************************************************************************
*/

typedef  enum  https_rest_state {
    HTTPs_REST_STATE_INIT,
    HTTPs_REST_STATE_RX,
    HTTPs_REST_STATE_ERROR,
    HTTPs_REST_STATE_TX,
    HTTPs_REST_STATE_CLOSE,
} HTTPs_REST_STATE;


/*
*********************************************************************************************************
*                                           REST HOOK STATE TYPE
*********************************************************************************************************
*/

typedef  enum  https_rest_hook_state {
    HTTPs_REST_HOOK_STATE_ERROR,
    HTTPs_REST_HOOK_STATE_CONTINUE,
    HTTPs_REST_HOOK_STATE_STAY,
} HTTPs_REST_HOOK_STATE;


/*
*********************************************************************************************************
*                                       REST KEY-VALUE PAIR TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_key_val {
    const   CPU_CHAR     *KeyPtr;
            CPU_SIZE_T    KeyLen;
    const   CPU_CHAR     *ValPtr;
            CPU_SIZE_T    ValLen;
} HTTPs_REST_KEY_VAL;


/*
*********************************************************************************************************
*                                          REST PARSED URI TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_parsed_uri {
    CPU_CHAR     *PathPtr;
    CPU_SIZE_T    PathLen;
} HTTPs_REST_PARSED_URI;


/*
*********************************************************************************************************
*                                          REST MATCHED URI TYPE
*********************************************************************************************************
*/

typedef  struct https_rest_matched_uri {
    HTTPs_REST_PARSED_URI  ParsedURI;
    HTTPs_REST_KEY_VAL     WildCards[HTTPs_REST_MAX_URI_WILD_CARD];
    CPU_SIZE_T             WildCardsNbr;
} HTTPs_REST_MATCHED_URI;


/*
*********************************************************************************************************
*                                          REST HOOK FUNCTION TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_resource  HTTPs_REST_RESOURCE;

typedef  HTTPs_REST_HOOK_STATE  (*HTTPs_REST_HOOK_FNCT)  (const  HTTPs_REST_RESOURCE      *p_resource,
                                                          const  HTTPs_REST_MATCHED_URI   *p_uri,
                                                          const  HTTPs_REST_STATE          state,
                                                                 void                    **p_data,
                                                          const  HTTPs_INSTANCE           *p_instance,
                                                                 HTTPs_CONN               *p_conn,
                                                                 void                     *p_buf,
                                                          const  CPU_SIZE_T                buf_size,
                                                                 CPU_SIZE_T               *p_buf_size_used);


/*
*********************************************************************************************************
*                                             REST METHOD HOOKS TYPE
*********************************************************************************************************
*/

typedef  struct  https_rest_method_hooks {
    HTTPs_REST_HOOK_FNCT  Delete;
    HTTPs_REST_HOOK_FNCT  Get;
    HTTPs_REST_HOOK_FNCT  Head;
    HTTPs_REST_HOOK_FNCT  Post;
    HTTPs_REST_HOOK_FNCT  Put;
} HTTPs_REST_METHOD_HOOKS;


/*
*********************************************************************************************************
*                                              REST RESOURCE TYPE
*********************************************************************************************************
*/

struct  https_rest_resource {
    const   CPU_CHAR                 *PatternPtr;               /* Access path to the resource ending with an EOF char. */
    const   HTTP_HDR_FIELD           *HTTP_Hdrs;                /* HTTP headers to keep.                                */
    const   CPU_SIZE_T                HTTP_HdrsNbr;
    const   HTTPs_REST_METHOD_HOOKS   MethodHooks;
};


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

extern  const  HTTPs_HOOK_CFG  HTTPs_REST_HookCfg;
extern  const  HTTPs_REST_CFG  HTTPs_REST_Cfg;

/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void         HTTPsREST_Publish  (const  HTTPs_REST_RESOURCE  *p_resource,
                                        CPU_INT32U            list_ID,
                                        RTOS_ERR             *p_err);

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

#endif  /* _HTTP_SERVER_REST_ADDON_H_ */
