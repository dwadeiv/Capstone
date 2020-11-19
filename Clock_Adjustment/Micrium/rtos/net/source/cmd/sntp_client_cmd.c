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
*                                        SNTP CLIENT CMD MODULE
*
* File : sntp_client_cmd.c
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

#if (defined(RTOS_MODULE_NET_SNTP_CLIENT_AVAIL) && \
     defined(RTOS_MODULE_COMMON_SHELL_AVAIL))

#if (!defined(RTOS_MODULE_NET_AVAIL))
#error SNTP Client Module requires Network Core module. Make sure it is part of your project            \
       and that RTOS_MODULE_NET_AVAIL is defined in rtos_description.h.
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/sntp_client.h>
#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/net_ip.h>

#include  <rtos/common/include/shell.h>
#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/lib_str.h>
#include  <rtos/common/include/lib_ascii.h>
#include  <rtos/common/include/toolchains.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                                  (NET, SNTP)
#define  RTOS_MODULE_CUR                               RTOS_CFG_MODULE_NET

#define SNTPc_CMD_HELP                                 "\r\nusage: sntp_get [options]\r\n\r\n"                      \
                                                       " -6,           Test SNTPc using IPv6 \r\n"                  \
                                                       " -4,           Test SNTPc using IPv4 \r\n"                  \
                                                       " -d,           Test SNTPc using server domain name\r\n\r\n" \

#define SNTPc_GET_MSG_STR1                             "\r\nNTP Time             : "
#define SNTPc_GET_MSG_STR2                             "\r\nRound trip delay (us): "

#define SNTPc_CMD_FAIL                                 "FAIL "

#define SNTPc_CMD_SERVER_IPV4                          "192.168.0.2"
#define SNTPc_CMD_SERVER_IPV6                          "fe80::1234:5678"
#define SNTPc_CMD_SERVER_DOMAIN_NAME                   "0.pool.ntp.org"

#define SNTPc_CMD_PARSER_DNS                           ASCII_CHAR_LATIN_LOWER_D
#define SNTPc_CMD_PARSER_IPv6                          ASCII_CHAR_DIGIT_SIX
#define SNTPc_CMD_PARSER_IPv4                          ASCII_CHAR_DIGIT_FOUR
#define SNTPc_CMD_ARG_PARSER_CMD_BEGIN                 ASCII_CHAR_HYPHEN_MINUS


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_INT16S  SNTPcCmd_Get  (CPU_INT16U        argc,
                           CPU_CHAR         *p_argv[],
                           SHELL_OUT_FNCT    out_fnct,
                           SHELL_CMD_PARAM  *p_cmd_param);

CPU_INT16S  SNTPcCmd_Help (CPU_INT16U        argc,
                           CPU_CHAR         *p_argv[],
                           SHELL_OUT_FNCT    out_fnct,
                           SHELL_CMD_PARAM  *p_cmd_param);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  SHELL_CMD SNTPc_CmdTbl[] =
{
    {"sntp_get" , SNTPcCmd_Get},
    {"sntp_help" , SNTPcCmd_Help},
    {0, 0 }
};


/*
*********************************************************************************************************
*                                           SNTPcCmd_Init()
*
* Description : Adds the SNTP client command stubs to Shell table.
*
* Argument(s) : p_err    Pointer to an error code which will be returned to your application.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  SNTPcCmd_Init (RTOS_ERR  *p_err)
{
    Shell_CmdTblAdd("sntp", SNTPc_CmdTbl, p_err);
}


/*
*********************************************************************************************************
*                                           SNTPcCmd_Get()
*
* Description : Get NTP timestamp from the server and compute the roundtrip delay.
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
*               SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed
*               SHELL_OUT_ERR, otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16S  SNTPcCmd_Get (CPU_INT16U        argc,
                          CPU_CHAR         *p_argv[],
                          SHELL_OUT_FNCT    out_fnct,
                          SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16S           ret_val;
    CPU_INT08U           i;
    SNTP_PKT             pkt;
    SNTP_TS              ts;
    CPU_INT32U           roundtrip;
    CPU_CHAR             str_output[32u];
    CPU_INT16U           str_len;
    CPU_CHAR            *hostname       = SNTPc_CMD_SERVER_DOMAIN_NAME;
    NET_PORT_NBR         port_nbr       = SNTP_CLIENT_CFG_SERVER_PORT_NBR_DFLT;
    NET_IP_ADDR_FAMILY   addr_family    = SNTP_CLIENT_CFG_SERVER_ADDR_FAMILY_DFLT;
    CPU_INT32U           rx_timeout_ms  = SNTP_CLIENT_CFG_REQ_RX_TIMEOUT_MS_DFLT;
    RTOS_ERR             local_err;


                                                                /* ----------------- PARSE ARGUMENTS ------------------ */
    if (argc != 0u) {
        for (i = 1u; i < argc; i++) {
            if (*p_argv[i] == SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                switch (*(p_argv[i] + 1)) {
                    case SNTPc_CMD_PARSER_IPv6:
                         hostname    = SNTPc_CMD_SERVER_IPV6;
                         addr_family = NET_IP_ADDR_FAMILY_IPv6;
                         if (argc != i + 1u) {
                             if (*p_argv[i+1u] != SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 hostname = p_argv[i+1u];
                                 i++;
                             }
                         }
                         break;


                    case SNTPc_CMD_PARSER_IPv4:
                         hostname    = SNTPc_CMD_SERVER_IPV4;
                         addr_family = NET_IP_ADDR_FAMILY_IPv4;
                         if (argc != i + 1u) {
                             if (*p_argv[i+1u] != SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 hostname = p_argv[i+1u];
                                 i++;
                             }
                         }
                         break;


                    case SNTPc_CMD_PARSER_DNS:
                         hostname    = SNTPc_CMD_SERVER_DOMAIN_NAME;
                         addr_family = NET_IP_ADDR_FAMILY_NONE;
                         if (argc != i + 1u) {
                             if (*p_argv[i+1u] != SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 hostname = p_argv[i+1u];
                                 i++;
                             }
                         }
                         break;


                    default:
                         goto exit_fail;
                         break;
                }
            }
        }
    } else {
        goto exit_fail;
    }
                                                                /* ------------- REQ NTP TIME FROM SERVER ------------- */
    SNTPc_DfltCfgSet(port_nbr,
                     addr_family,
                     rx_timeout_ms,
                    &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        goto exit_fail;
    }

    SNTPc_ReqRemoteTime(hostname, &pkt, &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        goto exit_fail;
    }

    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    ts         = SNTPc_GetRemoteTime(&pkt, &local_err);

    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    roundtrip  = SNTPc_GetRoundTripDly_us(&pkt, &local_err);

                                                                /* ------------------ PRINT RESULTS ------------------- */
    str_len = Str_Len(SNTPc_GET_MSG_STR1);                      /* NTP Time.                                            */
    out_fnct(SNTPc_GET_MSG_STR1,
             str_len,
             p_cmd_param->OutputOptPtr);
    Str_FmtNbr_Int32U(ts.Sec,
                      DEF_INT_32U_NBR_DIG_MAX,
                      DEF_NBR_BASE_DEC,
                      DEF_NULL,
                      DEF_NO,
                      DEF_YES,
                      str_output);
    str_len = Str_Len(str_output);
    out_fnct(str_output,
             str_len,
             p_cmd_param->OutputOptPtr);
                                                                /* Roundtrip delay.                                     */
    str_len = Str_Len(SNTPc_GET_MSG_STR2);
    out_fnct(SNTPc_GET_MSG_STR2,
             str_len,
             p_cmd_param->OutputOptPtr);
    Str_FmtNbr_Int32U(roundtrip,
                      DEF_INT_32U_NBR_DIG_MAX,
                      DEF_NBR_BASE_DEC,
                      DEF_NULL,
                      DEF_NO,
                      DEF_YES,
                      str_output);
    str_len = Str_Len(str_output);
    out_fnct(str_output,
             str_len,
             p_cmd_param->OutputOptPtr);

    out_fnct(STR_NEW_LINE,
             STR_NEW_LINE_LEN,
             p_cmd_param->OutputOptPtr);

    out_fnct(STR_NEW_LINE,
             STR_NEW_LINE_LEN,
             p_cmd_param->OutputOptPtr);

    ret_val = str_len;

    return (ret_val);

exit_fail:
    out_fnct(SNTPc_CMD_FAIL,
             sizeof(SNTPc_CMD_FAIL),
             p_cmd_param->OutputOptPtr);
    out_fnct(STR_NEW_LINE,
             STR_NEW_LINE_LEN,
             p_cmd_param->OutputOptPtr);
    return (1u);
}


/*
*********************************************************************************************************
*                                           SNTPc_Cmd_Help()
*
* Description : Print SNTPc command help.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*               SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed
*               SHELL_OUT_ERR, otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16S  SNTPcCmd_Help (CPU_INT16U        argc,
                           CPU_CHAR         *p_argv[],
                           SHELL_OUT_FNCT    out_fnct,
                           SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16U  cmd_namd_len;
    CPU_INT16S  output;
    CPU_INT16S  ret_val;


    PP_UNUSED_PARAM(argc);
    PP_UNUSED_PARAM(p_argv);

    cmd_namd_len = Str_Len(SNTPc_CMD_HELP);
    output       = out_fnct(SNTPc_CMD_HELP,
                            cmd_namd_len,
                            p_cmd_param->OutputOptPtr);
    switch (output) {
        case SHELL_OUT_RTN_CODE_CONN_CLOSED:
        case SHELL_OUT_ERR:
             ret_val = SHELL_EXEC_ERR;
             break;

        default:
             ret_val = output;
    }

    return (ret_val);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_SNTP_CLIENT_AVAIL && RTOS_MODULE_COMMON_SHELL_AVAIL */

