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
*                                          NETWORK MLDP LAYER
*                                (MULTICAST LISTENER DISCOVERY PROTOCOL)
*
* File : net_mldp.h
*********************************************************************************************************
* Note(s) : (1) Supports Neighbor Discovery Protocol as described in RFC #2710.
*
*           (2) Only the MLDPv1 is supported. MLDPv2 as described in RFC #3810 is not yet
*               supported.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_MLDP_H_
#define  _NET_MLDP_H_

#include  "net_cfg_net.h"

#ifdef   NET_MLDP_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_err.h>


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

CPU_BOOLEAN         NetMLDP_HostGrpJoin   (NET_IF_NBR         if_nbr,
                                           NET_IPv6_ADDR     *p_addr,
                                           RTOS_ERR          *p_err);

CPU_BOOLEAN         NetMLDP_HostGrpLeave  (NET_IF_NBR         if_nbr,
                                           NET_IPv6_ADDR     *p_addr,
                                           RTOS_ERR          *p_err);


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

#endif  /* NET_MLDP_MODULE_EN */
#endif  /* _NET_MLDP_H_       */

