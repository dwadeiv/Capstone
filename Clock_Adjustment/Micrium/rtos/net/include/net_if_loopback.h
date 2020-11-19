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
*                                       NETWORK INTERFACE LAYER
*                                              LOOPBACK
*
* File : net_if_loopback.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_IF_LOOPBACK_H_
#define  _NET_IF_LOOPBACK_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_if.h>
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
*                          NETWORK LOOPBACK INTERFACE CONFIGURATION DATA TYPE
*
* Note(s) : (1) The network loopback interface configuration data type is a specific instantiation of a
*               network device configuration data type.  ALL specific network device configuration data
*               types MUST be defined with ALL of the generic network device configuration data type's
*               configuration parameters, synchronized in both the sequential order & data type of each
*               parameter.
*
*               Thus ANY modification to the sequential order or data types of generic configuration
*               parameters MUST be appropriately synchronized between the generic network device
*               configuration data type & the network loopback interface configuration data type.
*
*               See also 'net_if.h  GENERIC NETWORK DEVICE CONFIGURATION DATA TYPE  Note #1'.
*********************************************************************************************************
*/

                                                    /* --------------------- NET LOOPBACK IF CFG ---------------------- */
typedef  struct  net_if_cfg_loopback {
                                                    /* ---------------- GENERIC  LOOPBACK CFG MEMBERS ----------------- */
    NET_IF_MEM_TYPE     RxBufPoolType;              /* Rx buf mem pool type :                                           */
                                                    /*   NET_IF_MEM_TYPE_MAIN        bufs alloc'd from main      mem    */
                                                    /*   NET_IF_MEM_TYPE_DEDICATED   bufs alloc'd from dedicated mem    */
    NET_BUF_SIZE        RxBufLargeSize;             /* Size  of loopback IF large buf data areas     (in octets).       */
    NET_BUF_QTY         RxBufLargeNbr;              /* Nbr   of loopback IF large buf data areas.                       */
    NET_BUF_SIZE        RxBufAlignOctets;           /* Align of loopback IF       buf data areas     (in octets).       */
    NET_BUF_SIZE        RxBufIxOffset;              /* Offset from base ix to rx data into data area (in octets).       */


    NET_IF_MEM_TYPE     TxBufPoolType;              /* Tx buf mem pool type :                                           */
                                                    /*   NET_IF_MEM_TYPE_MAIN        bufs alloc'd from main      mem    */
                                                    /*   NET_IF_MEM_TYPE_DEDICATED   bufs alloc'd from dedicated mem    */
    NET_BUF_SIZE        TxBufLargeSize;             /* Size  of loopback IF large buf data areas     (in octets).       */
    NET_BUF_QTY         TxBufLargeNbr;              /* Nbr   of loopback IF large buf data areas.                       */
    NET_BUF_SIZE        TxBufSmallSize;             /* Size  of loopback IF small buf data areas     (in octets).       */
    NET_BUF_QTY         TxBufSmallNbr;              /* Nbr   of loopback IF small buf data areas.                       */
    NET_BUF_SIZE        TxBufAlignOctets;           /* Align of loopback IF       buf data areas     (in octets).       */
    NET_BUF_SIZE        TxBufIxOffset;              /* Offset from base ix to tx data from data area (in octets).       */


    CPU_ADDR            MemAddr;                    /* Base addr of (loopback IF's) dedicated mem, if avail.            */
    CPU_ADDR            MemSize;                    /* Size      of (loopback IF's) dedicated mem, if avail.            */


    NET_DEV_CFG_FLAGS   Flags;                      /* Opt'l bit flags.                                                 */

                                                    /* ---------------- SPECIFIC LOOPBACK CFG MEMBERS ----------------- */

} NET_IF_CFG_LOOPBACK;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_IF_LOOPBACK_H_ */
