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
*                                       NETWORK BUFFER MANAGEMENT
*
* File : net_buf.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_BUF_H_
#define  _NET_BUF_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_stat.h>
#include  <rtos/net/include/net_type.h>


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

NET_STAT_POOL  NetBuf_PoolStatGet                  (NET_IF_NBR        if_nbr);

void           NetBuf_PoolStatResetMaxUsed         (NET_IF_NBR        if_nbr);


NET_STAT_POOL  NetBuf_RxLargePoolStatGet           (NET_IF_NBR        if_nbr);

void           NetBuf_RxLargePoolStatResetMaxUsed  (NET_IF_NBR        if_nbr);


NET_STAT_POOL  NetBuf_TxLargePoolStatGet           (NET_IF_NBR        if_nbr);

void           NetBuf_TxLargePoolStatResetMaxUsed  (NET_IF_NBR        if_nbr);


NET_STAT_POOL  NetBuf_TxSmallPoolStatGet           (NET_IF_NBR        if_nbr);

void           NetBuf_TxSmallPoolStatResetMaxUsed  (NET_IF_NBR        if_nbr);

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

#endif  /* _NET_BUF_H_ */
