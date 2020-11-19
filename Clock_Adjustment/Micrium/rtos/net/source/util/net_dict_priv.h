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
*                                          NETWORK DICTIONARY
*
* File : net_dict_priv.h
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_DICT_PRIV_H_
#define  _NET_DICT_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/lib_def.h>
#include  <rtos/cpu/include/cpu.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  CPU_INT32U  DICT_KEY;

#define  DICT_KEY_INVALID                       DEF_INT_32U_MAX_VAL

typedef  struct  dict {
    const  DICT_KEY        Key;
    const  CPU_CHAR       *StrPtr;
    const  CPU_INT32U      StrLen;
} NET_DICT;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_INT32U   NetDict_KeyGet    (const  NET_DICT    *p_dict_tbl,
                                       CPU_INT32U   dict_size,
                                const  CPU_CHAR    *p_str_cmp,
                                       CPU_BOOLEAN  case_sensitive,
                                       CPU_INT32U   str_len);

NET_DICT    *NetDict_EntryGet  (const  NET_DICT    *p_dict_tbl,
                                      CPU_INT32U    dict_size,
                                      CPU_INT32U    key);

CPU_CHAR    *NetDict_ValCopy   (const  NET_DICT    *p_dict_tbl,
                                       CPU_INT32U   dict_size,
                                       CPU_INT32U   key,
                                       CPU_CHAR    *p_buf,
                                       CPU_SIZE_T   buf_len);

CPU_CHAR    *NetDict_StrKeySrch(const  NET_DICT    *p_dict_tbl,
                                       CPU_INT32U   dict_size,
                                       CPU_INT32U   key,
                                const  CPU_CHAR    *p_str,
                                       CPU_SIZE_T   str_len);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* _NET_DICT_PRIV_H_ */
