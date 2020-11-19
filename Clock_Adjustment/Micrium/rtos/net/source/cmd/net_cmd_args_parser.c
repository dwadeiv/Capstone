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
*                             NETWORK COMMAND ARGUMENT PARSING UTILITIES
*
* File : net_cmd_args_parser.c
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


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_cfg_net.h>
#include  <rtos/net/include/net_def.h>
#include  <rtos/net/include/net_ascii.h>
#include  <rtos/net/include/net_if.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/dhcp_client_types.h>
#include  <rtos/net/source/tcpip/net_if_802x_priv.h>
#include  <rtos/net/source/cmd/net_cmd_priv.h>
#include  <rtos/net/source/cmd/net_cmd_args_parser_priv.h>
#include  <rtos/net/source/cmd/net_cmd_output_priv.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_ascii.h>
#include  <rtos/common/include/lib_str.h>
#include  <rtos/common/include/toolchains.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_CMD_ARG_PARSER_CMD_BEGIN              ASCII_CHAR_HYPHEN_MINUS

#define  NET_CMD_ARG_PARSER_CMD_WINDOWS_IF_NBR     ASCII_CHAR_LATIN_LOWER_W
#define  NET_CMD_ARG_PARSER_CMD_IPv4_ADDR_CFG      ASCII_CHAR_DIGIT_FOUR
#define  NET_CMD_ARG_PARSER_CMD_IPv6_ADDR_CFG      ASCII_CHAR_DIGIT_SIX
#define  NET_CMD_ARG_PARSER_CMD_MAC_ADDR_CFG       ASCII_CHAR_LATIN_LOWER_M
#define  NET_CMD_ARG_PARSER_CMD_ID_CFG             ASCII_CHAR_LATIN_LOWER_I
#define  NET_CMD_ARG_PARSER_CMD_TELNET_CFG         ASCII_CHAR_LATIN_LOWER_T
#define  NET_CMD_ARG_PARSER_CMD_DHCPc_CFG          ASCII_CHAR_LATIN_LOWER_D


static  NET_CMD_STR_ARGS  NetCmd_ArgsParserParse (CPU_INT16U   argc,
                                                  CPU_CHAR    *p_argv[],
                                                  RTOS_ERR    *p_err);

#if 0
static  NET_CMD_ARGS  NetCmd_ArgsParserTranslate (NET_CMD_STR_ARGS   cmd_args,
                                                  RTOS_ERR          *p_err);
#endif


/*
*********************************************************************************************************
*                                       NetCmd_ArgsParserCmdParse()
*
* Description : Parse command line arguments.
*
* Argument(s) : argc    is a count of the arguments supplied.
*
*               p_argv  an array of pointers to the strings which are those arguments.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Return Network command arguments parsed.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_CMD_STR_ARGS  NetCmd_ArgsParserCmdParse (CPU_INT16U   argc,
                                             CPU_CHAR    *p_argv[],
                                             RTOS_ERR    *p_err)
{
    NET_CMD_STR_ARGS  cmd_args;
#if 0
    NET_CMD_ARGS      args;


    args.IPv4_CfgEn = DEF_NO;
    args.IPv6_CfgEn = DEF_NO;
#endif

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    NET_CMD_ARGS_PARSER_CMD_ARGS_INIT(cmd_args);

    NET_CMD_ARGS_PARSER_CMD_ARGS_INIT(cmd_args);
    if (argc <= 1) {
        return (cmd_args);
    }

    cmd_args = NetCmd_ArgsParserParse(argc, p_argv, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return (cmd_args);
    }

#if 0
    args = NetCmd_ArgsParserTranslate(cmd_args, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         return (args);
    }
#endif

    return (cmd_args);
}


/*
*********************************************************************************************************
*                                    NetCmd_ArgsParserParseID_Nbr()
*
* Description : Validate that the argument value is an interface number.
*
* Argument(s) : p_argv          an array of pointers to the strings which are those arguments.
*
*               p_str_if_nbr    Pointer which will receive the location of the Interface values to convert.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_OK, Argument is an interface number.
*               DEF_FAIL, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseID_Nbr (CPU_CHAR   *p_argv[],
                                          CPU_CHAR  **p_str_if_nbr,
                                          RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  dig;


   *p_str_if_nbr = p_argv[1];
    dig = ASCII_IS_DIG(**p_str_if_nbr);
    if (dig == DEF_NO) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (DEF_FAIL);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  NetCmd_ArgsParserTranslateID_Nbr()
*
* Description : Translate Interface number argument.
*
* Argument(s) : p_str_if_nbr    String that contains the interface number.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Interface number translated, if successfully converted,
*               NET_IF_NBR_NONE, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16U  NetCmd_ArgsParserTranslateID_Nbr (CPU_CHAR  *p_str_if_nbr,
                                              RTOS_ERR  *p_err)
{
    CPU_INT16U  id;


    id = NET_IF_NBR_NONE;

    if (p_str_if_nbr != DEF_NULL) {
        id = Str_ParseNbr_Int32U(p_str_if_nbr, DEF_NULL, DEF_NBR_BASE_DEC);

    } else {
        id = NET_IF_NBR_NONE;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (id);
}


/*
*********************************************************************************************************
*                                     NetCmd_ArgsParserParseIPv4()
*
* Description : Validate and local argument values of an IPv4 address configuration structure.
*
* Argument(s) : p_argv    an array of pointers to the strings which are those arguments
*
*               p_ip_cfg  Pointer to a variable that will receive the location of each IPv4 configuration field
*                         to be converted.
*
*               p_err     Pointer to variable that will receive the return error code from this function
*
* Return(s)   : 3, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseIPv4 (CPU_CHAR                *p_argv[],
                                        NET_IF_IPv4_STATIC_CFG  *p_ip_cfg,
                                        RTOS_ERR                *p_err)
{
    CPU_BOOLEAN  dig;


    dig = ASCII_IS_DIG_HEX(*p_argv[1]);
    if (dig == DEF_YES) {
        p_ip_cfg->Addr = p_argv[1];
    } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    dig = ASCII_IS_DIG_HEX(*p_argv[2]);
    if (dig == DEF_YES) {
        p_ip_cfg->Mask = p_argv[2];
    } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    dig = ASCII_IS_DIG_HEX(*p_argv[3]);
    if (dig == DEF_YES) {
        p_ip_cfg->Gateway = p_argv[3];
    } else {
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
         return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    return (3);
}


/*
*********************************************************************************************************
*                                   NetCmd_ArgsParserTranslateIPv4()
*
* Description : Translate IPv4 argument value to an IPv4 address configuration structure.
*
* Argument(s) : p_ip_cfg    Structure that contains IPv4 addresses to convert.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Address configuration structure.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_CMD_IPv4_CFG  NetCmd_ArgsParserTranslateIPv4 (NET_IF_IPv4_STATIC_CFG  *p_ip_cfg,
                                                  RTOS_ERR                *p_err)
{
    NET_CMD_IPv4_CFG  ip_cfg;
    RTOS_ERR          local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);


    ip_cfg.Host = NetASCII_Str_to_IPv4((CPU_CHAR *)p_ip_cfg->Addr, &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (ip_cfg);
    }


    ip_cfg.Mask = NetASCII_Str_to_IPv4((CPU_CHAR *)p_ip_cfg->Mask, &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (ip_cfg);
    }


    ip_cfg.Gateway = NetASCII_Str_to_IPv4((CPU_CHAR *)p_ip_cfg->Gateway, &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (ip_cfg);
    }

     RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);


    return (ip_cfg);
}


/*
*********************************************************************************************************
*                                     NetCmd_ArgsParserParseIPv6()
*
* Description : Validate and local argument values of an IPv6 address configuration structure.
*
* Argument(s) : p_argv    an array of pointers to the strings which are those arguments
*
*               p_ip_cfg  Pointer to a variable that will receive the location of each IPv6 configuration field
*                         to be converted.
*
*               p_err     Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : 2, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseIPv6 (CPU_CHAR                *p_argv[],
                                        NET_IF_IPv6_STATIC_CFG  *p_ip_cfg,
                                        RTOS_ERR                *p_err)
{
    CPU_BOOLEAN  dig_hex;


    dig_hex = ASCII_IS_DIG_HEX(*p_argv[1]);
    if (dig_hex == DEF_YES) {
        p_ip_cfg->Addr = p_argv[1];
    } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    dig_hex = ASCII_IS_DIG_HEX(*p_argv[2]);
    if (dig_hex == DEF_YES) {
        p_ip_cfg->PrefixLen = Str_ParseNbr_Int32U(p_argv[2], DEF_NULL, DEF_NBR_BASE_DEC);
    } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    return (2);
}


/*
*********************************************************************************************************
*                                   NetCmd_ArgsParserTranslateIPv6()
*
* Description : Translate IPv6 argument value to an IPv6 address configuration structure.
*
* Argument(s) : p_ip_cfg    Structure that contains IPv4 addresses to convert.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Address configuration structure.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_CMD_IPv6_CFG  NetCmd_ArgsParserTranslateIPv6 (NET_IF_IPv6_STATIC_CFG  *p_ip_cfg,
                                                  RTOS_ERR                *p_err)
{
    NET_CMD_IPv6_CFG  ip_cfg;
    RTOS_ERR          local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    ip_cfg.Host = NetASCII_Str_to_IPv6((CPU_CHAR *)p_ip_cfg->Addr, &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (ip_cfg);
    }

    ip_cfg.PrefixLen = p_ip_cfg->PrefixLen;


    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (ip_cfg);
}


/*
*********************************************************************************************************
*                                     NetCmd_ArgsParserParseMAC()
*
* Description : Validate and local argument values of an MAC address configuration structure.
*
* Argument(s) : p_argv    an array of pointers to the strings which are those arguments
*
*               p_ip_cfg  Pointer to a variable that will receive the location of each IPv6 configuration field
*                         to be converted.
*
*               p_err     Pointer to variable that will receive the return error code from this function
*
* Return(s)   : 1, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseMAC (CPU_CHAR   *p_argv[],
                                       CPU_CHAR  **p_str_mac,
                                       RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  dig_hex;


    dig_hex = ASCII_IS_DIG_HEX(*p_argv[1]);
    if (dig_hex == DEF_YES) {
       *p_str_mac = p_argv[1];
    } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (1);
}


/*
*********************************************************************************************************
*                                   NetCmd_ArgsParserTranslateMAC()
*
* Description : Translate MAC address argument value to a MAC address configuration structure.
*
* Argument(s) : p_str_mac    String that contains MAC address to convert.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : MAC Address configuration structure.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_CMD_MAC_CFG  NetCmd_ArgsParserTranslateMAC (CPU_CHAR  *p_str_mac,
                                                RTOS_ERR  *p_err)
{
    NET_CMD_MAC_CFG  mac_cfg;
#if 0
    RTOS_ERR         local_err;
#endif

    mac_cfg.MAC_AddrStr = p_str_mac;
#if 0
    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    NetASCII_Str_to_MAC(                p_str_mac,
                        (CPU_INT08U  *)&mac_cfg.MAC_Addr,
                                       &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (mac_cfg);
    }
#endif
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (mac_cfg);
}


/*
*********************************************************************************************************
*                                    NetCmd_ArgsParserParseDataLen()
*
* Description : Validate and local argument values of data length.
*
* Argument(s) : p_argv    an array of pointers to the strings which are those arguments
*
*               p_ip_cfg  Pointer to a string will receive the location of the argument value.
*
*               p_err     Pointer to variable that will receive the return error code from this function
*
* Return(s)   : 1, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseDataLen (CPU_CHAR   *p_argv[],
                                           CPU_CHAR  **p_str_len,
                                           RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  dig;


   *p_str_len = p_argv[1];
    dig = ASCII_IS_DIG(**p_str_len);
    if (dig == DEF_NO) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (1);
}


/*
*********************************************************************************************************
*                                  NetCmd_ArgsParserTranslateDataLen()
*
* Description : Translate MAC address argument value to a MAC address configuration structure.
*
* Argument(s) : p_str_mac    String that contains MAC length to convert.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Length converted.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16U  NetCmd_ArgsParserTranslateDataLen (CPU_CHAR  *p_str_len,
                                               RTOS_ERR  *p_err)
{
    CPU_INT16U  data_len;


    if (p_str_len != DEF_NULL) {
        data_len = Str_ParseNbr_Int32U(p_str_len, DEF_NULL, DEF_NBR_BASE_DEC);
    } else {
        data_len = 0;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (data_len);
}


/*
*********************************************************************************************************
*                                    NetCmd_ArgsParserParseDataLen()
*
* Description : Validate and local argument values of data length.
*
* Argument(s) : p_argv    an array of pointers to the strings which are those arguments
*
*               p_ip_cfg  Pointer to a string will receive the location of the argument value.
*
*               p_err     Pointer to variable that will receive the return error code from this function
*
* Return(s)   : 1, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  NetCmd_ArgsParserTranslateVal32U (CPU_CHAR  *p_str_len,
                                              RTOS_ERR  *p_err)
{
    CPU_INT32U  val;


    if (p_str_len != DEF_NULL) {
        val = Str_ParseNbr_Int32U(p_str_len, DEF_NULL, DEF_NBR_BASE_DEC);
    } else {
        val = 0;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (val);
}

/*
*********************************************************************************************************
*                                    NetCmd_ArgsParserParseDataLen()
*
* Description : Validate and local argument values of data length.
*
* Argument(s) : p_argv    an array of pointers to the strings which are those arguments
*
*               p_ip_cfg  Pointer to a string will receive the location of the argument value.
*
*               p_err     Pointer to variable that will receive the return error code from this function
*
* Return(s)   : 1, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseFmt (CPU_CHAR   *p_argv[],
                                       CPU_CHAR  **p_str_len,
                                       RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  char_val;


   *p_str_len = p_argv[1];

   char_val = ASCII_IS_PRINT(**p_str_len);
    if (char_val == DEF_NO) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (1);
}


/*
*********************************************************************************************************
*                                NetCmd_ArgsParserTranslateSockFamily()
*
* Description : Translate Interface number argument.
*
* Argument(s) : p_str_if_nbr    String that contains the interface number.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Interface number translated, if successfully converted,
*               NET_IF_NBR_NONE, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_CMD_OUTPUT_FMT  NetCmd_ArgsParserTranslateFmt (CPU_CHAR  *p_char_type,
                                                   RTOS_ERR  *p_err)
{
    NET_CMD_OUTPUT_FMT  fmt;


    switch (*p_char_type) {
        case 'h':
        default:
             fmt = NET_CMD_OUTPUT_FMT_HEX;
             break;

        case 's':
             fmt = NET_CMD_OUTPUT_FMT_STRING;
             break;
    }


    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (fmt);
}


/*
*********************************************************************************************************
*                                  NetCmd_ArgsParserParseCredential()
*
* Description : Validate and local argument values of credential structure.
*
* Argument(s) : p_argv          an array of pointers to the strings which are those arguments.
*
*               p_credential    Pointer to a credential arguments values.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : 2, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseCredential (CPU_CHAR          *p_argv[],
                                              NET_CMD_AUTH_CFG  *p_credential,
                                              RTOS_ERR          *p_err)
{
    p_credential->User     = p_argv[1];
    p_credential->Password = p_argv[2];

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (2);
}


/*
*********************************************************************************************************
*                                NetCmd_ArgsParserTranslateCredential()
*
* Description : Translate credential argument values to a Credential configuration structure.
*
* Argument(s) : p_credential    Structure that contains credential values.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Length converted.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_CMD_CREDENTIAL_CFG  NetCmd_ArgsParserTranslateCredential (NET_CMD_AUTH_CFG  *p_credential,
                                                              RTOS_ERR          *p_err)
{
    NET_CMD_CREDENTIAL_CFG  credential_cfg;


    if ((p_credential->User     != DEF_NULL) &&
        (p_credential->Password != DEF_NULL)) {
        Str_Copy_N(credential_cfg.User,     p_credential->User,     NET_CMD_STR_USER_MAX_LEN);
        Str_Copy_N(credential_cfg.Password, p_credential->Password, NET_CMD_STR_PASSWORD_MAX_LEN);

    } else {
        credential_cfg.User[0]     = ASCII_CHAR_NULL;
        credential_cfg.Password[0] = ASCII_CHAR_NULL;
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (credential_cfg);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (credential_cfg);
}


/*
*********************************************************************************************************
*                                   NetCmd_ArgsParserParseMTU()
*
* Description : Validate and local argument values of MTU.
*
* Argument(s) : p_argv     an array of pointers to the strings which are those arguments
*
*               p_str_len  Pointer to a string will receive the location of the argument value.
*
*               p_err      Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : 1, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseMTU (CPU_CHAR   *p_argv[],
                                       CPU_CHAR  **p_str_len,
                                       RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  dig;


   *p_str_len = p_argv[1];
    dig = ASCII_IS_DIG(**p_str_len);
    if (dig == DEF_NO) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (1);
}


/*
*********************************************************************************************************
*                                  NetCmd_ArgsParserTranslateMTU()
*
* Description : Translate MTU argument value.
*
* Argument(s) : p_str_mac    String that contains MTU value to convert.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : MTU converted.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16U  NetCmd_ArgsParserTranslateMTU (CPU_CHAR  *p_str_len,
                                           RTOS_ERR  *p_err)
{
    CPU_INT16U  mtu = 0;


#ifdef NET_IF_802x_MODULE_EN
    if (p_str_len != DEF_NULL) {
        mtu = Str_ParseNbr_Int32U(p_str_len, DEF_NULL, DEF_NBR_BASE_DEC);

    } else {
        mtu = NET_IF_MTU_ETHER;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
#else

    RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_AVAIL);
#endif

    return (mtu);
}


/*
*********************************************************************************************************
*                                   NetCmd_ArgsParserParseMTU()
*
* Description : Validate and convert argument values of MTU.
*
* Argument(s) : p_argv     an array of pointers to the strings which are those arguments
*
*               p_str_len  Pointer to a string will receive the location of the argument value.
*
*               p_err      Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : 1, if no error.
*               0, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseSockFamily (CPU_CHAR   *p_argv[],
                                              CPU_CHAR  **p_str_family,
                                              RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  dig;


   *p_str_family = p_argv[0];
    dig = ASCII_IS_DIG(**p_str_family);
    if (dig == DEF_NO) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
         return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (1);
}


/*
*********************************************************************************************************
*                                NetCmd_ArgsParserTranslateSockFamily()
*
* Description : Translate Interface number argument.
*
* Argument(s) : p_str_if_nbr    String that contains the interface number.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Interface number translated, if successfully converted,
*               NET_IF_NBR_NONE, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_SOCK_PROTOCOL_FAMILY  NetCmd_ArgsParserTranslateSockFamily (CPU_CHAR  *p_char_family,
                                                                RTOS_ERR  *p_err)
{
    NET_SOCK_PROTOCOL_FAMILY  family = NET_SOCK_PROTOCOL_FAMILY_NONE;


    switch (*p_char_family) {
        case '4':
             family = NET_SOCK_PROTOCOL_FAMILY_IP_V4;
             break;

        case '6':
            family = NET_SOCK_PROTOCOL_FAMILY_IP_V6;
            break;

        default:
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
            goto exit;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

exit:
    return (family);
}


/*
*********************************************************************************************************
*                                   NetCmd_ArgsParserParseSockType()
*
* Description : Validate and convert argument values of sokcet type.
*
* Argument(s) : p_argv     an array of pointers to the strings which are those arguments
*
*               p_str_len  Pointer to a string will receive the location of the argument value.
*
*               p_err      Pointer to variable that will receive the return error code from this function
*
* Return(s)   : Socket type
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  NetCmd_ArgsParserParseSockType (CPU_CHAR   *p_argv[],
                                            CPU_CHAR  **p_str_len,
                                            RTOS_ERR   *p_err)
{
    CPU_BOOLEAN  char_val;


   *p_str_len = p_argv[0];

   char_val = ASCII_IS_PRINT(**p_str_len);
    if (char_val == DEF_NO) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
        return (0);
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (1);
}


/*
*********************************************************************************************************
*                                NetCmd_ArgsParserTranslateSockFamily()
*
* Description : Translate Interface number argument.
*
* Argument(s) : p_str_if_nbr    String that contains the interface number.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Interface number translated, if successfully converted,
*               NET_IF_NBR_NONE, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_SOCK_TYPE  NetCmd_ArgsParserTranslateSockType (CPU_CHAR  *p_char_type,
                                                   RTOS_ERR  *p_err)
{
    NET_SOCK_TYPE  type = NET_SOCK_TYPE_NONE;


    switch (*p_char_type) {
        case 's':
             type = NET_SOCK_TYPE_STREAM;
             break;

        case 'd':
             type = NET_SOCK_TYPE_DATAGRAM;
             break;

        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
             goto exit;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

exit:
    return (type);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       NetCmd_ArgsParserParse()
*
* Description : Parse command line arguments.
*
* Argument(s) : argc    is a count of the arguments supplied.
*
*               p_argv  an array of pointers to the strings which are those arguments.
*
*               p_err   Pointer to variable that will receive the return error code from this function
*
* Return(s)   : Network configuration structure.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  NET_CMD_STR_ARGS  NetCmd_ArgsParserParse (CPU_INT16U   argc,
                                                  CPU_CHAR    *p_argv[],
                                                  RTOS_ERR    *p_err)
{
    NET_CMD_STR_ARGS  cmd_arg;
    CPU_INT16U        i;
    CPU_INT16U        ix_start;


    NET_CMD_ARGS_PARSER_CMD_ARGS_INIT(cmd_arg);
    if (*p_argv[0] == NET_CMD_ARG_PARSER_CMD_BEGIN) {
         ix_start = 0;
    } else {
         ix_start = 1;
    }

    if (*p_argv[ix_start] == ASCII_CHAR_APOSTROPHE) {
        for (i = ix_start; i < argc; i++) {
            CPU_INT32U   len;
            CPU_CHAR    *p_char;


            len       = Str_Len(p_argv[i]);
            p_char    = p_argv[i] + len - 1;
           *p_char    = ASCII_CHAR_NULL;
            p_char    = p_argv[i] + 1;
            p_argv[i] = p_char;
        }

    }

    for (i = ix_start; i < argc; i++) {
        if (*p_argv[i] == NET_CMD_ARG_PARSER_CMD_BEGIN) {
            switch (*(p_argv[i] + 1)) {
                case NET_CMD_ARG_PARSER_CMD_WINDOWS_IF_NBR:
                     i += NetCmd_ArgsParserParseID_Nbr(&p_argv[i], &cmd_arg.WindowsIF_Nbr_Ptr, p_err);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          return (cmd_arg);
                     }
                     break;


                case NET_CMD_ARG_PARSER_CMD_IPv4_ADDR_CFG:
                     i += NetCmd_ArgsParserParseIPv4(&p_argv[i], &cmd_arg.IF_Cfg.IPv4.Static, p_err);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          return (cmd_arg);
                     }
                     break;


                case NET_CMD_ARG_PARSER_CMD_IPv6_ADDR_CFG:
                     i += NetCmd_ArgsParserParseIPv6(&p_argv[i], &cmd_arg.IF_Cfg.IPv6.Static, p_err);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          return (cmd_arg);
                     }
                     break;


                case NET_CMD_ARG_PARSER_CMD_MAC_ADDR_CFG:
                     i += NetCmd_ArgsParserParseMAC(&p_argv[i], (CPU_CHAR **)&cmd_arg.IF_Cfg.HW_AddrStr, p_err);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          return (cmd_arg);
                     }
                     break;

                case NET_CMD_ARG_PARSER_CMD_DHCPc_CFG:
                     cmd_arg.IF_Cfg.IPv4.DHCPc.En  = DEF_YES;
                     DHCPc_CFG_DFLT_INIT(cmd_arg.IF_Cfg.IPv4.DHCPc.Cfg);
                     break;

                case NET_CMD_ARG_PARSER_CMD_ID_CFG:
                     i += NetCmd_ArgsParserParseCredential(&p_argv[i], &cmd_arg.AuthCfg, p_err);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          return (cmd_arg);
                     }
                     break;


                default:
                     RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
                     return (cmd_arg);
            }
        } else {
             RTOS_ERR_SET(*p_err,  RTOS_ERR_INVALID_ARG);
             return (cmd_arg);
        }
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (cmd_arg);
}


/*
*********************************************************************************************************
*                                     NetCmd_ArgsParserTranslate()
*
* Description : Translate argument values.
*
* Argument(s) : cmd_args    Argument string values structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Network configuration structure.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if 0
static  NET_CMD_ARGS  NetCmd_ArgsParserTranslate (NET_CMD_STR_ARGS   cmd_args,
                                                  RTOS_ERR           *p_err)
{
    NET_CMD_ARGS  args;


    if (cmd_args.WindowsIF_Nbr_Ptr != DEF_NULL) {
        args.WindowsIF_Nbr = NetCmd_ArgsParserTranslateID_Nbr(cmd_args.WindowsIF_Nbr_Ptr, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             return (args);
        }

    } else {
        args.WindowsIF_Nbr = 0;
    }

#ifdef  NET_IPv4_MODULE_EN
    if (cmd_args.AddrIPv4.HostPtr != DEF_NULL) {
        args.IPv4 = NetCmd_ArgsParserTranslateIPv4(&cmd_args.AddrIPv4, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return (args);
        }

        args.IPv4_CfgEn = DEF_YES;

    } else {
        args.IPv4_CfgEn = DEF_NO;
    }
#endif

#ifdef  NET_IPv6_MODULE_EN
    if (cmd_args.AddrIPv6.HostPtr != DEF_NULL) {
        args.IPv6 = NetCmd_ArgsParserTranslateIPv6(&cmd_args.AddrIPv6, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return (args);
        }
        args.IPv6_CfgEn = DEF_YES;

    } else {
        args.IPv6_CfgEn = DEF_NO;
    }
#endif


    if (cmd_args.MAC_Addr_Ptr != DEF_NULL) {
        args.MAC_CfgEn = DEF_YES;
        args.MAC_Addr = NetCmd_ArgsParserTranslateMAC(cmd_args.MAC_Addr_Ptr, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             return (args);
        }
    } else {
        args.MAC_CfgEn = DEF_NO;
    }



    if (cmd_args.AuthCfg.User != DEF_NULL) {
        args.Credential_CfgEn = DEF_YES;
        args.AuthCfg = NetCmd_ArgsParserTranslateCredential(&cmd_args.AuthCfg, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             return (args);
        }

    } else {
        args.Credential_CfgEn       = DEF_NO;
        args.AuthCfg.User[0]     = DEF_NULL;
        args.AuthCfg.Password[0] = DEF_NULL;
    }


    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (args);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL && RTOS_MODULE_COMMON_SHELL_AVAIL */
