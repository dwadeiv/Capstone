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
*                                          NETWORK IGMP LAYER
*                                 (INTERNET GROUP MANAGEMENT PROTOCOL)
*
* File : net_igmp.h
*********************************************************************************************************
* Note(s) : (1) Internet Group Management Protocol ONLY required for network interfaces that require
*               reception of IP class-D (multicast) packets (see RFC #1112, Section 3 'Levels of
*               Conformance : Level 2').
*
*               (a) IGMP is NOT required for the transmission of IP class-D (multicast) packets
*                   (see RFC #1112, Section 3 'Levels of Conformance : Level 1').
*
*           (2) Supports Internet Group Management Protocol version 1, as described in RFC #1112
*               with the following restrictions/constraints :
*
*               (a) Only one socket may receive datagrams for a specific host group address & port
*                   number at any given time.
*
*               (b) Since sockets do NOT automatically leave IGMP host groups when closed,
*                   it is the application's responsibility to leave each host group once it is
*                   no longer needed by the application.
*
*               (c) Transmission of IGMP Query Messages NOT currently supported.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_IGMP_H_
#define  _NET_IGMP_H_

#include  "net_cfg_net.h"

#ifdef   NET_IGMP_MODULE_EN


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

                                                                    /* ------------- GRP MEMBERSHIP FNCTS ------------- */
CPU_BOOLEAN  NetIGMP_HostGrpJoin        (NET_IF_NBR      if_nbr,
                                         NET_IPv4_ADDR   addr_grp,
                                         RTOS_ERR       *p_err);

CPU_BOOLEAN  NetIGMP_HostGrpLeave       (NET_IF_NBR      if_nbr,
                                         NET_IPv4_ADDR   addr_grp,
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

#endif /* NET_IGMP_MODULE_EN */
#endif /* _NET_IGMP_H_       */
