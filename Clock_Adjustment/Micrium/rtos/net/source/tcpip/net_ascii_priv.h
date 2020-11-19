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
*                                         NETWORK ASCII LIBRARY
*
* File : net_ascii_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_ASCII_PRIV_H_
#define  _NET_ASCII_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "../../include/net_cfg_net.h"
#include  "net_type_priv.h"

#include <rtos/cpu/include/cpu.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_ASCII_CHAR_MIN_OCTET                          1u

#define  NET_ASCII_CHAR_MAX_OCTET_08                     DEF_INT_08U_NBR_DIG_MAX
#define  NET_ASCII_CHAR_MAX_OCTET_16                     DEF_INT_16U_NBR_DIG_MAX
#define  NET_ASCII_CHAR_MAX_OCTET_32                     DEF_INT_32U_NBR_DIG_MAX

#define  NET_ASCII_STR_LOCAL_HOST                        "localhost"


/*
*********************************************************************************************************
*                                    NETWORK ASCII ADDRESS DEFINES
*
* Note(s) : (1) ONLY supports 48-bit Ethernet MAC addresses.
*********************************************************************************************************
*/


#define  NET_ASCII_NBR_MAX_DEC_PARTS                       4u
#define  NET_ASCII_CHAR_MAX_PART_ADDR_IP                 DEF_INT_32U_NBR_DIG_MAX
#define  NET_ASCII_VAL_MAX_PART_ADDR_IP                  DEF_INT_32U_MAX_VAL
#define  NET_ASCII_NBR_MAX_DOT_ADDR_IP                     3u

#define  NET_ASCII_MASK_32_01_BIT                      0xFFFFFFFFu
#define  NET_ASCII_MASK_24_01_BIT                      0x00FFFFFFu
#define  NET_ASCII_MASK_16_01_BIT                      0x0000FFFFu
#define  NET_ASCII_MASK_08_01_BIT                      0x000000FFu
#define  NET_ASCII_MASK_16_09_BIT                      0x0000FF00u
#define  NET_ASCII_MASK_24_17_BIT                      0x00FF0000u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

NET_IPv4_ADDR  NetASCII_Str_to_IPv4_Handler (CPU_CHAR       *p_addr_ip_ascii,
                                             CPU_INT08U     *p_dot_nbr,
                                             RTOS_ERR       *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_ASCII_PRIV_H_ */
