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
*                                        NETWORK ASCII LIBRARY
*
* File : net_ascii.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_ASCII_H_
#define  _NET_ASCII_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>
#include  <rtos/net/include/net_ip.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_ASCII_CHAR_LEN_DOT                            1u
#define  NET_ASCII_CHAR_LEN_HYPHEN                         1u
#define  NET_ASCII_CHAR_LEN_COLON                          1u
#define  NET_ASCII_CHAR_LEN_NUL                            1u


/*
*********************************************************************************************************
*                                    NETWORK ASCII ADDRESS DEFINES
*
* Note(s) : (1) ONLY supports 48-bit Ethernet MAC addresses.
*********************************************************************************************************
*/

#define  NET_ASCII_NBR_OCTET_ADDR_MAC                      6u   /* See Note #1.                                         */
#define  NET_ASCII_CHAR_MAX_OCTET_ADDR_MAC                 2u

#define  NET_ASCII_LEN_MAX_ADDR_MAC                    ((NET_ASCII_NBR_OCTET_ADDR_MAC       * NET_ASCII_CHAR_MAX_OCTET_ADDR_MAC) + \
                                                       ((NET_ASCII_NBR_OCTET_ADDR_MAC - 1u) * NET_ASCII_CHAR_LEN_HYPHEN        ) + \
                                                         NET_ASCII_CHAR_LEN_NUL                                                )


                                                                /* IPv4 addresses.                                      */
#define  NET_ASCII_CHAR_MAX_OCTET_ADDR_IPv4                3u
#define  NET_ASCII_VAL_MAX_OCTET_ADDR_IPv4               255u

#define  NET_ASCII_NBR_OCTET_ADDR_IPv4           (sizeof(NET_IPv4_ADDR))


#define  NET_ASCII_LEN_MAX_ADDR_IPv4                   ((NET_ASCII_NBR_OCTET_ADDR_IPv4      * NET_ASCII_CHAR_MAX_OCTET_ADDR_IPv4) + \
                                                       ((NET_ASCII_NBR_OCTET_ADDR_IPv4 - 1) * NET_ASCII_CHAR_LEN_DOT            ) + \
                                                         NET_ASCII_CHAR_LEN_NUL                                                 )

                                                                /* IPv6 addresses.                                      */
#define  NET_ASCII_CHAR_MAX_OCTET_ADDR_IPv6                2u
#define  NET_ASCII_CHAR_MAX_DIG_ADDR_IPv6               (NET_ASCII_CHAR_MAX_OCTET_ADDR_IPv6 * 2)
#define  NET_ASCII_CHAR_MIN_COLON_IPv6                     2u
#define  NET_ASCII_CHAR_MAX_COLON_IPv6                     7u
#define  NET_ASCII_VAL_MAX_OCTET_ADDR_IPv6               255u

#define  NET_ASCII_NBR_OCTET_ADDR_IPv6           (sizeof(NET_IPv6_ADDR))


#define  NET_ASCII_LEN_MAX_ADDR_IPv6                   (((NET_ASCII_CHAR_MAX_DIG_ADDR_IPv6 * 4)      * NET_ASCII_CHAR_MAX_OCTET_ADDR_IPv6) + \
                                                       (((NET_ASCII_CHAR_MAX_DIG_ADDR_IPv6 * 2) - 1) * NET_ASCII_CHAR_LEN_COLON          ) + \
                                                          NET_ASCII_CHAR_LEN_NUL                                                         )

#ifdef   NET_IPv4_MODULE_EN
#undef   NET_ASCII_LEN_MAX_ADDR_IP
#define  NET_ASCII_LEN_MAX_ADDR_IP                        NET_ASCII_LEN_MAX_ADDR_IPv4
#endif

#ifdef   NET_IPv6_MODULE_EN
#undef   NET_ASCII_LEN_MAX_ADDR_IP
#define  NET_ASCII_LEN_MAX_ADDR_IP                        NET_ASCII_LEN_MAX_ADDR_IPv6
#endif

#define  NET_ASCII_LEN_MAX_PORT                           DEF_INT_16U_NBR_DIG_MAX


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

void                NetASCII_Str_to_MAC         (CPU_CHAR       *p_addr_mac_ascii,
                                                 CPU_INT08U     *p_addr_mac,
                                                 RTOS_ERR       *p_err);

void                NetASCII_MAC_to_Str         (CPU_INT08U     *p_addr_mac,
                                                 CPU_CHAR       *p_addr_mac_ascii,
                                                 CPU_BOOLEAN     hex_lower_case,
                                                 CPU_BOOLEAN     hex_colon_sep,
                                                 RTOS_ERR       *p_err);

NET_IP_ADDR_FAMILY  NetASCII_Str_to_IP          (CPU_CHAR       *p_addr_ip_ascii,
                                                 void           *p_addr,
                                                 CPU_INT08U      addr_max_len,
                                                 RTOS_ERR       *p_err);

NET_IPv4_ADDR       NetASCII_Str_to_IPv4        (CPU_CHAR       *p_addr_ip_ascii,
                                                 RTOS_ERR       *p_err);

NET_IPv6_ADDR       NetASCII_Str_to_IPv6        (CPU_CHAR       *p_addr_ip_ascii,
                                                 RTOS_ERR       *p_err);

void                NetASCII_IPv4_to_Str        (NET_IPv4_ADDR   addr_ip,
                                                 CPU_CHAR       *p_addr_ip_ascii,
                                                 CPU_BOOLEAN     lead_zeros,
                                                 RTOS_ERR       *p_err);

void                NetASCII_IPv6_to_Str        (NET_IPv6_ADDR  *p_addr_ip,
                                                 CPU_CHAR       *p_addr_ip_ascii,
                                                 CPU_BOOLEAN     hex_lower_case,
                                                 CPU_BOOLEAN     lead_zeros,
                                                 RTOS_ERR       *p_err);

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

#endif  /* _NET_ASCII_H_ */
