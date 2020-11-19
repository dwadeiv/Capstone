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
*                                          NETWORK NDP LAYER
*                                    (NEIGHBOR DISCOVERY PROTOCOL)
*
* File : net_ndp.h
*********************************************************************************************************
* Note(s) : (1) Supports Neighbor Discovery Protocol as described in RFC #2461 with the
*               following restrictions/constraints :
*
*               (a) ONLY supports the following hardware types :
*                   (1) 48-bit Ethernet
*
*               (b) ONLY supports the following protocol types :
*                   (1) 128-bit IPv6 addresses
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_NDP_H_
#define  _NET_NDP_H_

#include  "net_cfg_net.h"

#ifdef   NET_NDP_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>

#include  <rtos/cpu/include/cpu.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        NDP TIMEOUT DATA TYPE
*********************************************************************************************************
*/

typedef enum net_ndp_timeout {
    NET_NDP_TIMEOUT_REACHABLE,
    NET_NDP_TIMEOUT_DELAY,
    NET_NDP_TIMEOUT_SOLICIT,
} NET_NDP_TIMEOUT;


/*
*********************************************************************************************************
*                                  NDP SOLICITATION MESSAGE DATA TYPE
*********************************************************************************************************
*/

typedef enum net_ndp_solicit {
    NET_NDP_SOLICIT_MULTICAST,
    NET_NDP_SOLICIT_UNICAST,
    NET_NDP_SOLICIT_DAD,
} NET_NDP_SOLICIT;


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

                                                                /* --------- NDP NEIGHBOR CACHE CFG FUNCTIONS --------- */

CPU_BOOLEAN          NetNDP_CfgNeighborCacheTimeout  (CPU_INT16U        timeout_sec);

CPU_BOOLEAN          NetNDP_CfgReachabilityTimeout   (NET_NDP_TIMEOUT   timeout_type,
                                                      CPU_INT16U        timeout_sec);

CPU_BOOLEAN          NetNDP_CfgSolicitMaxNbr         (NET_NDP_SOLICIT   solicit_type,
                                                      CPU_INT08U        max_nbr);

CPU_BOOLEAN          NetNDP_CfgCacheTxQ_MaxTh        (NET_BUF_QTY       nbr_buf_max);
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

#endif  /* NET_NDP_MODULE_EN */
#endif  /* _NET_NDP_H_       */
