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
*                                          SNTP CLIENT MODULE
*
* File : sntp_client.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _SNTP_CLIENT_H_
#define  _SNTP_CLIENT_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_ip.h>
#include  <rtos/net/include/net_type.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/rtos_err.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       SNTP DFLT CONFIG VALUE
*********************************************************************************************************
*/

#define  SNTP_CLIENT_CFG_SERVER_PORT_NBR_DFLT                   123u
#define  SNTP_CLIENT_CFG_SERVER_ADDR_FAMILY_DFLT                NET_IP_ADDR_FAMILY_NONE
#define  SNTP_CLIENT_CFG_REQ_RX_TIMEOUT_MS_DFLT                 5000u


/*
*********************************************************************************************************
*                                            SNTP INIT CFG
*********************************************************************************************************
*/

typedef  struct  sntp_init_cfg {
    MEM_SEG  *MemSegPtr;                                        /* Ptr to mem seg used for internal data alloc.         */
} SNTPc_INIT_CFG;


/*
*********************************************************************************************************
*                                    SNTP MESSAGE PACKET DATA TYPE
*
* Note(s) : (1) See RFC #2030, Section 4 for NTP message format.
*
*           (2) The first 32 bits of the NTP message (CW) contain the following fields :
*
*                   1                   2                   3
*                   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*                   |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
*                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*********************************************************************************************************
*/

typedef struct sntp_ts {
    CPU_INT32U  Sec;
    CPU_INT32U  Frac;
} SNTP_TS;

typedef struct sntp_pkt {
    CPU_INT32U  CW;                                             /* Ctrl word (see Note #2).                             */
    CPU_INT32U  RootDly;                                        /* Round trip dly.                                      */
    CPU_INT32U  RootDispersion;                                 /* Nominal err returned by server.                      */
    CPU_INT32U  RefID;                                          /* Server ref ID.                                       */
    SNTP_TS     TS_Ref;                                         /* Timestamp of the last sync.                          */
    SNTP_TS     TS_Originate;                                   /* Local timestamp when sending req.                    */
    SNTP_TS     TS_Rx;                                          /* Remote timestamp when receiving req.                 */
    SNTP_TS     TS_Tx;                                          /* Remote timestamp when sending res.                   */
} SNTP_PKT;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
extern  const  SNTPc_INIT_CFG  SNTPc_InitCfgDflt;               /* SNTPc dflt configurations.                           */
#endif

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

#if (RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN == DEF_DISABLED)
void         SNTPc_ConfigureMemSeg    (MEM_SEG             *p_mem_seg);
#endif

void         SNTPc_Init               (RTOS_ERR            *p_err);

void         SNTPc_DfltCfgSet         (NET_PORT_NBR         port_nbr,
                                       NET_IP_ADDR_FAMILY   addr_family,
                                       CPU_INT32U           rx_timeout_ms,
                                       RTOS_ERR            *p_err);

void         SNTPc_ReqRemoteTime      (CPU_CHAR            *hostname,
                                       SNTP_PKT            *p_pkt,
                                       RTOS_ERR            *p_err);

SNTP_TS      SNTPc_GetRemoteTime      (SNTP_PKT            *ppkt,
                                       RTOS_ERR            *p_err);

CPU_INT32U   SNTPc_GetRoundTripDly_us (SNTP_PKT            *ppkt,
                                       RTOS_ERR            *p_err);

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
#endif  /* _SNTP_CLIENT_H_ */
