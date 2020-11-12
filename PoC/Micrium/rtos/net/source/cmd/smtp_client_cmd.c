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
*                                        SMTP CLIENT CMD MODULE
*
* File : smtp_client_cmd.c
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

#if (defined(RTOS_MODULE_NET_SMTP_CLIENT_AVAIL) && \
     defined(RTOS_MODULE_COMMON_SHELL_AVAIL))

#if (!defined(RTOS_MODULE_NET_AVAIL))
    #error SMTP Client Module requires Network Core module. Make sure it is part of your project         \
           and that RTOS_MODULE_NET_AVAIL is defined in rtos_description.h.
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/smtp_client.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/shell.h>
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

#define SMTPc_CMD_HELP                                 "\r\nusage: smtp_send [options]\r\n\r\n"                                         \
                                                       " -6,           Test SMTPc using IPv6 \r\n"                                      \
                                                       " -4,           Test SMTPc using IPv4 \r\n"                                      \
                                                       " -d,           Test SMTPc using server domain name (aspmx.l.google.com)\r\n"    \
                                                       " -t,           Set the TO address used to send the mail\r\n"                    \

#define SMTPc_CMD_OK                                   "OK"
#define SMTPc_CMD_FAIL                                 "FAIL"

#define SMTPc_CMD_SERVER_IPV4                          "192.168.0.2"
#define SMTPc_CMD_SERVER_IPV6                          "fe80::1234:5678:"
#define SMTPc_CMD_SERVER_DOMAIN_NAME                   "aspmx.l.google.com"

#define SMTPc_CMD_MAILBOX_FROM_NAME                    "Test Name From"
#define SMTPc_CMD_MAILBOX_FROM_ADDR                    "webmaster@mail.smtptest.com"
#define SMTPc_CMD_MAILBOX_TO_NAME                      "Test Name To"
#define SMTPc_CMD_MAILBOX_TO_ADDR                      "testto@mail.smtptest.com"


#define SMTPc_CMD_USERNAME                             "webmaster@mail.smtptest.com"
#define SMTPc_CMD_PW                                   "password"

#define SMTPc_CMD_MSG_SUBJECT                          "Test"
#define SMTPc_CMD_MSG                                  "This is a test message"
#define SMTPc_CMD_MAILBOX_CC_ADDR                      "testcc1@mail.smtptest.com"
#define SMTPc_CMD_MAILBOX_CC_NAME                      "Test Name CC1"
#define SMTPc_CMD_MAILBOX_BCC_ADDR                     "testbcc1@mail.smtptest.com"
#define SMTPc_CMD_MAILBOX_BCC_NAME                     "Test Name BCC 1"

#define SMTPc_CMD_PARSER_DNS                           ASCII_CHAR_LATIN_LOWER_D
#define SMTPc_CMD_PARSER_IPv6                          ASCII_CHAR_DIGIT_SIX
#define SMTPc_CMD_PARSER_IPv4                          ASCII_CHAR_DIGIT_FOUR
#define SMTPc_CMD_ARG_PARSER_CMD_BEGIN                 ASCII_CHAR_HYPHEN_MINUS
#define SMTPc_CMD_PARSER_TO                            ASCII_CHAR_LATIN_LOWER_T


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_INT16S  SMTPcCmd_Send            (CPU_INT16U          argc,
                                      CPU_CHAR           *p_argv[],
                                      SHELL_OUT_FNCT      out_fnct,
                                      SHELL_CMD_PARAM    *p_cmd_param);

CPU_INT16S  SMTPcCmd_Help            (CPU_INT16U          argc,
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

static  SHELL_CMD SMTPc_CmdTbl[] =
{
    {"smtp_send" , SMTPcCmd_Send},
    {"smtp_help" , SMTPcCmd_Help},
    {0, 0}
};


/*
*********************************************************************************************************
*                                           SMTPcCmd_Init()
*
* Description : Add Micrium OS SMTP Client cmd stubs to Micrium OS Shell.
*
* Argument(s) : p_err    is a pointer to an error code which will be returned to your application:
*
* Return(s)   : none.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  SMTPcCmd_Init (RTOS_ERR  *p_err)
{
    Shell_CmdTblAdd("smtp", SMTPc_CmdTbl, p_err);
}


/*
*********************************************************************************************************
*                                           SMTPc_Cmd_Send()
*
* Description : Send a predefine test e-mail.
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
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S SMTPcCmd_Send (CPU_INT16U        argc,
                          CPU_CHAR         *p_argv[],
                          SHELL_OUT_FNCT    out_fnct,
                          SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16S       output;
    CPU_INT16S       ret_val;
    CPU_CHAR        *p_server_addr;
    CPU_CHAR        *p_to_addr;
    CPU_INT16U       error_code;
    CPU_CHAR         reply_buf[16];
    CPU_INT16U       cmd_namd_len;
    CPU_INT08U       i;
    SMTPc_MSG       *p_msg         = DEF_NULL;
    RTOS_ERR         err;


    RTOS_ERR_SET(err, RTOS_ERR_NONE);

    p_to_addr     = SMTPc_CMD_MAILBOX_TO_ADDR;
    p_server_addr = SMTPc_CMD_SERVER_IPV4;
                                                                /* Parse Arguments.                                     */
    if (argc != 0) {
        for (i = 1; i < argc; i++) {
            if (*p_argv[i] == SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                switch (*(p_argv[i] + 1)) {
                    case SMTPc_CMD_PARSER_IPv6:
                         p_server_addr = SMTPc_CMD_SERVER_IPV6;

                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 p_server_addr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SMTPc_CMD_PARSER_IPv4:
                         p_server_addr = SMTPc_CMD_SERVER_IPV4;

                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 p_server_addr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SMTPc_CMD_PARSER_DNS:
                         p_server_addr = SMTPc_CMD_SERVER_DOMAIN_NAME;

                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 p_server_addr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SMTPc_CMD_PARSER_TO:
                          if (argc != i + 1) {
                              if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                  p_to_addr = p_argv[i+1];
                                  i++;
                              } else {
                                  RTOS_ERR_SET(err, RTOS_ERR_INVALID_ARG);
                                  goto exit;
                              }
                          } else {
                              RTOS_ERR_SET(err, RTOS_ERR_INVALID_ARG);
                              goto exit;
                          }
                          break;

                    default:
                         RTOS_ERR_SET(err, RTOS_ERR_INVALID_ARG);
                         goto exit;
                         break;

                }
            }
        }
    } else {
        RTOS_ERR_SET(err, RTOS_ERR_INVALID_ARG);
        goto exit;
    }


    p_msg = (SMTPc_MSG *)SMTPc_MsgAlloc(&err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    SMTPc_MsgSetParam(p_msg, SMTPc_FROM_ADDR, SMTPc_CMD_MAILBOX_FROM_ADDR, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    SMTPc_MsgSetParam(p_msg, SMTPc_FROM_DISPL_NAME, SMTPc_CMD_MAILBOX_FROM_NAME, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    SMTPc_MsgSetParam(p_msg, SMTPc_TO_ADDR, p_to_addr, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    SMTPc_MsgSetParam(p_msg, SMTPc_MSG_SUBJECT, SMTPc_CMD_MSG_SUBJECT, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    SMTPc_MsgSetParam(p_msg, SMTPc_MSG_BODY, SMTPc_CMD_MSG, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }


                                                                /* Send Mail.                                           */
    SMTPc_SendMail(p_server_addr,
                   0,
                   SMTPc_CMD_USERNAME,
                   SMTPc_CMD_PW,
                   DEF_NULL,
                   p_msg,
                  &err);
    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
        Str_Copy(&reply_buf[0], SMTPc_CMD_OK);
        cmd_namd_len = Str_Len(reply_buf);
    }

exit:
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        error_code = RTOS_ERR_CODE_GET(err);
        Str_Copy(&reply_buf[0], SMTPc_CMD_FAIL);
        cmd_namd_len = Str_Len(reply_buf);

        Str_FmtNbr_Int32U(error_code, 5, 10, ' ', DEF_YES, DEF_YES, &reply_buf[cmd_namd_len]);
        cmd_namd_len = Str_Len(reply_buf);
    }

    if (p_msg != DEF_NULL) {
        SMTPc_MsgFree(p_msg, &err);
    }

    reply_buf[cmd_namd_len + 0] = '\r';
    reply_buf[cmd_namd_len + 1] = '\n';
    reply_buf[cmd_namd_len + 2] = '\0';

    cmd_namd_len = Str_Len(reply_buf);

    output = out_fnct(&reply_buf[0],
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
*                                           SMTPc_Cmd_Help()
*
* Description : Print SMTPc command help.
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
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S SMTPcCmd_Help (CPU_INT16U        argc,
                          CPU_CHAR         *p_argv[],
                          SHELL_OUT_FNCT    out_fnct,
                          SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16U cmd_namd_len;
    CPU_INT16S output;
    CPU_INT16S ret_val;


    PP_UNUSED_PARAM(argc);
    PP_UNUSED_PARAM(p_argv);

    cmd_namd_len = Str_Len(SMTPc_CMD_HELP);
    output       = out_fnct(SMTPc_CMD_HELP,
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

#endif  /* RTOS_MODULE_NET_SMTP_CLIENT_AVAIL && RTOS_MODULE_COMMON_SHELL_AVAIL */

