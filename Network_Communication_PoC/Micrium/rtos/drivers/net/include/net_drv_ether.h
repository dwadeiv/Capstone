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
*                                        NETWORK DEVICE DRIVER
*                                               ETHERNET
*
* File : net_drv_ether.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  NET_DRV_ETHER_H
#define  NET_DRV_ETHER_H

#include  <rtos/net/include/net_cfg_net.h>

#ifdef  NET_IF_ETHER_MODULE_EN
#include  <rtos/net/include/net_if_ether.h>
#include  <rtos/net/source/tcpip/net_if_ether_priv.h>
#include  <rtos/common/include/rtos_types.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               GEM DRIVER
*********************************************************************************************************
*/

extern  const  NET_DEV_API_ETHER  NetDev_API_GEM;


/*
*********************************************************************************************************
*                                              GMAC DRIVER
*
* Note(s) : (1) The following MCUs are support by NetDev_API_GMAC API:
*
*                          STMicroelectronics  STM32F107xx  series
*                          STMicroelectronics  STM32F2xxx   series
*                          STMicroelectronics  STM32F4xxx   series
*                          FUJITSU             MB9BFD10T    series
*                          FUJITSU             MB9BF610T    series
*                          FUJITSU             MB9BF210T    series
*
*           (2) The following MCUs are support by NetDev_API_LPCXX_ENET API:
*
*                          NXP                 LPC185x      series
*                          NXP                 LPC183x      series
*                          NXP                 LPC435x      series
*                          NXP                 LPC433x      series
*
*           (3) The following MCUs are support by NetDev_API_XMC4000_ENET API:
*
*                          INFINEON            XMC4500      series
*                          INFINEON            XMC4400      series
*
*           (4) The following MCUs are support by NetDev_API_TM4C12X_ENET API:
*
*                          TEXAS INSTRUMENTS   TM4C12x      series
*********************************************************************************************************
*/

extern  const  NET_DEV_API_ETHER  NetDev_API_GMAC;
extern  const  NET_DEV_API_ETHER  NetDev_API_LPCXX_ENET;
extern  const  NET_DEV_API_ETHER  NetDev_API_XMC4000_ENET;
extern  const  NET_DEV_API_ETHER  NetDev_API_TM4C12X_ENET;
extern  const  NET_DEV_API_ETHER  NetDev_API_HPS_EMAC;
extern  const  NET_DEV_API_ETHER  NetDev_API_STM32Fx_EMAC;


/*
*********************************************************************************************************
*                                             MACNET DRIVER
*********************************************************************************************************
*/

extern  const  NET_DEV_API_ETHER  NetDev_API_MACNET;



/*
********************************************************************************************************
*                                          RENESAS RX ETHER-C
*********************************************************************************************************
*/

extern  const  NET_DEV_API_ETHER  NetDev_API_RX_EtherC;


/*
********************************************************************************************************
*                                 RENESAS RX64M EXTENDED CONFIGURATION
*********************************************************************************************************
*/

                                                    /* ---------------------- NET ETHER DEV CFG ----------------------- */
typedef  struct  net_dev_cfg_ether_rx64_ext {

    CPU_ADDR            PTPRSTRegister;             /* EPTPC software reset register PTRSTR address                     */
    CPU_ADDR            SYMACRURegister;            /* EPTC MAC Frame filtering upper register SYMACRU address          */
    CPU_ADDR            PHYCtrl_BaseAddr;           /* Base addr to the Ether dev which control MDIO and MDC to the PHY */

} NET_DEV_CFG_ETHER_RX64_EXT;


/*
*********************************************************************************************************
*                                             USBD CDC-EEM
*********************************************************************************************************
*/

                                                    /* ----------------- NET USBD CDCEEM DEV CFG EXT ------------------ */
typedef  struct  net_dev_cfg_usbd_cdc_eem_ext {
    CPU_INT08U  ClassNbr;                           /* Class number.                                                    */
} NET_DEV_CFG_USBD_CDC_EEM_EXT;

extern  const  NET_DEV_API_ETHER  NetDev_API_USBD_CDCEEM;


/*
*********************************************************************************************************
*                                              WINPCAP DRIVER
*********************************************************************************************************
*/

extern  const  NET_DEV_API_ETHER  NetDev_API_WinPcap;

typedef  struct  net_dev_cfg_ext_winpcap {
    RTOS_TASK_CFG  TaskCfg;
    CPU_INT08U     PC_IFNbr;
} NET_DEV_CFG_EXT_WINPCAP;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* NET_IF_ETHER_MODULE_EN */
#endif /* NET_DRV_ETHER_H        */
