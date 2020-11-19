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
*                                      DNS CLIENT COMMAND MODULE
*
* File : dns_client_cmd.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>
#if (defined(RTOS_MODULE_NET_AVAIL) && \
     defined(RTOS_MODULE_COMMON_SHELL_AVAIL))


#include  <rtos/net/include/net_cfg_net.h>

#ifdef  NET_DNS_CLIENT_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/dns_client.h>
#include  <rtos/net/include/net_util.h>
#include  <rtos/net/include/net_ascii.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/source/cmd/net_cmd_output_priv.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_str.h>
#include  <rtos/common/include/shell.h>
#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/include/toolchains.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>
#include  <rtos/common/source/logging/logging_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                       (NET, DNS)
#define  RTOS_MODULE_CUR                    RTOS_CFG_MODULE_NET_APP


#define  DNSc_CMD_OUTPUT_ERR_REQ_FAIL          ("Request fail : ")
#define  DNSc_CMD_OUTPUT_ERR_STATUS_PENDING    ("Request is pending")
#define  DNSc_CMD_OUTPUT_ERR_STATUS_FAILED     ("Request failed")

#define  DNSc_CMD_OUTPUT_ERR_CLR_CACHE_FAIL    ("Cache clear failed : ")
#define  DNSc_CMD_OUTPUT_ERR_SET_SERVER_FAIL   ("Set server failed : ")


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_INT16S  DNScCmd_GetHost      (CPU_INT16U          argc,
                                          CPU_CHAR           *p_argv[],
                                          SHELL_OUT_FNCT      out_fnct,
                                          SHELL_CMD_PARAM    *p_cmd_param);

static  CPU_INT16S DNScCmd_SetServerAddr (CPU_INT16U          argc,
                                          CPU_CHAR           *p_argv[],
                                          SHELL_OUT_FNCT      out_fnct,
                                          SHELL_CMD_PARAM    *p_cmd_param);

static  CPU_INT16S DNScCmd_ClrCache      (CPU_INT16U          argc,
                                          CPU_CHAR           *p_argv[],
                                          SHELL_OUT_FNCT      out_fnct,
                                          SHELL_CMD_PARAM    *p_cmd_param);

static  CPU_INT16S  DNScCmd_Help         (CPU_INT16U          argc,
                                          CPU_CHAR           *p_argv[],
                                          SHELL_OUT_FNCT      out_fnct,
                                          SHELL_CMD_PARAM    *p_cmd_param);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  SHELL_CMD DNSc_CmdTbl[] =
{
    {"dns_get_host",   DNScCmd_GetHost},
    {"dns_server_set", DNScCmd_SetServerAddr},
    {"dns_cache_clr",  DNScCmd_ClrCache},
    {"dns_help",       DNScCmd_Help},
    {0, 0}
};


/*
*********************************************************************************************************
*                                           DNScCmd_Init()
*
* Description : Add <Network Application> test stubs to uC-Shell.
*
* Argument(s) : p_err    Pointer to variable that will receive the return error code from this function :
*
*                             RTOS_ERR_NONE
*                             RTOS_ERR_OWNERSHIP
*                             RTOS_ERR_ALREADY_EXISTS
*                             RTOS_ERR_BLK_ALLOC_CALLBACK
*                             RTOS_ERR_SEG_OVF
*                             RTOS_ERR_OS_SCHED_LOCKED
*                             RTOS_ERR_NOT_AVAIL
*                             RTOS_ERR_POOL_EMPTY
*                             RTOS_ERR_WOULD_OVF
*                             RTOS_ERR_OS_OBJ_DEL
*                             RTOS_ERR_WOULD_BLOCK
*                             RTOS_ERR_INVALID_ARG
*                             RTOS_ERR_NO_MORE_RSRC
*                             RTOS_ERR_IS_OWNER
*                             RTOS_ERR_ABORT
*                             RTOS_ERR_TIMEOUT
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCmd_Init (RTOS_ERR  *p_err)
{
    LOG_VRB(("Adding DNS Client Command to Shell"));
    Shell_CmdTblAdd("dns", DNSc_CmdTbl, p_err);
}


/*
*********************************************************************************************************
*                                        DNScCmd_SetServerAddr()
*
* Description : Command to configure DNS client server to be used by default.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_RTN_CODE_CONN_CLOSED,                 if implemented connection closed
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT16S  DNScCmd_SetServerAddr (CPU_INT16U          argc,
                                           CPU_CHAR           *p_argv[],
                                           SHELL_OUT_FNCT      out_fnct,
                                           SHELL_CMD_PARAM    *p_cmd_param)
{
    CPU_INT16S  output;
    RTOS_ERR    local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    if (argc != 2) {
        output = NetCmd_OutputCmdArgInvalid(out_fnct, p_cmd_param);
        goto exit;
    }

    DNSc_CfgServerByStr(p_argv[1], &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        CPU_CHAR  msg[25];


        Str_Copy(msg, DNSc_CMD_OUTPUT_ERR_SET_SERVER_FAIL);
        Str_FmtNbr_Int32U(RTOS_ERR_CODE_GET(local_err), 5, 10, '\0', DEF_YES, DEF_YES, msg + sizeof(DNSc_CMD_OUTPUT_ERR_SET_SERVER_FAIL));
        output = NetCmd_OutputError(msg, out_fnct, p_cmd_param);
        goto exit;
    }

    output = NetCmd_OutputSuccess(out_fnct, p_cmd_param);

exit:
    return (output);
}

/*
*********************************************************************************************************
*                                           DNScCmd_GetHost()
*
* Description : Command to resolve an host name.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_RTN_CODE_CONN_CLOSED,                 if implemented connection closed
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16S  DNScCmd_GetHost (CPU_INT16U        argc,
                             CPU_CHAR         *p_argv[],
                             SHELL_OUT_FNCT    out_fnct,
                             SHELL_CMD_PARAM  *p_cmd_param)
{
    DNSc_STATUS      status;
    NET_IP_ADDR_OBJ  addrs[50];
    CPU_INT08U       addr_ctr = 50u;
    CPU_INT08U       ix;
    CPU_INT16S       output;
    RTOS_ERR         local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    if (argc != 2) {
        output = NetCmd_OutputCmdArgInvalid(out_fnct, p_cmd_param);
        goto exit;
    }


    status = DNSc_GetHost(p_argv[1],
                          addrs,
                         &addr_ctr,
                          DNSc_FLAG_NONE,
                          DEF_NULL,
                         &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        CPU_CHAR  msg[20];


        Str_Copy(msg, DNSc_CMD_OUTPUT_ERR_REQ_FAIL);
        Str_FmtNbr_Int32U(RTOS_ERR_CODE_GET(local_err), 5, 10, '\0', DEF_YES, DEF_YES, msg + sizeof(DNSc_CMD_OUTPUT_ERR_REQ_FAIL));
        output = NetCmd_OutputError(msg, out_fnct, p_cmd_param);
        goto exit;
    }


    switch (status) {
        case DNSc_STATUS_RESOLVED:
             break;

        case DNSc_STATUS_PENDING:
             output = NetCmd_OutputError(DNSc_CMD_OUTPUT_ERR_STATUS_PENDING, out_fnct, p_cmd_param);
             goto exit;

        case DNSc_STATUS_FAILED:
        default:
             output = NetCmd_OutputError(DNSc_CMD_OUTPUT_ERR_STATUS_FAILED, out_fnct, p_cmd_param);
             goto exit;
    }

    for (ix = 0u; ix < addr_ctr; ix++) {
        CPU_CHAR  addr_str[NET_ASCII_LEN_MAX_ADDR_IP];
        RTOS_ERR  local_err;

        if (addrs[ix].AddrLen == NET_IPv4_ADDR_LEN) {
            NET_IPv4_ADDR *p_addr = (NET_IPv4_ADDR *)&addrs[ix].Addr.Array[0];


            NetASCII_IPv4_to_Str(*p_addr, addr_str, NET_ASCII_LEN_MAX_ADDR_IP, &local_err);
        } else {
            NET_IPv6_ADDR *p_addr = (NET_IPv6_ADDR *)&addrs[ix].Addr.Array[0];


            NetASCII_IPv6_to_Str(p_addr, addr_str, DEF_NO, DEF_YES, &local_err);
        }

        output += NetCmd_OutputMsg(addr_str, DEF_YES, DEF_NO, DEF_YES, out_fnct, p_cmd_param);
    }

    output = NetCmd_OutputSuccess(out_fnct, p_cmd_param);

exit:
    return (output);
}


/*
*********************************************************************************************************
*                                          DNScCmd_ClrCache()
*
* Description : Command function to clear the cache.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_RTN_CODE_CONN_CLOSED,                 if implemented connection closed
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT16S  DNScCmd_ClrCache (CPU_INT16U        argc,
                                      CPU_CHAR         *p_argv[],
                                      SHELL_OUT_FNCT    out_fnct,
                                      SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16S output;
    RTOS_ERR   local_err;


    PP_UNUSED_PARAM(p_argv);

    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    if (argc != 1) {
        output = NetCmd_OutputCmdArgInvalid(out_fnct, p_cmd_param);
        goto exit;
    }

    DNSc_CacheClrAll(&local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        CPU_CHAR  msg[25];


        Str_Copy(msg, DNSc_CMD_OUTPUT_ERR_CLR_CACHE_FAIL);
        Str_FmtNbr_Int32U(RTOS_ERR_CODE_GET(local_err), 5, 10, '\0', DEF_YES, DEF_YES, msg + sizeof(DNSc_CMD_OUTPUT_ERR_CLR_CACHE_FAIL));
        output = NetCmd_OutputError(msg, out_fnct, p_cmd_param);
        goto exit;
    }

    output = NetCmd_OutputSuccess(out_fnct, p_cmd_param);

exit:
    return (output);
}


/*
*********************************************************************************************************
*                                            DNScCmd_Help()
*
* Description : Output DNSc command help.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_RTN_CODE_CONN_CLOSED,                 if implemented connection closed
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT16S DNScCmd_Help (CPU_INT16U        argc,
                                 CPU_CHAR         *p_argv[],
                                 SHELL_OUT_FNCT    out_fnct,
                                 SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16S  ret_val;


    PP_UNUSED_PARAM(argc);
    PP_UNUSED_PARAM(p_argv);

    ret_val = NetCmd_OutputCmdTbl(DNSc_CmdTbl, out_fnct, p_cmd_param);

    return (ret_val);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_DNS_CLIENT_MODULE_EN   */
#endif  /* RTOS_MODULE_NET_AVAIL && RTOS_MODULE_COMMON_SHELL_AVAIL */
