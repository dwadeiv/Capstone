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
*                        NETWORK APPLICATION PROGRAMMING INTERFACE (API) LAYER
*
* File : net_app_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef   _NET_APP_PRIV_H_
#define   _NET_APP_PRIV_H_

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "../../include/net_cfg_net.h"
#include  "net_sock_priv.h"
#include  "net_type_priv.h"
#include  "net_ip_priv.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                NETWORK APPLICATION TIME DELAY DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_APP_TIME_DLY_MIN_SEC                        DEF_INT_32U_MIN_VAL
#define  NET_APP_TIME_DLY_MAX_SEC                        DEF_INT_32U_MAX_VAL

#define  NET_APP_TIME_DLY_MIN_mS                         DEF_INT_32U_MIN_VAL
#define  NET_APP_TIME_DLY_MAX_mS                        (DEF_TIME_NBR_mS_PER_SEC - 1u)


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_APP_PRIV_H_ */

