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
*                               NETWORK SHELL COMMAND OUTPUT UTILITIES
*
* File : net_cmd_output_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_CMD_OUTPUT_PRIV_H_
#define  _NET_CMD_OUTPUT_PRIV_H_

#include  <rtos_description.h>
#ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/shell.h>
#include  <rtos/common/include/rtos_err.h>
#include  <rtos/net/include/net_stat.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  enum  net_cmd_output_fmt {
    NET_CMD_OUTPUT_FMT_NONE,
    NET_CMD_OUTPUT_FMT_STRING,
    NET_CMD_OUTPUT_FMT_HEX
} NET_CMD_OUTPUT_FMT;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_INT16S  NetCmd_OutputBeginning     (SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputEnd           (SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputCmdTbl        (SHELL_CMD           *p_cmd_tbl,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputNotImplemented(SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputCmdArgInvalid (SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputError         (CPU_CHAR            *p_error,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputErrorStr      (RTOS_ERR             err,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputSuccess       (SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputInt32U        (CPU_INT32U           nbr,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputSockID        (CPU_INT16S           sock_id,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);


CPU_INT16S  NetCmd_OutputMsg           (CPU_CHAR            *p_msg,
                                        CPU_BOOLEAN          new_line_start,
                                        CPU_BOOLEAN          new_line_end,
                                        CPU_BOOLEAN          tab_start,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputData          (CPU_INT08U          *p_buf,
                                        CPU_INT16U           len,
                                        NET_CMD_OUTPUT_FMT   out_fmt,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputNetStat       (NET_STAT_POOL       *p_net_stat,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);

CPU_INT16S  NetCmd_OutputStatVal       (CPU_CHAR            *p_stat_title,
                                        CPU_INT32U           stat_val,
                                        SHELL_OUT_FNCT       out_fnct,
                                        SHELL_CMD_PARAM     *p_cmd_param);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_COMMON_SHELL_AVAIL */
#endif  /* _NET_CMD_OUTPUT_PRIV_H_        */

