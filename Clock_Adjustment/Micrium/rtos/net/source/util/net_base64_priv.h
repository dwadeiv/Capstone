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
*                                       NETWORK BASE64 LIBRARY
*
* File : net_base64_priv.h
*********************************************************************************************************
* Note(s) : (1) No compiler-supplied standard library functions are used by the network protocol suite.
*               'net_util.*' implements ALL network-specific library functions.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_BASE64_PRIV_H_
#define  _NET_BASE64_PRIV_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/source/tcpip/net_priv.h>
#include  <rtos/common/include/lib_mem.h>


/*
*********************************************************************************************************
*                                       BASE 64 ENCODER DEFINES
*
* Note(s) : (1) The size of the output buffer the base 64 encoder produces is typically bigger than the
*               input buffer by a factor of (4 x 3).  However, when padding is necessary, up to 3
*               additional characters could by appended.  Finally, one more character is used to NULL
*               terminate the buffer.
*********************************************************************************************************
*/

#define  NET_BASE64_ENCODER_OCTETS_IN_GRP                3
#define  NET_BASE64_ENCODER_OCTETS_OUT_GRP               4

#define  NET_BASE64_DECODER_OCTETS_IN_GRP                4
#define  NET_BASE64_DECODER_OCTETS_OUT_GRP               3

#define  NET_BASE64_ENCODER_PAD_CHAR                    '='
                                                                /* See Note #1.                                         */
#define  NET_BASE64_ENCODER_OUT_MAX_LEN(length)         (((length / 3) * 4) + ((length % 3) == 0 ? 0 : 4) + 1)



void        NetBase64_Encode (CPU_CHAR               *pin_buf,
                              CPU_INT16U              in_len,
                              CPU_CHAR               *pout_buf,
                              CPU_INT16U              out_len,
                              RTOS_ERR               *p_err);

void        NetBase64_Decode (CPU_CHAR               *pin_buf,
                              CPU_INT16U              in_len,
                              CPU_CHAR               *pout_buf,
                              CPU_INT16U              out_len,
                              RTOS_ERR               *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_BASE64_PRIV_H_ */
