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
*                                              WIRELESS
*
* File : net_if_wifi_priv.h
*********************************************************************************************************
* Note(s) : (1) Implements code common to the following network interface layers :
*
*               (a) Ethernet as described in RFC # 894
*               (b) IEEE 802 as described in RFC #1042
*
*           (2) Wireless implementation conforms to RFC #1122, Section 2.3.3, bullets (a) & (b), but
*               does NOT implement bullet (c) :
*
*               RFC #1122                  LINK LAYER                  October 1989
*
*               2.3.3  ETHERNET (RFC-894) and IEEE 802 (RFC-1042) ENCAPSULATION
*
*                      Every Internet host connected to a 10Mbps Ethernet cable :
*
*                      (a) MUST be able to send and receive packets using RFC-894 encapsulation;
*
*                      (b) SHOULD be able to receive RFC-1042 packets, intermixed with RFC-894 packets; and
*
*                      (c) MAY be able to send packets using RFC-1042 encapsulation.
*
*           (3) Wireless implemtation doesn't supports wireless supplicant and/or IEEE 802.11. The
*               wireless module-hardware must provide the wireless suppliant and all relevant IEEE 802.11
*               supports.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_IF_WIFI_PRIV_H_
#define  _NET_IF_WIFI_PRIV_H_

#include  "../../include/net_cfg_net.h"

#ifdef  NET_IF_WIFI_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "../../include/net_type.h"
#include  "../../include/net_if.h"
#include  "../../include/net_if_wifi.h"

#include  "net_if_802x_priv.h"

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_IF_WIFI_BUF_RX_LEN_MIN     NET_IF_802x_BUF_RX_LEN_MIN
#define  NET_IF_WIFI_BUF_TX_LEN_MIN     NET_IF_802x_BUF_TX_LEN_MIN

#define  NET_IF_WIFI_CFG_RX_BUF_IX_OFFSET_MIN              1u           /* The rx buf must be prefixed with pkt type.   */

#define  NET_IF_WIFI_CFG_RX_BUF_LARGE_SIZE              1568u           /* TODO_NET review size to 1518                 */
#define  NET_IF_WIFI_CFG_TX_BUF_LARGE_SIZE              1568u           /* TODO_NET review size to 1518                 */
#define  NET_IF_WIFI_CFG_RX_BUF_SMALL_SIZE                60u

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                    /* ----------------------- NET WIFI IF DATA ----------------------- */
typedef  struct  net_if_data_wifi {
    CPU_INT08U    HW_Addr[NET_IF_802x_ADDR_SIZE];   /* WiFi IF's dev hw addr.                                           */
    NET_DEV_BAND  Band;
} NET_IF_DATA_WIFI;


/*
*********************************************************************************************************
*                                     WIRELESS DEVICE DATA TYPES
*
* Note(s) : (1) The Wireless interface configuration data type is a specific definition of a network
*               device configuration data type.  Each specific network device configuration data type
*               MUST define ALL generic network device configuration parameters, synchronized in both
*               the sequential order & data type of each parameter.
*
*               Thus, ANY modification to the sequential order or data types of configuration parameters
*               MUST be appropriately synchronized between the generic network device configuration data
*               type & the Ethernet interface configuration data type.
*
*               See also 'net_if.h  GENERIC NETWORK DEVICE CONFIGURATION DATA TYPE  Note #1'.
*********************************************************************************************************
*/

                                                        /* ----------------------- NET WIFI DEV CFG ----------------------- */
typedef  struct  net_dev_cfg_wifi {
    NET_IF_MEM_TYPE                 RxBufPoolType;      /* Rx buf mem pool type :                                           */
                                                        /*   NET_IF_MEM_TYPE_MAIN        bufs alloc'd from main      mem    */
                                                        /*   NET_IF_MEM_TYPE_DEDICATED   bufs alloc'd from dedicated mem    */
    NET_BUF_SIZE                    RxBufLargeSize;     /* Size  of dev rx large buf data areas (in octets).                */
    NET_BUF_QTY                     RxBufLargeNbr;      /* Nbr   of dev rx large buf data areas.                            */
    NET_BUF_SIZE                    RxBufAlignOctets;   /* Align of dev rx       buf data areas (in octets).                */
    NET_BUF_SIZE                    RxBufIxOffset;      /* Offset from base ix to rx data into data area (in octets).       */


    NET_IF_MEM_TYPE                 TxBufPoolType;      /* Tx buf mem pool type :                                           */
                                                        /*   NET_IF_MEM_TYPE_MAIN        bufs alloc'd from main      mem    */
                                                        /*   NET_IF_MEM_TYPE_DEDICATED   bufs alloc'd from dedicated mem    */
    NET_BUF_SIZE                    TxBufLargeSize;     /* Size  of dev tx large buf data areas (in octets).                */
    NET_BUF_QTY                     TxBufLargeNbr;      /* Nbr   of dev tx large buf data areas.                            */
    NET_BUF_SIZE                    TxBufSmallSize;     /* Size  of dev tx small buf data areas (in octets).                */
    NET_BUF_QTY                     TxBufSmallNbr;      /* Nbr   of dev tx small buf data areas.                            */
    NET_BUF_SIZE                    TxBufAlignOctets;   /* Align of dev tx       buf data areas (in octets).                */
    NET_BUF_SIZE                    TxBufIxOffset;      /* Offset from base ix to tx data from data area (in octets).       */

    CPU_ADDR                        MemAddr;            /* Base addr of (WiFi dev's) dedicated mem, if avail.               */
    CPU_ADDR                        MemSize;            /* Size      of (WiFi dev's) dedicated mem, if avail.               */


    NET_DEV_CFG_FLAGS               Flags;              /* Opt'l bit flags.                                                 */

    NET_DEV_BAND                    Band;               /* Wireless Band to use by the device.                              */
                                                        /*   NET_DEV_2_4_GHZ        Wireless band is 2.4Ghz.                */
                                                        /*   NET_DEV_5_0_GHZ        Wireless band is 5.0Ghz.                */
                                                        /*   NET_DEV_DUAL           Wireless band is dual (2.4 & 5.0 Ghz).  */

    CPU_CHAR                        HW_AddrStr[NET_IF_802x_ADDR_SIZE_STR];  /*  WiFi IF's dev hw addr str.                  */

    void                           *CfgExtPtr;          /* Configuration Extension Ptr.                                     */

} NET_DEV_CFG_WIFI;


/*
*********************************************************************************************************
*                                    WIRELESS FRAME TYPE DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_INT08U     NET_IF_WIFI_FRAME_TYPE;

#define  NET_IF_WIFI_DATA_PKT                              1u
#define  NET_IF_WIFI_MGMT_FRAME                            2u


/*
*********************************************************************************************************
*                                       WIRELESS SCAN DATA TYPE
*********************************************************************************************************
*/

typedef  struct  net_if_wifi_scan {
    NET_IF_WIFI_SSID        SSID;                   /* WiFi scan SSID.                                                  */
    NET_IF_WIFI_CH          Ch;                     /* WiFi Scan Channel                                                */
} NET_IF_WIFI_SCAN;

                                                    /* -------------------- NET WIFI DEV LINK STATE ------------------- */
typedef  struct net_dev_link_state_wifi {
    NET_IF_LINK_STATE       LinkState;              /* Link   state.                                                    */
    NET_IF_WIFI_AP          AP;                     /* Access Point.                                                    */
    NET_IF_WIFI_DATA_RATE   DataRate;               /* Link   spd.                                                      */
    NET_IF_WIFI_PWR_LEVEL   PwrLevel;               /* Power  Level.                                                    */
} NET_DEV_LINK_WIFI;


/*
*********************************************************************************************************
*                                   WIRELESS DEVICE API DATA TYPES
*
* Note(s) : (1) (a) The Wireless device application programming interface (API) data type is a specific
*                   network device API data type definition which MUST define ALL generic network device
*                   API functions, synchronized in both the sequential order of the functions & argument
*                   lists for each function.
*
*                   Thus, ANY modification to the sequential order or argument lists of the API functions
*                   MUST be appropriately synchronized between the generic network device API data type &
*                   the Wireless device API data type definition/instantiations.
*
*                   However, specific Wireless device API data type definitions/instantiations MAY include
*                   additional API functions after all generic Ethernet device API functions.
*
*               (b) ALL API functions MUST be defined with NO NULL functions for all specific Wireless
*                   device API instantiations.  Any specific Ethernet device API instantiation that does
*                   NOT require a specific API's functionality MUST define an empty API function.
*
*               See also 'net_if.h  GENERIC NETWORK DEVICE API DATA TYPE  Note #1'.
*********************************************************************************************************
*/

                                                                        /* ------------- NET WIFI DEV API ------------- */
                                                                        /* Net WiFi dev API fnct ptrs :                 */
typedef  struct  net_dev_api_if_wifi {
                                                                        /*   Init/add                                   */
    void  (*Init)               (NET_IF               *p_if,
                                 RTOS_ERR             *p_err);
                                                                        /*   Start                                      */
    void  (*Start)              (NET_IF               *p_if,
                                 RTOS_ERR             *p_err);
                                                                        /*   Stop                                       */
    void  (*Stop)               (NET_IF               *p_if,
                                 RTOS_ERR             *p_err);
                                                                        /*   Rx                                         */
    void  (*Rx)                 (NET_IF               *p_if,
                                 CPU_INT08U          **p_data,
                                 CPU_INT16U           *size,
                                 RTOS_ERR             *p_err);
                                                                        /*   Tx                                         */
    void  (*Tx)                 (NET_IF               *p_if,
                                 CPU_INT08U           *p_data,
                                 CPU_INT16U            size,
                                 RTOS_ERR             *p_err);
                                                                        /*   Multicast addr add                         */
    void  (*AddrMulticastAdd)   (NET_IF               *p_if,
                                 CPU_INT08U           *p_addr_hw,
                                 CPU_INT08U            addr_hw_len,
                                 RTOS_ERR             *p_err);
                                                                        /*   Multicast addr remove                      */
    void  (*AddrMulticastRemove)(NET_IF               *p_if,
                                 CPU_INT08U           *p_addr_hw,
                                 CPU_INT08U            addr_hw_len,
                                 RTOS_ERR             *p_err);
                                                                        /*   ISR handler                                */
    void  (*ISR_Handler)        (NET_IF               *p_if,
                                 NET_DEV_ISR_TYPE      type);

                                                                        /*   Demux Pkt Mgmt                             */
    void  (*MgmtDemux)          (NET_IF               *p_if,
                                 NET_BUF              *p_buf,
                                 RTOS_ERR             *p_err);
} NET_DEV_API_IF_WIFI;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void       NetIF_WiFi_Init      (RTOS_ERR  *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_IF_WIFI_MODULE_EN */
#endif  /* _NET_IF_WIFI_PRIV_H_  */

