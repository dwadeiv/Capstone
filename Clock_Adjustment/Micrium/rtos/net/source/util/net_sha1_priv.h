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
*                                       NETWORK CRYPTO SHA1 UTILITY
*
* File : net_sha1_priv.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_SHA1_PRIV_H_
#define  _NET_SHA1_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "../tcpip/net_priv.h"

#include  <rtos/common/include/lib_mem.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_SHA1_HASH_SIZE                 20

#define  NET_SHA1_INTERMEDIATE_HASH_SIZE    NET_SHA1_HASH_SIZE / 4


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef struct net_sha1_context {

    CPU_INT32U      Intermediate_Hash[NET_SHA1_INTERMEDIATE_HASH_SIZE];

    CPU_INT32U      Length_Low;                                 /* Message length in bits                               */
    CPU_INT32U      Length_High;                                /* Message length in bits                               */

                                                                /* Index into message block array                       */
    CPU_INT16U      Message_Block_Index;
    CPU_INT08U      Message_Block[64];                          /* 512-bit message blocks                               */

    CPU_BOOLEAN     Computed;                                   /* Is the digest computed?                              */
    RTOS_ERR        Corrupted;                                  /* Is the message digest corrupted?                     */

} NET_SHA1_CTX;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN NetSHA1_Reset     (      NET_SHA1_CTX  *p_ctx,
                                     RTOS_ERR     *p_err);

CPU_BOOLEAN NetSHA1_Input     (      NET_SHA1_CTX  *p_ctx,
                               const CPU_CHAR      *p_msg,
                                     CPU_INT32U     len,
                                     RTOS_ERR      *p_err);

CPU_BOOLEAN NetSHA1_Result    (      NET_SHA1_CTX  *p_ctx,
                                     CPU_CHAR      *p_msg_digest,
                                     RTOS_ERR      *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_SHA1_PRIV_H_ */
