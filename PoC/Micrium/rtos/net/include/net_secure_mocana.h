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
*                                     NETWORK SECURITY PORT LAYER
*                                            Mocana nanoSSL
*
* File : net_secure_mocana.h
*********************************************************************************************************
* Note(s) : (1) Assumes the following versions (or more recent) of software modules are included in
*               the project build :
*
*               (a) Mocana nanoSSL V6.3
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_SECURE_MOCANA_H_
#define  _NET_SECURE_MOCANA_H_

#include  <rtos_description.h>

#ifdef  RTOS_MODULE_NET_SSL_TLS_MOCANA_NANOSSL_AVAIL


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* Mocana includes.                                     */
#include  <common/mtypes.h>
#include  <common/moptions.h>
#include  <common/mtypes.h>
#include  <common/mdefs.h>
#include  <common/merrors.h>
#include  <common/mrtos.h>
#include  <common/mtcp.h>
#include  <common/mocana.h>
#include  <common/debug_console.h>
#include  <common/mstdlib.h>
#include  <crypto/hw_accel.h>
#include  <crypto/ca_mgmt.h>
#include  <common/sizedbuffer.h>
#include  <crypto/cert_store.h>
#include  <ssl/ssl.h>

                                                                /* Micrium includes.                                    */
#include  <rtos/net/source/ssl_tls/net_secure_priv.h>

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

CPU_BOOLEAN         NetSecure_CA_CertIntall (const  void                   *p_ca_cert,
                                                    CPU_INT32U              ca_cert_len,
                                                    NET_SECURE_CERT_FMT     fmt,
                                                    RTOS_ERR               *p_err);

void                NetSecure_Log           (       CPU_CHAR               *p_str);

CPU_BOOLEAN         NetSecure_DN_Print      (       CPU_CHAR               *p_buf,
                                                    CPU_INT32U              buf_len,
                                                    certDistinguishedName  *p_dn);


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

#endif  /* NET_SECURE_MODULE_EN  */
#endif  /* _NET_SECURE_MOCANA_H_ */

