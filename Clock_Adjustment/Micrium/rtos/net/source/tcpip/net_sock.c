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
*                                         NETWORK SOCKET LAYER
*
* File : net_sock.c
*********************************************************************************************************
* Note(s) : (1) Supports BSD 4.x Socket Layer with the following restrictions/constraints :
*
*               (a) ONLY supports a single address family from the following families :
*                   (1) IPv4 (AF_INET)
*
*               (b) ONLY supports the following socket types :
*                   (1) Datagram (SOCK_DGRAM)
*                   (2) Stream   (SOCK_STREAM)
*
*               (c) ONLY supports a single protocol family from the following families :
*                   (1) IPv4 (PF_INET) & IPv6 (PF_INET6)
*                       (A) ONLY supports the following protocols :
*                           (1) UDP (IPPROTO_UDP)
*                           (2) TCP (IPPROTO_TCP)
*
*               (d) ONLY supports the following socket options :
*
*                       Blocking
*                       Secure (TLS/SSL)
*                       Rx Queue size
*                       Tx Queue size
*                       Time of server (IPv4-TOS)
*                       Time to life   (IPv4-TTL)
*                       Time to life multicast
*                       UDP connection receive         timeout
*                       TCP connection accept          timeout
*                       TCP connection close           timeout
*                       TCP connection connect request timeout
*                       TCP connection receive         timeout
*                       TCP connection transmit        timeout
*                       TCP keep alive
*                       TCP MSL
*                       Force connection using a specific Interface
*
*               (e) Multiple socket connections with the same local & remote address -- both
*                   addresses & port numbers -- OR multiple socket connections with only a
*                   local address but the same local address -- both address & port number --
*                   is NOT currently supported.
*
*           (2) The Institute of Electrical and Electronics Engineers and The Open Group, have given
*               us permission to reprint portions of their documentation.  Portions of this text are
*               reprinted and reproduced in electronic form from the IEEE Std 1003.1, 2004 Edition,
*               Standard for Information Technology -- Portable Operating System Interface (POSIX),
*               The Open Group Base Specifications Issue 6, Copyright (C) 2001-2004 by the Institute
*               of Electrical and Electronics Engineers, Inc and The Open Group.  In the event of any
*               discrepancy between these versions and the original IEEE and The Open Group Standard,
*               the original IEEE and The Open Group Standard is the referee document.  The original
*               Standard can be obtained online at http://www.opengroup.org/unix/online.html.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_NET_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net.h>
#include  <rtos/net/include/net_util.h>

#include  "net_sock_priv.h"
#include  <rtos/net/include/net_cfg_net.h>
#include  "net_priv.h"
#include  "net_tcp_priv.h"
#include  "net_udp_priv.h"
#include  "net_conn_priv.h"
#include  "net_buf_priv.h"
#include  "net_if_priv.h"
#include  "net_util_priv.h"
#include  "net_igmp_priv.h"

#ifdef  NET_SECURE_MODULE_EN
#include  <rtos/net/source/ssl_tls/net_secure_priv.h>
#endif

#include  <rtos/common/include/lib_utils.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>
#include  <rtos/common/source/collections/slist_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                     (NET)
#define  RTOS_MODULE_CUR                  RTOS_CFG_MODULE_NET

#define  NET_SOCK_RX_Q_NAME              "Net Sock Rx Q"
#define  NET_SOCK_CONN_REQ_NAME          "Net Sock Conn Req"
#define  NET_SOCK_CONN_ACCEPT_NAME       "Net Sock Conn Accept Q"
#define  NET_SOCK_CONN_CLOSE_NAME        "Net Sock Conn Close"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
NET_STAT_POOL      NetSock_PoolStat;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  NET_SOCK              *NetSock_Tbl;

static  NET_SOCK              *NetSock_PoolPtr;                 /* Ptr to pool of free socks.                   */

static  NET_PORT_NBR           NetSock_RandomPortNbrCur;

static  MEM_DYN_POOL           NetSock_AcceptQ_ObjPool;

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  MEM_DYN_POOL           NetSock_SelObjPool;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void                NetSock_InitObj                      (NET_SOCK                  *p_sock,
                                                                  MEM_SEG                   *p_mem_seg,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN         NetSock_ListenQ_IsAvail              (NET_SOCK_ID                sock_id);
#endif
                                                                                    /* ----------- RX FNCTS ----------- */
static  void                NetSock_RxPktDemux                   (NET_BUF                   *p_buf,
                                                                  NET_BUF_HDR               *p_buf_hdr,
                                                                  RTOS_ERR                  *p_err);

                                                                                    /* ------ SOCK STATUS FNCTS ------- */

static  CPU_BOOLEAN         NetSock_IsValidAddrLocal             (NET_SOCK_PROTOCOL_FAMILY   protocol,
                                                                  NET_SOCK_ADDR             *p_addr,
                                                                  NET_SOCK_ADDR_LEN          addr_len,
                                                                  NET_IF_NBR                *p_if_nbr,
                                                                  RTOS_ERR                  *p_err);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
static  CPU_BOOLEAN         NetSock_IsValidAddrRemote            (NET_SOCK_ADDR             *p_addr,
                                                                  NET_SOCK_ADDR_LEN          addr_len,
                                                                  NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

                                                                                    /* ------ SOCK HANDLER FNCTS ------ */
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_SOCK_RTN_CODE   NetSock_CloseHandlerStream           (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

static  NET_SOCK_RTN_CODE   NetSock_BindHandler                  (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK_ADDR             *p_addr_local,
                                                                  NET_SOCK_ADDR_LEN          addr_len,
                                                                  CPU_BOOLEAN                addr_random_reqd,
                                                                  NET_SOCK_ADDR             *p_addr_dest,
                                                                  RTOS_ERR                  *p_err);

static  NET_SOCK_RTN_CODE   NetSock_ConnHandlerDatagram          (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_SOCK_RTN_CODE   NetSock_ConnHandlerStream            (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  RTOS_ERR                  *p_err);

static  NET_SOCK_RTN_CODE   NetSock_ConnHandlerStreamWait        (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

static  void                NetSock_ConnHandlerAddr              (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  CPU_BOOLEAN                addr_validate,
                                                                  CPU_BOOLEAN                addr_over_wr,
                                                                  RTOS_ERR                  *p_err);

static  void                NetSock_ConnHandlerAddrLocalBind     (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  RTOS_ERR                  *p_err);

static  void                NetSock_ConnHandlerAddrRemoteValidate(NET_SOCK                  *p_sock,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  RTOS_ERR                  *p_err);

static  void                NetSock_ConnHandlerAddrRemoteSet     (NET_SOCK                  *p_sock,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  CPU_BOOLEAN                addr_over_wr,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  void                NetSock_ConnAcceptQ_Init             (NET_SOCK                  *p_sock,
                                                                  NET_SOCK_Q_SIZE            sock_q_size);

static  void                NetSock_ConnAcceptQ_Clr              (NET_SOCK                  *p_sock);

static  CPU_BOOLEAN         NetSock_ConnAcceptQ_IsAvail          (NET_SOCK                  *p_sock);

static  CPU_BOOLEAN         NetSock_ConnAcceptQ_IsRdy            (NET_SOCK                  *p_sock);

static  void                NetSock_ConnAcceptQ_ConnID_Add       (NET_SOCK                  *p_sock,
                                                                  NET_CONN_ID                conn_id,
                                                                  RTOS_ERR                  *p_err);

static  NET_CONN_ID         NetSock_ConnAcceptQ_ConnID_Get       (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);

static  NET_SOCK_ACCEPT_Q_OBJ  *NetSock_ConnAcceptQ_ConnID_Srch  (NET_SOCK                  *p_sock,
                                                                  NET_CONN_ID                conn_id);

static  CPU_BOOLEAN         NetSock_ConnAcceptQ_ConnID_Remove    (NET_SOCK                  *p_sock,
                                                                  NET_CONN_ID                conn_id);
#endif

static  NET_SOCK_RTN_CODE   NetSock_RxDataHandler                (NET_SOCK_ID                sock_id,
                                                                  void                      *p_data_buf,
                                                                  CPU_INT16U                 data_buf_len,
                                                                  NET_SOCK_API_FLAGS         flags,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  NET_SOCK_ADDR_LEN         *p_addr_len,
                                                                  void                      *p_ip_opts_buf,
                                                                  CPU_INT08U                 ip_opts_buf_len,
                                                                  CPU_INT08U                *p_ip_opts_len,
                                                                  RTOS_ERR                  *p_err);

static  NET_SOCK_RTN_CODE   NetSock_RxDataHandlerDatagram        (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  void                      *p_data_buf,
                                                                  CPU_INT16U                 data_buf_len,
                                                                  NET_SOCK_API_FLAGS         flags,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  NET_SOCK_ADDR_LEN         *p_addr_len,
                                                                  void                      *p_ip_opts_buf,
                                                                  CPU_INT08U                 ip_opts_buf_len,
                                                                  CPU_INT08U                *p_ip_opts_len,
                                                                  RTOS_ERR                  *p_err);

static  NET_SOCK_RTN_CODE   NetSock_TxDataHandler                (NET_SOCK_ID                sock_id,
                                                                  void                      *p_data,
                                                                  CPU_INT16U                 data_len,
                                                                  NET_SOCK_API_FLAGS         flags,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  NET_SOCK_ADDR_LEN          addr_len,
                                                                  RTOS_ERR                  *p_err);

static  NET_SOCK_RTN_CODE   NetSock_TxDataHandlerDatagram        (NET_SOCK_ID                sock_id,
                                                                  NET_SOCK                  *p_sock,
                                                                  void                      *p_data,
                                                                  CPU_INT16U                 data_len,
                                                                  NET_SOCK_API_FLAGS         flags,
                                                                  NET_SOCK_ADDR             *p_addr_remote,
                                                                  RTOS_ERR                  *p_err);

#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
static  void                NetSock_AppPostRx                    (NET_CONN_ID                conn_id_app);
static  void                NetSock_AppPostTx                    (NET_CONN_ID                conn_id_app);
#endif

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  void                NetSock_SelPost                      (NET_SOCK                  *p_sock,
                                                                  NET_SOCK_EVENT_TYPE        event);

static  NET_SOCK_QTY        NetSock_SelDescHandler               (NET_SOCK_QTY               sock_nbr_max,
                                                                  NET_SOCK_DESC             *p_sock_desc_rd,
                                                                  NET_SOCK_DESC             *p_sock_desc_wr,
                                                                  NET_SOCK_DESC             *p_sock_desc_err,
                                                                  RTOS_ERR                  *p_err);

static  CPU_BOOLEAN         NetSock_SelDescHandlerRd             (NET_SOCK_ID                sock_id,
                                                                  RTOS_ERR                  *p_err);

static  CPU_BOOLEAN         NetSock_SelDescHandlerRdDatagram     (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN         NetSock_SelDescHandlerRdStream       (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

static  CPU_BOOLEAN         NetSock_SelDescHandlerWr             (NET_SOCK_ID                sock_id,
                                                                  RTOS_ERR                  *p_err);

static  CPU_BOOLEAN         NetSock_SelDescHandlerWrDatagram     (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN         NetSock_SelDescHandlerWrStream       (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

static  CPU_BOOLEAN         NetSock_SelDescHandlerErr            (NET_SOCK_ID                sock_id,
                                                                  RTOS_ERR                  *p_err);

static  CPU_BOOLEAN         NetSock_SelDescHandlerErrDatagram    (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN         NetSock_SelDescHandlerErrStream      (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

static  CPU_BOOLEAN         NetSock_IsAvailRxDatagram            (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);

static  CPU_BOOLEAN         NetSock_IsRdyTxDatagram              (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN         NetSock_IsAvailRxStream              (NET_SOCK                  *p_sock,
                                                                  NET_CONN_ID                conn_id_transport,
                                                                  RTOS_ERR                  *p_err);

static  CPU_BOOLEAN         NetSock_IsRdyTxStream                (NET_SOCK                  *p_sock,
                                                                  NET_CONN_ID                conn_id_transport,
                                                                  RTOS_ERR                  *p_err);
#endif
#endif

                                                                                    /* ---------- SOCK FNCTS ---------- */

static  NET_SOCK           *NetSock_Get                          (RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_CONN_ID         NetSock_GetConnTransport             (NET_SOCK                  *p_sock,
                                                                  RTOS_ERR                  *p_err);
#endif

static  void                NetSock_CloseHandler                 (NET_SOCK                  *p_sock,
                                                                  CPU_BOOLEAN                close_conn,
                                                                  CPU_BOOLEAN                close_conn_transport);

static  void                NetSock_CloseSockHandler             (NET_SOCK                  *p_sock,
                                                                  CPU_BOOLEAN                close_conn,
                                                                  CPU_BOOLEAN                close_conn_transport,
                                                                  RTOS_ERR                  *p_err);

static  void                NetSock_CloseSockFromClose           (NET_SOCK                  *p_sock);

#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
static  void                NetSock_CloseConn                    (NET_CONN_ID                conn_id);

static  void                NetSock_CloseConnFree                (NET_CONN_ID                conn_id);
#endif

static  void                NetSock_Free                         (NET_SOCK                  *p_sock);

static  void                NetSock_FreeHandler                  (NET_SOCK                  *p_sock);

static  void                NetSock_FreeBufQ                     (NET_BUF                  **p_buf_q_head,
                                                                  NET_BUF                  **p_buf_q_tail);

static  void                NetSock_Clr                          (NET_SOCK                  *p_sock);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  void                NetSock_Copy                         (NET_SOCK                  *p_sock_dest,
                                                                  NET_SOCK                  *p_sock_src);
#endif

                                                                                    /* ----- RANDOM PORT Q FNCTS ------ */
static  NET_PORT_NBR        NetSock_RandomPortNbrGet             (NET_PROTOCOL_TYPE          protocol);

static  NET_SOCK_RTN_CODE   NetSock_OpGetSock                    (NET_SOCK                  *p_sock,
                                                                  NET_SOCK_OPT_NAME          opt_name,
                                                                  void                      *p_opt_val,
                                                                  NET_SOCK_OPT_LEN          *p_opt_len,
                                                                  RTOS_ERR                  *p_err);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN         NetSock_CfgTxNagleEnHandler          (NET_SOCK_ID                sock_id,
                                                                  CPU_BOOLEAN                nagle_en,
                                                                  RTOS_ERR                  *p_err);
#endif

static  CPU_BOOLEAN         NetSock_CfgIF_Handler                (NET_SOCK_ID                sock_id,
                                                                  NET_IF_NBR                 if_nbr,
                                                                  RTOS_ERR                  *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          PUBLIC FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           NetSock_Open()
*
* Description : Open a network socket.
*
* Argument(s) : protocol_family     Socket protocol family :
*
*                                       NET_SOCK_PROTOCOL_FAMILY_IP_V4
*                                       NET_SOCK_PROTOCOL_FAMILY_IP_V6
*
*               sock_type           Socket type :
*
*                                       NET_SOCK_TYPE_DATAGRAM
*                                       NET_SOCK_TYPE_STREAM
*
*               protocol            Socket protocol :
*
*                                       NET_SOCK_PROTOCOL_DFLT
*                                       NET_SOCK_PROTOCOL_TCP
*                                       NET_SOCK_PROTOCOL_UDP
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       RTOS_ERR_NONE
*                                       RTOS_ERR_POOL_EMPTY
*
* Return(s)   : Socket descriptor/handle identifier, if NO error(s).
*               NET_SOCK_BSD_ERR_OPEN, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

NET_SOCK_ID  NetSock_Open (NET_SOCK_PROTOCOL_FAMILY   protocol_family,
                           NET_SOCK_TYPE              sock_type,
                           NET_SOCK_PROTOCOL          protocol,
                           RTOS_ERR                  *p_err)
{
    NET_SOCK     *p_sock  = DEF_NULL;
    NET_SOCK_ID   sock_id = NET_SOCK_BSD_ERR_OPEN;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_OPEN);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_Open);


                                                                /* ---------------- VALIDATE SOCK ARGS ---------------- */
    switch (protocol_family) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
#endif
#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
#endif
             switch (sock_type) {
                 case NET_SOCK_TYPE_DATAGRAM:
                      switch (protocol) {
                          case NET_SOCK_PROTOCOL_UDP:
                               break;

                          case NET_SOCK_PROTOCOL_DFLT:
                               protocol = NET_SOCK_PROTOCOL_UDP;
                               break;

                          default:
                               NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                               Net_GlobalLockRelease();
                               RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPEN);
                      }
                      break;


                 case NET_SOCK_TYPE_STREAM:
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
                      switch (protocol) {
                          case NET_SOCK_PROTOCOL_TCP:
                               break;

                          case NET_SOCK_PROTOCOL_DFLT:
                               protocol = NET_SOCK_PROTOCOL_TCP;
                               break;

                          default:
                               NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                               Net_GlobalLockRelease();
                               RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPEN);
                      }
#else
                      Net_GlobalLockRelease();
                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPEN);
#endif
                      break;


                 case NET_SOCK_TYPE_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
                      Net_GlobalLockRelease();
                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_TYPE, NET_SOCK_BSD_ERR_OPEN);
             }
             break;


        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPEN);
    }


                                                                /* --------------------- GET SOCK --------------------- */
    p_sock = NetSock_Get(p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit_release;                                      /* Rtn err from NetSock_Get().                          */
    }


                                                                /* -------------------- INIT SOCK --------------------- */
    p_sock->ProtocolFamily = protocol_family;
    p_sock->Protocol       = protocol;
    p_sock->SockType       = sock_type;
    p_sock->IF_Nbr         = NET_IF_NBR_NONE;

    sock_id                = p_sock->ID;


exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (sock_id);                                            /* ------------------- RTN SOCK ID -------------------- */
}


/*
*********************************************************************************************************
*                                           NetSock_Close()
*
* Description : Close a network socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to close.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_NET_RETRY_MAX
*                               RTOS_ERR_NET_SOCK_CLOSED
*                               RTOS_ERR_BLK_ALLOC_CALLBACK
*                               RTOS_ERR_SEG_OVF
*                               RTOS_ERR_NOT_SUPPORTED
*                               RTOS_ERR_OS_SCHED_LOCKED
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_NET_INVALID_ADDR_SRC
*                               RTOS_ERR_WOULD_OVF
*                               RTOS_ERR_NET_IF_LINK_DOWN
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_WOULD_BLOCK
*                               RTOS_ERR_INVALID_STATE
*                               RTOS_ERR_ABORT
*                               RTOS_ERR_TIMEOUT
*                               RTOS_ERR_NET_OP_IN_PROGRESS
*                               RTOS_ERR_TX
*                               RTOS_ERR_NOT_FOUND
*                               RTOS_ERR_NET_INVALID_CONN
*                               RTOS_ERR_RX
*                               RTOS_ERR_NOT_READY
*                               RTOS_ERR_POOL_EMPTY
*                               RTOS_ERR_OS_OBJ_DEL
*                               RTOS_ERR_NET_NEXT_HOP
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE,  if NO error(s).
*               NET_SOCK_BSD_ERR_CLOSE, otherwise.
*
* Note(s)     : (1) Once an application closes its socket, NO further operations on the socket are allowed
*                   & the application MUST NOT continue to access the socket.
*
*                   Continued access to the closed socket by the application layer will likely corrupt
*                   the network socket layer.
*
*               (2) NO BSD socket error is returned for any internal error while closing the socket.
*
*               (3) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (4) [INTERNAL] If the socket is secure, a close notify alert MUST be transmitted to the
*                   remote host before closing the socket.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_Close (NET_SOCK_ID   sock_id,
                                  RTOS_ERR     *p_err)
{
    NET_SOCK           *p_sock   = DEF_NULL;
    NET_SOCK_RTN_CODE   rtn_code = NET_SOCK_BSD_ERR_CLOSE;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN         is_used;
#endif
#ifdef  NET_SECURE_MODULE_EN
    CPU_BOOLEAN         secure;
    RTOS_ERR            local_err;
#endif


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_CLOSE);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_Close);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
   is_used = NetSock_IsUsed(sock_id);
   if (is_used != DEF_YES) {
       RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
       goto exit_release;
   }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             rtn_code = NET_SOCK_BSD_ERR_CLOSE;
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

        case NET_SOCK_STATE_CLOSED:                             /* If CLOSED from init open  ...                        */
             NetSock_Free(p_sock);                              /* ... sock need ONLY be freed.                         */
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_SOCK_CLOSED);    /* Rtn net sock err and rtn BSD err (see Note #4).      */
             rtn_code = NET_SOCK_BSD_ERR_NONE;
             goto exit_release;

        case NET_SOCK_STATE_CLOSED_FAULT:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             break;

        case NET_SOCK_STATE_NONE:
        default:
             rtn_code = NET_SOCK_BSD_ERR_CLOSE;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);          /* Rtn net sock err but rtn NO BSD err (see Note #4).   */
             goto exit_release;
    }

                                                                /* -------------------- CLOSE SOCK -------------------- */
    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             NetSock_CloseHandler(p_sock, DEF_YES, DEF_NO);
             rtn_code = NET_SOCK_BSD_ERR_NONE;
             break;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
#ifdef  NET_SECURE_MODULE_EN
             secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
             if (secure == DEF_YES) {                           /* If sock secure, ...                                  */
                 RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                 NetSecure_SockCloseNotify(p_sock, &local_err); /* ... tx close notify alert (see Note #5).             */
             }
#endif
             rtn_code = NetSock_CloseHandlerStream(sock_id, p_sock, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  goto exit_release;
             }
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:                                                /* See Note #6.                                         */
             rtn_code = NET_SOCK_BSD_ERR_CLOSE;
             NetSock_CloseSockFromClose(p_sock);
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);           /* Rtn net sock err but rtn NO BSD err (see Note #4).   */
             goto exit_release;
    }


exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();


    return (rtn_code);
}


/*
*********************************************************************************************************
*                                           NetSock_Bind()
*
* Description : Bind a network socket to a local address.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to bind to a local address.
*
*               p_addr_local    Pointer to socket address structure.
*
*               addr_len        Length  of socket address structure (in octets).
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_TYPE
*                                   RTOS_ERR_POOL_EMPTY
*                                   RTOS_ERR_NOT_FOUND
*                                   RTOS_ERR_ALREADY_EXISTS
*                                   RTOS_ERR_INVALID_HANDLE
*                                   RTOS_ERR_NET_INVALID_CONN
*                                   RTOS_ERR_INVALID_STATE
*                                   RTOS_ERR_NET_CONN_CLOSED_FAULT
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE, if NO error(s).
*               NET_SOCK_BSD_ERR_BIND, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) (a) Socket address structure 'AddrFamily' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_Bind (NET_SOCK_ID         sock_id,
                                 NET_SOCK_ADDR      *p_addr_local,
                                 NET_SOCK_ADDR_LEN   addr_len,
                                 RTOS_ERR           *p_err)
{
    NET_SOCK_RTN_CODE  rtn_code = NET_SOCK_BSD_ERR_BIND;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN        is_used;
#endif


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_BIND);

   RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                 /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_Bind);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
   is_used = NetSock_IsUsed(sock_id);
   if (is_used != DEF_YES) {
       RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
       goto exit_release;
   }
#endif

                                                                /* -------------------- BIND SOCK --------------------- */
   rtn_code = NetSock_BindHandler(sock_id,
                                  p_addr_local,
                                  addr_len,
                                  DEF_NO,
                                  DEF_NULL,
                                  p_err);
   if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
       goto exit_release;
   }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                           NetSock_Conn()
*
* Description : Connect a network socket to a remote host.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to connect.
*
*               p_addr_remote   Pointer to socket address structure.
*
*               addr_len        Length of socket address structure (in octets).
*
*               p_err           Pointer to variable that will receive the return error code from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_TYPE
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_SEG_OVF
*                                   RTOS_ERR_NOT_SUPPORTED
*                                   RTOS_ERR_OS_SCHED_LOCKED
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_FAIL
*                                   RTOS_ERR_NET_INVALID_ADDR_SRC
*                                   RTOS_ERR_WOULD_OVF
*                                   RTOS_ERR_NET_IF_LINK_DOWN
*                                   RTOS_ERR_INVALID_HANDLE
*                                   RTOS_ERR_WOULD_BLOCK
*                                   RTOS_ERR_INVALID_STATE
*                                   RTOS_ERR_ABORT
*                                   RTOS_ERR_TIMEOUT
*                                   RTOS_ERR_NET_OP_IN_PROGRESS
*                                   RTOS_ERR_TX
*                                   RTOS_ERR_ALREADY_EXISTS
*                                   RTOS_ERR_NOT_FOUND
*                                   RTOS_ERR_NET_INVALID_CONN
*                                   RTOS_ERR_RX
*                                   RTOS_ERR_NOT_READY
*                                   RTOS_ERR_POOL_EMPTY
*                                   RTOS_ERR_OS_OBJ_DEL
*                                   RTOS_ERR_NET_NEXT_HOP
*                                   RTOS_ERR_NET_CONN_CLOSED_FAULT
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE, if NO error(s).
*               NET_SOCK_BSD_ERR_CONN, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) (a) Socket address structure 'AddrFamily' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_Conn (NET_SOCK_ID         sock_id,
                                 NET_SOCK_ADDR      *p_addr_remote,
                                 NET_SOCK_ADDR_LEN   addr_len,
                                 RTOS_ERR           *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN         is_used;
    CPU_BOOLEAN         valid;
#endif
#ifdef  NET_SECURE_MODULE_EN
    CPU_BOOLEAN         secure;
#endif
    NET_SOCK           *p_sock;
    NET_SOCK_RTN_CODE   rtn_code = NET_SOCK_BSD_ERR_CONN;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_CONN);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                 /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_Conn);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                         /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
         goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* --------------- VALIDATE REMOTE ADDR --------------- */
    valid = NetSock_IsValidAddrRemote(p_addr_remote, addr_len, p_sock, p_err);
    if (valid != DEF_YES) {
        goto exit_release;
    }
#else
    PP_UNUSED_PARAM(addr_len);                                  /* Prevent 'variable unused' compiler warning.          */
#endif


                                                                /* -------------------- CONN SOCK --------------------- */
    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             rtn_code = NetSock_ConnHandlerDatagram(sock_id, p_sock, p_addr_remote, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  rtn_code = NET_SOCK_BSD_ERR_CONN;
                  goto exit_release;
             }
             break;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             rtn_code = NetSock_ConnHandlerStream(sock_id, p_sock, p_addr_remote, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  rtn_code = NET_SOCK_BSD_ERR_CONN;
                  goto exit_release;
             }
#ifdef  NET_SECURE_MODULE_EN
             if (rtn_code != NET_SOCK_BSD_ERR_CONN) {
                 secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
                 if (secure == DEF_YES) {                       /* If sock secure, ...                                  */
                     DEF_BIT_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE_NEGO);
                     NetSecure_SockConn(p_sock, p_err);         /* ... handle conn secure session.                      */
                     DEF_BIT_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE_NEGO);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          rtn_code = NET_SOCK_BSD_ERR_CONN;
                          goto exit_release;
                     }
                 }
             }
#endif
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:                                                /* See Note #4.                                         */
             rtn_code = NET_SOCK_BSD_ERR_CONN;
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit_release;
    }


exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                          NetSock_Listen()
*
* Description : Set socket to listen for connection requests.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to listen.
*
*               sock_q_size     Maximum number of connection requests to accept & queue on listen socket.
*
*               p_err           Pointer to variable that will receive the return error code from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_TYPE
*                                   RTOS_ERR_POOL_EMPTY
*                                   RTOS_ERR_INVALID_HANDLE
*                                   RTOS_ERR_NET_INVALID_CONN
*                                   RTOS_ERR_INVALID_STATE
*                                   RTOS_ERR_NET_CONN_CLOSED_FAULT
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE,   if NO error(s).
*               NET_SOCK_BSD_ERR_LISTEN, otherwise.
*
* Note(s)     : (1) Socket listen operation valid for stream-type sockets only.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (3) [INTENRAL] On ANY error(s) after the transport connection is allocated, the transport
*                   connection MUST be freed.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
NET_SOCK_RTN_CODE  NetSock_Listen (NET_SOCK_ID       sock_id,
                                   NET_SOCK_Q_SIZE   sock_q_size,
                                   RTOS_ERR         *p_err)
{
    NET_SOCK           *p_sock;
    NET_CONN_ID         conn_id_transport;
    CPU_BOOLEAN         is_used           = DEF_NO;
    NET_SOCK_RTN_CODE   rtn_code          = NET_SOCK_BSD_ERR_LISTEN;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_LISTEN);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_Listen);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        rtn_code = NET_SOCK_BSD_ERR_CONN;
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                /* ---------------- VALIDATE SOCK TYPE ---------------- */
    switch (p_sock->SockType) {                                 /* Validate stream type sock(s) [see Note #3].          */
        case NET_SOCK_TYPE_DATAGRAM:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit_release;

        case NET_SOCK_TYPE_STREAM:
             break;

        case NET_SOCK_TYPE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }


                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

        case NET_SOCK_STATE_BOUND:
             break;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit_release;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             Net_GlobalLockRelease();
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_LISTEN);
    }


                                                                /* ---------------- CFG TRANSPORT CONN ---------------- */
                                                                /* Get transport conn.                                  */
    conn_id_transport = NetSock_GetConnTransport(p_sock, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }

                                                                /* Cfg transport to LISTEN.                             */
    NetTCP_ConnSetStateListen(conn_id_transport,
                             &NetSock_ListenQ_IsAvail,
                              p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        NetTCP_ConnFree((NET_TCP_CONN_ID)conn_id_transport);    /* If any errs, free transport conn (see Note #4).      */
        goto exit_release;
    }


                                                                /* -------------- UPDATE SOCK CONN STATE -------------- */
    NetSock_ConnAcceptQ_Init(p_sock, sock_q_size);              /* Init listen sock conn accept Q.                      */

    p_sock->State = NET_SOCK_STATE_LISTEN;

    rtn_code      = NET_SOCK_BSD_ERR_NONE;

    PP_UNUSED_PARAM(is_used);

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                          NetSock_Accept()
*
* Description : Return a new socket accepted from a listen socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of listen socket.
*
*               p_addr_remote   Pointer to an address buffer that will receive the socket address structure
*
*               p_addr_len      Pointer to a variable to pass the size of the socket address structure and that
*                                   will received the size of the accepted connection socket address structure,
*                                   if no errors, else a 0 size will be returned.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_TYPE
*                                   RTOS_ERR_NOT_FOUND
*                                   RTOS_ERR_NET_INVALID_CONN
*                                   RTOS_ERR_OS_SCHED_LOCKED
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_POOL_EMPTY
*                                   RTOS_ERR_OS_OBJ_DEL
*                                   RTOS_ERR_INVALID_HANDLE
*                                   RTOS_ERR_WOULD_BLOCK
*                                   RTOS_ERR_INVALID_STATE
*                                   RTOS_ERR_NET_CONN_CLOSED_FAULT
*                                   RTOS_ERR_ABORT
*                                   RTOS_ERR_TIMEOUT
*
* Return(s)   : Socket descriptor/handle identifier of new accepted socket, if NO error(s).
*               NET_SOCK_BSD_ERR_ACCEPT, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (3) [INTERNAL]
*                   (a) Socket address structure 'AddrFamily' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*
*               (4) Since 'p_addr_len' argument is both an input & output argument:
*
*                   (a) Its input value SHOULD be validated prior to use;
*                       (1) In the case that the 'p_addr_len' argument is passed a null pointer,
*                           NO input value is validated or used.
*
*                   (b) While its output value MUST be initially configured to return a default value
*                       PRIOR to all other validation or function handling in case of any error(s).
*
*               (5) Socket accept operation valid for stream-type sockets only.
*
*               (6) [INTERNAL] On ANY error(s), network resources MUST be appropriately freed :
*
*                   (a) PRIOR to network connection handle identifier dequeued from listen socket's
*                       connection accept queue, only the listen socket need be freed for certain
*                       errors; NO network resources need be freed.
*
*                   (b) After network connection handle identifier dequeued from listen socket's
*                       connection accept queue, free network connection.
*
*                   (c) After new accept socket allocated, free network connection & new socket.
*                       (1) Socket close handler frees network connection.
*
*               (7) [INTERNAL] Socket connection addresses are maintained in network-order.
*
*                   (a) However, since the port number & address are copied from a network-order multi-
*                       octet array directly into a network-order socket address structure, they do NOT
*                       need to be converted from host-order to network-order.
*
*              (8) [INTERNAL] Socket descriptor read availability determined by other socket handler
*                   function(s). For correct inter-operability between socket descriptor read handler
*                   functionality & all other appropriate socket handler function(s); ANY modification to
*                   any of these functions MUST be appropriately synchronized.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
NET_SOCK_ID  NetSock_Accept (NET_SOCK_ID         sock_id,
                             NET_SOCK_ADDR      *p_addr_remote,
                             NET_SOCK_ADDR_LEN  *p_addr_len,
                             RTOS_ERR           *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4  *p_addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6  *p_addr_ipv6;
#endif
#ifdef  NET_SECURE_MODULE_EN
    CPU_BOOLEAN          secure;
#endif
    CPU_INT08U           addr_remote[NET_SOCK_ADDR_LEN_MAX];
    NET_CONN_ADDR_LEN    addr_len;
    NET_SOCK            *p_sock_listen;
    NET_SOCK            *p_sock_accept;
    NET_SOCK_ID          sock_id_accept    = NET_SOCK_BSD_ERR_ACCEPT;
    NET_CONN_ID          conn_id_accept;
    NET_CONN_ID          conn_id_transport;
    CPU_BOOLEAN          block;
    CPU_BOOLEAN          is_used           = DEF_NO;
    RTOS_ERR             local_err;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_LISTEN);
    RTOS_ASSERT_DBG_ERR_SET((p_addr_remote != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_LISTEN);
    RTOS_ASSERT_DBG_ERR_SET((p_addr_len    != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_LISTEN);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                                                                        /* ------------- ACQUIRE NET LOCK ------------- */
    Net_GlobalLockAcquire((void *)NetSock_Accept);

    p_sock_listen = &NetSock_Tbl[sock_id];

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                        /* -------- VALIDATE LISTEN SOCK USED --------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    addr_len   = (NET_CONN_ADDR_LEN)*p_addr_len;
   *p_addr_len = 0u;                                            /* Cfg dflt addr len for err (see Note #4b).            */

    PP_UNUSED_PARAM(is_used);

                                                                        /* ----- VALIDATE LISTEN SOCK CONN STATE ------ */
    switch (p_sock_listen->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

        case NET_SOCK_STATE_LISTEN:
             break;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit_release;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             Net_GlobalLockRelease();
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_ACCEPT);
    }

                                                                        /* -------- VALIDATE LISTEN SOCK TYPE --------- */
    switch (p_sock_listen->SockType) {                                  /* Validate stream type sock(s) [see Note #5].  */
        case NET_SOCK_TYPE_STREAM:
             break;

        case NET_SOCK_TYPE_DATAGRAM:
        case NET_SOCK_TYPE_NONE:
        default:                                                        /* See Note #6.                                 */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit_release;
    }

                                                                /* --------------- VALIDATE CONNECTION ---------------- */
    is_used = NetConn_IsUsed(p_sock_listen->ID_Conn);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
        goto exit;
    }

                                                                        /* ------- WAIT ON LISTEN SOCK ACCEPT Q ------- */
    block = DEF_BIT_IS_CLR(p_sock_listen->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    if (block != DEF_YES) {                                             /* If non-blocking sock rx  ...                 */
        CPU_BOOLEAN  is_rdy_found;

        is_rdy_found = NetSock_ConnAcceptQ_IsRdy(p_sock_listen);        /* ... & no conn req's q'd, ...                 */
        if (is_rdy_found == DEF_NO) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_BLOCK);                 /* ... rtn  conn accept Q empty err.            */
            goto exit_release;
        }
    }


    Net_GlobalLockRelease();
    NetSock_ConnAcceptQ_Wait(p_sock_listen, p_err);
    Net_GlobalLockAcquire((void *)NetSock_Accept);

    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit_release;
    }

                                                                       /* Get conn id from sock conn accept Q.         */
    conn_id_accept = NetSock_ConnAcceptQ_ConnID_Get(p_sock_listen, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }
    if  (conn_id_accept == NET_CONN_ID_NONE) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
         goto exit_release;
    }

                                                                        /* Validate transport conn.                     */
    conn_id_transport = NetConn_ID_TransportGet(conn_id_accept);

    if (conn_id_transport == NET_CONN_ID_NONE) {
        NetSock_CloseConn(conn_id_accept);
        RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
        goto exit_release;
    }


                                                                        /* ----------- CFG NEW ACCEPT SOCK ------------ */
    p_sock_accept = NetSock_Get(p_err);
    if (p_sock_accept == DEF_NULL) {                                    /* See Note #8b.                                */
        NetSock_CloseConn(conn_id_accept);
        goto exit_release;                                              /* Rtn err from NetSock_Get().                  */
    }

                                                                        /* Copy listen sock into new accept sock.       */
    NetSock_Copy(p_sock_accept, p_sock_listen);

                                                                        /* Set new conn & app id's.                     */
    p_sock_listen->ConnChildQ_SizeCur++;
    p_sock_accept->ID_SockParent = p_sock_listen->ID;
    p_sock_accept->ID_Conn       = conn_id_accept;
    sock_id_accept               = p_sock_accept->ID;
    NetConn_ID_AppSet((NET_CONN_ID)conn_id_accept,
                      (NET_CONN_ID)sock_id_accept);


#ifndef  NET_TCP_CFG_OLD_WINDOW_MGMT_EN
    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                        /* Signal TCP that the connection is accepted.  */
    NetTCP_ConnAppAcceptRdySignal(p_sock_listen->ID_Conn, conn_id_transport, &local_err);
                                                                        /* Set the Rx Window (greater than 0).          */
#endif
                                                                        /* Update accept sock conn state.               */
    p_sock_accept->State = NET_SOCK_STATE_CONN;

#ifdef  NET_SECURE_MODULE_EN
    secure = DEF_BIT_IS_SET(p_sock_accept->Flags, NET_SOCK_FLAG_SOCK_SECURE);
    if (secure == DEF_YES) {                                            /* If sock secure, ...                          */
        DEF_BIT_SET(p_sock_accept->Flags, NET_SOCK_FLAG_SOCK_SECURE_NEGO);
        NetSecure_SockAccept(p_sock_listen, p_sock_accept, p_err);      /* ... handle accept secure session.            */
        DEF_BIT_CLR(p_sock_listen->Flags, NET_SOCK_FLAG_SOCK_SECURE_NEGO);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             NetSock_CloseHandler(p_sock_accept, DEF_YES, DEF_YES);
             goto exit_release;
        }
    }
#endif

                                                                        /* ------ RTN ACCEPT CONN'S REMOTE ADDR ------- */
                                                                        /* Get conn's remote addr.                      */
    addr_len = sizeof(addr_remote);
    NetConn_AddrRemoteGet(conn_id_accept,
                         &addr_remote[0],
                         &addr_len,
                          p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {                   /* See Note #8c.                                */
        NetSock_CloseHandler(p_sock_accept, DEF_YES, DEF_YES);
        sock_id_accept = NET_SOCK_BSD_ERR_ACCEPT;
        goto exit_release;
    }

                                                                        /* Cfg & rtn sock conn's remote addr.           */
    switch (p_sock_listen->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             p_addr_ipv4  = (NET_SOCK_ADDR_IPv4 *)p_addr_remote;        /* Cfg remote addr struct (see Notes #3 & #9b). */
             NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv4->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V4);
             NET_UTIL_VAL_COPY_16(&p_addr_ipv4->Port, &addr_remote[NET_SOCK_ADDR_IP_IX_PORT]);
             NET_UTIL_VAL_COPY_32(&p_addr_ipv4->Addr, &addr_remote[NET_SOCK_ADDR_IP_V4_IX_ADDR]);
             Mem_Clr(&p_addr_ipv4->Unused[0], NET_SOCK_ADDR_IPv4_NBR_OCTETS_UNUSED);
            *p_addr_len = (NET_SOCK_ADDR_LEN )NET_SOCK_ADDR_IPv4_SIZE;
             break;
#endif
#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
             p_addr_ipv6  = (NET_SOCK_ADDR_IPv6 *)p_addr_remote;        /* Cfg remote addr struct (see Notes #3 & #9b). */
             NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv6->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V6);
             NET_UTIL_VAL_COPY_16(&p_addr_ipv6->Port, &addr_remote[NET_SOCK_ADDR_IP_IX_PORT]);
             Mem_Copy(&p_addr_ipv6->Addr, &addr_remote[NET_SOCK_ADDR_IP_V6_IX_ADDR], NET_IPv6_ADDR_SIZE);
            *p_addr_len = (NET_SOCK_ADDR_LEN )NET_SOCK_ADDR_IPv6_SIZE;
             break;
#endif

        default:
                                                                        /* See Notes #7 & #8c.                          */
             NetSock_CloseHandler(p_sock_accept, DEF_YES, DEF_YES);
             sock_id_accept = NET_SOCK_BSD_ERR_ACCEPT;
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             Net_GlobalLockRelease();
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_ACCEPT);
    }

    PP_UNUSED_PARAM(local_err);


exit_release:
                                                                        /* ------------- RELEASE NET LOCK ------------- */
    Net_GlobalLockRelease();

exit:

    return (sock_id_accept);                                             /* ------------- RTN NEW SOCK ID -------------- */
}
#endif


/*
*********************************************************************************************************
*                                        NetSock_RxDataFrom()
*
* Description : Receive data from a network socket.
*
* Argument(s) : sock_id             Socket descriptor/handle identifier of socket to receive data.
*
*               p_data_buf          Pointer to an application data buffer that will receive the socket's received
*                                   data.
*
*               data_buf_len        Size of the   application data buffer (in octets).
*
*               flags               Flags to select receive options; bit-field flags logically OR'd :
*
*                                       NET_SOCK_FLAG_NONE
*                                       NET_SOCK_FLAG_RX_DATA_PEEK
*                                       NET_SOCK_FLAG_NO_BLOCK
*
*               p_addr_remote       Pointer to an address buffer that will receive the socket address structure.
*
*               p_addr_len          Pointer to a variable to pass the size of the socket address structure and that
*                                       will received the size of the accepted connection's socket address structure,
*                                       if no errors, else a 0 size will be returned.
*
*               p_ip_opts_buf       Pointer to buffer to receive possible IP options, if NO error(s).
*
*               ip_opts_buf_len     Size of IP options receive buffer (in octets).
*
*               p_ip_opts_len       Pointer to variable that will receive the return size of any received IP options,
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       RTOS_ERR_NONE
*                                       RTOS_ERR_NOT_FOUND
*                                       RTOS_ERR_NET_INVALID_CONN
*                                       RTOS_ERR_NET_CONN_CLOSE_RX
*                                       RTOS_ERR_OS_SCHED_LOCKED
*                                       RTOS_ERR_NOT_AVAIL
*                                       RTOS_ERR_OS_OBJ_DEL
*                                       RTOS_ERR_INVALID_HANDLE
*                                       RTOS_ERR_WOULD_BLOCK
*                                       RTOS_ERR_INVALID_STATE
*                                       RTOS_ERR_NET_CONN_CLOSED_FAULT
*                                       RTOS_ERR_ABORT
*                                       RTOS_ERR_TIMEOUT
*                                       RTOS_ERR_WOULD_OVF
*
* Return(s)   : Number of positive data octets received, if NO error(s).
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED, if socket connection closed.
*               NET_SOCK_BSD_ERR_RX, otherwise.
*
* Note(s)     : (1) (a) (1) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                           single, complete datagram transmitted MUST be received as a single, complete
*                           datagram.
*
*                       (2) Thus if the socket's type is datagram & the receive data buffer size is
*                           NOT large enough for the received data, the receive data buffer is maximally
*                           filled with receive data but the remaining data octets are discarded &
*                           RTOS_ERR_WOULD_OVF error is returned.
*
*                   (b) (1) (A) Stream-type sockets transmit & receive all data octets in one or more
*                               non-distinct packets.  In other words, the application data is NOT
*                               bounded by any specific packet(s); rather, it is contiguous & sequenced
*                               from one packet to the next.
*
*                           (B) Thus if the socket's type is stream & the receive data buffer size is NOT
*                               large enough for the received data, the receive data buffer is maximally
*                               filled with receive data & the remaining data octets remain queued for
*                               later application-socket receives.
*
*                       (2) Thus it is typical -- but NOT absolutely required -- that a single application
*                           task ONLY receive or request to receive data from a stream-type socket.
*
*               (2) Only some socket receive flag options are implemented.  If other flag options are
*                   requested, NetSock_RxData() handler function(s) abort & return appropriate error codes
*                   so that requested flag options are NOT silently ignored.
*
*               (3) (a) If :
*
*                       (1) NO IP options were received OR
*                       (2) NO IP options receive buffer is provided OR
*                       (3) IP options receive buffer NOT large enough for the received IP options
*
*                       then NO IP options are returned & any received IP options are silently discarded.
*
*                   (b) The IP options receive buffer size SHOULD be large enough to receive the maximum
*                       IP options size, NET_IPv4_HDR_OPT_SIZE_MAX.
*
*                   (c) Received IP options should be provided/decoded via appropriate IP layer API.
*
*               (4) IP options arguments may NOT be necessary (remove if unnecessary).
*
*               (5) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (6) [INTENRAL]
*                   (a) Socket address structure 'AddrFamily' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*
*               (7) [INTERNAL] Since 'p_addr_len' argument is both an input & output argument :
*
*                   (a) Its input value SHOULD be validated prior to use;
*                       (1) In the case that the 'p_addr_len' argument is passed a null pointer,
*                           NO input value is validated or used.
*
*                   (b) While its output value MUST be initially configured to return a default value
*                       PRIOR to all other validation or function handling in case of any error(s).

*
*               (8) [INTNERAL] Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*
*                   (a) However, the following pointers MUST NOT be re-configured to unused
*                       local variables : 'p_ip_opts_len'.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_RxDataFrom (NET_SOCK_ID          sock_id,
                                       void                *p_data_buf,
                                       CPU_INT16U           data_buf_len,
                                       NET_SOCK_API_FLAGS   flags,
                                       NET_SOCK_ADDR       *p_addr_remote,
                                       NET_SOCK_ADDR_LEN   *p_addr_len,
                                       void                *p_ip_opts_buf,
                                       CPU_INT08U           ip_opts_buf_len,
                                       CPU_INT08U          *p_ip_opts_len,
                                       RTOS_ERR            *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    NET_SOCK           *p_sock;
    CPU_BOOLEAN         is_used;
    NET_SOCK_ADDR_LEN   addr_len;
#endif
    NET_SOCK_RTN_CODE   rtn_code = NET_SOCK_BSD_ERR_RX;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_RX);
    RTOS_ASSERT_DBG_ERR_SET((p_addr_remote != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_RX);
    RTOS_ASSERT_DBG_ERR_SET((p_addr_len    != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_RX);


    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    if (p_ip_opts_len != DEF_NULL) {                            /* Init len for err (see Note #9).                      */
       *p_ip_opts_len  = 0u;
    }

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_RxDataFrom);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)

    addr_len   = *p_addr_len;

    is_used = NetSock_IsUsed(sock_id);
    if (is_used!= DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (NET_SOCK_BSD_ERR_RX);
    }

    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->ProtocolFamily) {
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
#ifdef  NET_IPv4_MODULE_EN
             if (addr_len != DEF_NULL                           /* Validate initial addr len (see Note #6a).   */
                 && addr_len < (NET_SOCK_ADDR_LEN)NET_SOCK_ADDR_IPv4_SIZE) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrLenCtr);
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_RX);
             }
#else
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_RX);
#endif
             break;


        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
#ifdef  NET_IPv6_MODULE_EN
             if (addr_len < (NET_SOCK_ADDR_LEN)NET_SOCK_ADDR_IPv6_SIZE) {/* Validate initial addr len (see Note #6a).   */
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrLenCtr);
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_RX);
             }
#else
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_RX);
#endif
             break;

        default:
             Net_GlobalLockRelease();
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             return (NET_SOCK_BSD_ERR_RX);
    }
#endif

   *p_addr_len =  0;                                            /* Cfg dflt addr len for err (see Note #6b).            */

                                                                /* -------------- VALIDATE/RX SOCK DATA --------------- */
    rtn_code = NetSock_RxDataHandler(sock_id,
                                     p_data_buf,
                                     data_buf_len,
                                     flags,
                                     p_addr_remote,
                                     p_addr_len,
                                     p_ip_opts_buf,
                                     ip_opts_buf_len,
                                     p_ip_opts_len,
                                     p_err);

                                                               /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                          NetSock_RxData()
*
* Description : Receive data from a network socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to receive data.
*
*               p_data_buf      Pointer to an application data buffer that will receive the socket's received
*                               data.
*
*               data_buf_len    Size of the application data buffer (in octets).
*
*               flags           Flags to select receive options; bit-field flags logically OR'd :
*
*                                   NET_SOCK_FLAG_NONE              No socket flags selected.
*                                   NET_SOCK_FLAG_RX_DATA_PEEK      Receive socket data without consuming
*                                                                   the socket data; i.e. socket data
*                                                                   NOT removed from application receive
*                                                                   queue(s).
*                                   NET_SOCK_FLAG_RX_NO_BLOCK       Receive socket data without blocking.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_NOT_FOUND
*                                   RTOS_ERR_NET_INVALID_CONN
*                                   RTOS_ERR_NET_CONN_CLOSE_RX
*                                   RTOS_ERR_OS_SCHED_LOCKED
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_OS_OBJ_DEL
*                                   RTOS_ERR_INVALID_HANDLE
*                                   RTOS_ERR_WOULD_BLOCK
*                                   RTOS_ERR_INVALID_STATE
*                                   RTOS_ERR_NET_CONN_CLOSED_FAULT
*                                   RTOS_ERR_ABORT
*                                   RTOS_ERR_TIMEOUT
*                                   RTOS_ERR_WOULD_OVF
*
* Return(s)   : Number of positive data octets received, if NO error(s).
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED, if socket connection closed.
*               NET_SOCK_BSD_ERR_RX, otherwise.
*
* Note(s)     : (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (3) (a) (1) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                           single, complete datagram transmitted MUST be received as a single, complete
*                           datagram.
*
*                       (2) Thus if the socket's type is datagram & the receive data buffer size is
*                           NOT large enough for the received data, the receive data buffer is maximally
*                           filled with receive data but the remaining data octets are discarded &
*                           RTOS_ERR_WOULD_OVF error is returned.
*
*                   (b) (1) (A) Stream-type sockets transmit & receive all data octets in one or more
*                               non-distinct packets.  In other words, the application data is NOT
*                               bounded by any specific packet(s); rather, it is contiguous & sequenced
*                               from one packet to the next.
*
*                           (B) Thus if the socket's type is stream & the receive data buffer size is NOT
*                               large enough for the received data, the receive data buffer is maximally
*                               filled with receive data & the remaining data octets remain queued for
*                               later application-socket receives.
*
*                       (2) Thus it is typical -- but NOT absolutely required -- that a single application
*                           task ONLY receive or request to receive data from a stream-type socket.
*
*               (4) Only some socket receive flag options are implemented.  If other flag options are
*                   requested, NetSock_RxData() handler function(s) abort & return appropriate error codes
*                   so that requested flag options are NOT silently ignored.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_RxData (NET_SOCK_ID          sock_id,
                                   void                *p_data_buf,
                                   CPU_INT16U           data_buf_len,
                                   NET_SOCK_API_FLAGS   flags,
                                   RTOS_ERR            *p_err)
{
    NET_SOCK_RTN_CODE  rtn_code = NET_SOCK_BSD_ERR_RX;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_RX);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    Net_GlobalLockAcquire((void *)NetSock_RxData);

                                                                /* -------------- VALIDATE/RX SOCK DATA --------------- */
    rtn_code = NetSock_RxDataHandler(sock_id,
                                     p_data_buf,
                                     data_buf_len,
                                     flags,
                                     DEF_NULL,
                                     DEF_NULL,
                                     DEF_NULL,
                                     0u,
                                     DEF_NULL,
                                     p_err);

    Net_GlobalLockRelease();

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                         NetSock_TxDataTo()
*
* Description : Transmit data through a network socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to transmit data.
*
*               p_data          Pointer to application data to transmit.
*
*               data_len        Length of  application data to transmit (in octets).
*
*               flags           Flags to select transmit options; bit-field flags logically OR'd :
*
*                                   NET_SOCK_FLAG_NONE              No socket flags selected.
*                                   NET_SOCK_FLAG_TX_NO_BLOCK       Transmit socket data without blocking.
*
*               p_addr_remote   Pointer to destination address buffer.
*
*               addr_len        Length of  destination address buffer (in octets).
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_TYPE
*                                   RTOS_ERR_NET_RETRY_MAX
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_NOT_SUPPORTED
*                                   RTOS_ERR_SEG_OVF
*                                   RTOS_ERR_OS_SCHED_LOCKED
*                                   RTOS_ERR_NOT_AVAIL
*                                   RTOS_ERR_FAIL
*                                   RTOS_ERR_NET_INVALID_ADDR_SRC
*                                   RTOS_ERR_WOULD_OVF
*                                   RTOS_ERR_NET_IF_LINK_DOWN
*                                   RTOS_ERR_INVALID_HANDLE
*                                   RTOS_ERR_WOULD_BLOCK
*                                   RTOS_ERR_INVALID_STATE
*                                   RTOS_ERR_ABORT
*                                   RTOS_ERR_TIMEOUT
*                                   RTOS_ERR_TX
*                                   RTOS_ERR_NOT_FOUND
*                                   RTOS_ERR_ALREADY_EXISTS
*                                   RTOS_ERR_NET_INVALID_CONN
*                                   RTOS_ERR_RX
*                                   RTOS_ERR_NOT_READY
*                                   RTOS_ERR_POOL_EMPTY
*                                   RTOS_ERR_OS_OBJ_DEL
*                                   RTOS_ERR_NET_NEXT_HOP
*                                   RTOS_ERR_NET_CONN_CLOSED_FAULT
*                                   RTOS_ERR_NET_ADDR_UNRESOLVED
*
* Return(s)   : Number of positive data octets transmitted, if NO error(s).
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED, if socket connection closed.
*               NET_SOCK_BSD_ERR_TX, otherwise.
*
* Note(s)     : (1) (a) (1) (A) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                               single, complete datagram transmitted MUST be received as a single, complete
*                               datagram.  Thus each call to transmit data MUST be transmitted in a single,
*                               complete datagram.
*
*                           (B) Since IP transmit fragmentation is NOT currently supported, if the socket's
*                               type is datagram & the requested transmit data length is greater than the
*                               socket/transport layer MTU, then NO data is transmitted &
*                               RTOS_ERR_WOULD_OVF error is returned.
*
*                       (2) (A) (1) Stream-type sockets transmit & receive all data octets in one or more
*                                   non-distinct packets.  In other words, the application data is NOT
*                                   bounded by any specific packet(s); rather, it is contiguous & sequenced
*                                   from one packet to the next.
*
*                               (2) Thus if the socket's type is stream & the socket's transmit data queue(s)
*                                   are NOT large enough for the transmitted data, the  transmit data queue(s)
*                                   are maximally filled with transmit data & the remaining data octets are
*                                   discarded but may be re-transmitted by later application-socket transmits.
*
*                               (3) Therefore, NO stream-type socket transmit data length should be "too long
*                                   to pass through the underlying protocol" & cause the socket transmit to
*                                   "fail ... [with] no data ... transmitted".
*
*                           (B) Thus it is typical -- but NOT absolutely required -- that a single application
*                               task ONLY transmit or request to transmit data to a stream-type socket.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*               (2) Only some socket transmit flag options are implemented.  If other flag options are
*                   requested, NetSock_TxData() handler function(s) abort & return appropriate error codes
*                   so that requested flag options are NOT silently ignored.
*
*               (3) Although NO socket send() specification states to return '0' when the socket's
*                   connection is closed, it seems reasonable to return '0' since it is possible for the
*                   socket connection to be close()'d or shutdown() by the remote host.
*
*               (4) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (5) [INTERNAL]
*                   (a) Socket address structure 'AddrFamily' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
**********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_TxDataTo (NET_SOCK_ID          sock_id,
                                     void                *p_data,
                                     CPU_INT16U           data_len,
                                     NET_SOCK_API_FLAGS   flags,
                                     NET_SOCK_ADDR       *p_addr_remote,
                                     NET_SOCK_ADDR_LEN    addr_len,
                                     RTOS_ERR            *p_err)
{
    NET_SOCK_RTN_CODE  rtn_code = NET_SOCK_BSD_ERR_DFLT;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, rtn_code);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* --------------- VALIDATE/TX APP DATA --------------- */
    rtn_code = NetSock_TxDataHandler(sock_id,
                                     p_data,
                                     data_len,
                                     flags,
                                     p_addr_remote,
                                     addr_len,
                                     p_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                          NetSock_TxData()
*
* Description : Transmit data through a socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to transmit data.
*
*               p_data      Pointer to application data to transmit.
*
*               data_len    Length of  application data to transmit (in octets).
*
*               flags       Flags to select transmit options; bit-field flags logically OR'd :
*
*                               NET_SOCK_FLAG_NONE              No socket flags selected.
*                               NET_SOCK_FLAG_TX_NO_BLOCK       Transmit socket data without blocking.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_NET_RETRY_MAX
*                               RTOS_ERR_BLK_ALLOC_CALLBACK
*                               RTOS_ERR_NOT_SUPPORTED
*                               RTOS_ERR_SEG_OVF
*                               RTOS_ERR_OS_SCHED_LOCKED
*                               RTOS_ERR_NOT_AVAIL
*                               RTOS_ERR_FAIL
*                               RTOS_ERR_NET_INVALID_ADDR_SRC
*                               RTOS_ERR_WOULD_OVF
*                               RTOS_ERR_NET_IF_LINK_DOWN
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_WOULD_BLOCK
*                               RTOS_ERR_INVALID_STATE
*                               RTOS_ERR_ABORT
*                               RTOS_ERR_TIMEOUT
*                               RTOS_ERR_TX
*                               RTOS_ERR_NOT_FOUND
*                               RTOS_ERR_ALREADY_EXISTS
*                               RTOS_ERR_NET_INVALID_CONN
*                               RTOS_ERR_RX
*                               RTOS_ERR_NOT_READY
*                               RTOS_ERR_POOL_EMPTY
*                               RTOS_ERR_OS_OBJ_DEL
*                               RTOS_ERR_NET_NEXT_HOP
*                               RTOS_ERR_NET_CONN_CLOSED_FAULT
*                               RTOS_ERR_NET_ADDR_UNRESOLVED
*
* Return(s)   : Number of positive data octets transmitted, if NO error(s).
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED, if socket connection closed.
*               NET_SOCK_BSD_ERR_TX, otherwise.
*
* Note(s)     : (1) (a) (1) (A) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                               single, complete datagram transmitted MUST be received as a single, complete
*                               datagram.  Thus each call to transmit data MUST be transmitted in a single,
*                               complete datagram.
*
*                           (B) Since IP transmit fragmentation is NOT currently supported (see 'net_ip.h
*                               Note #1d'), if the socket's type is datagram & the requested transmit
*                               data length is greater than the socket/transport layer MTU, then NO data
*                               is transmitted & RTOS_ERR_WOULD_OVF error is returned.
*
*                       (2) (A) (1) Stream-type sockets transmit & receive all data octets in one or more
*                                   non-distinct packets.  In other words, the application data is NOT
*                                   bounded by any specific packet(s); rather, it is contiguous & sequenced
*                                   from one packet to the next.
*
*                               (2) Thus if the socket's type is stream & the socket's transmit data queue(s)
*                                   are NOT large enough for the transmitted data, the  transmit data queue(s)
*                                   are maximally filled with transmit data & the remaining data octets are
*                                   discarded but may be re-transmitted by later application-socket transmits.
*
*                               (3) Therefore, NO stream-type socket transmit data length should be "too long
*                                   to pass through the underlying protocol" & cause the socket transmit to
*                                   "fail ... [with] no data ... transmitted" (see Note #3a1B1).
*
*                           (B) Thus it is typical -- but NOT absolutely required -- that a single application
*                               task ONLY transmit or request to transmit data to a stream-type socket.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*               (2) Only some socket transmit flag options are implemented. If other flag options are
*                   requested, NetSock_TxData() handler function(s) abort & return appropriate error codes
*                   so that requested flag options are NOT silently ignored.
*
*               (3) Although NO socket send() specification states to return '0' when the socket's
*                   connection is closed, it seems reasonable to return '0' since it is possible for the
*                   socket connection to be close()'d or shutdown() by the remote host.
*
*               (4) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
**********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_TxData (NET_SOCK_ID          sock_id,
                                   void                *p_data,
                                   CPU_INT16U           data_len,
                                   NET_SOCK_API_FLAGS   flags,
                                   RTOS_ERR            *p_err)
{
    NET_SOCK_RTN_CODE  rtn_code = NET_SOCK_BSD_ERR_DFLT;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, rtn_code);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* --------------- VALIDATE/TX APP DATA --------------- */
    rtn_code = NetSock_TxDataHandler(sock_id,
                                     p_data,
                                     data_len,
                                     flags,
                                     DEF_NULL,
                                     0u,
                                     p_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                           NetSock_Sel()
*
* Description : Check multiple sockets for available resources &/or operations.
*
* Argument(s) : sock_nbr_max        Maximum number of socket descriptors/handle identifiers in the socket
*                                   descriptor sets.
*
*               p_sock_desc_rd      Pointer to a set of socket descriptors/handle identifiers to :
*
*                                       (a) Check for available read operation(s).
*
*                                       (b) (1) Return the actual socket descriptors/handle identifiers
*                                                   ready for available read  operation(s), if NO error(s).
*                                           (2) Return the initial, non-modified set of socket descriptors/
*                                                   handle identifiers, on any error(s).
*                                           (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                                   if any timeout expires.
*
*               p_sock_desc_wr      Pointer to a set of socket descriptors/handle identifiers to :
*
*                                       (a) Check for available write operation(s).
*
*                                       (b) (1) Return the actual socket descriptors/handle identifiers
*                                                   ready for available write operation(s), if NO error(s).
*                                           (2) Return the initial, non-modified set of socket descriptors/
*                                                   handle identifiers, on any error(s).
*                                           (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                                   if any timeout expires.
*
*               p_sock_desc_err     Pointer to a set of socket descriptors/handle identifiers to :
*
*                                       (a) Check for any error(s) and/or exception(s).
*
*                                       (b) (1) Return the actual socket descriptors/handle identifiers
*                                                   flagged with any error(s) &/or exception(s), if NO
*                                                   non-descriptor-related error(s).
*                                           (2) Return the initial, non-modified set of socket descriptors/
*                                                   handle identifiers, on any error(s).
*                                           (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                                   if any timeout expires.
*
*               p_timeout           Pointer to a timeout.
*
*               p_err               Pointer to the variable that will receive one of the following error code(s) from this function:
*
*                                       RTOS_ERR_NONE
*                                       RTOS_ERR_INVALID_TYPE
*                                       RTOS_ERR_NET_INVALID_CONN
*                                       RTOS_ERR_BLK_ALLOC_CALLBACK
*                                       RTOS_ERR_SEG_OVF
*                                       RTOS_ERR_OS_SCHED_LOCKED
*                                       RTOS_ERR_NOT_AVAIL
*                                       RTOS_ERR_OS_ILLEGAL_RUN_TIME
*                                       RTOS_ERR_POOL_EMPTY
*                                       RTOS_ERR_OS_OBJ_DEL
*                                       RTOS_ERR_INVALID_HANDLE
*                                       RTOS_ERR_WOULD_BLOCK
*                                       RTOS_ERR_INVALID_STATE
*                                       RTOS_ERR_NET_CONN_CLOSED_FAULT
*                                       RTOS_ERR_TIMEOUT
*                                       RTOS_ERR_ABORT
*
* Return(s)   : Number of socket descriptors with available resources &/or operations, if any.
*               NET_SOCK_BSD_RTN_CODE_TIMEOUT, on timeout.
*               NET_SOCK_BSD_ERR_SEL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) (a) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that :
*
*                       (1) (A) "select() ... shall support" the following file descriptor types :
*
*                               (1) "regular files,"                        ...
*                               (2) "terminal and pseudo-terminal devices," ...
*                               (3) "STREAMS-based files,"                  ...
*                               (4) "FIFOs,"                                ...
*                               (5) "pipes,"                                ...
*                               (6) "sockets."
*
*                           (B) "The behavior of ... select() on ... other types of ... file descriptors
*                                ... is unspecified."
*
*                       (2) Network Socket Layer supports BSD 4.x select() functionality with the following
*                           restrictions/constraints :
*
*                           (A) ONLY supports the following file descriptor types :
*                               (1) Sockets
*
*                   (b) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that :
*
*                       (1) (A) "The 'nfds' argument ('sock_nbr_max') specifies the range of descriptors to be
*                                tested.  The first 'nfds' descriptors shall be checked in each set; that is,
*                                the [following] descriptors ... in the descriptor sets shall be examined" :
*
*                               (1) "from zero" ...
*                               (2) "through nfds-1."
*
*                           (B) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                               6th Printing, Section 6.3, Page 163 states that :
*
*                               (1) ... since "descriptors start at 0" ...
*
*                               (2) "the ['nfds'] argument" specifies :
*                                   (a) "the number of descriptors," ...
*                                   (b) "not the largest value."
*
*                       (2) "The select() function shall ... examine the file descriptor sets whose addresses
*                            are passed in the 'readfds' ('p_sock_desc_rd'), 'writefds' ('p_sock_desc_wr'), and
*                            'errorfds' ('p_sock_desc_err') parameters to see whether some of their descriptors
*                            are ready for reading, are ready for writing, or have an exceptional condition
*                            pending, respectively."
*
*                           (A) (1) (a) "If the 'readfds'  argument ('p_sock_desc_rd')  is not a null pointer,
*                                        it points to an object of type 'fd_set' ('NET_SOCK_DESC') that on
*                                        input specifies the file descriptors to be checked for being ready
*                                        to read,  and on output indicates which file descriptors are ready
*                                        to read."
*
*                                   (b) "A descriptor shall be considered ready for reading when a call to
*                                        an input function ... would not block, whether or not the function
*                                        would transfer data successfully.  (The function might return data,
*                                        an end-of-file indication, or an error other than one indicating
*                                        that it is blocked, and in each of these cases the descriptor shall
*                                        be considered ready for reading.)" :
*
*                                       (1) "If the socket is currently listening, then it shall be marked
*                                            as readable if an incoming connection request has been received,
*                                            and a call to the accept() function shall complete without
*                                            blocking."
*
*                                   (c) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                                       6th Printing, Section 6.3, Pages 164-165 states that "a socket is ready
*                                       for reading if any of the following ... conditions is true" :
*
*                                       (1) "A read operation on the socket will not block and will return a
*                                            value greater than 0 (i.e., the data that is ready to be read)."
*
*                                       (2) "The read half of the connection is closed (i.e., a TCP connection
*                                            that has received a FIN).  A read operation ... will not block and
*                                            will return 0 (i.e., EOF)."
*
*                                       (3) "The socket is a listening socket and the number of completed
*                                            connections is nonzero.  An accept() on the listening socket
*                                            will ... not block."
*
*                                       (4) "A socket error is pending.  A read operation on the socket will
*                                            not block and will return an error (-1) with 'errno' set to the
*                                            specific error condition."
*
*                                           (A) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1,
*                                               3rd Edition, 6th Printing, Section 6.3, Page 165 states "that
*                                               when an error occurs on a socket, it is [also] marked as both
*                                               readable and writeable by select()".
*
*                                               See also Notes #3b2A2c4A & #3b2A3d.
*
*                               (2) (a) "If the 'writefds' argument ('p_sock_desc_wr')  is not a null pointer,
*                                        it points to an object of type 'fd_set' ('NET_SOCK_DESC') that on
*                                        input specifies the file descriptors to be checked for being ready
*                                        to write, and on output indicates which file descriptors are ready
*                                        to write."
*
*                                   (b) "A descriptor shall be considered ready for writing when a call to
*                                        an output function ... would not block, whether or not the function
*                                        would transfer data successfully" :
*
*                                       (1) "If a non-blocking call to the connect() function has been made
*                                            for a socket, and the connection attempt has either succeeded
*                                            or failed leaving a pending error, the socket shall be marked
*                                            as writable."
*
*                                   (c) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                                       6th Printing, Section 6.3, Page 165 states that "a socket is ready for
*                                       writing if any of the following ... conditions is true" :
*
*                                       (1) "A write operation will not block and will return a positive value
*                                            (e.g., the number of bytes accepted by the transport layer)."
*
*                                       (2) "The write half of the connection is closed."
*
*                                       (3) "A socket using a non-blocking connect() has completed the
*                                            connection, or the connect() has failed."
*
*                                       (4) "A socket error is pending.  A write operation on the socket will
*                                            not block and will return an error (-1) with 'errno' set to the
*                                            specific error condition."
*
*                                           (A) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1,
*                                               3rd Edition, 6th Printing, Section 6.3, Page 165 states "that
*                                               when an error occurs on a socket, it is [also] marked as both
*                                               readable and writeable by select()".
*
*                                               See also Notes #3b2A1c4A & #3b2A3d.
*
*                               (3) (a) "If the 'errorfds' argument ('p_sock_desc_err') is not a null pointer,
*                                        it points to an object of type 'fd_set' ('NET_SOCK_DESC') that on
*                                        input specifies the file descriptors to be checked for error
*                                        conditions pending, and on output indicates which file descriptors
*                                        have error conditions pending."
*
*                                   (b) "A file descriptor ... shall be considered to have an exceptional
*                                        condition pending ... as noted below" :
*
*                                       (1) (A) "A socket ... receive operation ... [that] would return
*                                                out-of-band data without blocking."
*                                           (B) "A socket ... [with] out-of-band data ... present in the
*                                                receive queue."
*
*                                           Out-of-band data NOT supported.
*
*                                       (2) "If a socket has a pending error."
*
*                                       (3) "Other circumstances under which a socket may be considered to
*                                            have an exceptional condition pending are protocol-specific
*                                            and implementation-defined."
*
*                                   (c) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                                       6th Printing, Section 6.3, Page 165 states that "a socket has an
*                                       exception condition pending if ... any of the following ... conditions
*                                       is true" :
*
*                                       (1) (A) "Out-of-band data for the socket" is currently available; ...
*                                           (B) "Or the socket is still at the out-of-band mark."
*
*                                           Out-of-band data NOT supported (see 'net_tcp.h  Note #1b').
*
*                                   (d) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                                       6th Printing, Section 6.3, Page 165 states "that when an error occurs on
*                                       a socket, it is [also] marked as both readable and writeable by select()".
*
*                           (B) (1) (a) "Upon successful completion, ... select() ... shall" :
*
*                                       (1) "modify the objects pointed to by the 'readfds' ('p_sock_desc_rd'),
*                                            'writefds' ('p_sock_desc_wr'), and 'errorfds' ('p_sock_desc_err')
*                                            arguments to indicate which file descriptors are ready for
*                                            reading, ready for writing, or have an error condition pending,
*                                            respectively," ...
*
*                                       (2) "and shall return the total number of ready descriptors in all
*                                            the output sets."
*
*                                   (b) (1) "For each file descriptor less than nfds ('sock_nbr_max'), the
*                                            corresponding bit shall be set on successful completion" :
*
*                                           (A) "if it was set on input" ...
*                                           (B) "and the associated condition is true for that file descriptor."
*
*                                       (2) Conversely, "for each file descriptor ... the corresponding bit
*                                           shall be ... [clear] ... on ... completion" :
*
*                                           (A) If it was clear on input;                                         ...
*                                           (B) If the associated condition is NOT true for that file descriptor; ...
*                                           (C) Or it was set on input, but any timeout occurs (see Note #3c2B).
*
*                               (2) select() can NOT absolutely guarantee that descriptors returned as ready
*                                   will still be ready during subsequent operations since any higher priority
*                                   tasks or processes may asynchronously consume the descriptors' operations
*                                   &/or resources.  This can occur since select() functionality & subsequent
*                                   operations are NOT atomic operations protected by network, file system,
*                                   or operating system mechanisms.
*
*                                   However, as long as no higher priority tasks or processes access any of
*                                   the same descriptors, then a single task or process can assume that all
*                                   descriptors returned as ready by select() will still be ready during
*                                   subsequent operations.
*
*                       (3) (A) "The 'timeout' parameter ('p_timeout') controls how long ... select() ... shall
*                                take before timing out."
*
*                               (1) (a) "If the 'timeout' parameter ('p_timeout') is not a null pointer, it
*                                        specifies a maximum interval to wait for the selection to complete."
*
*                                       (1) "If none of the selected descriptors are ready for the requested
*                                            operation, ... select() ... shall block until at least one of the
*                                            requested operations becomes ready ... or ... until the timeout
*                                            occurs."
*
*                                       (2) "If the specified time interval expires without any requested
*                                            operation becoming ready, the function shall return."
*
*                                       (3) "To effect a poll, the 'timeout' parameter ('p_timeout') should not be
*                                            a null pointer, and should point to a zero-valued timespec structure
*                                            ('NET_SOCK_TIMEOUT')."
*
*                                   (b) (1) (A) "If the 'readfds' ('p_sock_desc_rd'), 'writefds' ('p_sock_desc_wr'),
*                                                and 'errorfds' ('p_sock_desc_err') arguments are"                ...
*                                               (1) "all null pointers"                                          ...
*                                               (2) [or all null-valued (i.e. no file descriptors set)]          ...
*                                           (B) "and the 'timeout' argument ('p_timeout') is not a null pointer," ...
*
*                                       (2) ... then "select() ... shall block for the time specified".
*
*                               (2) "If the 'timeout' parameter ('p_timeout') is a null pointer, then the call to
*                                    ... select() shall block indefinitely until at least one descriptor meets the
*                                    specified criteria."
*
*                                   (a) (1) Although IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION'
*                                           states that select() may "block indefinitely until ... one of the
*                                           requested operations becomes ready ... or until interrupted by a signal",
*                                           it does NOT explicity specify how to handle the case where the descriptor
*                                           arguments & the timeout parameter argument are all NULL pointers.
*
*                                           Therefore, it seems reasonable that select() should "block indefinitely
*                                           ... until interrupted by a signal" if the descriptor arguments & the
*                                           timeout parameter argument are all NULL pointers.
*
*                                       (2) However, since inter-process signals are NOT currently supported, it
*                                           does NOT seem reasonable to block a task or process indefinitely (i.e.
*                                           forever) if the descriptor arguments & the timeout parameter argument
*                                           are all NULL pointers.  Instead, an 'invalid timeout interval' error
*                                           should be returned.
*
*                                           See also Note #3d2B.
*
*                           (B) (1) "For the select() function, the timeout period is given ... in an argument
*                                   ('p_timeout') of type struct 'timeval' ('NET_SOCK_TIMEOUT')" ... :
*
*                                   (a) "in seconds" ...
*                                   (b) "and microseconds."
*
*                               (2) (a) (1) "Implementations may place limitations on the maximum timeout interval
*                                            supported" :
*
*                                           (A) "All implementations shall support a maximum timeout interval of
*                                                at least 31 days."
*
*                                               (1) However, since maximum timeout interval values are dependent
*                                                   on the specific OS implementation; a maximum timeout interval
*                                                   can NOT be guaranteed.
*
*                                           (B) "If the 'timeout' argument ('p_timeout') specifies a timeout interval
*                                                greater than the implementation-defined maximum value, the maximum
*                                                value shall be used as the actual timeout value."
*
*                                       (2) "Implementations may also place limitations on the granularity of
*                                            timeout intervals" :
*
*                                           (A) "If the requested 'timeout' interval requires a finer granularity
*                                                than the implementation supports, the actual timeout interval
*                                                shall be rounded up to the next supported value."
*
*                                   (b) Operating systems may have different minimum/maximum ranges & resolutions
*                                       for timeouts while pending or waiting on an operating system resource to
*                                       become available (see Note #3b3A1a) versus time delays for suspending a
*                                       task or process that is NOT pending or waiting for an operating system
*                                       resource (see Note #3b3A1b).
*
*                   (c) (1) (A) IEEE Std 1003.1, 2004 Edition, Section 'select() : RETURN VALUE' states that :
*
*                               (1) "Upon successful completion, ... select() ... shall return the total
*                                    number of bits set in the bit masks."
*
*                               (2) (a) "Otherwise, -1 shall be returned," ...
*                                   (b) "and 'errno' shall be set to indicate the error."
*                                       'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                           (B) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                               6th Printing, Section 6.3, Page 161 states that BSD select() function
*                               "returns ... 0 on timeout".
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that :
*
*                           (A) "On failure, the objects pointed to by the 'readfds' ('p_sock_desc_rd'),
*                                'writefds' ('p_sock_desc_wr'), and 'errorfds' ('p_sock_desc_err') arguments
*                                shall not be modified."
*
*                               (1) Since 'p_sock_desc_rd', 'p_sock_desc_wr', & 'p_sock_desc_err' arguments
*                                   are both input & output arguments; their input values, prior to use,
*                                   MUST be copied to return their initial input values PRIOR to all other
*                                   validation or function handling in case of any error(s).
*
*                           (B) "If the 'timeout' interval expires without the specified condition being
*                                true for any of the specified file descriptors, the objects pointed to
*                                by the 'readfds' ('p_sock_desc_rd'), 'writefds' ('p_sock_desc_wr'), and
*                                'errorfds' ('p_sock_desc_err') arguments shall have all bits set to 0."
*
*                   (d) IEEE Std 1003.1, 2004 Edition, Section 'select() : ERRORS' states that "under the
*                       following conditions, ... select() shall fail and set 'errno' to" :
*
*                       (1) "[EBADF] - One or more of the file descriptor sets specified a file descriptor
*                            that is not a valid open file descriptor."
*
*                       (2) "[EINVAL]" -
*
*                           (A) "The 'nfds' argument ('sock_nbr_max') is" :
*                               (1) "less than 0 or" ...
*                               (2) "greater than FD_SETSIZE."
*
*                           (B) "An invalid timeout interval was specified."
*
*                           'errno' NOT currently supported.
*
*               (3) A socket events table lists requested socket or connection ID numbers/handle identifiers
*                   & their respective socket event operations.
*
*                   (a) Socket event tables are terminated with NULL table entry values.
*
*                   (b) (1) NET_SOCK_CFG_SEL_NBR_EVENTS_MAX configures socket event tables' maximum number
*                           of socket events/operations.
*
*                       (2) This value is used to declare the size of the socket events tables.  Note that
*                           one additional table entry is added for a terminating NULL table entry at a
*                           maximum table index 'NET_SOCK_CFG_SEL_NBR_EVENTS_MAX'.
*
*               (4) Since datagram-type sockets typically never wait on transmit operations, no socket
*                   event should wait on datagram-type socket transmit operations or transmit errors.
*********************************************************************************************************
*/
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
NET_SOCK_RTN_CODE  NetSock_Sel (NET_SOCK_QTY       sock_nbr_max,
                                NET_SOCK_DESC     *p_sock_desc_rd,
                                NET_SOCK_DESC     *p_sock_desc_wr,
                                NET_SOCK_DESC     *p_sock_desc_err,
                                NET_SOCK_TIMEOUT  *p_timeout,
                                RTOS_ERR          *p_err)
{
    NET_SOCK_SEL_OBJ   *p_sel_obj = DEF_NULL;
    KAL_SEM_HANDLE      task_sem;
    NET_SOCK           *p_sock;
    NET_SOCK_ID         sock_id;
    NET_SOCK_QTY        sock_nbr_max_actual;
    NET_SOCK_RTN_CODE   sock_nbr_rdy;
    NET_SOCK_DESC       sock_desc_rtn_rd;
    NET_SOCK_DESC       sock_desc_rtn_wr;
    NET_SOCK_DESC       sock_desc_rtn_err;
    CPU_BOOLEAN         sock_desc_used_rd;
    CPU_BOOLEAN         sock_desc_used_wr;
    CPU_BOOLEAN         sock_desc_used_err;
    NET_SOCK_SEL_OBJ   *p_sel_obj_cur  = DEF_NULL;
    NET_SOCK_SEL_OBJ   *p_sel_obj_next = DEF_NULL;
    CPU_INT32U          timeout_sec;
    CPU_INT32U          timeout_us;
    CPU_INT32U          timeout_ms;
    RTOS_ERR            local_err;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_SEL);

    RTOS_ASSERT_DBG_ERR_SET((sock_nbr_max <= (NET_SOCK_QTY)NET_SOCK_DESC_NBR_MAX_DESC), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
#if (NET_SOCK_DESC_NBR_MIN_DESC > DEF_INT_16U_MIN_VAL)          /* (see Note #3d2A1).                                   */
    RTOS_ASSERT_DBG_ERR_SET((sock_nbr_max >= NET_SOCK_DESC_NBR_MIN_DESC), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
#endif

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* ----------------- VALIDATE TIMEOUT ----------------- */
    if (p_timeout != DEF_NULL) {
        if (p_timeout->timeout_sec < 0) {
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
        }
        if (p_timeout->timeout_us  < 0) {
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
        }
    }
#endif

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_Sel);

                                                                /* ---- VALIDATE / CHK / CFG SOCK DESC(S) FOR RDY ----- */
    sock_nbr_max_actual = DEF_MIN(sock_nbr_max, (NET_SOCK_ID)NET_SOCK_DESC_NBR_MAX_DESC);

    if (sock_nbr_max_actual > 0) {
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_COPY(&sock_desc_rtn_rd,  p_sock_desc_rd);
        } else {
            NET_SOCK_DESC_INIT(&sock_desc_rtn_rd);
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_COPY(&sock_desc_rtn_wr,  p_sock_desc_wr);
        } else {
            NET_SOCK_DESC_INIT(&sock_desc_rtn_wr);
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_COPY(&sock_desc_rtn_err,  p_sock_desc_err);
        } else {
            NET_SOCK_DESC_INIT(&sock_desc_rtn_err);
        }

        sock_nbr_rdy = NetSock_SelDescHandler(sock_nbr_max_actual,
                                             &sock_desc_rtn_rd,
                                             &sock_desc_rtn_wr,
                                             &sock_desc_rtn_err,
                                              p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            if (sock_nbr_rdy == 0) {
                goto exit_release;
            }
        }
    } else {
        sock_nbr_rdy = 0;
    }



    if (sock_nbr_rdy != 0) {                                    /* If any     sock desc's rdy,                  ..      */
                                                                /* .. rtn rdy sock desc's (see Note #3b2B1a1) & ..      */
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_COPY(p_sock_desc_rd,  &sock_desc_rtn_rd );
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_COPY(p_sock_desc_wr,  &sock_desc_rtn_wr );
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_COPY(p_sock_desc_err, &sock_desc_rtn_err);
        }

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        goto exit_release;
    }


                                                                /* ---------- CFG TIMEOUT / HANDLE TIME DLY ----------- */
    if (p_timeout != DEF_NULL) {                                /* If avail, cfg timeout vals    (see Note #3b3A1),     */
        timeout_sec = (CPU_INT32U)p_timeout->timeout_sec;
        timeout_us  = (CPU_INT32U)p_timeout->timeout_us;
    } else {                                                    /* .. else  cfg timeout to block (see Note #3b3A2).     */
        timeout_sec = NET_TMR_TIME_INFINITE;
        timeout_us  = NET_TMR_TIME_INFINITE;
    }


    if ((timeout_sec == 0) && (timeout_us == 0)) {              /* If timeout = 0, handle as non-blocking poll ..       */
                                                                /* .. timeout (see Note #3b3A1a3).                      */

                                                                /* Zero-clr rtn sock desc's (see Note #3c2B).           */
                                                                /* Clr because no sock is ready at this point.          */
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_rd );
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_wr );
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_err);
        }

        sock_nbr_rdy = NET_SOCK_BSD_RTN_CODE_TIMEOUT;
        RTOS_ERR_SET(*p_err, RTOS_ERR_TIMEOUT);
        goto exit_release;
    }


    if (sock_nbr_max < 1) {                                     /* If NO sock events cfg'd to wait on ...               */

        Net_GlobalLockRelease();

        if (p_timeout == DEF_NULL) {                            /* ... & NO timeout req'd,            ...               */
                                                                /* ... rtn err (see Note #3b3A2a2).                     */
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
        }

        RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

        Net_TimeDly(timeout_sec, timeout_us, &local_err);       /* Else dly for timeout (see Note #3b3A1b2).            */
        PP_UNUSED_PARAM(local_err);

                                                                /* Zero-clr rtn sock desc's (see Note #3c2B).           */
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_rd );
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_wr );
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_err);
        }

        sock_nbr_rdy = NET_SOCK_BSD_RTN_CODE_TIMEOUT;
        RTOS_ERR_SET(*p_err, RTOS_ERR_TIMEOUT);
        goto exit;
    }


    task_sem  =  KAL_SemCreate("Net Sock Sel Task",
                                DEF_NULL,
                                p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         sock_nbr_rdy = NET_SOCK_BSD_ERR_SEL;
         RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
         goto exit_release;
    }

    for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {
         p_sock = &NetSock_Tbl[sock_id];

         if (p_sock_desc_rd != DEF_NULL) {
             sock_desc_used_rd = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_rd);
         } else {
             sock_desc_used_rd = DEF_NO;
         }

         if (p_sock_desc_wr != DEF_NULL) {
             sock_desc_used_wr = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_wr);
         } else {
             sock_desc_used_wr = DEF_NO;
         }

         if (p_sock_desc_err != DEF_NULL) {
             sock_desc_used_err = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_err);
         } else {
             sock_desc_used_err = DEF_NO;
         }


         if ((sock_desc_used_rd  == DEF_YES) ||
             (sock_desc_used_wr  == DEF_YES) ||
             (sock_desc_used_err == DEF_YES)) {

             p_sel_obj = (NET_SOCK_SEL_OBJ *)Mem_DynPoolBlkGet(&NetSock_SelObjPool, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  sock_nbr_rdy = NET_SOCK_BSD_ERR_SEL;
                  RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
                  goto exit_release;
             }

             p_sel_obj->SockSelPendingFlags = NET_SOCK_SEL_EVENT_FLAG_NONE;

             if (sock_desc_used_rd == DEF_YES) {
                 p_sel_obj->SockSelPendingFlags |= NET_SOCK_SEL_EVENT_FLAG_RD;
             }

             if (sock_desc_used_wr == DEF_YES) {
                 p_sel_obj->SockSelPendingFlags |= NET_SOCK_SEL_EVENT_FLAG_WR;
             }

             if (sock_desc_used_err == DEF_YES) {
                 p_sel_obj->SockSelPendingFlags |= NET_SOCK_SEL_EVENT_FLAG_ERR;
             }

             p_sel_obj->SockSelTaskSignalObj = task_sem;
             p_sel_obj->ObjPrevPtr           = p_sock->SelObjTailPtr;
             p_sock->SelObjTailPtr           = p_sel_obj;
         }
    }


    timeout_ms = NetUtil_TimeSec_uS_To_ms(timeout_sec, timeout_us);
    if (timeout_ms == NET_TMR_TIME_INFINITE) {
        timeout_ms =  KAL_TIMEOUT_INFINITE;
    }

                                                                /* ------ WAIT ON MULTIPLE SOCK DESC'S / EVENTS ------- */
    Net_GlobalLockRelease();

    KAL_SemPend(task_sem, KAL_OPT_PEND_NONE, timeout_ms, p_err);

    Net_GlobalLockAcquire((void *)NetSock_Sel);

    for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {
         p_sock         = &NetSock_Tbl[sock_id];


         if (p_sock_desc_rd != DEF_NULL) {
             sock_desc_used_rd = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_rd);
         } else {
             sock_desc_used_rd = DEF_NO;
         }

         if (p_sock_desc_wr != DEF_NULL) {
             sock_desc_used_wr = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_wr);
         } else {
             sock_desc_used_wr = DEF_NO;
         }

         if (p_sock_desc_err != DEF_NULL) {
             sock_desc_used_err = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_err);
         } else {
             sock_desc_used_err = DEF_NO;
         }


         if (p_sock->SelObjTailPtr != DEF_NULL) {               /* Remove p_sel_obj from Socket Select objects list     */

             p_sel_obj_cur  = p_sock->SelObjTailPtr;
             p_sel_obj_next = DEF_NULL;

             while (p_sel_obj_cur != DEF_NULL) {
                 NET_SOCK_SEL_OBJ  *p_sel_obj_prev = p_sel_obj_cur->ObjPrevPtr;


                 if (p_sel_obj_cur->SockSelTaskSignalObj.SemObjPtr == task_sem.SemObjPtr) {
                     NET_SOCK_SEL_OBJ   *p_sel_obj_to_free  = DEF_NULL;


                     if (p_sock->SelObjTailPtr == p_sel_obj_cur) {
                         p_sock->SelObjTailPtr  = p_sel_obj_prev;

                     } else if (p_sel_obj_next != DEF_NULL) {
                         p_sel_obj_next->ObjPrevPtr = p_sel_obj_prev;
                     }

                     p_sel_obj_to_free = p_sel_obj_cur;
                     p_sel_obj_cur  = p_sel_obj_cur->ObjPrevPtr;

                     RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                     Mem_DynPoolBlkFree(&NetSock_SelObjPool, p_sel_obj_to_free, &local_err);
                     RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, 0);

                 } else {
                     p_sel_obj_next = p_sel_obj_cur;
                     p_sel_obj_cur  = p_sel_obj_cur->ObjPrevPtr;
                 }
             }
         }
    }

    KAL_SemDel(task_sem);

    switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_TIMEOUT:
             if (p_sock_desc_rd != DEF_NULL) {
                 NET_SOCK_DESC_INIT(p_sock_desc_rd );
             }

             if (p_sock_desc_wr != DEF_NULL) {
                 NET_SOCK_DESC_INIT(p_sock_desc_wr );
             }

             if (p_sock_desc_err != DEF_NULL) {
                 NET_SOCK_DESC_INIT(p_sock_desc_err);
             }
             sock_nbr_rdy = NET_SOCK_BSD_RTN_CODE_TIMEOUT;
             goto exit_release;

        default:
             sock_nbr_rdy = NET_SOCK_BSD_ERR_SEL;
             goto exit_release;
    }


    if (p_sock_desc_rd != DEF_NULL) {
        NET_SOCK_DESC_COPY(&sock_desc_rtn_rd, p_sock_desc_rd);
    } else {
        NET_SOCK_DESC_INIT(&sock_desc_rtn_rd);
    }

    if (p_sock_desc_wr != DEF_NULL) {
        NET_SOCK_DESC_COPY(&sock_desc_rtn_wr, p_sock_desc_wr);
    } else {
        NET_SOCK_DESC_INIT(&sock_desc_rtn_wr);
    }

    if (p_sock_desc_err != DEF_NULL) {
        NET_SOCK_DESC_COPY(&sock_desc_rtn_err, p_sock_desc_err);
    } else {
        NET_SOCK_DESC_INIT(&sock_desc_rtn_err);
    }


    sock_nbr_rdy = NetSock_SelDescHandler(sock_nbr_max_actual,
                                         &sock_desc_rtn_rd,
                                         &sock_desc_rtn_wr,
                                         &sock_desc_rtn_err,
                                          p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        if (sock_nbr_rdy == 0) {
            goto exit_release;
        }
    }

    if (p_sock_desc_rd != DEF_NULL) {
        NET_SOCK_DESC_COPY(p_sock_desc_rd, &sock_desc_rtn_rd);
    }

    if (p_sock_desc_wr != DEF_NULL) {
        NET_SOCK_DESC_COPY(p_sock_desc_wr,  &sock_desc_rtn_wr );
    }

    if (p_sock_desc_err != DEF_NULL) {
        NET_SOCK_DESC_COPY(p_sock_desc_err, &sock_desc_rtn_err);
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

exit:
    return (sock_nbr_rdy);                                      /* Rtn nbr rdy sock desc's (see Note #3c1A1).           */
}
#endif


/*
*********************************************************************************************************
*                                          NetSock_SelAbort()
*
* Description : Abort (unblock) all tasks that are pending on a particular socket using the select.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to abort the select.
*
*               p_err       Pointer to variable that will receive the return error code from this function:
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : None.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
void  NetSock_SelAbort (NET_SOCK_ID   sock_id,
                        RTOS_ERR     *p_err)
{
    NET_SOCK           *p_sock;
    NET_SOCK_SEL_OBJ   *p_sel_obj;
    CPU_BOOLEAN         is_used   = DEF_NO;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_SelAbort);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit;
    }
#endif

    p_sock = NetSock_GetObj(sock_id);
    if (p_sock == DEF_NULL) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit;
    }

    p_sel_obj = p_sock->SelObjTailPtr;
    if (p_sel_obj == DEF_NULL) {
        goto exit;
    }

    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_SEL_ABORT);

    PP_UNUSED_PARAM(is_used);

exit:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();
}
#endif


/*
*********************************************************************************************************
*                                          NetSock_IsConn()
*
* Description : Validate socket in use & connected.
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to validate.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_YES, socket valid and connected.
*               DEF_NO,  socket invalid or NOT connected.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_IsConn (NET_SOCK_ID   sock_id,
                             RTOS_ERR     *p_err)
{
   NET_SOCK     *p_sock = DEF_NULL;
   CPU_BOOLEAN   conn   = DEF_NO;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
   CPU_BOOLEAN   is_used;
#endif


   RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NO);

   RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_IsConn);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_NO);
    }
#endif

                                                                /* --------------- GET SOCK CONN STATUS --------------- */
   p_sock = &NetSock_Tbl[sock_id];
   switch (p_sock->State) {
       case NET_SOCK_STATE_LISTEN:
       case NET_SOCK_STATE_CONN:
       case NET_SOCK_STATE_CONN_IN_PROGRESS:
       case NET_SOCK_STATE_CONN_DONE:
       case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
       case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
            conn = DEF_YES;
            break;

       case NET_SOCK_STATE_NONE:
       case NET_SOCK_STATE_FREE:
       case NET_SOCK_STATE_CLOSED:
       case NET_SOCK_STATE_CLOSED_FAULT:
       case NET_SOCK_STATE_BOUND:
       default:
            conn = DEF_NO;
            break;
   }


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (conn);
}


/*
*********************************************************************************************************
*                                    NetSock_GetConnTransportID()
*
* Description : Get a socket's transport layer handle identifier.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get transport layer handle
*                           identifier.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : Socket's transport layer handle identifier, if NO error(s).
*               NET_CONN_ID_NONE, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

NET_CONN_ID  NetSock_GetConnTransportID (NET_SOCK_ID   sock_id,
                                         RTOS_ERR     *p_err)
{
   NET_SOCK     *p_sock;
   NET_CONN_ID   conn_id;
   NET_CONN_ID   conn_id_transport = NET_CONN_ID_NONE;
   CPU_BOOLEAN   is_used;


   RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_CONN_ID_NONE);

   RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_GetConnTransportID);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* ------------ GET SOCK TRANSPORT CONN ID ------------ */
   p_sock = &NetSock_Tbl[sock_id];

   switch (p_sock->SockType) {
       case NET_SOCK_TYPE_DATAGRAM:
       case NET_SOCK_TYPE_STREAM:
            conn_id = p_sock->ID_Conn;

            is_used = NetConn_IsUsed(conn_id);
            if (is_used != DEF_YES) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                goto exit_release;
            }

            conn_id_transport = NetConn_ID_TransportGet(conn_id);
            break;

       case NET_SOCK_TYPE_NONE:
       default:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            goto exit_release;
   }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (conn_id_transport);
}


/*
*********************************************************************************************************
*                                         NetSock_CfgBlock()
*
* Description : Configure socket's blocking mode.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure blocking mode.
*
*               block       Desired value for socket blocking mode :
*
*                               NET_SOCK_BLOCK_SEL_DFLT
*                               NET_SOCK_BLOCK_SEL_BLOCK
*                               NET_SOCK_BLOCK_SEL_NO_BLOCK
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket blocking mode successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgBlock (NET_SOCK_ID   sock_id,
                               CPU_INT08U    block,
                               RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock  = DEF_NULL;
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgBlock);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_FAIL);
    }
#endif

                                                                /* --------------- CFG SOCK BLOCK MODE ---------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (block) {
        case NET_SOCK_BLOCK_SEL_DFLT:
        case NET_SOCK_BLOCK_SEL_BLOCK:
             DEF_BIT_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
             break;

        case NET_SOCK_BLOCK_SEL_NO_BLOCK:
             DEF_BIT_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
             break;

        case NET_SOCK_BLOCK_SEL_NONE:
        default:
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);
    }

    rtn_val = DEF_OK;

                                                               /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}


/*
*********************************************************************************************************
*                                      NetSock_BlockGet()
*
* Description : Get the blocking mode configuration of the specified socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get option from.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : NET_SOCK_BLOCK_SEL_NO_BLOCK : Socket is in none blocking mode.
*               NET_SOCK_BLOCK_SEL_BLOCK    : Socket is in blocking mode.
*               NET_SOCK_BLOCK_SEL_NONE     : Error in operation.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_INT08U  NetSock_BlockGet (NET_SOCK_ID   sock_id,
                              RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   is_no_block;
    CPU_INT08U    block;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif

                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BLOCK_SEL_NONE);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgBlock);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
         Net_GlobalLockRelease();
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
         return (NET_SOCK_BLOCK_SEL_NONE);
    }
#endif

                                                                /* --------------- CFG SOCK BLOCK MODE ---------------- */
    p_sock = &NetSock_Tbl[sock_id];

    is_no_block = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    if (is_no_block == DEF_YES) {
        block = NET_SOCK_BLOCK_SEL_NO_BLOCK;
    } else {
        block = NET_SOCK_BLOCK_SEL_BLOCK;
    }

                                                                  /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (block);
}


/*
*********************************************************************************************************
*                                         NetSock_CfgSecure()
*
* Description : Configure socket's secure mode.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure secure mode.
*
*               secure      Desired value for socket secure mode :
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : DEF_OK,   socket secure mode successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) A socket's secure session ('p_sock->SecureSession') will be initialized if a secure
*                   port session buffer/object is available.
*********************************************************************************************************
*/
#ifdef  NET_SECURE_MODULE_EN
CPU_BOOLEAN  NetSock_CfgSecure (NET_SOCK_ID   sock_id,
                                CPU_BOOLEAN   secure,
                                RTOS_ERR     *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
   NET_SOCK      *p_sock;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif

                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgSecure);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ---------------- VALIDATE SOCK TYPE ---------------- */
    switch (p_sock->SockType) {
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        case NET_SOCK_TYPE_DATAGRAM:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit_release;
    }


                                                                 /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_BOUND:
             break;

        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        case NET_SOCK_STATE_CLOSED_FAULT:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;
    }


                                                                 /* --------------- CFG SOCK SECURE MODE --------------- */
    switch (secure) {
        case DEF_ENABLED:
             NetSecure_InitSession(p_sock, p_err);               /* Init new secure session (see Note #3).               */
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  goto exit_release;
             }
             DEF_BIT_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
             break;

        case DEF_DISABLED:
             DEF_BIT_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
             break;

        default:
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);
    }

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_CfgSecureServerCertKeyInstall()
*
* Description : Install certificate and key that must be used by a server socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of server socket to configure secure certificate and key.
*
*               p_cert      Pointer to buffer that contains the certificate.
*
*               cert_len    Certificate length.
*
*               p_key       Pointer to buffer that contains the key.
*
*               key_len     Key length.
*
*               fmt         Certificate and key format:
*
*                               NET_SOCK_SECURE_CERT_KEY_FMT_PEM
*                               NET_SOCK_SECURE_CERT_KEY_FMT_DER
*
*               cert_chain  Certificate point to a chain of certificate.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   certificate and key successfully installed.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) MUST BE CALLED ONLY AFTER NetSock_CfgSecure() has been called.
*********************************************************************************************************
*/
#ifdef  NET_SECURE_MODULE_EN
CPU_BOOLEAN  NetSock_CfgSecureServerCertKeyInstall (       NET_SOCK_ID                    sock_id,
                                                    const  void                          *p_cert,
                                                           CPU_INT32U                     cert_len,
                                                    const  void                          *p_key,
                                                           CPU_INT32U                     key_len,
                                                           NET_SOCK_SECURE_CERT_KEY_FMT   fmt,
                                                           CPU_BOOLEAN                    cert_chain,
                                                           RTOS_ERR                      *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   secure;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif

                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((p_cert != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((p_key  != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET(((fmt == NET_SOCK_SECURE_CERT_KEY_FMT_PEM) ||
                                 (fmt == NET_SOCK_SECURE_CERT_KEY_FMT_DER)), *p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgSecureServerCertKeyInstall);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ---------------- VALIDATE SOCK TYPE ---------------- */
    secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
    if (secure != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }

                                                                /* ----------- CFG SERVER'S SOCK CERT & KEY ----------- */
    rtn_val = NetSecure_SockCertKeyCfg(p_sock,
                                       NET_SOCK_SECURE_TYPE_SERVER,
                                       p_cert,
                                       cert_len,
                                       p_key,
                                       key_len,
                                       fmt,
                                       cert_chain,
                                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_CfgSecureClientCertKeyInstall()
*
* Description : Install certificate and key that must be used by a client for mutual authentication.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of server socket to configure secure certificate and key.
*
*               p_cert      Pointer to buffer that contains the certificate.
*
*               cert_len    Certificate length.
*
*               p_key       Pointer to buffer that contains the key.
*
*               key_len     Key length.
*
*               fmt         Certificate and key format:
*
*                               NET_SOCK_SECURE_CERT_KEY_FMT_PEM
*                               NET_SOCK_SECURE_CERT_KEY_FMT_DER
*
*               cert_chain  Certificate point to a chain of certificate.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   certificate and key successfully installed.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) MUST BE CALLED ONLY AFTER NetSock_CfgSecure() has been called.
*********************************************************************************************************
*/
#ifdef  NET_SECURE_MODULE_EN
CPU_BOOLEAN  NetSock_CfgSecureClientCertKeyInstall (       NET_SOCK_ID                    sock_id,
                                                    const  void                          *p_cert,
                                                           CPU_INT32U                     cert_len,
                                                    const  void                          *p_key,
                                                           CPU_INT32U                     key_len,
                                                           NET_SOCK_SECURE_CERT_KEY_FMT   fmt,
                                                           CPU_BOOLEAN                    cert_chain,
                                                           RTOS_ERR                      *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   secure;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif

                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((p_cert != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((p_key  != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET(((fmt == NET_SOCK_SECURE_CERT_KEY_FMT_PEM) ||
                                 (fmt == NET_SOCK_SECURE_CERT_KEY_FMT_DER)), *p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgSecureServerCertKeyInstall);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ---------------- VALIDATE SOCK TYPE ---------------- */
    secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
    if (secure != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }

                                                                /* ----------- CFG SERVER'S SOCK CERT & KEY ----------- */
    rtn_val = NetSecure_SockCertKeyCfg(p_sock,
                                       NET_SOCK_SECURE_TYPE_CLIENT,
                                       p_cert,
                                       cert_len,
                                       p_key,
                                       key_len,
                                       fmt,
                                       cert_chain,
                                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_CfgSecureClientCommonName()
*
* Description : Configure client socket's common name.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of client socket to configure common name.
*
*               p_common_name   Pointer to string that contain the common name.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   common name successfully installed.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) MUST BE CALLED ONLY AFTER NetSock_CfgSecure() has been called.
*********************************************************************************************************
*/
#if  ((defined(NET_SECURE_MODULE_EN)) && \
      (NET_SECURE_CFG_MAX_NBR_SOCK_CLIENT > 0u))
CPU_BOOLEAN  NetSock_CfgSecureClientCommonName (NET_SOCK_ID   sock_id,
                                                CPU_CHAR     *p_common_name,
                                                RTOS_ERR     *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   secure;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((p_common_name != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgSecureClientCommonName);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ---------------- VALIDATE SOCK TYPE ---------------- */

    secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
    if (secure != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }

                                                                /* CFG SOCK's COMMON NAME                               */
    rtn_val = NetSecure_ClientCommonNameSet(p_sock,
                                            p_common_name,
                                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_CfgSecureClientTrustCallBack()
*
* Description : Configure client socket's trust call back function.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of client socket to configure trust call back function.
*
*               call_back_fnct  Pointer to the trust call back function
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   trust call back function successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) MUST BE CALLED ONLY AFTER NetSock_CfgSecure() has been called.
*********************************************************************************************************
*/
#if  ((defined(NET_SECURE_MODULE_EN)) && \
      (NET_SECURE_CFG_MAX_NBR_SOCK_CLIENT > 0u))
CPU_BOOLEAN  NetSock_CfgSecureClientTrustCallBack (NET_SOCK_ID                  sock_id,
                                                   NET_SOCK_SECURE_TRUST_FNCT   call_back_fnct,
                                                   RTOS_ERR                    *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   secure;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((call_back_fnct != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgSecureClientTrustCallBack);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ---------------- VALIDATE SOCK TYPE ---------------- */

    secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
    if (secure != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }

                                                                /* CFG SOCK's TRUST CALL BACK FNCT                      */

    rtn_val = NetSecure_ClientTrustCallBackSet(p_sock,
                                               call_back_fnct,
                                               p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                         NetSock_CfgIF()
*
* Description : Configure the interface that must be used by the socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of the socket to configure the interface
*                           number.
*
*               if_nbr      Interface number to bind to the socket.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   If the interface number was successfully set,
*               DEF_FAIL, Otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgIF (NET_SOCK_ID   sock_id,
                            NET_IF_NBR    if_nbr,
                            RTOS_ERR     *p_err)
{
    CPU_BOOLEAN  rtn_val = DEF_FAIL;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgIF);

   (void)NetSock_CfgIF_Handler(sock_id,                         /* ------------------- CFG SOCK IF -------------------- */
                               if_nbr,
                               p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit_release;
    }

    rtn_val = DEF_OK;

exit_release:
                                                                  /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}


/*
*********************************************************************************************************
*                                        NetSock_CfgRxQ_Size()
*
* Description : Configure socket's receive queue size.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure receive queue size.
*
*               size        Desired receive queue size.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : DEF_OK,   socket receive queue size successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) For datagram sockets, configured size does NOT :
*
*                   (a) Limit or remove any received data currently queued but becomes effective
*                       for later received data.
*                   (b) Partially truncate any received data but instead allows data from exactly one
*                       received packet buffer to overflow the configured size since each datagram
*                       MUST be received atomically.
*
*               (3) For stream sockets, size MAY be required to be configured prior to connecting.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgRxQ_Size (NET_SOCK_ID          sock_id,
                                  NET_SOCK_DATA_SIZE   size,
                                  RTOS_ERR            *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   rtn_val   = DEF_FAIL;
    CPU_BOOLEAN   is_used;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((size >= NET_SOCK_DATA_SIZE_MIN), *p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_CfgRxQ_Size);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
   is_used = NetSock_IsUsed(sock_id);
   if (is_used != DEF_YES) {
       RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
       goto exit_release;
   }
#endif


                                                                /* ---------------- CFG SOCK RX Q SIZE ---------------- */
   p_sock = &NetSock_Tbl[sock_id];

   switch (p_sock->SockType) {
       case NET_SOCK_TYPE_DATAGRAM:
            p_sock->RxQ_SizeCfgd = size;
            break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
       case NET_SOCK_TYPE_STREAM:
            switch (p_sock->Protocol) {
                case NET_SOCK_PROTOCOL_TCP:
                     conn_id = p_sock->ID_Conn;

                     is_used = NetConn_IsUsed(conn_id);
                     if (is_used == DEF_YES) {

                         conn_id_transport = NetConn_ID_TransportGet(conn_id);

                         is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                         if (is_used == DEF_YES) {

                             NetTCP_ConnCfgRxWinSizeHandler((NET_TCP_CONN_ID )conn_id_transport,
                                                                              size,
                                                                              p_err);
                             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                                 goto exit_release;
                             }
                         } else {
                             p_sock->RxQ_SizeCfgd = size;
                         }

                     } else {
                         p_sock->RxQ_SizeCfgd = size;
                     }
                     break;

                case NET_SOCK_PROTOCOL_NONE:
                default:
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                     goto exit_release;
            }
            break;
#endif


       case NET_SOCK_TYPE_NONE:
       default:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
            goto exit_release;
   }

   PP_UNUSED_PARAM(is_used);                                    /* Prevent possible 'variable unused' warnings.         */

   rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}


/*
*********************************************************************************************************
*                                        NetSock_CfgTxQ_Size()
*
* Description : Configure socket's transmit queue size.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure transmit queue size.
*
*               size        Desired transmit queue size.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : DEF_OK,   socket transmit queue size successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) For datagram sockets, configured size does NOT :
*
*                   (a) Partially truncate any received data but instead allows data from exactly one
*                       received packet buffer to overflow the configured size since each datagram
*                       MUST be received atomically.
*
*               (3) For stream sockets, size MAY be required to be configured prior to connecting.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTxQ_Size (NET_SOCK_ID          sock_id,
                                  NET_SOCK_DATA_SIZE   size,
                                  RTOS_ERR            *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   is_used;
    CPU_BOOLEAN   rtn_val    = DEF_FAIL;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((size >= NET_SOCK_DATA_SIZE_MIN), *p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTxQ_Size);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* ---------------- CFG SOCK TX Q SIZE ---------------- */
   p_sock = &NetSock_Tbl[sock_id];

   switch (p_sock->SockType) {
       case NET_SOCK_TYPE_DATAGRAM:
            p_sock->TxQ_SizeCfgd = size;
            break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
       case NET_SOCK_TYPE_STREAM:
            switch (p_sock->Protocol) {
                case NET_SOCK_PROTOCOL_TCP:
                     conn_id = p_sock->ID_Conn;

                     is_used = NetConn_IsUsed(conn_id);
                     if (is_used == DEF_YES) {

                         conn_id_transport = NetConn_ID_TransportGet(conn_id);

                         is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                         if (is_used == DEF_YES) {

                             NetTCP_ConnCfgTxWinSizeHandler((NET_TCP_CONN_ID )conn_id_transport,
                                                            (NET_TCP_WIN_SIZE)size,
                                                                              p_err);
                             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                                  goto exit_release;
                             }
                         } else {
                             p_sock->TxQ_SizeCfgd = size;
                         }
                     } else {
                         p_sock->TxQ_SizeCfgd = size;
                     }
                     break;

                case NET_SOCK_PROTOCOL_NONE:
                default:
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                     goto exit_release;
            }
            break;
#endif


       case NET_SOCK_TYPE_NONE:
       default:                                                 /* See Note #4a.                                        */
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            goto exit_release;
   }

   PP_UNUSED_PARAM(is_used);                                    /* Prevent possible 'variable unused' warnings.         */

   rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}


/*
*********************************************************************************************************
*                                   NetSock_CfgConnChildQ_SizeSet()
*
* Description : Configure socket's child connection queue size.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure receive queue size.
*
*               queue_size  Desired child connection queue size :
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket child connection queue size successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) NetSock_CfgConnChildQ_SizeSet() allows a listen socket to reject new incomming connection
*                   when the number of connection already accepted ("currently being processed) plus the
*                   listen queue reachs the maximum number of child connection.
*
*                   (a) It should be used when ressources such as number of received buffers are limited.
*
*                   (b) It doesn't remove any connection currently accepted. It becomes effective
*                       for later connection request.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgConnChildQ_SizeSet (NET_SOCK_ID       sock_id,
                                            NET_SOCK_Q_SIZE   queue_size,
                                            RTOS_ERR         *p_err)
{
    NET_SOCK     *p_sock = DEF_NULL;
    CPU_BOOLEAN   rtn    = DEF_FAIL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);
    RTOS_ASSERT_DBG_ERR_SET((queue_size > NET_SOCK_Q_SIZE_NONE), *p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgConnChildQ_SizeSet);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif
                                                                /* ---------- CFG SOCK CONN CHILD Q SIZE MAX ---------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      p_sock->ConnChildQ_SizeMax = queue_size;
                      break;

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit_release;
             }
             break;
#endif


        case NET_SOCK_TYPE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn);
}


/*
*********************************************************************************************************
*                                   NetSock_CfgConnChildQ_SizeGet()
*
* Description : Get socket's connection child queue size value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure receive queue size.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : Socket's connection child queue size value :
*                   NET_SOCK_Q_SIZE_NONE, on any error(s).
*                   NET_SOCK_Q_SIZE_UNLIMITED, if unlimited (i.e. NO limit) value configured.
*                   child connection queue size, otherwise.
*
* Note(s)     : (1) Despite inconsistency with other 'Get' status functions,
*                   NetSock_CfgConnChildQ_SizeGet() includes 'Cfg' for consistency with other
*                   NetSock_CfgConn&&&() functions.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
NET_SOCK_Q_SIZE  NetSock_CfgConnChildQ_SizeGet (NET_SOCK_ID   sock_id,
                                                RTOS_ERR     *p_err)
{
    NET_SOCK         *p_sock     = DEF_NULL;
    NET_SOCK_Q_SIZE   queue_size = NET_SOCK_Q_SIZE_NONE;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_Q_SIZE_NONE);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ---------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgConnChildQ_SizeGet);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif
                                                                /* ----------- GET SOCK CONN CHILD Q SIZE ------------ */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      queue_size = p_sock->ConnChildQ_SizeMax;
                      break;

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit_release;
             }
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (queue_size);
}


/*
*********************************************************************************************************
*                                        NetSock_CfgTxIP_TOS()
*
* Description : Configure socket's transmit IP TOS.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure transmit IP TOS.
*
*               ip_tos      Desired transmit IP TOS.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : DEF_OK,   socket transmit IP TOS successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
#ifdef  NET_IPv4_MODULE_EN
CPU_BOOLEAN  NetSock_CfgTxIP_TOS (NET_SOCK_ID    sock_id,
                                  NET_IPv4_TOS   ip_tos,
                                  RTOS_ERR      *p_err)
{
    NET_SOCK     *p_sock;
    NET_CONN_ID   conn_id;
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif

                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTxIP_TOS);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
   p_sock = &NetSock_Tbl[sock_id];

   switch (p_sock->State) {
       case NET_SOCK_STATE_FREE:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            goto exit_release;

       case NET_SOCK_STATE_BOUND:
       case NET_SOCK_STATE_LISTEN:
       case NET_SOCK_STATE_CONN:
       case NET_SOCK_STATE_CONN_IN_PROGRESS:
       case NET_SOCK_STATE_CONN_DONE:
            break;

       case NET_SOCK_STATE_CLOSED:
       case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
       case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
       case NET_SOCK_STATE_CLOSED_FAULT:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
            goto exit_release;

       case NET_SOCK_STATE_NONE:
       default:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
            goto exit_release;
   }

                                                                /* ---------------- CFG SOCK TX IP TOS ---------------- */
   conn_id = p_sock->ID_Conn;
   NetConn_IPv4TxTOS_Set(conn_id, ip_tos);

   rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                        NetSock_CfgTxIP_TTL()
*
* Description : Configure socket's transmit IP TTL.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure transmit IP TTL.
*
*               ip_ttl      Desired transmit IP TTL :
*
*                               NET_IPv4_TTL_MIN                  Minimum TTL transmit value   (1)
*                               NET_IPv4_TTL_MAX                  Maximum TTL transmit value (255)
*                               NET_IPv4_TTL_DFLT                 Default TTL transmit value (128)
*                               NET_IPv4_TTL_NONE                 Replace with default TTL
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : DEF_OK,   socket transmit IP TTL successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
#ifdef  NET_IPv4_MODULE_EN
CPU_BOOLEAN  NetSock_CfgTxIP_TTL (NET_SOCK_ID    sock_id,
                                  NET_IPv4_TTL   ip_ttl,
                                  RTOS_ERR      *p_err)
{
    NET_SOCK     *p_sock;
    NET_CONN_ID   conn_id;
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif

                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTxIP_TTL);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             break;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        case NET_SOCK_STATE_CLOSED_FAULT:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;
    }

                                                                /* ---------------- CFG SOCK TX IP TTL ---------------- */
    conn_id = p_sock->ID_Conn;
    NetConn_IPv4TxTTL_Set(conn_id, ip_ttl);

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                   NetSock_CfgTxIP_TTL_Multicast()
*
* Description : Configure socket's transmit IP multicast TTL.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to configure transmit IP multicast TTL.
*
*               ip_ttl      Desired transmit IP multicast TTL:
*
*                               NET_IPv4_TTL_MIN                  Minimum TTL transmit value   (1)
*                               NET_IPv4_TTL_MAX                  Maximum TTL transmit value (255)
*                               NET_IPv4_TTL_DFLT                 Default TTL transmit value   (1)
*                               NET_IPv4_TTL_NONE                 Replace with default TTL
*
*               p_err       Pointer to the variable that will receive one of the following error code(s) from this function:
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : DEF_OK,   socket transmit IP multicast TTL successfully configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
#ifdef  NET_IPv4_MODULE_EN
CPU_BOOLEAN  NetSock_CfgTxIP_TTL_Multicast (NET_SOCK_ID   sock_id,
                                            NET_IPv4_TTL  ip_ttl,
                                            RTOS_ERR     *p_err)
{
#ifdef  NET_MCAST_TX_MODULE_EN
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
    NET_SOCK     *p_sock;
    NET_CONN_ID   conn_id;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
                                                                /* See Note #2b.                                        */
    Net_GlobalLockAcquire((void *)NetSock_CfgTxIP_TTL_Multicast);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;

        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             break;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        case NET_SOCK_STATE_CLOSED_FAULT:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit_release;
    }

                                                                /* ----------- CFG SOCK TX IP MULTICAST TTL ----------- */
    conn_id = p_sock->ID_Conn;
    NetConn_IPv4TxTTL_MulticastSet(conn_id, ip_ttl);

    /* TODO_NET IPv6 */

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();
    return (rtn_val);

#else
    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(ip_ttl);

    RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, DEF_FAIL);
#endif
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_CfgTimeoutRxQ_Dflt()
*
* Description : Set socket's receive queue configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set receive queue configured-
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : DEF_OK,   socket receive queue configured-default timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket receive queue timeout in
*                   progress but becomes effective the next time a socket pends on its receive queue
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutRxQ_Dflt (NET_SOCK_ID   sock_id,
                                         RTOS_ERR     *p_err)
{
   NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
   NET_CONN_ID   conn_id;
   NET_CONN_ID   conn_id_transport;
#endif
   CPU_BOOLEAN   is_used;
   CPU_BOOLEAN   rtn_val  = DEF_FAIL;


                                                               /* ---------------- VALIDATE ARGUMENTS ---------------- */
   RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

   RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutRxQ_Dflt);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
   is_used = NetSock_IsUsed(sock_id);
   if (is_used != DEF_YES) {
       RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
       goto exit_release;
   }
#endif

                                                                /* ------------ CFG SOCK RX Q DFLT TIMEOUT ------------ */
   p_sock = &NetSock_Tbl[sock_id];

   switch (p_sock->SockType) {
       case NET_SOCK_TYPE_DATAGRAM:
            NetSock_RxQ_TimeoutDflt(p_sock);
            break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
       case NET_SOCK_TYPE_STREAM:
            switch (p_sock->Protocol) {
                case NET_SOCK_PROTOCOL_TCP:
                     conn_id = p_sock->ID_Conn;

                     is_used = NetConn_IsUsed(conn_id);
                     if (is_used != DEF_YES) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                         goto exit_release;
                     }

                     conn_id_transport = NetConn_ID_TransportGet(conn_id);

                     is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                     if (is_used != DEF_YES) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                         goto exit_release;
                     }

                     NetTCP_RxQ_TimeoutDflt((NET_TCP_CONN_ID)conn_id_transport);
                     break;

                case NET_SOCK_PROTOCOL_NONE:
                default:
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                     goto exit_release;
            }
            break;
#endif


       case NET_SOCK_TYPE_NONE:
       default:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            goto exit_release;
   }


   PP_UNUSED_PARAM(is_used);                                    /* Prevent possible 'variable unused' warnings.         */

   rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}


/*
*********************************************************************************************************
*                                     NetSock_CfgTimeoutRxQ_Set()
*
* Description : Set socket's receive queue timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set receive queue timeout.
*
*               timeout_ms  Desired timeout value :
*
*                               NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value desired.
*                               In number of milliseconds, otherwise.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : DEF_OK,   socket receive queue timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket receive queue timeout in
*                   progress but becomes effective the next time a socket pends on its receive queue
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutRxQ_Set (NET_SOCK_ID   sock_id,
                                        CPU_INT32U    timeout_ms,
                                        RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   is_used;
    CPU_BOOLEAN   rtn_val  = DEF_FAIL;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
                                                                /* See Note #2b.                                        */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutRxQ_Set);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* -------------- CFG SOCK RX Q TIMEOUT --------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             NetSock_RxQ_TimeoutSet(p_sock, timeout_ms);
             break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_id = p_sock->ID_Conn;

                      is_used = NetConn_IsUsed(conn_id);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      conn_id_transport = NetConn_ID_TransportGet(conn_id);

                      is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      NetTCP_RxQ_TimeoutSet((NET_TCP_CONN_ID)conn_id_transport,
                                                             timeout_ms);
                      break;

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit_release;
             }
             break;
#endif


        case NET_SOCK_TYPE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }

    PP_UNUSED_PARAM(is_used);                                   /* Prevent possible 'variable unused' warnings.         */

    rtn_val = DEF_OK;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
}


/*
*********************************************************************************************************
*                                   NetSock_CfgTimeoutRxQ_Get_ms()
*
* Description : Get socket's receive queue timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get receive queue timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : Socket's receive queue network timeout value :
*                   0, on any error(s).
*                   NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value configured.
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) Despite inconsistency with other 'Get' status functions,
*                   NetSock_CfgTimeoutRxQ_Get_ms() includes 'Cfg' for consistency with other
*                   NetSock_CfgTimeout&&&() functions.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_INT32U  NetSock_CfgTimeoutRxQ_Get_ms (NET_SOCK_ID   sock_id,
                                          RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   is_used;
    CPU_INT32U    timeout_ms = 0u;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutRxQ_Get_ms);/* See Note #3b.                                        */


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* -------------- GET SOCK RX Q TIMEOUT --------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             timeout_ms = NetSock_RxQ_TimeoutGet_ms(p_sock);
             break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_id           = p_sock->ID_Conn;
                      conn_id_transport = NetConn_ID_TransportGet(conn_id);

                      is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      timeout_ms = NetTCP_RxQ_TimeoutGet_ms((NET_TCP_CONN_ID)conn_id_transport);
                      break;

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      timeout_ms = 0u;
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit_release;
             }
             break;
#endif


        case NET_SOCK_TYPE_NONE:
        default:
             timeout_ms = 0u;
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }

    PP_UNUSED_PARAM(is_used);                                   /* Prevent possible 'variable unused' warnings.         */

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (timeout_ms);                                        /* -------------- RTN SOCK RX Q TIMEOUT --------------- */
}


/*
*********************************************************************************************************
*                                    NetSock_CfgTimeoutTxQ_Dflt()
*
* Description : (1) Set socket's transmit queue configured-default timeout value.
*
*               (2) Socket transmit queues apply to the following socket type(s)/protocol(s) :
*
*                   (a) Stream
*                       (1) TCP
*
*                   (b) Datagram sockets currently NOT blocked during transmit
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set transmit queue configured-
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : DEF_OK,   socket transmit queue configured-default timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (3) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (4) Configured timeout does NOT reschedule any current socket transmit queue timeout in
*                   progress but becomes effective the next time a socket pends on its transmit queue
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutTxQ_Dflt (NET_SOCK_ID   sock_id,
                                         RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   is_used;
    CPU_BOOLEAN   rtn_val    = DEF_FAIL;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutTxQ_Dflt);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
   is_used = NetSock_IsUsed(sock_id);
   if (is_used != DEF_YES) {
       RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
       goto exit_release;
   }
#endif

                                                                /* ------------ CFG SOCK TX Q DFLT TIMEOUT ------------ */
   p_sock = &NetSock_Tbl[sock_id];

   switch (p_sock->SockType) {
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
       case NET_SOCK_TYPE_STREAM:
            switch (p_sock->Protocol) {
                case NET_SOCK_PROTOCOL_TCP:
                     conn_id = p_sock->ID_Conn;

                     is_used = NetConn_IsUsed(conn_id);
                     if (is_used != DEF_YES) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                         goto exit_release;
                     }

                     conn_id_transport = NetConn_ID_TransportGet(conn_id);

                     is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                     if (is_used != DEF_YES) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                         goto exit_release;
                     }

                     NetTCP_TxQ_TimeoutDflt((NET_TCP_CONN_ID)conn_id_transport);

                     rtn_val = DEF_OK;
                     break;

                case NET_SOCK_PROTOCOL_NONE:
                default:
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                     goto exit_release;
            }
            break;
#endif


       case NET_SOCK_TYPE_DATAGRAM:                             /* See Note #2b.                                        */
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
            goto exit_release;


       case NET_SOCK_TYPE_NONE:
       default:
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            goto exit_release;
   }


exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}


/*
*********************************************************************************************************
*                                     NetSock_CfgTimeoutTxQ_Set()
*
* Description : (1) Set socket's transmit queue timeout value.
*
*               (2) Socket transmit queues apply to the following socket type(s)/protocol(s) :
*
*                   (a) Stream
*                       (1) TCP
*
*                   (b) Datagram sockets currently NOT blocked during transmit
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set transmit queue timeout.
*
*               timeout_ms  Desired timeout value :
*
*                               NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value desired.
*                               In number of milliseconds, otherwise.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : DEF_OK,   socket transmit queue timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (3) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (4) Configured timeout does NOT reschedule any current socket transmit queue timeout in
*                   progress but becomes effective the next time a socket pends on its transmit queue
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutTxQ_Set (NET_SOCK_ID   sock_id,
                                        CPU_INT32U    timeout_ms,
                                        RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   is_used;
    CPU_BOOLEAN   rtn_val    = DEF_FAIL;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutTxQ_Set);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

#ifndef  NET_SOCK_TYPE_STREAM_MODULE_EN
    PP_UNUSED_PARAM(timeout_ms);                                /* Prevent possible 'variable unused' warnings.         */
#endif

                                                                /* -------------- CFG SOCK TX Q TIMEOUT --------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_id = p_sock->ID_Conn;

                      is_used = NetConn_IsUsed(conn_id);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      conn_id_transport = NetConn_ID_TransportGet(conn_id);

                      is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      NetTCP_TxQ_TimeoutSet((NET_TCP_CONN_ID)conn_id_transport,
                                                             timeout_ms);

                      rtn_val = DEF_OK;
                      break;

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit_release;
             }
             break;
#endif


        case NET_SOCK_TYPE_DATAGRAM:                            /* See Note #2b.                                        */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit_release;


        case NET_SOCK_TYPE_NONE:
        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_val);
}


/*
*********************************************************************************************************
*                                   NetSock_CfgTimeoutTxQ_Get_ms()
*
* Description : (1) Get socket's transmit queue timeout value.
*
*               (2) Socket transmit queues apply to the following socket type(s)/protocol(s) :
*
*                   (a) Stream
*                       (1) TCP
*
*                   (b) Datagram sockets currently NOT blocked during transmit
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get transmit queue timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : Socket's transmit queue network timeout value :
*                   0, on any error(s).
*                   NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value configured.
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (3) Despite inconsistency with other 'Get' status functions,
*                   NetSock_CfgTimeoutTxQ_Get_ms() includes 'Cfg' for consistency with other
*                   NetSock_CfgTimeout&&&() functions.
*
*               (4) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_INT32U  NetSock_CfgTimeoutTxQ_Get_ms (NET_SOCK_ID   sock_id,
                                          RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
#endif
    CPU_BOOLEAN   is_used;
    CPU_INT32U    timeout_ms = 0u;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutTxQ_Get_ms);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

                                                                /* -------------- GET SOCK TX Q TIMEOUT --------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_id = p_sock->ID_Conn;

                      is_used = NetConn_IsUsed(conn_id);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      conn_id_transport = NetConn_ID_TransportGet(conn_id);

                      is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit_release;
                      }

                      timeout_ms = NetTCP_TxQ_TimeoutGet_ms((NET_TCP_CONN_ID)conn_id_transport);
                      break;

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit_release;
             }
             break;
#endif


        case NET_SOCK_TYPE_DATAGRAM:                             /* See Note #2b.                                        */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit_release;


        case NET_SOCK_TYPE_NONE:
        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit_release;
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (timeout_ms);                                        /* -------------- RTN SOCK TX Q TIMEOUT --------------- */
}


/*
*********************************************************************************************************
*                                   NetSock_CfgTimeoutConnReqDflt()
*
* Description : Set socket's connection request configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection request
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket connection request configured-default timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket connection request timeout
*                   in progress but becomes effective the next time a socket pends on a connection request
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutConnReqDflt (NET_SOCK_ID   sock_id,
                                            RTOS_ERR     *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_SOCK     *p_sock  = DEF_NULL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnReqDflt);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_FAIL);
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                 /* ---------- CFG SOCK CONN REQ DFLT TIMEOUT ---------- */
    NetSock_ConnReqTimeoutDflt(p_sock);

    rtn_val = DEF_OK;


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

#else
    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' compiler warning.          */
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
#endif

    return (rtn_val);
}


/*
*********************************************************************************************************
*                                   NetSock_CfgTimeoutConnReqSet()
*
* Description : Set socket's connection request timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection request timeout.
*
*               timeout_ms  Desired timeout value :
*
*                               NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value desired.
*                               In number of milliseconds, otherwise.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket connection request timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket connection request timeout
*                   in progress but becomes effective the next time a socket pends on a connection request
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutConnReqSet (NET_SOCK_ID   sock_id,
                                           CPU_INT32U    timeout_ms,
                                           RTOS_ERR     *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_SOCK     *p_sock  = DEF_NULL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
                                                                /* See Note #2b.                                        */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnReqSet);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_FAIL);
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                 /* ------------ CFG SOCK CONN REQ TIMEOUT ------------- */
    NetSock_ConnReqTimeoutSet(p_sock, timeout_ms);

    rtn_val = DEF_OK;


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

#else
   PP_UNUSED_PARAM(sock_id);                                    /* Prevent 'variable unused' compiler warnings.         */
   PP_UNUSED_PARAM(timeout_ms);
   RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
#endif

   return (rtn_val);
}


/*
*********************************************************************************************************
*                                  NetSock_CfgTimeoutConnReqGet_ms()
*
* Description : Get socket's connection request timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get connection request timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : Socket's connection request network timeout value :
*                   0, on any error(s).
*                   NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value configured.
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) Despite inconsistency with other 'Get' status functions,
*                   NetSock_CfgTimeoutConnReqGet_ms() includes 'Cfg' for consistency with other
*                   NetSock_CfgTimeout&&&() functions.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_INT32U  NetSock_CfgTimeoutConnReqGet_ms (NET_SOCK_ID   sock_id,
                                             RTOS_ERR     *p_err)
{
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_SOCK    *p_sock;
    CPU_INT32U   timeout_ms = 0u;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnReqGet_ms);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (0u);
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                 /* ------------ GET SOCK CONN REQ TIMEOUT ------------- */
    timeout_ms = NetSock_ConnReqTimeoutGet_ms(p_sock);


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (timeout_ms);                                        /* ------------ RTN SOCK CONN REQ TIMEOUT ------------- */

#else
    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' compiler warning.          */
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    return (0u);
#endif
}


/*
*********************************************************************************************************
*                                 NetSock_CfgTimeoutConnAcceptDflt()
*
* Description : Set socket's connection accept configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection accept
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket connection accept configured-default timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket connection accept timeout
*                   in progress but becomes effective the next time a socket pends on a connection accept
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutConnAcceptDflt (NET_SOCK_ID   sock_id,
                                               RTOS_ERR     *p_err)
{
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnAcceptDflt);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    {
        CPU_BOOLEAN  is_used;


        is_used = NetSock_IsUsed(sock_id);
        if (is_used != DEF_YES) {
            Net_GlobalLockRelease();
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            return (DEF_FAIL);
        }
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                /* -------- CFG SOCK CONN ACCEPT DFLT TIMEOUT --------- */
    NetSock_ConnAcceptQ_TimeoutDflt(p_sock);

    rtn_val = DEF_OK;


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);
#else
    RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, DEF_FAIL);
#endif

}


/*
*********************************************************************************************************
*                                  NetSock_CfgTimeoutConnAcceptSet()
*
* Description : Set socket's connection accept timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection accept timeout.
*
*               timeout_ms  Desired timeout value :
*
*                               NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value desired.
*                               In number of milliseconds, otherwise.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket connection accept timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket connection accept timeout
*                   in progress but becomes effective the next time a socket pends on a connection accept
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutConnAcceptSet (NET_SOCK_ID   sock_id,
                                              CPU_INT32U    timeout_ms,
                                              RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock  = DEF_NULL;
#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#endif
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used = DEF_NO;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    PP_UNUSED_PARAM(p_sock);                                    /* Prevent 'variable unused' compiler warning.          */

#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnAcceptSet);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_FAIL);
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ----------- CFG SOCK CONN ACCEPT TIMEOUT ----------- */
    NetSock_ConnAcceptQ_TimeoutSet(p_sock, timeout_ms);

    rtn_val = DEF_OK;


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);

#else
    PP_UNUSED_PARAM(is_used);

    RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                 NetSock_CfgTimeoutConnAcceptGet_ms()
*
* Description : Get socket's connection accept timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get connection accept timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : Socket's connection accept network timeout value :
*                   0, on any error(s).
*                   NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value configured.
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) Despite inconsistency with other 'Get' status functions,
*                   NetSock_CfgTimeoutConnAcceptGet_ms() includes 'Cfg' for consistency with other
*                   NetSock_CfgTimeout&&&() functions.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_INT32U  NetSock_CfgTimeoutConnAcceptGet_ms (NET_SOCK_ID   sock_id,
                                                RTOS_ERR     *p_err)
{
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    CPU_INT32U   timeout_ms = 0u;
    NET_SOCK    *p_sock;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnAcceptGet_ms);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    {
        CPU_BOOLEAN  is_used;


        is_used = NetSock_IsUsed(sock_id);
        if (is_used != DEF_YES) {
            Net_GlobalLockRelease();
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            return (0u);
        }
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                /* ----------- GET SOCK CONN ACCEPT TIMEOUT ----------- */
    timeout_ms = NetSock_ConnAcceptQ_TimeoutGet_ms(p_sock);


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

                                                                /* ----------- RTN SOCK CONN ACCEPT TIMEOUT ----------- */
    return (timeout_ms);

#else
    RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, 0u);
#endif
}


/*
*********************************************************************************************************
*                                  NetSock_CfgTimeoutConnCloseDflt()
*
* Description : Set socket's connection close configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection close
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket connection close configured-default timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket connection close timeout
*                   in progress but becomes effective the next time a socket pends on a connection close
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutConnCloseDflt (NET_SOCK_ID   sock_id,
                                              RTOS_ERR     *p_err)
{
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
    NET_SOCK     *p_sock  = DEF_NULL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used = DEF_NO;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnCloseDflt);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_FAIL);
    }
#endif


    p_sock = &NetSock_Tbl[sock_id];
                                                                 /* --------- CFG SOCK CONN CLOSE DFLT TIMEOUT --------- */
    NetSock_ConnCloseTimeoutDflt(p_sock);

    rtn_val = DEF_OK;


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_val);

#else
    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' compiler warning.          */
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    return (DEF_OK);
#endif
}


/*
*********************************************************************************************************
*                                  NetSock_CfgTimeoutConnCloseSet()
*
* Description : Set socket's connection close timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection close timeout.
*
*               timeout_ms  Desired timeout value :
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : DEF_OK,   socket connection close timeout successfully set.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) Configured timeout does NOT reschedule any current socket connection close timeout
*                   in progress but becomes effective the next time a socket pends on a connection close
*                   with timeout.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_CfgTimeoutConnCloseSet (NET_SOCK_ID   sock_id,
                                             CPU_INT32U    timeout_ms,
                                             RTOS_ERR     *p_err)
{
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_SOCK     *p_sock;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnCloseSet);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (DEF_FAIL);
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ----------- CFG SOCK CONN CLOSE TIMEOUT ------------ */
    NetSock_ConnCloseTimeoutSet(p_sock, timeout_ms);

    rtn_val = DEF_OK;


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

#else
    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' compiler warnings.         */
    PP_UNUSED_PARAM(timeout_ms);
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
#endif

    return (rtn_val);
}


/*
*********************************************************************************************************
*                                 NetSock_CfgTimeoutConnCloseGet_ms()
*
* Description : Get socket's connection close timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get connection close timeout.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_HANDLE
*
* Return(s)   : Socket's connection close network timeout value :
*                   0, on any error(s).
*                   NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value configured.
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) Despite inconsistency with other 'Get' status functions,
*                   NetSock_CfgTimeoutConnCloseGet_ms() includes 'Cfg' for consistency with other
*                   NetSock_CfgTimeout&&&() functions.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_INT32U  NetSock_CfgTimeoutConnCloseGet_ms (NET_SOCK_ID   sock_id,
                                               RTOS_ERR     *p_err)
{
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    CPU_INT32U    timeout_ms = 0u;
    NET_SOCK     *p_sock;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_FAIL);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_CfgTimeoutConnCloseGet_ms);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        Net_GlobalLockRelease();
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        return (0u);
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
                                                                /* ----------- GET SOCK CONN CLOSE TIMEOUT ------------ */
    timeout_ms = NetSock_ConnCloseTimeoutGet_ms(p_sock);


                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

                                                                /* ----------- RTN SOCK CONN CLOSE TIMEOUT ------------ */
    return (timeout_ms);

#else
    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' compiler warning.          */
    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    return (0u);
#endif

}


/*
*********************************************************************************************************
*                                           NetSock_OptGet()
*
* Description : Get the specified socket option from the sock_id socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get option from.
*
*               level       Protocol level from which to retrieve the socket option.
*
*               opt_name    Socket option to get the value.
*
*               p_opt_val   Pointer to a socket option value buffer.
*
*               p_opt_len   Pointer to a socket option value buffer length.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_NOT_FOUND
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE,    if NO error(s).
*               NET_SOCK_BSD_ERR_OPT_GET, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) The supported options are:
*
*                   (a) Level NET_SOCK_PROTOCOL_SOCK:
*
*                       Option name                     Returned data type    Option decription
*                       ----------------------------    -------------------   ------------------
*                       NET_SOCK_OPT_SOCK_TYPE          NET_SOCK_TYPE         Socket type:
*                                                                                 NET_SOCK_TYPE_STREAM
*                                                                                 NET_SOCK_TYPE_DATAGRAM
*
*                       NET_SOCK_OPT_SOCK_KEEP_ALIVE    CPU_BOOLEAN           Socket keep-alive status:
*                                                                                 DEF_ENABLED
*                                                                                 DEF_DISABLED
*
*                       NET_SOCK_OPT_SOCK_ACCEPT_CONN   CPU_BOOLEAN           Socket is in listen state:
*                                                                                 DEF_YES
*                                                                                 DEF_NO
*
*                       NET_SOCK_OPT_SOCK_TX_BUF_SIZE   NET_TCP_WIN_SIZE      TCP connection transmit windows size  value
*                       NET_SOCK_OPT_SOCK_RX_BUF_SIZE   NET_TCP_WIN_SIZE      TCP connection receive  windows size  value
*                       NET_SOCK_OPT_SOCK_TX_TIMEOUT    CPU_INT32U            TCP connection transmit queue timeout value
*                       NET_SOCK_OPT_SOCK_RX_TIMEOUT    CPU_INT32U            TCP connection receive  queue timeout value
*
*                   (b) Level NET_SOCK_PROTOCOL_IP:
*
*                       Option name                     Returned data type    Option decription
*                       -----------------------------   ------------------    ------------------
*                       NET_SOCK_OPT_IP_TOS             NET_IPv4_TOS          TCP connection transmit IP TOS
*                       NET_SOCK_OPT_IP_TTL             NET_IPv4_TTL          TCP connection transmit IP TTL
*                       NET_SOCK_OPT_IP_RX_IF           NET_IF_NBR            Receive interface number
*
*                   (c) Level NET_SOCK_PROTOCOL_TCP:
*
*                       Option name                     Returned data type    Option decription
*                       -----------------------------   ------------------    ------------------
*                       NET_SOCK_OPT_TCP_NO_DELAY       CPU_BOOLEAN           TCP connection transmit Nagle algorithm status:
*                                                                                 DEF_ENABLED
*                                                                                 DEF_DISABLED
*
*                       NET_SOCK_OPT_TCP_KEEP_CNT       NET_PKT_CTR           TCP keep alive maximum probe value
*                       NET_SOCK_OPT_TCP_KEEP_IDLE      NET_TCP_TIMEOUT_SEC   TCP keep alive timeout       value (in seconds)
*                       NET_SOCK_OPT_TCP_KEEP_INTVL     NET_TCP_TIMEOUT_SEC   TCP keep alive probe re-transmit timeout
*                                                                                                          value (in seconds)
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_OptGet (NET_SOCK_ID         sock_id,
                                   NET_SOCK_PROTOCOL   level,
                                   NET_SOCK_OPT_NAME   opt_name,
                                   void               *p_opt_val,
                                   NET_SOCK_OPT_LEN   *p_opt_len,
                                   RTOS_ERR           *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
   NET_IPv4_FLAGS      ip_flags;
   NET_IPv4_TOS        ip_tos;
   NET_IPv4_TTL        ip_ttl;
   NET_SOCK_ADDR       net_sock_addr;
   NET_IF_NBR          if_nbr;
#endif
   NET_SOCK           *p_sock;
   CPU_INT08U          addr_local[NET_CONN_ADDR_LEN_MAX];
   NET_CONN_ADDR_LEN   addr_len;
#ifdef  NET_TCP_MODULE_EN
   NET_CONN_ID         conn_id;
   NET_CONN_ID         conn_id_tcp;
   NET_TCP_CONN       *p_conn;
#endif
   CPU_BOOLEAN         is_used;
   NET_SOCK_RTN_CODE   rtn_code = NET_SOCK_BSD_ERR_OPT_GET;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
   RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_OPT_GET);
    RTOS_ASSERT_DBG_ERR_SET(( p_opt_val != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_OPT_GET);
    RTOS_ASSERT_DBG_ERR_SET(( p_opt_len != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_OPT_GET);
    RTOS_ASSERT_DBG_ERR_SET((*p_opt_len != 0), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);


                                                                /* -------------- VALIDATE OPTION LEVEL --------------- */
   switch (opt_name) {
       case NET_SOCK_OPT_IP_TOS:                                /* IP-level op.                                         */
       case NET_SOCK_OPT_IP_TTL:
       case NET_SOCK_OPT_IP_RX_IF:
                                                               /* Sock opt incompatible with other proto level than IP.  */
            RTOS_ASSERT_DBG_ERR_SET((level == NET_SOCK_PROTOCOL_IP), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            break;

       case NET_SOCK_OPT_TCP_NO_DELAY:                          /* TCP-level op.                                        */
       case NET_SOCK_OPT_TCP_KEEP_CNT:
       case NET_SOCK_OPT_TCP_KEEP_IDLE:
       case NET_SOCK_OPT_TCP_KEEP_INTVL:
                                                                /* Sock opt incompatible with other proto level than TCP.*/
            RTOS_ASSERT_DBG_ERR_SET((level == NET_SOCK_PROTOCOL_TCP), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            break;

       case NET_SOCK_OPT_SOCK_TYPE:                             /* Sock-level op.                                       */
       case NET_SOCK_OPT_SOCK_TX_BUF_SIZE:
       case NET_SOCK_OPT_SOCK_RX_BUF_SIZE:
       case NET_SOCK_OPT_SOCK_TX_TIMEOUT:
       case NET_SOCK_OPT_SOCK_RX_TIMEOUT:
       case NET_SOCK_OPT_SOCK_ACCEPT_CONN:
       case NET_SOCK_OPT_SOCK_KEEP_ALIVE:
                                                                /* Opt incompatible with other proto level than Sock.   */
            RTOS_ASSERT_DBG_ERR_SET((level == NET_SOCK_PROTOCOL_SOCK), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            break;

       default:
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
   }

   RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_OptGet);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

   p_sock = &NetSock_Tbl[sock_id];

   switch (opt_name) {
                                                                /* ----------------- IP-LEVEL OPTIONS ----------------- */
#ifdef  NET_IPv4_MODULE_EN
       case NET_SOCK_OPT_IP_TOS:
            if (*p_opt_len < (CPU_INT32S)sizeof(NET_IPv4_TOS)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

           *p_opt_len = sizeof(NET_IPv4_TOS);

            NetConn_IPv4TxParamsGet(p_sock->ID_Conn,            /* Get the conn's flags, TOS & TTL.                     */
                                   &ip_flags,
                                   &ip_tos,
                                   &ip_ttl);

            Mem_Copy(p_opt_val,
                    &ip_tos,
                    *p_opt_len);
            break;
#endif
#ifdef  NET_IPv4_MODULE_EN
       case NET_SOCK_OPT_IP_TTL:
            if (*p_opt_len < (CPU_INT32S)sizeof(NET_IPv4_TTL)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

           *p_opt_len = sizeof(NET_IPv4_TTL);

            NetConn_IPv4TxParamsGet(p_sock->ID_Conn,            /* Get the conn's flags, TOS & TTL.                     */
                                   &ip_flags,
                                   &ip_tos,
                                   &ip_ttl);

            Mem_Copy(p_opt_val,
                    &ip_ttl,
                    *p_opt_len);
            break;
#endif

       case NET_SOCK_OPT_IP_RX_IF:
            if (*p_opt_len < (CPU_INT32S)sizeof(NET_IF_NBR)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

           *p_opt_len = sizeof(NET_IF_NBR);
            addr_len = NET_CONN_ADDR_LEN_MAX;

            NetConn_AddrLocalGet(p_sock->ID_Conn,               /* Get the local addr of the sock.                      */
                                &addr_local[0],
                                &addr_len,
                                 p_err);
            if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                goto exit_release;
            }

#ifdef  NET_IPv4_MODULE_EN
            net_sock_addr.AddrFamily = NET_SOCK_ADDR_FAMILY_IP_V4;

            Mem_Copy(&net_sock_addr.Addr[0],
                     &addr_local[0],
                      NET_CONN_ADDR_LEN_MAX);

            addr_len = sizeof(NET_SOCK_ADDR);
                                                                /* Get the IF corresponding to the net_sock_addr.       */

           (void)NetSock_IsValidAddrLocal(                    NET_SOCK_PROTOCOL_FAMILY_IP_V4,
                                                             &net_sock_addr,
                                          (NET_SOCK_ADDR_LEN) addr_len,
                                                             &if_nbr,
                                                              p_err);
            if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                goto exit_release;
            }

            Mem_Copy(p_opt_val,
                    &if_nbr,
                    *p_opt_len);
#else
            Net_GlobalLockRelease();
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPT_GET);
#endif
            break;

#ifdef  NET_TCP_MODULE_EN
       case NET_SOCK_OPT_TCP_NO_DELAY:                          /* ---------------- TCP-LEVEL OPTIONS ----------------- */
            if (*p_opt_len < (CPU_INT32S)sizeof(CPU_BOOLEAN)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

            conn_id = p_sock->ID_Conn;

            is_used = NetConn_IsUsed(conn_id);
            if (is_used != DEF_YES) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                goto exit_release;
            }

            conn_id_tcp =  NetConn_ID_TransportGet(conn_id);

            p_conn      = &NetTCP_ConnTbl[conn_id_tcp];
           *p_opt_len   =  sizeof(CPU_BOOLEAN);

            Mem_Copy(p_opt_val,
                    &p_conn->TxWinSizeNagleEn,
                    *p_opt_len);
            break;


       case NET_SOCK_OPT_TCP_KEEP_CNT:
            if (*p_opt_len < (CPU_INT32S)sizeof(NET_PKT_CTR)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

            conn_id = p_sock->ID_Conn;

            is_used = NetConn_IsUsed(conn_id);
            if (is_used != DEF_YES) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                goto exit_release;
            }

            conn_id_tcp =  NetConn_ID_TransportGet(conn_id);

            p_conn      = &NetTCP_ConnTbl[conn_id_tcp];
           *p_opt_len   =  sizeof(NET_PKT_CTR);

            Mem_Copy(p_opt_val,
                    &p_conn->TxKeepAliveTh,
                    *p_opt_len);
            break;


       case NET_SOCK_OPT_TCP_KEEP_IDLE:
            if (*p_opt_len < (CPU_INT32S)sizeof(NET_TCP_TIMEOUT_SEC)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

            conn_id = p_sock->ID_Conn;

            is_used = NetConn_IsUsed(conn_id);
            if (is_used != DEF_YES) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                goto exit_release;
            }

            conn_id_tcp =  NetConn_ID_TransportGet(conn_id);

            p_conn      = &NetTCP_ConnTbl[conn_id_tcp];
           *p_opt_len   =  sizeof(NET_TCP_TIMEOUT_SEC);

            Mem_Copy(p_opt_val,
                    &p_conn->TimeoutConn_sec,
                    *p_opt_len);
            break;


       case NET_SOCK_OPT_TCP_KEEP_INTVL:
            if (*p_opt_len < (CPU_INT32S)sizeof(NET_TCP_TIMEOUT_SEC)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
            }

            conn_id = p_sock->ID_Conn;

            is_used = NetConn_IsUsed(conn_id);
            if (is_used != DEF_YES) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                goto exit_release;
            }

            conn_id_tcp =  NetConn_ID_TransportGet(conn_id);

            p_conn      = &NetTCP_ConnTbl[conn_id_tcp];
           *p_opt_len   =  sizeof(NET_TCP_TIMEOUT_SEC);

            Mem_Copy(p_opt_val,
                    &p_conn->TxKeepAliveRetryTimeout_sec,
                    *p_opt_len);
            break;
#endif

       case NET_SOCK_OPT_SOCK_TYPE:                             /* ---------------- SOCK-LEVEL OPTIONS ---------------- */
       case NET_SOCK_OPT_SOCK_TX_BUF_SIZE:
       case NET_SOCK_OPT_SOCK_RX_BUF_SIZE:
       case NET_SOCK_OPT_SOCK_TX_TIMEOUT:
       case NET_SOCK_OPT_SOCK_RX_TIMEOUT:
       case NET_SOCK_OPT_SOCK_ACCEPT_CONN:
       case NET_SOCK_OPT_SOCK_KEEP_ALIVE:
                                                                /* Get the sock-level opt.                              */
           (void)NetSock_OpGetSock(p_sock,
                                   opt_name,
                                   p_opt_val,
                                   p_opt_len,
                                   p_err);
            if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                goto exit_release;
            }
            break;


       default:
            Net_GlobalLockRelease();
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
   }

   PP_UNUSED_PARAM(is_used);

   rtn_code = NET_SOCK_BSD_ERR_NONE;

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_code);
}


/*
*********************************************************************************************************
*                                           NetSock_OptSet()
*
* Description : Set the specified socket option of the socket to a specified value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get option from.
*
*               level       Protocol level at which the option resides.
*
*               opt_name    Name of the single option to set.
*
*               p_opt_val   Pointer to the value to set to the socket option.
*
*               opt_len     Option length.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*                               RTOS_ERR_INVALID_STATE
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE,    if NO error(s).
*               NET_SOCK_BSD_ERR_OPT_SET, otherwise.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*
*               (2) The size of the p_opt_val and the value of opt_len must be equal to the size of
*                   the socket option requested to be set.
*
*               (3) The supported options are:
*
*                   (a) Level NET_SOCK_PROTOCOL_SOCK:
*
*                       Option name                     Returned data type    Option decription
*                       -----------------------------   ------------------    ------------------
*                       NET_SOCK_OPT_SOCK_KEEP_ALIVE    CPU_BOOLEAN           Socket keep-alive status:
*                                                                                 DEF_ENABLED
*                                                                                 DEF_DISABLED
*
*                       NET_SOCK_OPT_SOCK_TX_BUF_SIZE   NET_TCP_WIN_SIZE      TCP connection transmit windows size  value
*                       NET_SOCK_OPT_SOCK_RX_BUF_SIZE   NET_TCP_WIN_SIZE      TCP connection receive  windows size  value
*                       NET_SOCK_OPT_SOCK_TX_TIMEOUT    CPU_INT32U            TCP connection transmit queue timeout value
*                       NET_SOCK_OPT_SOCK_RX_TIMEOUT    CPU_INT32U            TCP connection receive  queue timeout value
*
*                   (b) Level NET_SOCK_PROTOCOL_IP:
*
*                       Option name                     Returned data type    Option decription
*                       -----------------------------   ------------------    ------------------
*                       NET_SOCK_OPT_IP_TOS             NET_IPv4_TOS          TCP connection transmit IP TOS
*                       NET_SOCK_OPT_IP_TTL             NET_IPv4_TTL          TCP connection transmit IP TTL
*                       NET_SOCK_OPT_IP_ADD_MEMBERSHIP  NET_IPv4_MREQ         Join  a multicast group
*                       NET_SOCK_OPT_IP_DROP_MEMBERSHIP NET_IPv4_MREQ         Leave a multicast group
*
*                   (c) Level NET_SOCK_PROTOCOL_TCP:
*
*                       Option name                     Returned data type    Option decription
*                       -----------------------------   ------------------    ------------------
*                       NET_SOCK_OPT_TCP_NO_DELAY       CPU_BOOLEAN           TCP connection transmit Nagle algorithm status:
*                                                                                 DEF_ENABLED
*                                                                                 DEF_DISABLED
*
*                       NET_SOCK_OPT_TCP_KEEP_CNT       NET_PKT_CTR           TCP keep alive maximum probe value
*                       NET_SOCK_OPT_TCP_KEEP_IDLE      NET_TCP_TIMEOUT_SEC   TCP keep alive timeout       value (in seconds)
*                       NET_SOCK_OPT_TCP_KEEP_INTVL     NET_TCP_TIMEOUT_SEC   TCP keep alive probe re-transmit timeout
*                                                                                                          value (in seconds)
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSock_OptSet (NET_SOCK_ID         sock_id,
                                   NET_SOCK_PROTOCOL   level,
                                   NET_SOCK_OPT_NAME   opt_name,
                                   void               *p_opt_val,
                                   NET_SOCK_OPT_LEN    opt_len,
                                   RTOS_ERR           *p_err)
{
    NET_SOCK           *p_sock;
    NET_SOCK_RTN_CODE   rtn_code          = NET_SOCK_BSD_ERR_OPT_SET;
    CPU_BOOLEAN         is_used;
#ifdef  NET_IPv4_MODULE_EN
    CPU_INT08U         *p_int08u_val;
#ifdef NET_IGMP_MODULE_EN
    NET_IPv4_MREQ      *mcast_info;
    NET_IPv4_ADDR       mcast_addr;
    NET_IPv4_ADDR       if_ip_addr;
    NET_IF_NBR          if_nbr;
#endif
#endif
#ifdef  NET_TCP_MODULE_EN
    NET_CONN_ID         conn_id_transport;
    CPU_INT16U         *p_int16u_val;
    CPU_INT32U         *p_int32u_val;
    CPU_BOOLEAN        *p_bool_val;
#endif


    RTOS_ASSERT_DBG_ERR_SET((p_opt_val != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_OPT_SET);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    rtn_code = NET_SOCK_BSD_ERR_NONE;

                                                                /* -------------- VALIDATE OPTION LEVEL --------------- */
    switch (opt_name) {
        case NET_SOCK_OPT_IP_TOS:                               /* IP-level op.                                         */
        case NET_SOCK_OPT_IP_TTL:
        case NET_SOCK_OPT_IP_ADD_MEMBERSHIP:
        case NET_SOCK_OPT_IP_DROP_MEMBERSHIP:
                                                                /* Opt incompatible with protocol level other than IP.  */
             RTOS_ASSERT_DBG_ERR_SET((level == NET_SOCK_PROTOCOL_IP), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
             break;


        case NET_SOCK_OPT_TCP_NO_DELAY:                         /* TCP-level op.                                        */
        case NET_SOCK_OPT_TCP_KEEP_CNT:
        case NET_SOCK_OPT_TCP_KEEP_IDLE:
        case NET_SOCK_OPT_TCP_KEEP_INTVL:
                                                                /* Opt incompatible with protocol level other than TCP. */
             RTOS_ASSERT_DBG_ERR_SET((level == NET_SOCK_PROTOCOL_TCP), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
             break;


        case NET_SOCK_OPT_SOCK_TX_BUF_SIZE:                     /* Sock-level op.                                       */
        case NET_SOCK_OPT_SOCK_RX_BUF_SIZE:
        case NET_SOCK_OPT_SOCK_TX_TIMEOUT:
        case NET_SOCK_OPT_SOCK_RX_TIMEOUT:
        case NET_SOCK_OPT_SOCK_KEEP_ALIVE:
                                                                /* Opt incompatible with protocol level other than sock.*/
             RTOS_ASSERT_DBG_ERR_SET((level == NET_SOCK_PROTOCOL_SOCK), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
             break;


        default:
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
    }


                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_OptSet);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif

    p_sock = &NetSock_Tbl[sock_id];
    if ((level == NET_SOCK_PROTOCOL_TCP) ||
        (level == NET_SOCK_PROTOCOL_SOCK)) {

        switch (p_sock->SockType) {
            case NET_SOCK_TYPE_STREAM:
    #ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
                switch (p_sock->Protocol) {
                    case NET_SOCK_PROTOCOL_TCP:
                         is_used = NetConn_IsUsed(p_sock->ID_Conn);
                         if (is_used != DEF_YES) {
                             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                             goto exit_release;
                         }

                         conn_id_transport = NetConn_ID_TransportGet(p_sock->ID_Conn);

                         is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                         if (is_used != DEF_YES) {
                             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                             goto exit_release;
                         }

                         switch (opt_name) {
                             case NET_SOCK_OPT_TCP_NO_DELAY:
                                  if (opt_len != sizeof(CPU_BOOLEAN)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_bool_val = (CPU_BOOLEAN *)p_opt_val;
                                 (void)NetSock_CfgTxNagleEnHandler(sock_id, *p_bool_val, p_err);
                                  goto exit_release;



                             case NET_SOCK_OPT_SOCK_TX_BUF_SIZE:
                                  if (opt_len != sizeof(NET_TCP_WIN_SIZE)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int16u_val = (CPU_INT16U *)p_opt_val;
                                 (void)NetTCP_ConnCfgTxWinSizeHandler(conn_id_transport, *p_int16u_val, p_err);
                                  goto exit_release;


                             case NET_SOCK_OPT_SOCK_TX_TIMEOUT:
                                  if (opt_len != sizeof(CPU_INT32U)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int32u_val = (CPU_INT32U *)p_opt_val;
                                  NetTCP_TxQ_TimeoutSet(conn_id_transport, *p_int32u_val);
                                  goto exit_release;


                             case NET_SOCK_OPT_SOCK_RX_BUF_SIZE:
                                  if (opt_len != sizeof(NET_TCP_WIN_SIZE)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int16u_val = (CPU_INT16U *)p_opt_val;
                                 (void)NetTCP_ConnCfgRxWinSizeHandler(conn_id_transport, *p_int16u_val, p_err);
                                  goto exit_release;


                             case NET_SOCK_OPT_SOCK_RX_TIMEOUT:
                                  if (opt_len != sizeof(CPU_INT32U)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int32u_val = (CPU_INT32U *)p_opt_val;
                                  NetTCP_RxQ_TimeoutSet(conn_id_transport, *p_int32u_val);
                                  goto exit_release;


                             case NET_SOCK_OPT_SOCK_KEEP_ALIVE:
                                  if (opt_len != sizeof(CPU_BOOLEAN)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_bool_val = (CPU_BOOLEAN *)p_opt_val;
                                 (void)NetTCP_ConnCfgTxKeepAliveEnHandler(conn_id_transport, *p_bool_val);
                                  goto exit_release;


                             case NET_SOCK_OPT_TCP_KEEP_CNT:
                                  if (opt_len != sizeof(NET_PKT_CTR)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int16u_val = (CPU_INT16U *)p_opt_val;
                                 (void)NetTCP_ConnCfgTxKeepAliveThHandler(conn_id_transport, *p_int16u_val);
                                  goto exit_release;


                             case NET_SOCK_OPT_TCP_KEEP_IDLE:
                                  if (opt_len != sizeof(NET_TCP_TIMEOUT_SEC)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int16u_val = (CPU_INT16U *)p_opt_val;

                                  (void)NetTCP_ConnCfgIdleTimeoutHandler(conn_id_transport, *p_int16u_val);
                                  goto exit_release;


                             case NET_SOCK_OPT_TCP_KEEP_INTVL:
                                  if (opt_len != sizeof(NET_TCP_TIMEOUT_SEC)) {
                                      Net_GlobalLockRelease();
                                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                                  }

                                  p_int16u_val = (CPU_INT16U *)p_opt_val;
                                 (void)NetTCP_ConnCfgTxKeepAliveRetryHandler(conn_id_transport, *p_int16u_val);
                                  goto exit_release;


                             default:                               /* Unsupported options.                                 */
                                  Net_GlobalLockRelease();
                                  RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                         }
                         break;


                    case NET_SOCK_PROTOCOL_NONE:
                    default:                                        /* Invalid of unspecified protocol.                     */
                         Net_GlobalLockRelease();
                         RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
                 }
    #else
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPT_SET);
    #endif
                 break;


            case NET_SOCK_TYPE_NONE:
            case NET_SOCK_TYPE_DATAGRAM:
            default:
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
        }
    }


    switch (opt_name) {                                         /* ----------------- IP-LEVEL OPTIONS ----------------- */
        case NET_SOCK_OPT_IP_TOS:
#ifdef  NET_IPv4_MODULE_EN
             if (opt_len != sizeof(NET_IPv4_TOS)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
             }

             is_used = NetConn_IsUsed(p_sock->ID_Conn);
             if (is_used != DEF_YES) {
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                 goto exit_release;
             }

             p_int08u_val = (CPU_INT08U *)p_opt_val;
             NetConn_IPv4TxTOS_Set(p_sock->ID_Conn, *p_int08u_val);
#else
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPT_SET);
#endif
             break;


        case NET_SOCK_OPT_IP_TTL:
#ifdef  NET_IPv4_MODULE_EN
             if (opt_len != sizeof(NET_IPv4_TTL)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
             }

             is_used = NetConn_IsUsed(p_sock->ID_Conn);
             if (is_used != DEF_YES) {
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                 goto exit_release;
             }

             p_int08u_val = (CPU_INT08U *)p_opt_val;
             NetConn_IPv4TxTTL_Set(p_sock->ID_Conn, *p_int08u_val);
#else
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPT_SET);
#endif
             break;

        case NET_SOCK_OPT_IP_ADD_MEMBERSHIP:
        case NET_SOCK_OPT_IP_DROP_MEMBERSHIP:
#if  defined(NET_IGMP_MODULE_EN)
             if (opt_len != sizeof(NET_IPv4_MREQ)) {
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
             }
             if (p_sock->SockType != NET_SOCK_TYPE_DATAGRAM) {  /* Multicast applies to datagram only                   */
                 Net_GlobalLockRelease();
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_TYPE, NET_SOCK_BSD_ERR_OPT_SET);
             }

             mcast_info = (NET_IPv4_MREQ *)p_opt_val;           /* Obtain multicast address structure                   */
             mcast_addr =  mcast_info->mcast_addr;              /* Obtain multicast address of group                    */
             if_ip_addr =  mcast_info->if_ip_addr;              /* Obtain local interface IP address                    */

             if (if_ip_addr == NET_IPv4_ADDR_NONE) {            /* If INADDR_ANY, determine the default interface       */
                 if_nbr = NetIF_GetDflt();
             }
             else {                                             /* Determine IF number from provided local IP address   */
                 if_nbr = NetIPv4_GetAddrHostIF_Nbr(if_ip_addr);
             }

             if (opt_name == NET_SOCK_OPT_IP_ADD_MEMBERSHIP) {
                 NetIGMP_HostGrpJoinHandler(if_nbr, mcast_addr, p_err);
             }
             else {                                             /* NET_SOCK_OPT_IP_DROP_MEMBERSHIP                      */
                 NetIGMP_HostGrpLeaveHandler(if_nbr, mcast_addr, p_err);
             }

#else
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPT_SET);
#endif
             break;

        default:
             Net_GlobalLockRelease();
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_SET);
    }


exit_release:
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        rtn_code = NET_SOCK_BSD_ERR_OPT_SET;
    }

                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                        NetSock_PoolStatGet()
*
* Description : Get socket statistics pool.
*
* Argument(s) : none.
*
* Return(s)   : Socket statistics pool, if NO error(s).
*               NULL   statistics pool, otherwise.
*
* Note(s)     : (1) [INTERNAL] 'NetSock_PoolStat' MUST ALWAYS be accessed exclusively in critical sections.
*********************************************************************************************************
*/

NET_STAT_POOL  NetSock_PoolStatGet (void)
{
    NET_STAT_POOL  stat_pool;
#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
    CPU_SR_ALLOC();
#endif

    NetStat_PoolClr(&stat_pool);                                /* Init rtn pool stat for err.                          */

#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
    CPU_CRITICAL_ENTER();
    stat_pool = NetSock_PoolStat;
    CPU_CRITICAL_EXIT();
#endif


    return (stat_pool);
}


/*
*********************************************************************************************************
*                                   NetSock_PoolStatResetMaxUsed()
*
* Description : Reset socket statistics pool's maximum number of entries used.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : (1) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

void  NetSock_PoolStatResetMaxUsed (void)
{
#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
                                                                /* Acquire net lock.                                    */
    Net_GlobalLockAcquire((void *)NetSock_PoolStatResetMaxUsed);

    NetStat_PoolResetUsedMax(&NetSock_PoolStat);                /* Reset net sock stat pool.                            */

                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();
#endif
}


/*
*********************************************************************************************************
*                                       NetSock_GetLocalIPAddr()
*
* Description : Get the local IP addr used in the socket connection.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier.
*
*               p_buf_addr  Pointer to a buffer to return the local IP addr.
*
*               p_family    Pointer to variable that will receive the conn family type of the local IP addr.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RTOS_ERR_NONE
*                               RTOS_ERR_INVALID_TYPE
*                               RTOS_ERR_INVALID_HANDLE
*                               RTOS_ERR_NET_INVALID_CONN
*
* Return(s)   : none.
*
* Note(s)     : (1) 'p_buf_addr' must be a buffer at least of NET_CONN_ADDR_LEN_MAX bytes large.
*
*               (2) [INTERNAL] This function is called by application function(s) :
*
*                   (a) MUST NOT be called with the global network lock already acquired;
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

void  NetSock_GetLocalIPAddr (NET_SOCK_ID        sock_id,
                              CPU_INT08U        *p_buf_addr,
                              NET_SOCK_FAMILY   *p_family,
                              RTOS_ERR          *p_err)
{
    NET_SOCK     *p_sock;
    NET_CONN_ID   conn_id;
    NET_CONN     *p_conn;
    CPU_BOOLEAN   is_used;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
                                                                /* See Note #2b.                                        */
    Net_GlobalLockAcquire((void *)NetSock_GetLocalIPAddr);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif
                                                                /* ----------------- GET SOCK CONN ID ----------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
       case NET_SOCK_TYPE_DATAGRAM:
       case NET_SOCK_TYPE_STREAM:
            conn_id = p_sock->ID_Conn;
            break;

       case NET_SOCK_TYPE_NONE:
       default:
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
            goto exit_release;
    }

                                                                /* --------------- VALIDATE CONNECTION ---------------- */
    is_used = NetConn_IsUsed(conn_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
        goto exit_release;
    }

                                                                /* ------------ GET LOCAL IP ADDR & FAMILY ------------ */
    p_conn   = &NetConn_Tbl[conn_id];
   *p_family = (NET_SOCK_FAMILY)p_conn->Family;
    switch (*p_family) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_FAMILY_IP_V4:
             Mem_Copy(p_buf_addr, &p_conn->AddrLocal[NET_SOCK_ADDR_IP_V4_IX_ADDR], NET_IPv4_ADDR_SIZE);
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_FAMILY_IP_V6:
            Mem_Copy(p_buf_addr, &p_conn->AddrLocal[NET_SOCK_ADDR_IP_V6_IX_ADDR], NET_IPv6_ADDR_SIZE);
            break;
#endif

        default:
            Net_GlobalLockRelease();
            RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           NetSock_Init()
*
* Description : (1) Initialize Network Socket Layer :
*
*                   (a) Perform    Socket/OS initialization
*                   (b) Initialize socket pool
*                   (c) Initialize socket table
*                   (d) Initialize random port number queue
*
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) The following network socket initialization MUST be sequenced as follows :
*
*                   (a) NetSock_Init()   MUST precede ALL other network socket initialization functions
*                   (b) Network socket pool MUST be initialized PRIOR to initializing the pool with pointers
*                            to sockets
*********************************************************************************************************
*/

void  NetSock_Init (MEM_SEG   *p_mem_seg,
                    RTOS_ERR  *p_err)
{
    NET_SOCK      *p_sock;
    NET_SOCK_QTY   i;


                                                                /* ------- GET MEMORY SEGMENT FOR SOCKET TABLE -------- */
    NetSock_Tbl = (NET_SOCK *)Mem_SegAlloc("Socket Table",
                                            p_mem_seg,
                                            sizeof(NET_SOCK) * NET_SOCK_NBR_SOCK,
                                            p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
                                                                /* --------------- INIT SOCK POOL/STATS --------------- */
    NetSock_PoolPtr = DEF_NULL;                                 /* Init-clr sock pool (see Note #2b).                   */

#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
    NetStat_PoolInit(&NetSock_PoolStat,
                      NET_SOCK_NBR_SOCK);
#endif

                                                                /* ------------------ INIT SOCK TBL ------------------- */
    p_sock = &NetSock_Tbl[0];
    for (i = 0; i < (NET_SOCK_QTY)NET_SOCK_NBR_SOCK; i++) {
        p_sock->ID    = (NET_SOCK_ID)i;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        p_sock->ID_SockParent =  NET_SOCK_ID_NONE;
#endif

        p_sock->State         =  NET_SOCK_STATE_FREE;           /* Init each sock as free/NOT used.                     */
        p_sock->Flags         =  NET_SOCK_FLAG_SOCK_NONE;

#ifdef  NET_SECURE_MODULE_EN
        p_sock->SecureSession = DEF_NULL;                       /* Init each sock w/ NULL secure session.               */
#endif

        NetSock_InitObj(p_sock, p_mem_seg, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             goto exit;
        }

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        SList_Init(&p_sock->ConnAcceptQ_Ptr);
#endif

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
        NetSock_Clr(p_sock);
#endif

                                                                /* --------------- CFG SOCK BLOCK MODE ---------------- */
#if  (NET_SOCK_DFLT_NO_BLOCK_EN == DEF_ENABLED)
        DEF_BIT_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
#else
        DEF_BIT_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
#endif

        p_sock->NextPtr = NetSock_PoolPtr;                      /* Free each sock to sock pool (see Note #2).           */
        NetSock_PoolPtr = p_sock;

        p_sock++;
    }

                                                                /* ---------- CREATE ACCEPT Q OBJECT'S POOL ----------- */
    Mem_DynPoolCreate("Accept Q object pool",
                      &NetSock_AcceptQ_ObjPool,
                       DEF_NULL,
                       sizeof(NET_SOCK_ACCEPT_Q_OBJ),
                       sizeof(CPU_ALIGN),
                       1u,
                       NET_SOCK_NBR_SOCK * NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }


#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    Mem_DynPoolCreate("Select object pool",
                      &NetSock_SelObjPool,
                       DEF_NULL,
                       sizeof(NET_SOCK_SEL_OBJ),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }
#endif

                                                                /* -------------- INIT RANDOM PORT NBR Q -------------- */
#ifndef NET_SOCK_CFG_PORT_RANDOM_START
    NetSock_RandomPortNbrCur = (NET_PORT_NBR)NetUtil_RandomRangeGet(NET_SOCK_PORT_NBR_RANDOM_MIN,
                                                                    NET_SOCK_PORT_NBR_RANDOM_MAX);

#else
    NetSock_RandomPortNbrCur = NET_SOCK_CFG_PORT_RANDOM_START;
#endif

exit:
    return;
}


/*
*********************************************************************************************************
*                                         NetSock_SelInternal()
*
* Description : Socket Select function for internal usage.
*
* Argument(s) : sem_handle          Semaphore handle to use for the socket select operation.
*
*               sock_nbr_max        Maximum number of socket descriptors/handle identifiers in the socket
*                                   descriptor sets.
*
*               p_sock_desc_rd      Pointer to a set of socket descriptors/handle identifiers to :
*
*                                       (a) Check for available read operation(s).
*
*                                       (b) (1) Return the actual socket descriptors/handle identifiers
*                                                   ready for available read  operation(s), if NO error(s).
*                                           (2) Return the initial, non-modified set of socket descriptors/
*                                                   handle identifiers, on any error(s).
*                                           (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                                   if any timeout expires.
*
*               p_sock_desc_wr      Pointer to a set of socket descriptors/handle identifiers to :
*
*                                       (a) Check for available write operation(s).
*
*                                       (b) (1) Return the actual socket descriptors/handle identifiers
*                                                   ready for available write operation(s), if NO error(s).
*                                           (2) Return the initial, non-modified set of socket descriptors/
*                                                   handle identifiers, on any error(s).
*                                           (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                                   if any timeout expires.
*
*               p_sock_desc_err     Pointer to a set of socket descriptors/handle identifiers to :
*
*                                       (a) Check for any error(s) and/or exception(s).
*
*                                       (b) (1) Return the actual socket descriptors/handle identifiers
*                                                   flagged with any error(s) &/or exception(s), if NO
*                                                   non-descriptor-related error(s).
*                                           (2) Return the initial, non-modified set of socket descriptors/
*                                                   handle identifiers, on any error(s).
*                                           (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                                   if any timeout expires.
*
*               p_timeout           Pointer to a timeout.
*
*               p_err               Pointer to the variable that will receive one of the following error code(s)
*                                   from this function.
*
* Return(s)   : Number of socket descriptors with available resources &/or operations, if any.
*               NET_SOCK_BSD_RTN_CODE_TIMEOUT, on timeout.
*               NET_SOCK_BSD_ERR_SEL, otherwise.
*
* Note(s)     : (1) Instead of creating a semaphore inside the function like the traditional Select
*                   function, this one take an already existing semaphore so that the Select pend can
*                   be stopped by other means that a socket activity.
*********************************************************************************************************
*/
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
NET_SOCK_RTN_CODE  NetSock_SelInternal (KAL_SEM_HANDLE     sem_handle,
                                        NET_SOCK_QTY       sock_nbr_max,
                                        NET_SOCK_DESC     *p_sock_desc_rd,
                                        NET_SOCK_DESC     *p_sock_desc_wr,
                                        NET_SOCK_DESC     *p_sock_desc_err,
                                        NET_SOCK_TIMEOUT  *p_timeout,
                                        RTOS_ERR          *p_err)
{
    NET_SOCK_SEL_OBJ   *p_sel_obj = DEF_NULL;
    NET_SOCK           *p_sock;
    NET_SOCK_ID         sock_id;
    NET_SOCK_QTY        sock_nbr_max_actual;
    NET_SOCK_RTN_CODE   sock_nbr_rdy;
    NET_SOCK_DESC       sock_desc_rtn_rd;
    NET_SOCK_DESC       sock_desc_rtn_wr;
    NET_SOCK_DESC       sock_desc_rtn_err;
    CPU_BOOLEAN         sock_desc_used_rd;
    CPU_BOOLEAN         sock_desc_used_wr;
    CPU_BOOLEAN         sock_desc_used_err;
    NET_SOCK_SEL_OBJ   *p_sel_obj_cur  = DEF_NULL;
    NET_SOCK_SEL_OBJ   *p_sel_obj_next = DEF_NULL;
    CPU_INT32U          timeout_sec;
    CPU_INT32U          timeout_us;
    CPU_INT32U          timeout_ms;
    RTOS_ERR            local_err;


                                                                /* ---------------- VALIDATE ARGUMENTS ---------------- */
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, NET_SOCK_BSD_ERR_SEL);
    RTOS_ASSERT_DBG_ERR_SET((KAL_SEM_HANDLE_IS_NULL(sem_handle) == DEF_NO), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
    RTOS_ASSERT_DBG_ERR_SET((sock_nbr_max <= (NET_SOCK_QTY)NET_SOCK_DESC_NBR_MAX_DESC), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
#if (NET_SOCK_DESC_NBR_MIN_DESC > DEF_INT_16U_MIN_VAL)          /* (see Note #3d2A1).                                   */
    RTOS_ASSERT_DBG_ERR_SET((sock_nbr_max >= NET_SOCK_DESC_NBR_MIN_DESC), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
#endif

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* ----------------- VALIDATE TIMEOUT ----------------- */
    if (p_timeout != DEF_NULL) {
        if (p_timeout->timeout_sec < 0) {
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
        }
        if (p_timeout->timeout_us  < 0) {
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_SEL);
        }
    }
#endif

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
    Net_GlobalLockAcquire((void *)NetSock_SelInternal);

                                                                /* ---- VALIDATE / CHK / CFG SOCK DESC(S) FOR RDY ----- */
    sock_nbr_max_actual = DEF_MIN(sock_nbr_max, (NET_SOCK_ID)NET_SOCK_DESC_NBR_MAX_DESC);

    if (sock_nbr_max_actual > 0) {
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_COPY(&sock_desc_rtn_rd,  p_sock_desc_rd);
        } else {
            NET_SOCK_DESC_INIT(&sock_desc_rtn_rd);
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_COPY(&sock_desc_rtn_wr,  p_sock_desc_wr);
        } else {
            NET_SOCK_DESC_INIT(&sock_desc_rtn_wr);
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_COPY(&sock_desc_rtn_err,  p_sock_desc_err);
        } else {
            NET_SOCK_DESC_INIT(&sock_desc_rtn_err);
        }

        sock_nbr_rdy = NetSock_SelDescHandler(sock_nbr_max_actual,
                                             &sock_desc_rtn_rd,
                                             &sock_desc_rtn_wr,
                                             &sock_desc_rtn_err,
                                              p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            if (sock_nbr_rdy == 0) {
                goto exit_release;
            }
        }
    } else {
        sock_nbr_rdy = 0;
    }



    if (sock_nbr_rdy != 0) {                                    /* If any     sock desc's rdy,                  ..      */
                                                                /* .. rtn rdy sock desc's (see Note #3b2B1a1) & ..      */
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_COPY(p_sock_desc_rd,  &sock_desc_rtn_rd );
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_COPY(p_sock_desc_wr,  &sock_desc_rtn_wr );
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_COPY(p_sock_desc_err, &sock_desc_rtn_err);
        }

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
        goto exit_release;
    }


                                                                /* ---------- CFG TIMEOUT / HANDLE TIME DLY ----------- */
    if (p_timeout != DEF_NULL) {                                /* If avail, cfg timeout vals    (see Note #3b3A1),     */
        timeout_sec = (CPU_INT32U)p_timeout->timeout_sec;
        timeout_us  = (CPU_INT32U)p_timeout->timeout_us;
    } else {                                                    /* .. else  cfg timeout to block (see Note #3b3A2).     */
        timeout_sec = NET_TMR_TIME_INFINITE;
        timeout_us  = NET_TMR_TIME_INFINITE;
    }


    if ((timeout_sec == 0) && (timeout_us == 0)) {              /* If timeout = 0, handle as non-blocking poll ..       */
                                                                /* .. timeout (see Note #3b3A1a3).                      */

                                                                /* Zero-clr rtn sock desc's (see Note #3c2B).           */
                                                                /* Clr because no sock is ready at this point.          */
        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_rd );
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_wr );
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_err);
        }

        sock_nbr_rdy = NET_SOCK_BSD_RTN_CODE_TIMEOUT;
        RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_BLOCK);
        goto exit_release;
    }


                                                                /* ------------ PREPARE SOCKET BEFORE PEND ------------ */
    for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {
         p_sock = &NetSock_Tbl[sock_id];

         if (p_sock_desc_rd != DEF_NULL) {
             sock_desc_used_rd = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_rd);
         } else {
             sock_desc_used_rd = DEF_NO;
         }

         if (p_sock_desc_wr != DEF_NULL) {
             sock_desc_used_wr = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_wr);
         } else {
             sock_desc_used_wr = DEF_NO;
         }

         if (p_sock_desc_err != DEF_NULL) {
             sock_desc_used_err = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_err);
         } else {
             sock_desc_used_err = DEF_NO;
         }


         if ((sock_desc_used_rd  == DEF_YES) ||
             (sock_desc_used_wr  == DEF_YES) ||
             (sock_desc_used_err == DEF_YES)) {

             p_sel_obj = (NET_SOCK_SEL_OBJ *)Mem_DynPoolBlkGet(&NetSock_SelObjPool, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  sock_nbr_rdy = NET_SOCK_BSD_ERR_SEL;
                  RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
                  goto exit_release;
             }

             p_sel_obj->SockSelPendingFlags = NET_SOCK_SEL_EVENT_FLAG_NONE;

             if (sock_desc_used_rd == DEF_YES) {
                 p_sel_obj->SockSelPendingFlags |= NET_SOCK_SEL_EVENT_FLAG_RD;
             }

             if (sock_desc_used_wr == DEF_YES) {
                 p_sel_obj->SockSelPendingFlags |= NET_SOCK_SEL_EVENT_FLAG_WR;
             }

             if (sock_desc_used_err == DEF_YES) {
                 p_sel_obj->SockSelPendingFlags |= NET_SOCK_SEL_EVENT_FLAG_ERR;
             }

             p_sel_obj->SockSelTaskSignalObj = sem_handle;
             p_sel_obj->ObjPrevPtr           = p_sock->SelObjTailPtr;
             p_sock->SelObjTailPtr           = p_sel_obj;
         }
    }


    timeout_ms = NetUtil_TimeSec_uS_To_ms(timeout_sec, timeout_us);
    if (timeout_ms == NET_TMR_TIME_INFINITE) {
        timeout_ms =  KAL_TIMEOUT_INFINITE;
    }

                                                                /* ------ WAIT ON MULTIPLE SOCK DESC'S / EVENTS ------- */
    Net_GlobalLockRelease();

    KAL_SemPend(sem_handle, KAL_OPT_PEND_NONE, timeout_ms, p_err);

    Net_GlobalLockAcquire((void *)NetSock_SelInternal);

                                                                /* -------- REMOVE SOCKET SELECT OBJ FROM LIST -------- */
    for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {

         p_sock = &NetSock_Tbl[sock_id];

         if (p_sock_desc_rd != DEF_NULL) {
             sock_desc_used_rd = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_rd);
         } else {
             sock_desc_used_rd = DEF_NO;
         }

         if (p_sock_desc_wr != DEF_NULL) {
             sock_desc_used_wr = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_wr);
         } else {
             sock_desc_used_wr = DEF_NO;
         }

         if (p_sock_desc_err != DEF_NULL) {
             sock_desc_used_err = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_err);
         } else {
             sock_desc_used_err = DEF_NO;
         }

         if (p_sock->SelObjTailPtr != DEF_NULL) {

             p_sel_obj_cur  = p_sock->SelObjTailPtr;
             p_sel_obj_next = DEF_NULL;

             while (p_sel_obj_cur != DEF_NULL) {
                 NET_SOCK_SEL_OBJ  *p_sel_obj_prev = p_sel_obj_cur->ObjPrevPtr;


                 if (p_sel_obj_cur->SockSelTaskSignalObj.SemObjPtr == sem_handle.SemObjPtr) {
                     NET_SOCK_SEL_OBJ   *p_sel_obj_to_free  = DEF_NULL;


                     if (p_sock->SelObjTailPtr == p_sel_obj_cur) {
                         p_sock->SelObjTailPtr  = p_sel_obj_prev;

                     } else if (p_sel_obj_next != DEF_NULL) {
                         p_sel_obj_next->ObjPrevPtr = p_sel_obj_prev;
                     }

                     p_sel_obj_to_free = p_sel_obj_cur;
                     p_sel_obj_cur  = p_sel_obj_cur->ObjPrevPtr;

                     RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                     Mem_DynPoolBlkFree(&NetSock_SelObjPool, p_sel_obj_to_free, &local_err);
                     RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, 0);

                 } else {
                     p_sel_obj_next = p_sel_obj_cur;
                     p_sel_obj_cur  = p_sel_obj_cur->ObjPrevPtr;
                 }
             }
         }
    }

                                                                /* ---------- CHECK ERROR RETURNED FROM PEND ---------- */
    switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_TIMEOUT:
             if (p_sock_desc_rd != DEF_NULL) {
                 NET_SOCK_DESC_INIT(p_sock_desc_rd );
             }

             if (p_sock_desc_wr != DEF_NULL) {
                 NET_SOCK_DESC_INIT(p_sock_desc_wr );
             }

             if (p_sock_desc_err != DEF_NULL) {
                 NET_SOCK_DESC_INIT(p_sock_desc_err);
             }
             sock_nbr_rdy = NET_SOCK_BSD_RTN_CODE_TIMEOUT;
             goto exit_release;

        default:
             sock_nbr_rdy = NET_SOCK_BSD_ERR_SEL;
             goto exit_release;
    }

                                                                /* --------------- RETURN IF NO SOCKET ---------------- */
    if (sock_nbr_max < 1) {

        if (p_sock_desc_rd != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_rd );
        }

        if (p_sock_desc_wr != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_wr );
        }

        if (p_sock_desc_err != DEF_NULL) {
            NET_SOCK_DESC_INIT(p_sock_desc_err);
        }

        sock_nbr_rdy = NET_SOCK_BSD_RTN_CODE_OK;
        goto exit_release;
    }

                                                                /* -------------- CHECK SOCKET ACTIVITY --------------- */
    if (p_sock_desc_rd != DEF_NULL) {
        NET_SOCK_DESC_COPY(&sock_desc_rtn_rd, p_sock_desc_rd);
    } else {
        NET_SOCK_DESC_INIT(&sock_desc_rtn_rd);
    }

    if (p_sock_desc_wr != DEF_NULL) {
        NET_SOCK_DESC_COPY(&sock_desc_rtn_wr, p_sock_desc_wr);
    } else {
        NET_SOCK_DESC_INIT(&sock_desc_rtn_wr);
    }

    if (p_sock_desc_err != DEF_NULL) {
        NET_SOCK_DESC_COPY(&sock_desc_rtn_err, p_sock_desc_err);
    } else {
        NET_SOCK_DESC_INIT(&sock_desc_rtn_err);
    }


    sock_nbr_rdy = NetSock_SelDescHandler(sock_nbr_max_actual,
                                         &sock_desc_rtn_rd,
                                         &sock_desc_rtn_wr,
                                         &sock_desc_rtn_err,
                                          p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        if (sock_nbr_rdy == 0) {
            goto exit_release;
        }
    }

    if (p_sock_desc_rd != DEF_NULL) {
        NET_SOCK_DESC_COPY(p_sock_desc_rd, &sock_desc_rtn_rd);
    }

    if (p_sock_desc_wr != DEF_NULL) {
        NET_SOCK_DESC_COPY(p_sock_desc_wr,  &sock_desc_rtn_wr );
    }

    if (p_sock_desc_err != DEF_NULL) {
        NET_SOCK_DESC_COPY(p_sock_desc_err, &sock_desc_rtn_err);
    }

exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
    Net_GlobalLockRelease();

    return (sock_nbr_rdy);                                      /* Rtn nbr rdy sock desc's (see Note #3c1A1).           */
}
#endif


/*
*********************************************************************************************************
*                                         NetSock_RxQ_Clr()
*
* Description : Clear socket receive queue signal.
*
*               (1) Socket receive queue signals apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to clear receive queue signal.
*               -------     Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSock_RxQ_Clr (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemSet(p_sock->RxQ_SignalObj, 0u, &local_err);          /* Clear TCP connection receive queue signal.           */
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}


/*
*********************************************************************************************************
*                                         NetSock_RxQ_Wait()
*
* Description : Wait on socket receive queue.
*
*               (1) Socket receive queue signals apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to wait on receive queue.
*               -------     Argument checked in NetSock_RxDataHandlerDatagram().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) If timeouts NOT desired, wait on socket receive queue forever (i.e. do NOT exit).
*
*                   (b) If timeout      desired, return NET_SOCK_ERR_RX_Q_EMPTY error on socket receive
*                       queue timeout.  Implement timeout with OS-dependent functionality.
*********************************************************************************************************
*/

void  NetSock_RxQ_Wait (NET_SOCK     *p_sock,
                        RTOS_ERR     *p_err)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    timeout_ms = p_sock->RxQ_SignalTimeout_ms;
    CPU_CRITICAL_EXIT();

                                                                /* Wait on socket receive queue ...                     */
                                                                /* ... with configured timeout (see Note #1b).          */
    KAL_SemPend(p_sock->RxQ_SignalObj, KAL_OPT_PEND_NONE, timeout_ms, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
    }
}


/*
*********************************************************************************************************
*                                        NetSock_RxQ_Signal()
*
* Description : Signal socket receive queue.
*
*               (1) Socket receive queue signals apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal receive queue.
*               -------     Argument validated in NetSock_RxPktDemux(),
*                                                 NetSock_RxDataHandlerDatagram().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSock_RxQ_Signal (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemPost(p_sock->RxQ_SignalObj, KAL_OPT_PEND_NONE, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_RX);
#endif
}


/*
*********************************************************************************************************
*                                        NetSock_RxQ_Abort()
*
* Description : Abort wait on socket receive queue.
*
*               (1) Socket receive queue signals apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to abort wait on socket receive
*               -------         queue.
*
*                           Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSock_RxQ_Abort (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                /* Abort wait on socket receive queue ...               */
                                                                /* ... for ALL waiting tasks.                           */
   KAL_SemPendAbort(p_sock->RxQ_SignalObj, &local_err);
   RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_RX_ABORT);
#endif
}


/*
*********************************************************************************************************
*                                      NetSock_RxQ_TimeoutDflt()
*
* Description : Set socket receive queue to configured-default timeout value.
*
*               (1) Socket receive queue timeouts apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set receive queue configured-
*               -------         default timeout.
*
*                           Argument checked in NetSock_CfgTimeoutRxQ_Dflt(),
*                                               NetSock_Clr(),
*                                               NetSock_Init().
*
* Return(s)   : none.
*
* Note(s)     : (2) NetSock_RxQ_TimeoutDflt() is called by network protocol suite function(s) &
*                   may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/

void  NetSock_RxQ_TimeoutDflt (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
                                                                /* Set socket receive queue  timeout ...                */
                                                                /* ... to configured-default timeout value.             */
    timeout_ms = NET_SOCK_DFLT_TIMEOUT_RX_Q_MS;

    NetSock_RxQ_TimeoutSet(p_sock, timeout_ms);
}


/*
*********************************************************************************************************
*                                       NetSock_RxQ_TimeoutSet()
*
* Description : Set socket receive queue timeout value.
*
*               (1) Socket receive queue timeouts apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : p_sock      Pointer to socket object.
*
*               timeout_ms  Timeout value.
*
* Return(s)   : none.
*
* Note(s)     : (2) NetSock_RxQ_TimeoutSet() is called by network protocol suite function(s) & may be
*                   called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/

void  NetSock_RxQ_TimeoutSet (NET_SOCK    *p_sock,
                              CPU_INT32U   timeout_ms)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Set socket receive queue timeout value ...           */
                                                                /* ... (in OS ticks).                                   */
    p_sock->RxQ_SignalTimeout_ms = timeout_ms;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                    NetSock_RxQ_TimeoutGet_ms()
*
* Description : Get socket receive queue timeout value.
*
*               (1) Socket receive queue timeouts apply to the following socket type(s)/protocol(s) :
*
*                   (a) Datagram
*                       (1) UDP
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get receive queue timeout.
*               -------     Argument checked in NetSock_CfgTimeoutRxQ_Get_ms().
*
* Return(s)   : Socket receive queue network timeout value  :
*
*                   NET_TMR_TIME_INFINITE, if infinite (i.e. NO timeout) value configured.
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (2) NetSock_RxQ_TimeoutGet_ms() is called by network protocol suite function(s) & may
*                   be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/

CPU_INT32U  NetSock_RxQ_TimeoutGet_ms (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Get socket receive queue timeout value ...           */
    timeout_ms = p_sock->RxQ_SignalTimeout_ms;                /* ... (in OS ticks).                                   */
    CPU_CRITICAL_EXIT();

    return (timeout_ms);
}


/*
*********************************************************************************************************
*                                        NetSock_ConnReqClr()
*
* Description : Clear socket connection request signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to clear connection request signal.
*               -------     Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if  defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnReqClr (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemSet(p_sock->ConnReqSignalObj, 0u, &local_err);       /* Clear socket connection request signal.              */
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_ConnReqWait()
*
* Description : Wait on socket connection request signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to wait on connection request signal.
*               -------     Argument checked in NetSock_ConnHandlerStream().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) (a) If timeouts NOT desired, wait on socket connection request signal forever
*                       (i.e. do NOT exit).
*
*                   (b) If timeout      desired, return NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT error on socket
*                       connection request timeout.  Implement timeout with OS-dependent functionality.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnReqWait (NET_SOCK  *p_sock,
                           RTOS_ERR  *p_err)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    timeout_ms = p_sock->ConnReqSignalTimeout_ms;
    CPU_CRITICAL_EXIT();
                                                                /* Wait on socket connection request signal ...         */
                                                                /* ... with configured timeout (see Note #1b).          */
    KAL_SemPend(p_sock->ConnReqSignalObj, KAL_OPT_PEND_NONE, timeout_ms, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
    }
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_ConnReqSignal()
*
* Description : Signal socket that connection request complete; socket now connected.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal connection request complete.
*               -------     Argument checked in NetSock_ConnSignalReq().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnReqSignal (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemPost(p_sock->ConnReqSignalObj, KAL_OPT_PEND_NONE, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_CONN_REQ_SIGNAL);
#endif
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_ConnReqAbort()
*
* Description : Abort wait on socket connection request signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to abort wait on connection request
*               -------         signal.
*
*                           Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnReqAbort (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                /* Abort wait on socket connection request signal ...   */
                                                                /* ... for ALL waiting tasks.                           */
    KAL_SemPendAbort(p_sock->ConnReqSignalObj, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_CONN_REQ_ABORT);
#endif
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_ConnReqTimeoutDflt()
*
* Description : Set socket connection request signal configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection request signal
*               -------         configured-default timeout.
*
*                           Argument checked in NetSock_CfgTimeoutConnReqDflt(),
*                                               NetSock_Clr(),
*                                               NetSock_Init().
*
* Return(s)   : none.
*
* Note(s)     : (1) NetSock_ConnReqTimeoutDflt() is called by network protocol suite function(s)
*                   & may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnReqTimeoutDflt (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
                                                                /* Set socket connection request timeout ...            */
                                                                /* ... to configured-default     timeout value.         */
    timeout_ms = NET_SOCK_DFLT_TIMEOUT_CONN_REQ_MS;

    NetSock_ConnReqTimeoutSet(p_sock, timeout_ms);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_ConnReqTimeoutSet()
*
* Description : Set socket connection request signal timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection request
*               -------         signal timeout.
*
*                               Argument checked in NetSock_CfgTimeoutConnReqSet(),
*                                                   NetSock_ConnReqTimeoutDflt().
*
*               timeout_m   Timeout value :
*
*                               NET_TMR_TIME_INFINITE,     if infinite (i.e. NO timeout) value desired.
*
*                               In number of milliseconds, otherwise.
*
* Return(s)   : none.
*
* Note(s)     : (1) NetSock_ConnReqTimeoutSet() is called by network protocol suite function(s) &
*                   may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnReqTimeoutSet (NET_SOCK    *p_sock,
                                 CPU_INT32U   timeout_ms)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Set socket connection request timeout value ...      */
                                                                /* ... (in OS ticks).                                   */
    p_sock->ConnReqSignalTimeout_ms = timeout_ms;
    CPU_CRITICAL_EXIT();
}
#endif


/*
*********************************************************************************************************
*                                   NetSock_ConnReqTimeoutGet_ms()
*
* Description : Get socket connection request signal timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get connection request signal
*                           timeout.
*
*                           Argument checked in NetSock_CfgTimeoutConnReqGet_ms().
*
* Return(s)   : Socket connection request network timeout value  :
*
*                   NET_TMR_TIME_INFINITE,     if infinite (i.e. NO timeout) value configured.
*
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) NetSock_ConnReqTimeoutGet_ms() is called by network protocol suite function(s) &
*                   may be called either with OR without the global network lock already acquired.
*
*               (2) 'NetSock_ConnReqTimeoutTbl_tick[]' variables MUST ALWAYS be accessed exclusively
*                    in critical sections.
*********************************************************************************************************
*/

#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
CPU_INT32U  NetSock_ConnReqTimeoutGet_ms (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Get socket connection request timeout value ...      */
                                                                /* ... (in OS ticks).                                   */
    timeout_ms = p_sock->ConnReqSignalTimeout_ms;
    CPU_CRITICAL_EXIT();

    return (timeout_ms);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_ConnAcceptQ_SemClr()
*
* Description : Clear socket connection accept queue signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to clear connection accept queue signal.
*               -------      Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnAcceptQ_SemClr (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemSet(p_sock->ConnAcceptQSignalObj, 0u, &local_err);   /* Clear socket connection accept queue signal.         */
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}
#endif



/*
*********************************************************************************************************
*                                     NetSock_ConnAcceptQ_Wait()
*
* Description : Wait on socket connection accept queue signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to wait on connection accept
*               -------         queue signal.
*
*                           Argument checked in NetSock_Accept().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) (a) If timeouts NOT desired, wait on socket connection accept queue signal forever
*                       (i.e. do NOT exit).
*
*                   (b) If timeout      desired, return NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT error on socket
*                       connection accept queue timeout.  Implement timeout with OS-dependent functionality.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnAcceptQ_Wait (NET_SOCK  *p_sock,
                                RTOS_ERR  *p_err)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    timeout_ms = p_sock->ConnAcceptQSignalTimeout_ms;
    CPU_CRITICAL_EXIT();

                                                                /* Wait on socket connection accept queue signal ...    */
                                                                /* ... with configured timeout (see Note #1b).          */
    KAL_SemPend(p_sock->ConnAcceptQSignalObj, KAL_OPT_PEND_NONE, timeout_ms, p_err);
    RTOS_ASSERT_CRITICAL(((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE)    ||
                          (RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_TIMEOUT) ||
                          (RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_ABORT)  ), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_ConnAcceptQ_Signal()
*
* Description : Signal socket that connection request received; socket now available in accept queue.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal connection accept queue.
*               -------     Argument checked in NetSock_ConnSignalAccept().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnAcceptQ_Signal (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                                                                /* Signal socket accept queue.                          */
    KAL_SemPost(p_sock->ConnAcceptQSignalObj, KAL_OPT_PEND_NONE, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_CONN_ACCEPT_SIGNAL);
#endif
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_ConnAcceptQ_Abort()
*
* Description : Abort wait on socket connection accept queue signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to abort wait on connection
*               -------         accept queue signal.
*
*                           Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnAcceptQ_Abort (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                                                                /* Abort wait on socket connection accept queue signal  */
                                                                /* ... for ALL waiting tasks.                           */
    KAL_SemPendAbort(p_sock->ConnAcceptQSignalObj, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_CONN_ACCEPT_ABORT);
#endif
}
#endif


/*
*********************************************************************************************************
*                                 NetSock_ConnAcceptQ_TimeoutDflt()
*
* Description : Set socket connection accept queue signal configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection accept queue
*               -------          signal configured-default timeout.
*
*                           Argument checked in NetSock_CfgTimeoutConnAcceptDflt(),
*                                               NetSock_Clr(),
*                                               NetSock_Init().
*
* Note(s)     : (1) Despite inconsistency with NetSock_CfgTimeoutConnAccept&&&() functions,
*                   NetSock_ConnAcceptQ_TimeoutDflt() includes 'Q_' for consistency with other
*                   NetSock_ConnAcceptQ_&&&() functions.
*
*               (2) NetSock_ConnAcceptQ_TimeoutDflt() is called by network protocol suite function(s)
*                   & may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnAcceptQ_TimeoutDflt (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
                                                                /* Set socket connection accept queue timeout ...       */
                                                                /* ... to configured-default          timeout value.    */
    timeout_ms = NET_SOCK_DFLT_TIMEOUT_CONN_ACCEPT_MS;

    NetSock_ConnAcceptQ_TimeoutSet(p_sock, timeout_ms);
}
#endif


/*
*********************************************************************************************************
*                                 NetSock_ConnAcceptQ_TimeoutSet()
*
* Description : Set socket connection accept queue signal timeout value.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to set connection accept
*               -------             queue signal timeout.
*
*                               Argument checked in NetSock_CfgTimeoutConnAcceptSet(),
*                                                   NetSock_ConnAcceptQ_TimeoutDflt().
*
*               timeout_ms      Timeout value :
*
*                                   NET_TMR_TIME_INFINITE,     if infinite (i.e. NO timeout) value desired.
*
*                                   In number of milliseconds, otherwise.
*
* Return(s)   : none.
*
* Note(s)     : (1) Despite inconsistency with NetSock_CfgTimeoutConnAccept&&&() functions,
*                   NetSock_ConnAcceptQ_TimeoutSet() includes 'Q_' for consistency with other
*                   NetSock_ConnAcceptQ_&&&() functions.
*
*               (2) NetSock_ConnAcceptQ_TimeoutSet() is called by network protocol suite function(s)
*                   & may be called either with OR without the global network lock already acquired.
*
*               (3) 'NetSock_ConnAcceptQ_TimeoutTbl_tick[]' variables MUST ALWAYS be accessed
*                    exclusively in critical sections.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnAcceptQ_TimeoutSet (NET_SOCK    *p_sock,
                                      CPU_INT32U   timeout_ms)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Set socket connection accept queue timeout value ... */
                                                                /* ... (in OS ticks).                                   */
    p_sock->ConnAcceptQSignalTimeout_ms = timeout_ms;
    CPU_CRITICAL_EXIT();
}
#endif


/*
*********************************************************************************************************
*                                NetSock_ConnAcceptQ_TimeoutGet_ms()
*
* Description : Get socket connection accept queue signal timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get connection accept queue
*               -------         signal timeout.
*
*                           Argument checked in NetSock_CfgTimeoutConnAcceptGet_ms().
*
* Return(s)   : Socket connection accept network timeout value :
*
*                   NET_TMR_TIME_INFINITE,     if infinite (i.e. NO timeout) value configured.
*
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) Despite inconsistency with NetSock_CfgTimeoutConnAccept&&&() functions,
*                   NetSock_ConnAcceptQ_TimeoutGet_ms() includes 'Q_' for consistency with other
*                   NetSock_ConnAcceptQ_&&&() functions.
*
*               (2) NetSock_ConnAcceptQ_TimeoutGet_ms() is called by network protocol suite function(s)
*                   & may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
CPU_INT32U  NetSock_ConnAcceptQ_TimeoutGet_ms (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Get socket connection accept queue timeout ...       */
                                                                /* ... value (in OS ticks).                             */
    timeout_ms      = p_sock->ConnAcceptQSignalTimeout_ms;
    CPU_CRITICAL_EXIT();

    return (timeout_ms);
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_ConnCloseClr()
*
* Description : Clear socket connection close signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to clear connection close signal.
*               -------     Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnCloseClr (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    KAL_SemSet(p_sock->ConnCloseSignalObj, 0u, &local_err);     /* Clear socket connection close signal.                */
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_ConnCloseWait()
*
* Description : Wait on socket connection close signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to wait on connection close signal.
*               -------     Argument checked in NetSock_CloseHandlerStream().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) (a) If timeouts NOT desired, wait on socket connection close signal forever
*                       (i.e. do NOT exit).
*
*                   (b) If timeout      desired, return NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT error on socket
*                       connection request timeout.  Implement timeout with OS-dependent functionality.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnCloseWait (NET_SOCK  *p_sock,
                             RTOS_ERR  *p_err)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    timeout_ms = p_sock->ConnCloseSignalTimeout_ms;
    CPU_CRITICAL_EXIT();

                                                                /* Wait on socket connection close signal ...           */
                                                                /* ... with configured timeout (see Note #1b).          */
    KAL_SemPend(p_sock->ConnCloseSignalObj, KAL_OPT_PEND_NONE, timeout_ms, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
    }
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_ConnCloseSignal()
*
* Description : Signal socket that connection close complete; socket connection now closed.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal connection close complete.
*               -------     Argument checked in NetSock_ConnSignalClose().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnCloseSignal (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                                                                /* Signal socket close.                                 */
    KAL_SemPost(p_sock->ConnCloseSignalObj, KAL_OPT_PEND_NONE, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_CONN_CLOSE_SIGNAL);
#endif
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_ConnCloseAbort()
*
* Description : Abort wait on socket connection close signal.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to abort wait on connection close
*               -------         signal.
*
*                           Argument validated in NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnCloseAbort (NET_SOCK  *p_sock)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                /* Abort wait on socket connection close signal ...     */
                                                                /* ... for ALL waiting tasks.                           */
    KAL_SemPendAbort(p_sock->ConnCloseSignalObj, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_CONN_CLOSE_ABORT);
#endif
}
#endif


/*
*********************************************************************************************************
*                                   NetSock_ConnCloseTimeoutDflt()
*
* Description : Set socket connection close signal configured-default timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection close signal
*               -------         configured-default timeout.
*
*                           Argument checked in NetSock_CfgTimeoutConnCloseDflt(),
*                                               NetSock_Clr(),
*                                               NetSock_Init().
*
* Return(s)   : none.
*
* Note(s)     : (1) NetSock_ConnCloseTimeoutDflt() is called by network protocol suite function(s)
*                   & may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnCloseTimeoutDflt (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
                                                                /* Set socket connection close timeout ...              */
                                                                /* ... to configured-default   timeout value.           */
    timeout_ms = NET_SOCK_DFLT_TIMEOUT_CONN_CLOSE_MS;

    NetSock_ConnCloseTimeoutSet(p_sock, timeout_ms);
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_ConnCloseTimeoutSet()
*
* Description : Set socket connection close signal timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set connection close
*               -------             signal timeout.
*
*                               Argument checked in NetSock_CfgTimeoutConnCloseSet(),
*                                                   NetSock_ConnCloseTimeoutDflt().
*
*               timeout_ms  Timeout value (in milliseconds).
*
* Return(s)   : none.
*
* Note(s)     : (1) NetSock_ConnCloseTimeoutSet() is called by network protocol suite function(s) &
*                   may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
void  NetSock_ConnCloseTimeoutSet (NET_SOCK    *p_sock,
                                   CPU_INT32U   timeout_ms)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Set socket connection close timeout value ...        */
                                                                /* ... (in OS ticks).                                   */
    p_sock->ConnCloseSignalTimeout_ms = timeout_ms;
    CPU_CRITICAL_EXIT();
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_ConnCloseTimeoutGet_ms()
*
* Description : Get socket connection close signal timeout value.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get connection close signal
*               -------         timeout.
*
*                           Argument checked in NetSock_CfgTimeoutConnCloseGet_ms().
*
* Return(s)   : Socket connection close network timeout value :
*
*                   NET_TMR_TIME_INFINITE,     if infinite (i.e. NO timeout) value configured.
*
*                   In number of milliseconds, otherwise.
*
* Note(s)     : (1) NetSock_ConnCloseTimeoutGet_ms() is called by network protocol suite function(s)
*                   & may be called either with OR without the global network lock already acquired.
*********************************************************************************************************
*/
#if    defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
CPU_INT32U  NetSock_ConnCloseTimeoutGet_ms (NET_SOCK  *p_sock)
{
    CPU_INT32U  timeout_ms;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Get socket connection close timeout value ...        */
                                                                /* ... (in OS ticks).                                   */
    timeout_ms = p_sock->ConnCloseSignalTimeout_ms;
    CPU_CRITICAL_EXIT();

    return (timeout_ms);
}
#endif


/*
*********************************************************************************************************
*                                            NetSock_Rx()
*
* Description : (1) Process received socket data & forward to application :
*
*                   (a) Demultiplex data to connection
*                   (b) Update receive statistics
*
*
* Argument(s) : p_buf        Pointer to network buffer that received socket data.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) Since RFC #792, Section 'Destination Unreachable Message : Description' states
*                   that "if, in the destination host, the IP module cannot deliver the datagram
*                   because the indicated ... process port is not active, the destination host may
*                   send a destination unreachable message to the source host"; the network buffer
*                   MUST NOT be freed by the socket layer but must be returned to the transport or
*                   internet layer(s) to send an appropriate ICMP error message.
*
*               (3) Network buffer freed by lower layer (see Note #3); only increment error counter.
*********************************************************************************************************
*/

void  NetSock_Rx (NET_BUF   *p_buf,
                  RTOS_ERR  *p_err)
{
    NET_BUF_HDR  *p_buf_hdr;


    NET_CTR_STAT_INC(Net_StatCtrs.Sock.RxPktCtr);

    p_buf_hdr = &p_buf->Hdr;

                                                                /* --------- DEMUX SOCK PKT / UPDATE RX STATS --------- */
    NetSock_RxPktDemux(p_buf, p_buf_hdr, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit_discard;
    }

    goto exit;

exit_discard:
    NET_CTR_ERR_INC(Net_ErrCtrs.Sock.RxPktDiscardedCtr);

exit:
    return;
}


/*
*********************************************************************************************************
*                                       NetSock_CloseFromConn()
*
* Description : Close a socket via a network connection.
*
*               (1) When a network connection closes a socket, the socket :
*
*                   (a) (1) Closes NO other network connection(s),
*                       (2) MUST NOT recursively re-close other network connection(s);
*
*                   (b) SHOULD clear network connection(s)' handle identifiers.
*
*                   See also             'NetSock_CloseSockHandler()  Note #2a',
*                            'net_tcp.c   NetTCP_ConnCloseFromConn()  Note #1',
*                          & 'net_conn.c  NetConn_CloseFromApp()      Note #1b'.
*
*               (2) Closes socket but does NOT free the socket since NO mechanism or API exists to close
*                   an application's reference to the socket.
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to close.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSock_CloseFromConn (NET_SOCK_ID  sock_id)
{
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   is_used;


                                                                /* ------------------ VALIDATE SOCK ------------------- */
    if (sock_id == NET_SOCK_ID_NONE) {
        goto exit;
    }

                                                                /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        goto exit;
    }

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             goto exit;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             break;
    }

    p_sock->State = NET_SOCK_STATE_CLOSED_FAULT;


exit:
    return;
}


/*
*********************************************************************************************************
*                                     NetSock_FreeConnFromSock()
*
* Description : (1) Free/de-reference network connection from socket :
*
*                   (a) Remove connection handle identifier from socket's connection accept queue
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to free network connection.
*
*               conn_id     Handle identifier of network connection.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) When a network connection is fully connected/established, it is queued to an
*                       application connection as a cloned network connection until the connection is
*                       accepted & a new application connection is created.
*
*                   (b) Therefore, network connections need only be de-referenced from cloned socket
*                       application connections.
*********************************************************************************************************
*/

void  NetSock_FreeConnFromSock (NET_SOCK_ID  sock_id,
                                NET_CONN_ID  conn_id)
{
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   is_used;


                                                                /* ------------------ VALIDATE SOCK ------------------- */
    if (sock_id == NET_SOCK_ID_NONE) {
        goto exit;
    }
                                                                 /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        goto exit;
    }

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             goto exit;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
        default:
             break;
    }


    if (DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE_NEGO) == DEF_YES) {
        p_sock->State = NET_SOCK_STATE_CLOSED_FAULT;
        goto exit;
    }

                                                                /* --------------- FREE/DE-REF CONN ID ---------------- */
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NetSock_ConnAcceptQ_ConnID_Remove(p_sock, conn_id);
#endif


   PP_UNUSED_PARAM(p_sock);                                     /* Prevent possible 'variable unused' warnings.         */
   PP_UNUSED_PARAM(conn_id);

exit:
    return;
}


/*
*********************************************************************************************************
*                                       NetSock_ConnSignalReq()
*
* Description : Signal socket that connection request complete; socket now connected.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal connection request.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
void  NetSock_ConnSignalReq (NET_SOCK_ID   sock_id)
{
    NET_SOCK     *p_sock;


    p_sock = &NetSock_Tbl[sock_id];


                                                                /* ----------------- SIGNAL SOCK CONN ----------------- */
    NetSock_ConnReqSignal(p_sock);                              /* Signal sock conn req.                                */

    p_sock->State = NET_SOCK_STATE_CONN_DONE;                   /* Update sock state as conn done.                      */
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_ConnChildAdd()
*
* Description : Signal socket to add a child connection.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal connection accept.
*
*               conn_id     Handle identifier of network connection.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) On any faults, network connection NOT freed/closed; caller function(s) SHOULD handle
*                   fault condition(s).
*********************************************************************************************************
*/
#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
void  NetSock_ConnChildAdd (NET_SOCK_ID   sock_id,
                            NET_CONN_ID   conn_id,
                            RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;


    p_sock = &NetSock_Tbl[sock_id];

    NetSock_ConnAcceptQ_ConnID_Add(p_sock, conn_id, p_err);     /* Add conn id to sock accept Q.                        */
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_ConnSignalAccept()
*
* Description : Signal socket that connection request received; socket accept now available.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to signal connection accept.
*
*               conn_id     Handle identifier of network connection.
*
* Return(s)   : none.
*
* Note(s)     : (1) On any faults, network connection NOT freed/closed; caller function(s) SHOULD handle
*                   fault condition(s).
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
void  NetSock_ConnSignalAccept (NET_SOCK_ID   sock_id,
                                NET_CONN_ID   conn_id)
{
    NET_SOCK               *p_sock;
    NET_SOCK_ACCEPT_Q_OBJ  *p_obj;



    p_sock = &NetSock_Tbl[sock_id];

    p_obj = NetSock_ConnAcceptQ_ConnID_Srch(p_sock, conn_id);
    RTOS_ASSERT_CRITICAL((p_obj != DEF_NULL), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

    p_obj->IsRdy = DEF_YES;

    NetSock_ConnAcceptQ_Signal(p_sock);                         /* Signal         sock accept Q.                        */

    PP_UNUSED_PARAM(conn_id);
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_ConnSignalClose()
*
* Description : Signal socket that connection close complete; socket connection now closed.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to signal connection close.
*
*               data_avail      Indicate whether application data is still available for the socket connection :
*
*                                   DEF_YES                         Application data is     available for the
*                                                                       closing socket connection.
*                                   DEF_NO                          Application data is NOT available for the
*                                                                       closing socket connection.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) NetSock_ConnSignalClose() blocked until network initialization completes.
*
*               (2) Once a socket connection has been signaled of its close :
*
*                   (a) Close socket connection
*                   (b) Close socket connection's reference to network connection
*                   (c) Do NOT close transport connection(s); transport layer responsible for closing its
*                           remaining connection(s)
*
*               (3) (a) Since sockets that have already closed are NOT to be accessed (see 'NetSock_Close()
*                       Note #2'), non-blocking socket close may NOT require close completion.
*
*                   (b) #### NET-798
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
void  NetSock_ConnSignalClose (NET_SOCK_ID   sock_id,
                               CPU_BOOLEAN   data_avail,
                               RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   block;


                                                                /* ------------------ VALIDATE SOCK ------------------- */
    if (sock_id == NET_SOCK_ID_NONE) {
        goto exit;
    }

    p_sock = &NetSock_Tbl[sock_id];

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit;

        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             break;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, ;);
    }


                                                                /* ----------------- SIGNAL SOCK CONN ----------------- */
    block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    if (block == DEF_YES) {                                     /* If blocking sock conn, ...                           */
        if (data_avail  != DEF_YES) {
            p_sock->State = NET_SOCK_STATE_CLOSED;
        } else {
            p_sock->State = NET_SOCK_STATE_CLOSING_DATA_AVAIL;
        }

        NetSock_ConnCloseSignal(p_sock);                        /* ...  signal sock conn close.                         */

    } else {                                                    /* Else complete sock close (see Note #3a).             */
        if (data_avail != DEF_YES) {
            NetSock_CloseHandler(p_sock,                        /* See Note #2a.                                        */
                                 DEF_YES,                       /* See Note #2b.                                        */
                                 DEF_NO);                       /* See Note #2c.                                        */
        } else {
            p_sock->State = NET_SOCK_STATE_CLOSING_DATA_AVAIL;
        }
    }

exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                            NetSock_GetObj()
*
* Description : Get socket object.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to validate.
*
* Return(s)   : Pointer to socket object, if not error.
*               DEF_NULL, otherwise.
*
* Note(s)     : (1) NetSock_IsUsed() MUST be called with the global network lock already acquired.
*********************************************************************************************************
*/

NET_SOCK  *NetSock_GetObj (NET_SOCK_ID  sock_id)
{
    CPU_BOOLEAN   is_used;
    NET_SOCK     *p_sock = DEF_NULL;


                                                                /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        goto exit;
    }

    p_sock = &NetSock_Tbl[sock_id];

exit:
    return (p_sock);
}


/*
*********************************************************************************************************
*                                          NetSock_IsUsed()
*
* Description : Validate socket in use.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to validate.
*
* Return(s)   : DEF_YES, socket   valid &      in use.
*               DEF_NO,  socket invalid or NOT in use.
*
* Note(s)     : (1) NetSock_IsUsed() MUST be called with the global network lock already acquired.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSock_IsUsed (NET_SOCK_ID   sock_id)
{
    NET_SOCK     *p_sock = DEF_NULL;
    CPU_BOOLEAN   used   = DEF_NO;


    if (sock_id == NET_SOCK_ID_NONE) {
        return (DEF_NO);
    }

    RTOS_ASSERT_DBG((sock_id >= (NET_SOCK_ID)NET_SOCK_ID_MIN), RTOS_ERR_INVALID_ARG, DEF_NO);
    RTOS_ASSERT_DBG((sock_id <= (NET_SOCK_ID)NET_SOCK_ID_MAX), RTOS_ERR_INVALID_ARG, DEF_NO);

                                                                /* ---------------- VALIDATE SOCK USED ---------------- */
    p_sock = &NetSock_Tbl[sock_id];
    used   =  DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_USED);

    return (used);
}


/*
*********************************************************************************************************
*                                    NetSock_RxDataHandlerStream()
*
* Description : (1) Receive data from a stream-type socket :
*
*                   (a) Validate socket connection state                                See Note #11
*                   (b) Get remote host address                                         See Notes #4 & #5
*                   (c) Configure socket receive :
*                       (1) Get       socket's transport connection identification handler
*                       (2) Configure socket flags
*                   (d) Receive socket data from appropriate transport layer
*                   (e) Return  socket data received
*
*
* Argument(s) : sock_id             Socket descriptor/handle identifier of socket to receive data.
*               -------             Argument checked   in NetSock_RxDataHandler().
*
*               p_sock              Pointer to a socket.
*               ------              Argument validated in NetSock_RxDataHandler().
*
*               p_data_buf          Pointer to an application data buffer that will receive the socket's received
*               ----------             data.
*
*                                   Argument checked   in NetSock_RxDataHandler().
*
*               data_buf_len        Size of the   application data buffer (in octets).
*               ------------        Argument checked   in NetSock_RxDataHandler().
*
*               flags               Flags to select receive options; bit-field flags logically OR'd :
*               -----
*                                       NET_SOCK_FLAG_NONE              No socket flags selected.
*                                       NET_SOCK_FLAG_RX_DATA_PEEK      Receive socket data without consuming
*                                                                           the socket data; i.e. socket data
*                                                                           NOT removed from application receive
*                                                                           queue(s).
*                                       NET_SOCK_FLAG_RX_NO_BLOCK       Receive socket data without blocking
*                                                                           (see Note #3).
*
*                                   Argument checked   in NetSock_RxDataHandler().
*
*               p_addr_remote       Pointer to an address buffer that will receive the socket address structure
*               -------------           with the received data's remote address (see Notes #4 & #5), if NO error(s).
*
*                                   Argument checked     in NetSock_RxDataFrom();
*                                            set to NULL in NetSock_RxData().
*
*               p_addr_len           Pointer to a variable to ... :
*               ----------
*                                       (a) Pass the size of the address buffer pointed to by 'p_addr_remote'.
*                                       (b) (1) Return the actual size of socket address structure with the
*                                                   received data's remote address, if NO error(s);
*                                           (2) Return 0,                           otherwise.
*
*                                   Argument checked     in NetSock_RxDataFrom();
*                                            set to NULL in NetSock_RxData().
*
*                                   See Note #4b.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of positive data octets received, if NO error(s)              [see Note #7a].
*
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED,       if socket connection closed (see Note #7b).
*
*               NET_SOCK_BSD_ERR_RX,                     otherwise                   (see Note #7c1).
*
* Note(s)     : (2) (a) (1) Stream-type sockets transmit & receive all data octets in one or more non-
*                           distinct packets.  In other words, the application data is NOT bounded by
*                           any specific packet(s); rather, it is contiguous & sequenced from one packet
*                           to the next.
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'recv() : DESCRIPTION' summarizes
*                           that "for stream-based sockets, such as SOCK_STREAM, message boundaries
*                           shall be ignored.  In this case, data shall be returned to the user as
*                           soon as it becomes available, and no data shall be discarded".
*
*                   (b) (1) Thus if the socket's type is stream & the receive data buffer size is NOT
*                           large enough for the received data, the receive data buffer is maximally
*                           filled with receive data & the remaining data octets remain queued for
*                           later application-socket receives.
*
*                       (2) Therefore, a stream-type socket receive is signaled ONLY when data is
*                           received for a socket connection where data was previously unavailable.
*
*                   (c) Consequently, it is typical -- but NOT absolutely required -- that a single
*                       application task only receive or request to receive application data from a
*                       stream-type socket.
*
*                   See also 'NetSock_RxDataHandler()  Note #2b'.
*
*               (3) If 'flags' argument set to 'NET_SOCK_FLAG_RX_NO_BLOCK'; socket receive does NOT configure
*                   the transport layer receive to block, regardless if the socket is configured to block.
*
*               (4) (a) If a pointer to remote address buffer is provided, it is assumed that the remote
*                       address buffer has been previously validated for the remote address to be returned.
*
*                   (b) If a pointer to remote address buffer is provided, it is assumed that a pointer to
*                       an address length buffer is also available & has been previously validated.
*
*                   (c) The remote address is obtained from the socket connection's remote address.
*
*               (5) (a) Socket address structure 'AddrFamily' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (6) (a) Socket connection addresses are maintained in network-order.
*
*                   (b) However, since the port number & address are copied from a network-order multi-octet
*                       array directly into a network-order socket address structure, they do NOT need to be
*                       converted from host-order to network-order.
*
*               (7) IEEE Std 1003.1, 2004 Edition, Section 'recv() : RETURN VALUE' states that :
*
*                   (a) "Upon successful completion, recv() shall return the length of the message in bytes."
*
*                   (b) "If no messages are available to be received and the peer has performed an orderly
*                        shutdown, recv() shall return 0."
*
*                       (1) Since the socket receive return value of '0' is reserved for socket connection
*                           closes; NO socket receive -- fault or non-fault -- should ever return '0' octets
*                           received.
*
*                   (c) (1) "Otherwise, -1 shall be returned" ...
*                       (2) "and 'errno' set to indicate the error."
*                           'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   See also 'NetSock_RxDataHandler()  Note #7'.
*
*               (8) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (9) Default case already invalidated in NetSock_Open().  However, the default case
*                   is included as an extra precaution in case 'Protocol' is incorrectly modified.
*
*              (10) On ANY error(s), network resources MUST be appropriately freed :
*
*                   (a) NetSock_RxDataHandlerStream() assumes that stream-type sockets have been previously
*                       validated as connected by caller function(s).  Therefore, on any internal socket
*                       connection error(s), the socket MUST be closed.
*
*                   (b) Since transport layer error(s) may NOT be critical &/or may be transitory, NO network
*                       or socket resource(s) are closed/freed.
*
*                   (c) If transport layer reports closed receive queue, socket layer is free to close/free
*                       ALL network or transport connection resource(s).
*
*                       See also 'net_tcp.c  NetTCP_RxAppData()  Note #2e3A'.
*
*              (11) Socket descriptor read availability determined by other socket handler function(s).
*                   For correct inter-operability between socket descriptor read handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*              (12) 'sock_id' may NOT be necessary but is included for consistency.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
NET_SOCK_RTN_CODE  NetSock_RxDataHandlerStream (NET_SOCK_ID          sock_id,
                                                NET_SOCK            *p_sock,
                                                void                *p_data_buf,
                                                CPU_INT16U           data_buf_len,
                                                NET_SOCK_API_FLAGS   flags,
                                                NET_SOCK_ADDR       *p_addr_remote,
                                                NET_SOCK_ADDR_LEN   *p_addr_len,
                                                RTOS_ERR            *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4  *p_addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6  *p_addr_ipv6;
#endif

    CPU_BOOLEAN          no_block;
    CPU_BOOLEAN          block;
    CPU_BOOLEAN          peek;
    NET_CONN_ID          conn_id;
    NET_CONN_ID          conn_id_transport;
    NET_CONN_ADDR_LEN    addr_len;
    CPU_INT08U           addr_remote[NET_SOCK_ADDR_LEN_MAX];
    NET_FLAGS            flags_transport;
    CPU_INT16U           data_len_tot;
    NET_SOCK_RTN_CODE    rtn_code  = NET_SOCK_BSD_ERR_RX;


    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' warning (see Note #12).    */

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit;

        case NET_SOCK_STATE_CONN_DONE:
             p_sock->State = NET_SOCK_STATE_CONN;
             break;

        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             break;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_RX);
    }


                                                                /* ----------------- GET REMOTE ADDR ------------------ */
    conn_id = p_sock->ID_Conn;
    if (p_addr_remote != DEF_NULL) {                            /* If remote addr buf avail, ...                        */
                                                                /* ... rtn sock conn's remote addr (see Note #4c).      */
        addr_len = sizeof(addr_remote);
        NetConn_AddrRemoteGet(conn_id,
                             &addr_remote[0],
                             &addr_len,
                              p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {                          /* See Note #10a.                                       */
             goto exit;
        }

        switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
            case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                                                                /* Cfg remote addr struct (see Notes #5 & #6b).         */
                 p_addr_ipv4  = (NET_SOCK_ADDR_IPv4 *)p_addr_remote;
                 NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv4->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V4);
                 NET_UTIL_VAL_COPY_16(&p_addr_ipv4->Port, &addr_remote[NET_SOCK_ADDR_IP_IX_PORT]);
                 NET_UTIL_VAL_COPY_32(&p_addr_ipv4->Addr, &addr_remote[NET_SOCK_ADDR_IP_V4_IX_ADDR]);
                 Mem_Clr(&p_addr_ipv4->Unused[0],
                          NET_SOCK_ADDR_IPv4_NBR_OCTETS_UNUSED);
                                                                /* See Note  #4b.                                       */
                *p_addr_len = (NET_SOCK_ADDR_LEN )NET_SOCK_ADDR_IPv4_SIZE;
                 break;
#endif
#ifdef  NET_IPv6_MODULE_EN
            case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                                                                /* Cfg remote addr struct (see Notes #5 & #6b).         */
                 p_addr_ipv6  = (NET_SOCK_ADDR_IPv6 *)p_addr_remote;
                 NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv6->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V6);
                 NET_UTIL_VAL_COPY_16(&p_addr_ipv6->Port, &addr_remote[NET_SOCK_ADDR_IP_IX_PORT]);
                 Mem_Copy(&p_addr_ipv6->Addr, &addr_remote[NET_SOCK_ADDR_IP_V6_IX_ADDR], NET_IPv6_ADDR_SIZE);
                                                                /* See Note  #4b.                                       */
                *p_addr_len = (NET_SOCK_ADDR_LEN )NET_SOCK_ADDR_IPv6_SIZE;
                 break;
#endif

            default:
                                                                /* See Notes #8 & #10a.                                 */
                NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_RX);
        }
    }

                                                                /* ------------------- CFG SOCK RX -------------------- */
                                                                /* Get transport conn id.                               */
    conn_id_transport = NetConn_ID_TransportGet(conn_id);

    if (conn_id_transport == NET_CONN_ID_NONE) {                /* See Note #10a.                                       */
        RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
        goto exit;
    }

                                                                /* Cfg sock rx   flags.                                 */
    no_block = DEF_BIT_IS_SET((NET_SOCK_FLAGS)flags, NET_SOCK_FLAG_RX_NO_BLOCK);
    if (no_block == DEF_YES) {                                  /* If 'No Block' flag set,         ...                  */
        block = DEF_NO;                                         /* ... do NOT block (see Note #3); ...                  */
    } else {                                                    /* ... else chk sock's no-block flag.                   */
        block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    }

                                                                /* Chk sock peek flags.                                 */
    peek = DEF_BIT_IS_SET((NET_SOCK_FLAGS)flags, NET_SOCK_FLAG_RX_DATA_PEEK);


                                                                /* ------------------- RX SOCK DATA ------------------- */
    data_len_tot = 0u;

    switch (p_sock->Protocol) {                                  /* Rx app data from transport layer rx.                 */
        case NET_SOCK_PROTOCOL_TCP:
                                                                /* Cfg transport rx flags.                              */
             flags_transport = NET_TCP_FLAG_NONE;
             if (block == DEF_YES) {
                 DEF_BIT_SET(flags_transport, NET_TCP_FLAG_RX_BLOCK);
             }
             if (peek == DEF_YES) {
                 DEF_BIT_SET(flags_transport, NET_TCP_FLAG_RX_DATA_PEEK);
             }

             data_len_tot = NetTCP_RxAppData(conn_id_transport,
                                             p_data_buf,
                                             data_buf_len,
                                             flags_transport,
                                             p_err);
             switch (RTOS_ERR_CODE_GET(*p_err)) {
                 case RTOS_ERR_NONE:
                      break;

                 case RTOS_ERR_TIMEOUT:
                 case RTOS_ERR_WOULD_BLOCK:
                      goto exit;

                 case RTOS_ERR_NET_CONN_CLOSE_RX:
                      if (p_sock->State == NET_SOCK_STATE_CLOSING_DATA_AVAIL) {
                          NetSock_CloseHandler(p_sock, DEF_YES, DEF_YES); /* close/free sock's conn(s) [see Note #10c]  */
                      }
                      rtn_code = NET_SOCK_BSD_RTN_CODE_CONN_CLOSED;
                      goto exit;

                 default:
                      goto exit;
             }
             break;

        case NET_SOCK_PROTOCOL_NONE:
        default:                                                /* See Notes #9 & #10a.                                 */
             PP_UNUSED_PARAM(conn_id_transport);                /* Prevent possible 'variable unused' warnings.         */
             PP_UNUSED_PARAM(flags_transport);
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_RX);
    }

    rtn_code = (NET_SOCK_RTN_CODE)data_len_tot;

exit:
    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_TxDataHandlerStream()
*
* Description : (1) Transmit data through a stream-type socket :
*
*                   (a) Validate  socket connection state                                   See Note #8
*                   (b) Configure socket transmit :
*                       (1) Get       socket's transport 0connection identification handler
*                       (2) Configure socket flags
*                   (c) Transmit  socket data via appropriate transport layer
*                   (d) Return    socket data transmitted length
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to receive data.
*               -------         Argument checked   in NetSock_TxDataHandler().
*
*               p_sock          Pointer to a socket.
*               ------          Argument validated in NetSock_TxDataHandler().
*
*               p_data          Pointer to application data.
*               ------          Argument checked   in NetSock_TxDataHandler().
*
*               data_len        Length of  application data (in octets) [see Note #3].
*               --------        Argument checked   in NetSock_TxDataHandler().
*
*               flags           Flags to select transmit options; bit-field flags logically OR'd :
*               -----
*                                   NET_SOCK_FLAG_NONE              No socket flags selected.
*                                   NET_SOCK_FLAG_TX_NO_BLOCK       Transmit socket data without blocking
*                                                                       (see Note #4).
*
*                               Argument checked   in NetSock_TxDataHandler().
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of positive data octets transmitted, if NO error(s)              [see Note #5a1].
*
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED,          if socket connection closed (see Note #5b).
*
*               NET_SOCK_BSD_ERR_TX,                        otherwise                   (see Note #5a2A).
*
* Note(s)     : (2) Stream-type sockets may transmit from the following states :
*
*                   (a) Unconnected states :
*
*                           (A) Unconnected state(s) transmit NOT yet implemented;
*                               see also 'net_tcp.c  NetTCP_TxConnAppData()  Note #2bA' #### NET-800
*
*                       (1) CLOSED
*                           (A) Bind to local port
*                           (B) Connect to remote host
*                           (C) (1) Transmit data with connection request(s)?
*                                     OR
*                               (2) Queue transmit data until connected to remote host?
*
*                       (2) BOUND
*                           (A) Connect to remote host
*                           (B) (1) Transmit data with connection request(s)?
*                                     OR
*                               (2) Queue transmit data until connected to remote host?
*
*                       (3) LISTEN
*                           (A) Connect to remote host
*                           (B) (1) Transmit data with connection request(s)?
*                                     OR
*                               (2) Queue transmit data until connected to remote host?
*
*                       (4) CONNECTION-IN-PROGRESS
*                           (A) Queue transmit data until connected to remote host?
*
*                   (b) Connected states :
*
*                       (1) CONNECTED
*
*               (3) (a) (1) (A) Stream-type sockets transmit & receive all data octets in one or more
*                               non-distinct packets.  In other words, the application data is NOT
*                               bounded by any specific packet(s); rather, it is contiguous & sequenced
*                               from one packet to the next.
*
*                           (B) Thus if the socket's type is stream & the socket's transmit data queue(s)
*                               are NOT large enough for the transmitted data, the  transmit data queue(s)
*                               are maximally filled with transmit data & the remaining data octets are
*                               discarded but may be re-transmitted by later application-socket transmits.
*
*                           (C) Therefore, NO stream-type socket transmit data length should be "too long
*                               to pass through the underlying protocol" & cause the socket transmit to
*                               "fail ... [with] no data ... transmitted" (IEEE Std 1003.1, 2004 Edition,
*                               Section 'send() : DESCRIPTION').
*
*                       (2) Thus it is typical -- but NOT absolutely required -- that a single application
*                           task ONLY transmit or request to transmit data to a stream-type socket.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*               (4) If 'flags' argument set to 'NET_SOCK_FLAG_TX_NO_BLOCK'; socket transmit does NOT configure
*                   the transport layer transmit to block, regardless if the socket is configured to block.
*
*               (5) (a) IEEE Std 1003.1, 2004 Edition, Section 'send() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, send() shall return the number of bytes sent."
*
*                           (A) Section 'send() : DESCRIPTION' elaborates that "successful completion
*                               of a call to sendto() does not guarantee delivery of the message".
*
*                           (B) (1) Thus applications SHOULD verify the actual returned number of data
*                                   octets transmitted &/or prepared for transmission.
*
*                               (2) In addition, applications MAY desire verification of receipt &/or
*                                   acknowledgement of transmitted data to the remote host -- either
*                                   inherently by the transport layer or explicitly by the application.
*
*                       (2) (A) "Otherwise, -1 shall be returned" ...
*                               (1) Section 'send() : DESCRIPTION' elaborates that "a return value of
*                                   -1 indicates only locally-detected errors".
*
*                           (B) "and 'errno' set to indicate the error."
*                               'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   (b) Although NO socket send() specification states to return '0' when the socket's
*                       connection is closed, it seems reasonable to return '0' since it is possible for the
*                       socket connection to be close()'d or shutdown() by the remote host.
*
*                       (1) Since the socket transmit return value of '0' is reserved for socket connection
*                           closes; NO socket transmit -- fault or non-fault -- should ever return '0' octets
*                           transmitted.
*
*               (6) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'Protocol' is incorrectly modified.
*
*               (7) On ANY error(s), network resources MUST be appropriately freed :
*
*                   (a) NetSock_TxDataHandlerStream() assumes that internal socket configuration
*                       has been previously validated by caller function(s).  Therefore, on any
*                       internal socket configuration error(s), the socket MUST be closed.
*
*                   (b) NetSock_TxDataHandlerStream() assumes that any internal socket connection
*                       error(s) on stream-type sockets may NOT be critical &/or may be transitory;
*                       thus NO network or socket resource(s) are closed/freed.
*
*                   (c) Since transport layer error(s) may NOT be critical &/or may be transitory, NO
*                       network or socket resource(s) are closed/freed.
*
*               (8) Socket descriptor write availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor write handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*               (9) 'sock_id' may NOT be necessary but is included for consistency.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
NET_SOCK_RTN_CODE  NetSock_TxDataHandlerStream (NET_SOCK_ID          sock_id,
                                                NET_SOCK            *p_sock,
                                                void                *p_data,
                                                CPU_INT16U           data_len,
                                                NET_SOCK_API_FLAGS   flags,
                                                RTOS_ERR            *p_err)
{
    CPU_BOOLEAN        no_block;
    CPU_BOOLEAN        block;
    NET_CONN_ID        conn_id;
    NET_CONN_ID        conn_id_transport;
    NET_FLAGS          flags_transport;
    CPU_INT16U         data_len_tot;
    NET_SOCK_RTN_CODE  rtn_code     = NET_SOCK_BSD_ERR_TX;


    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' warning (see Note #9).     */

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;

        case NET_SOCK_STATE_CLOSED:                             /* Bind to local port & ...                             */
        case NET_SOCK_STATE_BOUND:                              /* ...  tx conn req to remote host; Q tx data?          */
        case NET_SOCK_STATE_LISTEN:                             /* Q/tx data with  conn req?                            */
        case NET_SOCK_STATE_CONN_IN_PROGRESS:                   /* Q tx data until conn complete?                       */
                                                                /* NOT yet implemented (see Note #2aA).                 */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit;

        case NET_SOCK_STATE_CONN_DONE:
             p_sock->State = NET_SOCK_STATE_CONN;
             break;

        case NET_SOCK_STATE_CONN:
             break;

        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                  /* If sock already closing, ...                         */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             rtn_code = NET_SOCK_BSD_RTN_CODE_CONN_CLOSED;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);          /* ... rtn closed code (see Note #5b).                  */
             goto exit;

        case NET_SOCK_STATE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_TX);
    }

                                                                /* ------------------- CFG SOCK TX -------------------- */
                                                                /* Get transport conn id.                               */
    conn_id           = p_sock->ID_Conn;
    conn_id_transport = NetConn_ID_TransportGet(conn_id);

    if (conn_id_transport == NET_CONN_ID_NONE) {                /* See Note #7a.                                        */
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
         goto exit;
    }

                                                                /* Cfg sock tx flags.                                   */
    no_block = DEF_BIT_IS_SET((NET_SOCK_FLAGS)flags, NET_SOCK_FLAG_TX_NO_BLOCK);
    if (no_block == DEF_YES) {                                  /* If 'No Block' flag set,         ...                  */
        block = DEF_NO;                                         /* ... do NOT block (see Note #4); ...                  */
    } else {                                                    /* ... else chk sock's no-block flag.                   */
        block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    }


                                                                /* ------------------- TX SOCK DATA ------------------- */
    data_len_tot = 0u;

    switch (p_sock->Protocol) {                                 /* Tx app data via transport layer tx.                  */
        case NET_SOCK_PROTOCOL_TCP:
                                                                /* Cfg transport tx flags.                              */
             flags_transport = NET_TCP_FLAG_NONE;
             if (block == DEF_YES) {
                 DEF_BIT_SET(flags_transport, NET_TCP_FLAG_TX_BLOCK);
             }

             data_len_tot = NetTCP_TxConnAppData(conn_id_transport,
                                                 p_data,
                                                 data_len,
                                                 flags_transport,
                                                 p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  goto exit;
             }
             break;

        case NET_SOCK_PROTOCOL_NONE:
        default:                                                /* See Notes #6 & #7a.                                  */
             PP_UNUSED_PARAM(conn_id_transport);                /* Prevent possible 'variable unused' warnings.         */
             PP_UNUSED_PARAM(flags_transport);
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_TX);
    }

    rtn_code = (NET_SOCK_RTN_CODE)data_len_tot;

exit:
    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      NetSock_ListenQ_IsAvail()
*
* Description : Check if socket's listen queue is available to queue a new connection.
*
*               (1) Socket connection accept queue synonymous with socket listen queue.
*
*
* Argument(s) : conn_id_app     Connection handle identifier of socket to check listen queue.
*
* Return(s)   : DEF_YES, if socket listen queue is available to queue a new connection.
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) (a) Stevens, TCP/IP Illustrated, Volume 1, 8th Printing, Section 18.11, Pages 257-258
*                       states that :
*
*                       (1) "Each listening end point has a fixed length queue of connections that have been
*                            accepted [i.e. connected] ... but not yet accepted by the application."
*
*                       (2) "The application specifies a limit to this queue, commonly called the backlog" :
*
*                           (A) "This backlog must be between 0 and 5, inclusive."
*                           (B) "(Most applications specify the maximum value of 5.)"
*
*                   (b) Wright/Stevens, TCP/IP Illustrated, Volume 2, 3rd Printing, Section 15.9, Page 455
*                       reiterates that :
*
*                       (2) A "listen ... socket ... specifies a limit on the number of connections that can
*                           be queued on the socket," ...
*
*                       (5) "after which the socket layer refuses to queue additional connection requests."
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN  NetSock_ListenQ_IsAvail (NET_CONN_ID   conn_id_app)
{
    NET_SOCK     *p_sock  = DEF_NULL;
    CPU_BOOLEAN   q_avail = DEF_NO;


    p_sock = &NetSock_Tbl[conn_id_app];

                                                                /* ------------- CHK SOCK LISTEN Q AVAIL -------------- */
    q_avail = NetSock_ConnAcceptQ_IsAvail(p_sock);              /* Chk if listen Q avail for new conns (see Note #2).   */

    return (q_avail);
}
#endif


/*
*********************************************************************************************************
*                                          NetSock_AppPostRx()
*
* Description : Signal application layer that something happen on a connection.
*
* Argument(s) : conn_id_app     Connection handle identifier of socket to signal.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_AppPostRx (NET_CONN_ID  conn_id_app)
{
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NET_SOCK  *p_sock;

    if (conn_id_app == NET_CONN_ID_NONE) {
        return;
    }

    p_sock = &NetSock_Tbl[conn_id_app];
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_RX);
#endif
}
#endif


/*
*********************************************************************************************************
*                                          NetSock_AppPostTx()
*
* Description : Signal application layer that something happen on a connection.
*
* Argument(s) : conn_id_app     Connection handle identifier of socket to signal.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_AppPostTx (NET_CONN_ID  conn_id_app)
{
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    NET_SOCK    *p_sock;

    if (conn_id_app == NET_CONN_ID_NONE) {
        return;
    }

    p_sock = &NetSock_Tbl[conn_id_app];
    NetSock_SelPost(p_sock, NET_SOCK_EVENT_TYPE_TX);
#endif
}
#endif


/*
*********************************************************************************************************
*                                           NetSock_SelPost()
*
* Description : Signal select pending object that something happened on the connection.
*
* Argument(s) : p_sock  Pointer to socket.
*
*               event   socket even type.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  void  NetSock_SelPost (NET_SOCK             *p_sock,
                               NET_SOCK_EVENT_TYPE   event)
{
    NET_SOCK_SEL_EVENT_FLAG   flags_mask = NET_SOCK_SEL_EVENT_FLAG_NONE;
    NET_SOCK_SEL_OBJ         *p_sel_obj  = p_sock->SelObjTailPtr;


    if (p_sel_obj == DEF_NULL) {
        goto exit;
    }


    switch (event) {
        case NET_SOCK_EVENT_TYPE_CONN_REQ_SIGNAL:
        case NET_SOCK_EVENT_TYPE_CONN_ACCEPT_SIGNAL:
             flags_mask = NET_SOCK_SEL_EVENT_FLAG_RD
                        | NET_SOCK_SEL_EVENT_FLAG_WR;
             break;

        case NET_SOCK_EVENT_TYPE_CONN_ACCEPT_ABORT:
        case NET_SOCK_EVENT_TYPE_CONN_CLOSE_SIGNAL:
        case NET_SOCK_EVENT_TYPE_CONN_REQ_ABORT:
        case NET_SOCK_EVENT_TYPE_CONN_CLOSE_ABORT:
        case NET_SOCK_EVENT_TYPE_SEL_ABORT:
             flags_mask = NET_SOCK_SEL_EVENT_FLAG_RD
                        | NET_SOCK_SEL_EVENT_FLAG_WR
                        | NET_SOCK_SEL_EVENT_FLAG_ERR;
              break;

        case NET_SOCK_EVENT_TYPE_RX_ABORT:
             flags_mask = NET_SOCK_SEL_EVENT_FLAG_RD
                        | NET_SOCK_SEL_EVENT_FLAG_ERR;
             break;

        case NET_SOCK_EVENT_TYPE_RX:
             flags_mask = NET_SOCK_SEL_EVENT_FLAG_RD;
             break;

        case NET_SOCK_EVENT_TYPE_TX:
             flags_mask = NET_SOCK_SEL_EVENT_FLAG_WR;
             break;

        default:
             break;
    }


    while (p_sel_obj != DEF_NULL) {
        if ((p_sel_obj->SockSelPendingFlags & flags_mask) > 0) {
            RTOS_ERR  local_err;

            RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

            KAL_SemPost(p_sel_obj->SockSelTaskSignalObj, KAL_OPT_POST_NONE, &local_err);
            RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
        }

        p_sel_obj = p_sel_obj->ObjPrevPtr;
    }

exit:
    return;
}
#endif


/*
*********************************************************************************************************
*                                           NetSock_InitObj()
*
* Description : Initialize Socket OS objects.
*
* Argument(s) : p_sock      Pointer to socket descriptor/handle.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  NetSock_InitObj (NET_SOCK  *p_sock,
                               MEM_SEG   *p_mem_seg,
                               RTOS_ERR  *p_err)
{
    PP_UNUSED_PARAM(p_mem_seg);

    p_sock->RxQ_SignalObj = KAL_SemCreate(NET_SOCK_RX_Q_NAME,
                                          DEF_NULL,
                                          p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
        goto exit;
    }

                                                                /* Initialize socket receive queue timeout values.      */
    NetSock_RxQ_TimeoutDflt(p_sock);


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    p_sock->ConnReqSignalObj = KAL_SemCreate(NET_SOCK_CONN_REQ_NAME,
                                             DEF_NULL,
                                             p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
        goto exit_free_q_signal;
    }

                                                                /* Initialize socket connection signal timeout values.  */
    NetSock_ConnReqTimeoutDflt(p_sock);

    p_sock->ConnAcceptQSignalObj = KAL_SemCreate(NET_SOCK_CONN_ACCEPT_NAME,
                                                 DEF_NULL,
                                                 p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
        goto exit_free_conn_req_signal;
    }

    NetSock_ConnAcceptQ_TimeoutDflt(p_sock);

    p_sock->ConnCloseSignalObj = KAL_SemCreate(NET_SOCK_CONN_CLOSE_NAME,
                                               DEF_NULL,
                                               p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
        goto exit_free_conn_accept_signal;
    }

    NetSock_ConnCloseTimeoutDflt(p_sock);
#endif

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
    p_sock->SelObjTailPtr = DEF_NULL;                           /* Init Select Task Signal to NULL.                     */
#endif


    goto exit;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
exit_free_conn_accept_signal:
    KAL_SemDel(p_sock->ConnAcceptQSignalObj);

exit_free_conn_req_signal:
    KAL_SemDel(p_sock->ConnReqSignalObj);

exit_free_q_signal:
    KAL_SemDel(p_sock->RxQ_SignalObj);
#endif

exit:
    return;
}


/*
*********************************************************************************************************
*                                        NetSock_RxPktDemux()
*
* Description : (1) Demultiplex received packet to appropriate socket :
*
*                   (a) Determine appropriate receive socket :
*
*                       (1) Packet's socket demultiplexed in previous transport layer.
*
*                       (2) Search connection list for socket identifier whose local &/or remote addresses
*                           are identical to the received packet's destination & source addresses.
*
*                   (b) Validate socket connection
*                   (c) Demultiplex received packet to appropriate socket
*
*
* Argument(s) : p_buf        Pointer to network buffer that received socket data.
*               -----        Argument checked   in NetSock_Rx().
*
*               p_buf_hdr    Pointer to network buffer header.
*               ---------    Argument validated in NetSock_Rx().
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) Assumes packet buffer's internet protocol controls configured in previous layer(s).
*
*                   (b) Assumes packet's addresses & ports demultiplexed & decoded  in previous layer(s).
*
*               (3) (a) Since datagram-type sockets transmit & receive all data atomically, each datagram
*                       socket receive MUST always receive exactly one datagram.  Therefore, the socket
*                       receive queue  MUST be signaled for each datagram packet received.
*
*                   (b) Stream-type sockets transmit & receive all data octets in one or more non-distinct
*                       packets.  In other words, the application data is NOT bounded by any specific
*                       packet(s); rather, it is contiguous & sequenced from one packet to the next.
*
*                       Therefore, the socket receive queue is signaled ONLY when data is received for a
*                       connection where data was previously unavailable.
*
*               (4) Default case already invalidated in earlier internet protocol layer function(s).
*                   However, the default case is included as an extra precaution in case 'Protocol'
*                   is incorrectly modified.
*
*               (5) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (6) Default case already invalidated in NetSock_Open().  However, the default case
*                   is included as an extra precaution in case 'SockType' is incorrectly modified.
*
*               (7) Socket connection addresses are maintained in network-order.
*
*               (8) Some buffer controls were previously initialized in NetBuf_Get() when the packet
*                   was received at the network interface layer.  These buffer controls do NOT need
*                   to be re-initialized but are shown for completeness.
*********************************************************************************************************
*/

static  void  NetSock_RxPktDemux (NET_BUF      *p_buf,
                                  NET_BUF_HDR  *p_buf_hdr,
                                  RTOS_ERR     *p_err)
{
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    CPU_BOOLEAN            q_prevly_empty;
#endif
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_HDR          *pip_hdrv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_IPv6_HDR          *pip_hdrv6;
#endif
    CPU_INT08U             addr_local[NET_SOCK_ADDR_LEN_MAX];
    CPU_INT08U             addr_remote[NET_SOCK_ADDR_LEN_MAX];
    NET_CONN_PROTOCOL_IX   protocol_ix;
    NET_CONN_FAMILY        family;
    NET_CONN_ID            conn_id;
    NET_SOCK_ID            sock_id;
    NET_SOCK              *p_sock;
    NET_BUF               *p_buf_tail;
    NET_BUF_HDR           *p_buf_hdr_tail;
    NET_SOCK_DATA_SIZE     rx_q_size_max_rem;
    NET_CONN_STATE         conn_state;


    sock_id = (NET_SOCK_ID)p_buf_hdr->Conn_ID_App;
    conn_id = (NET_CONN_ID)p_buf_hdr->Conn_ID;

                                                                /* --------------- CHK PREV SOCK DEMUX ---------------- */
    if (sock_id == NET_CONN_ID_NONE) {                          /* If sock id demux'd by prev layer (see Note #1a1), ...*/

                                                                /* ------ SRCH CONN LIST(S) FOR PKT/SOCK ADDR(S) ------ */
        if (DEF_BIT_IS_CLR(p_buf_hdr->Flags, NET_BUF_FLAG_IPv6_FRAME)) {

#ifdef  NET_IPv4_MODULE_EN
            family    =  NET_CONN_FAMILY_IP_V4_SOCK;
            pip_hdrv4 = (NET_IPv4_HDR *)&p_buf->DataPtr[p_buf_hdr->IP_HdrIx];
            switch (pip_hdrv4->Protocol) {
                case NET_IP_HDR_PROTOCOL_UDP:
                     protocol_ix = NET_CONN_PROTOCOL_IX_IP_V4_UDP;
                     break;

#ifdef  NET_TCP_MODULE_EN
                case NET_IP_HDR_PROTOCOL_TCP:
                     protocol_ix = NET_CONN_PROTOCOL_IX_IP_V4_TCP;
                     break;
#endif

                default:                                            /* See Note #4.                                     */
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
                     goto exit;
            }

            Mem_Clr(&addr_local,  NET_SOCK_ADDR_LEN_MAX);
            Mem_Clr(&addr_remote, NET_SOCK_ADDR_LEN_MAX);
                                                                    /* Cfg srch local  addr as pkt dest addr.           */
            NET_UTIL_VAL_COPY_SET_NET_16(&addr_local [NET_SOCK_ADDR_IP_IX_PORT], &p_buf_hdr->TransportPortDest);
            NET_UTIL_VAL_COPY_SET_NET_32(&addr_local [NET_SOCK_ADDR_IP_V4_IX_ADDR], &p_buf_hdr->IP_AddrDest);
                                                                    /* Cfg srch remote addr as pkt src  addr.           */
            NET_UTIL_VAL_COPY_SET_NET_16(&addr_remote[NET_SOCK_ADDR_IP_IX_PORT], &p_buf_hdr->TransportPortSrc);
            NET_UTIL_VAL_COPY_SET_NET_32(&addr_remote[NET_SOCK_ADDR_IP_V4_IX_ADDR], &p_buf_hdr->IP_AddrSrc);

#else                                                               /* See Note #5.                                     */
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
            goto exit;
#endif

        } else {

#ifdef  NET_IPv6_MODULE_EN
            family    =  NET_CONN_FAMILY_IP_V6_SOCK;
            pip_hdrv6 = (NET_IPv6_HDR *)&p_buf->DataPtr[p_buf_hdr->IP_HdrIx];
            switch (pip_hdrv6->NextHdr) {
                case NET_IP_HDR_PROTOCOL_UDP:
                     protocol_ix = NET_CONN_PROTOCOL_IX_IP_V6_UDP;
                     break;

#ifdef  NET_TCP_MODULE_EN
                case NET_IP_HDR_PROTOCOL_TCP:
                     protocol_ix = NET_CONN_PROTOCOL_IX_IP_V6_TCP;
                     break;
#endif

                default:                                            /* See Note #4.                                     */
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
                     goto exit;
            }

            Mem_Clr(&addr_local,  NET_SOCK_ADDR_LEN_MAX);
            Mem_Clr(&addr_remote, NET_SOCK_ADDR_LEN_MAX);
                                                                    /* Cfg srch local  addr as pkt dest addr.           */
            NET_UTIL_VAL_COPY_SET_NET_16(&addr_local [NET_SOCK_ADDR_IP_IX_PORT], &p_buf_hdr->TransportPortDest);
            Mem_Copy(&addr_local [NET_SOCK_ADDR_IP_V6_IX_ADDR],
                     &p_buf_hdr->IPv6_AddrDest,
                      NET_IPv6_ADDR_SIZE);
                                                                    /* Cfg srch remote addr as pkt src  addr.           */
            NET_UTIL_VAL_COPY_SET_NET_16(&addr_remote[NET_SOCK_ADDR_IP_IX_PORT], &p_buf_hdr->TransportPortSrc);
            Mem_Copy(&addr_remote [NET_SOCK_ADDR_IP_V6_IX_ADDR],
                     &p_buf_hdr->IPv6_AddrSrc,
                      NET_IPv6_ADDR_SIZE);
#else                                                               /* See Note #5.                                     */
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_RX);
            goto exit;
#endif
        }

        conn_id = NetConn_Srch(family,                          /* Srch for conn'd OR non-conn'd sock.                  */
                               protocol_ix,
                              &addr_local[0],
                              &addr_remote[0],
                               NET_SOCK_ADDR_LEN_MAX,
                               DEF_NULL,
                              &sock_id,
                              &conn_state);

        if (conn_id != NET_CONN_ID_NONE) {
            switch (conn_state) {
                case NET_CONN_STATE_FULL:                       /* Fully-conn'd sock found.                             */
                case NET_CONN_STATE_FULL_WILDCARD:
                case NET_CONN_STATE_HALF:                       /* Non-  conn'd sock found.                             */
                case NET_CONN_STATE_HALF_WILDCARD:
                     break;

                default:
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.RxDestCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_RX);             /* ... rtn dest err.                                    */
                     goto exit;
            }

        } else {
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.RxDestCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_RX);                      /* ... rtn dest err.                                    */
            goto exit;
        }
    }

    PP_UNUSED_PARAM(conn_id);                                   /* Prevent possible 'variable unused' warning.          */


    p_sock = &NetSock_Tbl[sock_id];


                                                                /* ---------------- DEMUX PKT TO SOCK ----------------- */
    rx_q_size_max_rem = NET_SOCK_DATA_SIZE_MAX - p_sock->RxQ_SizeCur;
                                                                /* If sock rx Q full, rtn err.                          */
    if ((p_sock->RxQ_SizeCur >= p_sock->RxQ_SizeCfgd) ||
        (rx_q_size_max_rem   <  p_buf_hdr->DataLen)) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_OVF);
        goto exit;
    }

    p_sock->RxQ_SizeCur += p_buf_hdr->DataLen;                  /* Else inc cur rx Q size.                              */


    p_buf_tail = p_sock->RxQ_Tail;
    if (p_buf_tail != DEF_NULL) {                               /* If sock rx Q NOT empty, insert pkt after tail.       */
        p_buf_hdr_tail                  = &p_buf_tail->Hdr;
        p_buf_hdr_tail->NextPrimListPtr =  p_buf;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        q_prevly_empty                 =  DEF_NO;
#endif

    } else {                                                    /* Else add first pkt to sock rx Q.                     */
        p_sock->RxQ_Head = p_buf;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        q_prevly_empty   = DEF_YES;
#endif
    }

    p_sock->RxQ_Tail           = p_buf;                         /* Insert pkt @ Q tail.                                 */
    p_buf_hdr->PrevPrimListPtr = p_buf_tail;



    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             NetSock_RxQ_Signal(p_sock);                        /* Signal rx Q for each datagram pkt (see Note #3a).    */
             break;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             if (q_prevly_empty == DEF_YES) {                   /* If stream rx Q prev'ly empty, ...                    */
                 NetSock_RxQ_Signal(p_sock);                    /* .. signal rx Q now non-empty (see Note #3b).         */
             }
             break;
#endif

        default:                                                /* See Note #6.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit;
    }

    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {           /* If sock rx Q signal failed, unlink      pkt from Q.  */
        p_buf_tail = p_buf_hdr->PrevPrimListPtr;
        if (p_buf_tail != DEF_NULL) {                           /* If sock rx Q NOT yet empty, unlink last pkt from Q.  */
            p_buf_hdr_tail                  = &p_buf_tail->Hdr;
            p_buf_hdr_tail->NextPrimListPtr =  DEF_NULL;
        } else {                                                /* Else unlink last pkt from Q.                         */
            p_sock->RxQ_Head = DEF_NULL;
        }
        p_sock->RxQ_Tail           = p_buf_tail;                /* Set new sock rx Q tail.                              */
        p_buf_hdr->PrevPrimListPtr = DEF_NULL;
                                                                /* Dec cur sock rx Q size.                              */
        p_sock->RxQ_SizeCur       -= p_buf_hdr->DataLen;
        goto exit;                                              /* Rtn err from NetSock_RxQ_Signal().                   */
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                     NetSock_IsValidAddrLocal()
*
* Description : (1) Validate a socket address as a local address :
*
*                   (a) Validate socket address family type
*                   (b) Validate socket address
*                   (c) Validate socket port number
*
*
* Argument(s) : p_addr       Pointer to socket address structure             (see Notes #2a1B, #2a2, & #3).
*
*               addr_len    Length  of socket address structure (in octets) [see Note  #2a1C].
*
*               p_if_nbr     Pointer to variable that will receive the network interface number with this
*               --------         configured local address, if available.
*
*                           Argument validated in NetSock_BindHandler().
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if a valid local socket address.
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) (a) (1) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the bind()
*                           function takes the following arguments" :
*
*                           (A) 'socket' - "Specifies the file descriptor of the socket to be bound."
*
*                           (B) 'address' - "Points to a 'sockaddr' structure containing the address to be bound
*                                to the socket.  The length and format of the address depend on the address family
*                                of the socket."
*
*                           (C) 'address_len' - "Specifies the length of the 'sockaddr' structure pointed to by
*                                the address argument."
*
*                       (2) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                           Section 4.4, Page 102 states that "if ... bind() is called" with :
*
*                           (A) "A port number of 0, the kernel chooses an ephemeral port."
*
*                           (B) "A wildcard ... address, the kernel does not choose the local ... address."
*
*                               (1) "With IPv4, the wildcard address is specified by the constant INADDR_ANY,
*                                    whose value is normally 0."
*
*                   (b) (1) IEEE Std 1003.1, 2004 Edition, Section 'bind() : ERRORS' states that "the bind()
*                           function shall fail if" :
*
*                           (B) "[EAFNOSUPPORT]  - The specified address is not a valid address for the address
*                                family of the specified socket."
*
*                           (C) "[EADDRNOTAVAIL] - The specified address is not available from the local machine."
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'bind() : ERRORS' states that "the bind()
*                           function may fail if" :
*
*                           (A) "[EINVAL] - The 'address_len' argument is not a valid length for the address
*                                family."
*
*                   See also 'NetSock_BindHandler()  Note #2'.
*
*               (3) (a) Socket address structure 'AddrFamily' member MUST be configured in host-order & MUST
*                       NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order to
*                       network-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (4) Socket connection addresses are maintained in network-order.
*
*               (5) Pointers to variables that return values MUST be initialized PRIOR to all other validation
*                   or function handling in case of any error(s).
*
*               (6) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be compiled/linked
*                   since 'net_sock.h' ensures that the family type configuration constant (NET_SOCK_CFG_FAMILY)
*                   is configured with an appropriate family type value (see 'net_sock.h  CONFIGURATION ERRORS').
*                   The 'else'-conditional code is included for completeness & as an extra precaution in case
*                   'net_sock.h' is incorrectly modified.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetSock_IsValidAddrLocal (NET_SOCK_PROTOCOL_FAMILY   protocol,
                                               NET_SOCK_ADDR             *p_addr,
                                               NET_SOCK_ADDR_LEN          addr_len,
                                               NET_IF_NBR                *p_if_nbr,
                                               RTOS_ERR                  *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4    *p_addr_ipv4;
    NET_IPv4_ADDR          addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6    *p_addr_ipv6;
    NET_IPv6_ADDR          addr_ipv6;
#endif
    NET_SOCK_ADDR_FAMILY   addr_family;
    NET_PORT_NBR           port_nbr;
    CPU_BOOLEAN            rtn_val         = DEF_NO;
    NET_IF_NBR             if_nbr;


    RTOS_ASSERT_DBG_ERR_SET((p_addr != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NO);

   *p_if_nbr = NET_IF_NBR_NONE;                                 /* Init IF nbr for err (see Note #5).                   */


                                                                /* ------------------ VALIDATE ADDR ------------------- */
    switch (protocol) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                                                                /* ---------------- VALIDATE ADDR LEN ----------------- */
                                                                /* Validate addr len (see Notes #2a1C & #2b2A).         */
             if (addr_len < (NET_SOCK_ADDR_LEN)NET_SOCK_ADDR_IPv4_SIZE) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrLenCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }

             p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr;
                                                                /* Validate addr family (see Note #3a).                 */


             NET_UTIL_VAL_COPY_GET_HOST_16(&addr_family, &p_addr_ipv4->AddrFamily);
             if (addr_family != NET_SOCK_ADDR_FAMILY_IP_V4) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }
                                                                /* Validate addr     (see Note #3b).                    */
             NET_UTIL_VAL_COPY_GET_NET_32(&addr_ipv4, &p_addr_ipv4->Addr);
             if (addr_ipv4 == NET_SOCK_ADDR_IP_V4_WILDCARD) {   /* If req'd addr = wildcard addr (see Note #2a2B), ...  */
                 if_nbr   = NET_IF_NBR_WILDCARD;                /* ... cfg wildcard IF nbr;                        ...  */

             } else {                                           /* ... else if req'd addr                          ...  */
                 if_nbr = NetIPv4_GetAddrHostIF_Nbr(addr_ipv4);
                 if (if_nbr == NET_IF_NBR_NONE) {               /* ... NOT any of this host's addrs,               ...  */
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);      /* ... rtn err (see Note #2b1C).                      */
                     goto exit;
                 }
             }
                                                                /* Validate port nbr (see Note #3b).                    */
             NET_UTIL_VAL_COPY_GET_NET_16(&port_nbr, &p_addr_ipv4->Port);
             break;
#endif
#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                                                                /* ---------------- VALIDATE ADDR LEN ----------------- */
                                                                /* Validate addr len (see Notes #2a1C & #2b2A).         */
             if (addr_len < (NET_SOCK_ADDR_LEN)NET_SOCK_ADDR_IPv6_SIZE) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrLenCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }

             p_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)p_addr;
                                                                /* Validate addr family (see Note #3a).                 */
             NET_UTIL_VAL_COPY_GET_HOST_16(&addr_family, &p_addr_ipv6->AddrFamily);
             if (addr_family != NET_SOCK_ADDR_FAMILY_IP_V6) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }
                                                                /* Validate addr     (see Note #3b).                    */
             Mem_Copy(&addr_ipv6, &p_addr_ipv6->Addr, NET_IPv6_ADDR_SIZE);

             if (NetIPv6_IsAddrWildcard(&addr_ipv6)) {
                                                                /* If req'd addr = wildcard addr (see Note #2a2B), ...  */
                 if_nbr   = NET_IF_NBR_WILDCARD;                /* ... cfg wildcard IF nbr;                        ...  */

             } else {                                           /* ... else if req'd addr                          ...  */
                 if_nbr = NetIPv6_GetAddrHostIF_Nbr(&addr_ipv6);
                 if (if_nbr == NET_IF_NBR_NONE) {               /* ... NOT any of this host's addrs,               ...  */
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);      /* ... rtn err (see Note #2b1C).                      */
                     goto exit;
                 }
             }
                                                                /* Validate port nbr (see Note #3b).                    */
             NET_UTIL_VAL_COPY_GET_NET_16(&port_nbr, &p_addr_ipv6->Port);
             break;
#endif

        default:                                                /* See Note #6.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             PP_UNUSED_PARAM(addr_family);                      /* Prevent 'variable unused' compiler warnings.         */
             PP_UNUSED_PARAM(port_nbr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NO);
    }

   *p_if_nbr = if_nbr;

    rtn_val = DEF_YES;

exit:
    return (rtn_val);
}


/*
*********************************************************************************************************
*                                     NetSock_IsValidAddrRemote()
*
* Description : (1) Validate a socket address as an appropriate remote address :
*
*                   (a) Validate remote socket address :
*
*                       (1) Validate the following socket address fields :
*
*                           (A) Validate socket address family type
*                           (B) Validate socket port number
*
*                       (2) Validation ignores the following socket address fields :
*
*                           (A) Address field(s)                    Addresses will be validated by other
*                                                                       network layers
*
*                   (b) Validate remote socket address to socket's connection address
*
*
* Argument(s) : p_addr      Pointer to socket address structure (see Note #2).
*
*               addr_len    Length  of socket address structure (in octets).
*
*               p_sock      Pointer to socket.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if a valid remote socket address.
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) (a) Socket address structure 'AddrFamily' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (3) (a) Socket connection addresses are maintained in network-order.
*
*                   (b) However, since the port number & address are copied from a network-order socket
*                       address structure directly into a network-order multi-octet array, they do NOT
*                       need to be converted from host-order to network-order.
*
*               (4) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (5) (a) For datagram-type sockets, the remote address is NOT required to be static --
*                       even if the socket is in a connected state.  In other words, any datagram-type
*                       socket may receive or transmit from or to different remote addresses on each
*                       or any separate socket operation.
*
*                   (b) (1) For stream-type sockets, the remote address MUST be static.  In other words,
*                           a stream-type socket MUST be connected to & use the same remote address for
*                           ALL socket operations.
*
*                       (2) However, if the socket is NOT yet connected; then any valid remote address
*                           may be validated for the socket connection.
*
*               (6) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'SockType' is incorrectly modified.
*********************************************************************************************************
*/

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_IsValidAddrRemote (NET_SOCK_ADDR      *p_addr,
                                                NET_SOCK_ADDR_LEN   addr_len,
                                                NET_SOCK           *p_sock,
                                                RTOS_ERR           *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4    *p_addr_ipv4   = DEF_NULL;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6    *p_addr_ipv6   = DEF_NULL;
#endif
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_CONN_ID            conn_id       = NET_CONN_ID_NONE;
    NET_CONN_ADDR_LEN      conn_addr_len = 0u;
    CPU_INT08U             addr[NET_SOCK_ADDR_LEN_MAX];
    CPU_BOOLEAN            cmp           = DEF_FAIL;
    RTOS_ERR               local_err;
#endif
    NET_SOCK_ADDR_FAMILY   addr_family   = NET_SOCK_ADDR_FAMILY_IP_V4;
    NET_PORT_NBR           port_nbr      = NET_PORT_NBR_NONE;
    CPU_BOOLEAN            rtn_val       = DEF_NO;


    RTOS_ASSERT_DBG_ERR_SET((p_addr   != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NO);
    RTOS_ASSERT_DBG_ERR_SET((addr_len <= NET_SOCK_ADDR_SIZE), *p_err, RTOS_ERR_INVALID_ARG, DEF_NO);


                                                                /* ------------------ VALIDATE ADDR ------------------- */
    switch (p_addr->AddrFamily) {
        case NET_SOCK_ADDR_FAMILY_IP_V4:
#ifdef  NET_IPv4_MODULE_EN
             p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr;
                                                                /* Validate addr family (see Note #2a).                 */
             NET_UTIL_VAL_COPY_GET_HOST_16(&addr_family, &p_addr_ipv4->AddrFamily);
             if (addr_family != NET_SOCK_ADDR_FAMILY_IP_V4) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }
                                                                /* Validate port nbr    (see Note #2b).                 */
             NET_UTIL_VAL_COPY_GET_NET_16(&port_nbr, &p_addr_ipv4->Port);
             if (port_nbr == NET_SOCK_PORT_NBR_RESERVED) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidPortNbrCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }
#else
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, DEF_NO);
#endif
             break;


        case NET_SOCK_FAMILY_IP_V6:
#ifdef  NET_IPv6_MODULE_EN
             p_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)p_addr;
                                                                /* Validate addr family (see Note #2a).                 */
             NET_UTIL_VAL_COPY_GET_HOST_16(&addr_family, &p_addr_ipv6->AddrFamily);
             if (addr_family != NET_SOCK_ADDR_FAMILY_IP_V6) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }
                                                                /* Validate port nbr    (see Note #2b).                 */
             NET_UTIL_VAL_COPY_GET_NET_16(&port_nbr, &p_addr_ipv6->Port);
             if (port_nbr == NET_SOCK_PORT_NBR_RESERVED) {
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidPortNbrCtr);
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, DEF_NO);
             }
#else
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, DEF_NO);
#endif
             break;


        default:                                                /* See Note #4.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NO);
    }

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    if (p_sock != DEF_NULL) {                                   /* If sock avail, chk conn status/addr.                 */
        switch (p_sock->SockType) {
            case NET_SOCK_TYPE_DATAGRAM:
                 switch (p_sock->State) {
                     case NET_SOCK_STATE_FREE:
                          NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
                          RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                          goto exit;

                     case NET_SOCK_STATE_CLOSED:
                     case NET_SOCK_STATE_BOUND:
                     case NET_SOCK_STATE_CONN:
                          break;                                /* Remote addr validation NOT req'd (see Note #5a).     */

                     case NET_SOCK_STATE_NONE:
                     case NET_SOCK_STATE_CLOSED_FAULT:
                     case NET_SOCK_STATE_LISTEN:
                     case NET_SOCK_STATE_CONN_IN_PROGRESS:
                     case NET_SOCK_STATE_CONN_DONE:
                     case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
                     case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
                     default:
                          NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
                          RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, DEF_NO);
                 }
                 break;


            case NET_SOCK_TYPE_STREAM:
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
                 switch (p_sock->State) {
                     case NET_SOCK_STATE_FREE:
                          NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
                          RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                          goto exit;

                     case NET_SOCK_STATE_CLOSED_FAULT:
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
                          goto exit;

                     case NET_SOCK_STATE_CLOSED:
                     case NET_SOCK_STATE_BOUND:
                     case NET_SOCK_STATE_LISTEN:
                          break;                                /* Remote addr validation NOT req'd (see Note #5b2).    */

                     case NET_SOCK_STATE_CONN:                  /* Validate sock's conn remote addr (see Note #5b1).    */
                     case NET_SOCK_STATE_CONN_IN_PROGRESS:
                     case NET_SOCK_STATE_CONN_DONE:
                     case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
                     case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
                          conn_id = p_sock->ID_Conn;
                          Mem_Clr(&addr, NET_CONN_ADDR_LEN_MAX);
                          switch (p_addr->AddrFamily) {
#ifdef  NET_IPv4_MODULE_EN
                              case NET_SOCK_ADDR_FAMILY_IP_V4:
                                                                /* Cfg cmp addr in net-order (see Note #3).             */
                                   NET_UTIL_VAL_COPY_16(&addr[NET_SOCK_ADDR_IP_IX_PORT], &p_addr_ipv4->Port);
                                   NET_UTIL_VAL_COPY_32(&addr[NET_SOCK_ADDR_IP_V4_IX_ADDR], &p_addr_ipv4->Addr);
                                   conn_addr_len = NET_SOCK_ADDR_IP_V4_LEN_PORT_ADDR;
                                   break;
#endif

#ifdef  NET_IPv6_MODULE_EN
                              case NET_SOCK_ADDR_FAMILY_IP_V6:
                                                                /* Cfg cmp addr in net-order (see Note #3).             */
                                   NET_UTIL_VAL_COPY_16(&addr[NET_SOCK_ADDR_IP_IX_PORT], &p_addr_ipv6->Port);
                                   Mem_Copy(&addr[NET_SOCK_ADDR_IP_V6_IX_ADDR], &p_addr_ipv6->Addr, NET_IPv6_ADDR_SIZE);
                                   conn_addr_len = NET_SOCK_ADDR_IP_V6_LEN_PORT_ADDR;
                                   break;
#endif

                              default:                          /* See Note #4.                                         */
                                   NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                                   RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NO);
                          }

                          RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                          cmp = NetConn_AddrRemoteCmp(conn_id,
                                                     &addr[0],
                                                      conn_addr_len,
                                                     &local_err);
                          if (cmp != DEF_YES) {                 /* If sock's remote addr does NOT cmp, rtn err.         */
                              NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrCtr);
                              RTOS_ERR_SET(*p_err, RTOS_ERR_FAIL);
                              goto exit;
                          }
                          break;


                     case NET_SOCK_STATE_NONE:
                     default:
                          NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
                          RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, DEF_NO);
                 }
#else
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, DEF_NO);
#endif
                 break;


            case NET_SOCK_TYPE_NONE:
            default:                                            /* See Note #6.                                         */
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
                 RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_TYPE, DEF_NO);
        }
    }

    rtn_val = DEF_YES;

exit:
    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_CloseHandlerStream()
*
* Description : (1) Close a stream-type socket :
*
*                   (a) Validate socket connection state
*                   (b) Request transport layer connection close
*                   (c) Wait on transport layer connection close
*                   (d) Close socket
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to close.
*               -------     Argument checked   in NetSock_Close().
*
*               p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Close().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE,  if NO error(s) [see Note #3].
*               NET_SOCK_BSD_ERR_CLOSE, otherwise.
*
* Note(s)     : (2) Network resources MUST be appropriately closed :
*
*                   (a) For the following socket connection close conditions, close ALL socket connections :
*
*                       (1) Non-connected socket states
*                       (2) On any socket          fault(s)
*                       (3) On any transport layer fault(s)
*
*                   (b) For connection-closing socket states :
*
*                       (1) Close socket connection
*                       (2) Do NOT close socket    connection's reference to network connection
*                       (3) Do NOT close transport connection(s); transport layer responsible for
*                            closing its remaining connection(s)
*
*                   (c) (1) For the following socket connection close conditions ... :
*
*                           (A) For non-blocking, connected socket states
*                           (B) For connection-closed       socket states
*
*                       (2) ... perform the following close actions :
*
*                           (A) Close socket connection
*                           (B) Close socket connection's reference to network connection
*                           (C) Do NOT close transport connection(s); transport layer responsible for
*                                closing its remaining connection(s)
*
*               (3) NO BSD socket error is returned for any internal error while closing the socket.
*
*               (4) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be compiled/linked
*                   since 'net_sock.h' ensures that the family type configuration constant (NET_SOCK_CFG_FAMILY)
*                   is configured with an appropriate family type value (see 'net_sock.h  CONFIGURATION ERRORS').
*                   The 'else'-conditional code is included for completeness & as an extra precaution in case
*                   'net_sock.h' is incorrectly modified.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_SOCK_RTN_CODE  NetSock_CloseHandlerStream (NET_SOCK_ID   sock_id,
                                                       NET_SOCK     *p_sock,
                                                       RTOS_ERR     *p_err)
{
    NET_CONN_ID        conn_id;
    NET_CONN_ID        conn_id_transport;
    CPU_BOOLEAN        block;
    NET_SOCK_RTN_CODE  rtn_code   = NET_SOCK_BSD_ERR_CLOSE;
    NET_SOCK_FLAGS     local_flags;

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_BOUND:
             NetSock_CloseHandler(p_sock, DEF_YES, DEF_YES);    /* See Note #2a1.                                       */
             rtn_code = NET_SOCK_BSD_ERR_NONE;
             goto exit;

        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             p_sock->State = NET_SOCK_STATE_CLOSE_IN_PROGRESS;
             break;

        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                  /* See Note #2b.                                        */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:                 /* Net conn(s) prev'ly closed? (See Note #2b)           */
             NetSock_CloseHandler(p_sock,                       /* See Note #2b1.                                       */
                                  DEF_NO,                       /* See Note #2b2.                                       */
                                  DEF_NO);                      /* See Note #2b3.                                       */
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_OP_IN_PROGRESS); /* Rtn net sock err ...                                 */
                                                                /* ...  but rtn NO BSD err (see Note #3).               */
             rtn_code = NET_SOCK_BSD_ERR_NONE;
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             NetSock_CloseHandler(p_sock, DEF_YES, DEF_NO);
             rtn_code = NET_SOCK_BSD_ERR_NONE;
             goto exit;

        case NET_SOCK_STATE_FREE:
        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_NONE:
        default:                                                /* Socket State already validate in NetSock_Close().    */
             NetSock_CloseSockFromClose(p_sock);
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_CLOSE);
    }


                                                                /* ------------- REQ TRANSPORT CONN CLOSE ------------- */
    conn_id           = p_sock->ID_Conn;

    conn_id_transport = NetConn_ID_TransportGet(conn_id);

    if (conn_id_transport == NET_CONN_ID_NONE) {                /* See Note #2a2.                                       */
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
         goto exit_fault;
    }

    local_flags = p_sock->Flags;
    NetTCP_TxConnReqClose((NET_TCP_CONN_ID)conn_id_transport,
                                           NET_CONN_CLOSE_FULL,
                                           p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {           /* See Note #2a3.                                       */
        RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
        goto exit_fault;                                        /* Rtn net sock err ...                                 */
                                                                /* ... but rtn NO BSD err (see Note #3).                */
    }


                                                                /* ----------- WAIT ON TRANSPORT CONN CLOSE ----------- */
    block = DEF_BIT_IS_CLR(local_flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    if (block != DEF_YES) {                                     /* If non-blocking sock conn, ...                       */
        NetSock_CloseHandler(p_sock,                            /* ... close sock (see Note #2c1A).                     */
                             DEF_YES,                           /* See Note #2c2B.                                      */
                             DEF_NO);                           /* See Note #2c2C.                                      */
        rtn_code = NET_SOCK_BSD_ERR_NONE;
        goto exit;
    }

    Net_GlobalLockRelease();
    NetSock_ConnCloseWait(p_sock, p_err);
    Net_GlobalLockAcquire((void *)NetSock_CloseHandlerStream);

    switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_TIMEOUT:
        default:
             goto exit_fault;                                   /* See Note #2a2.                                       */
                                                                /* Rtn err from NetSock_ConnCloseWait() ...             */
                                                                /* ... but rtn NO BSD err (see Note #3).                */
    }


                                                                /* -------------------- CLOSE SOCK -------------------- */
    NetSock_CloseHandler(p_sock,                                /* See Note #2c1B.                                      */
                         DEF_YES,                               /* See Note #2c2B.                                      */
                         DEF_NO);                               /* See Note #2c2C.                                      */

    PP_UNUSED_PARAM(sock_id);

    rtn_code = NET_SOCK_BSD_ERR_NONE;

    goto exit;

 exit_fault:
    NetSock_CloseSockFromClose(p_sock);
    rtn_code = NET_SOCK_BSD_ERR_NONE;

 exit:
    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                        NetSock_BindHandler()
*
* Description : (1) Bind a local address to a socket :
*
*                   (a) Handle    socket type connection
*                   (b) Validate  socket local address
*                   (c) Configure socket local address
*                   (d) Search for other socket(s) with same local address              See Note #8b
*                   (e) Add local address into socket connection
*                       (1) Get & configure socket connection, if necessary
*                       (2) Set socket connection local address
*                       (3) Add socket connection into connection list
*                   (f) Update socket connection state
*
*
* Argument(s) : sock_id             Socket descriptor/handle identifier of socket to bind to a local address.
*               -------             Argument checked in NetSock_Bind(),
*                                                       NetSock_ConnHandlerDatagram(),
*                                                       NetSock_ConnHandlerStream(),
*                                                       NetSock_TxDataHandlerDatagram().
*
*               p_addr_local        Pointer to socket address structure             (see Notes #2b1B, #2b2, & #3).
*
*               addr_len            Length  of socket address structure (in octets) [see Note  #2b1C].
*
*               addr_random_reqd    Indicate whether a random address is requested  (see Note  #5) :
*
*                                       DEF_NO                      Random address NOT requested.
*                                       DEF_YES                     Random address is  requested.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE, if NO error(s) [see Note #2c1].
*               NET_SOCK_BSD_ERR_BIND, otherwise      (see Note #2c2A).
*
* Note(s)     : (2) (a) (1) IEEE Std 1003.1, 2004 Edition, Section 'bind() : DESCRIPTION' states that "the bind()
*                           function shall assign a local socket address ... to a socket".
*
*                       (2) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                           Section 4.4, Page 102 states that "bind() lets us specify the ... address, the port,
*                           both, or neither".
*
*                   (b) (1) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the bind()
*                           function takes the following arguments" :
*
*                           (A) 'socket' - "Specifies the file descriptor of the socket to be bound."
*
*                           (B) 'address' - "Points to a 'sockaddr' structure containing the address to be bound
*                                to the socket.  The length and format of the address depend on the address family
*                                of the socket."
*
*                           (C) 'address_len' - "Specifies the length of the 'sockaddr' structure pointed to by
*                                the address argument."
*
*                       (2) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                           Section 4.4, Page 102 states that "if ... bind() is called" with :
*
*                           (A) "A port number of 0, the kernel chooses an ephemeral port."
*
*                               (1) "bind() does not return the chosen value ... [of] an ephemeral port ... Call
*                                    getsockname() to return the protocol address ... to obtain the value of the
*                                    ephemeral port assigned by the kernel."
*
*                           (B) "A wildcard ... address, the kernel does not choose the local ... address until
*                                either the socket is connected (TCP) or a datagram is sent on the socket (UDP)."
*
*                               (1) "With IPv4, the wildcard address is specified by the constant INADDR_ANY,
*                                    whose value is normally 0."
*
*                   (c) IEEE Std 1003.1, 2004 Edition, Section 'bind() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, bind() shall return 0;" ...
*
*                       (2) (A) "Otherwise, -1 shall be returned," ...
*                           (B) "and 'errno' shall be set to indicate the error."
*                               'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   (d) (1) IEEE Std 1003.1, 2004 Edition, Section 'bind() : ERRORS' states that "the bind()
*                           function shall fail if" :
*
*                           (A) "[EBADF] - The 'socket' argument is not a valid file descriptor."
*
*                           (B) "[EAFNOSUPPORT]  - The specified address is not a valid address for the address
*                                family of the specified socket."
*
*                           (C) "[EADDRNOTAVAIL] - The specified address is not available from the local machine."
*
*                           (D) "[EADDRINUSE]    - The specified address is already in use."
*
*                               See also Note #8a.
*
*                           (E) "[EINVAL]" -
*
*                               (1) (a) "The socket is already bound to an address,"                  ...
*                                   (b) "and the protocol does not support binding to a new address;" ...
*
*                               (2) "or the socket has been shut down."
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'bind() : ERRORS' states that "the bind()
*                           function may fail if" :
*
*                           (A) "[EINVAL]  - The 'address_len' argument is not a valid length for the address
*                                family."
*
*                           (B) "[EISCONN] - The socket is already connected."
*
*                           (C) "[ENOBUFS] - Insufficient resources were available to complete the call."
*
*               (3) (a) Socket connection addresses MUST be maintained in network-order.
*
*                   (b) However, since the port number & address are copied from a network-order socket address
*                       structure into local variables & then into a network-order multi-octet array, they do NOT
*                       need to be converted from host-order to network-order.
*
*               (4) (a) For datagram-type sockets, the local & remote addresses are NOT required to be static --
*                       even if the socket is in a "connected" state.  In other words, any datagram-type socket
*                       may bind to different local addresses on each or any separate socket operation.
*
*                   (b) For stream-type sockets, the local & remote addresses MUST be static.  In other words,
*                       a stream-type socket may bind only once to a single local address.
*
*               (5) If a random local address is requested, configure the local address with ...
*
*                   (a) A random port number obtained from the random port number queue; ...
*                   (b) This host's primary/default protocol address.
*
*               (6) (a) Default case already invalidated in NetSock_Open().  However, the default case is included
*                       as an extra precaution in case 'SockType' is incorrectly modified.
*
*                   (b) Default case already invalidated in NetSock_Open().  However, the default case is included
*                       as an extra precaution in case 'Protocol' is incorrectly modified.
*
*               (7) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be compiled/linked
*                   since 'net_sock.h' ensures that the family type configuration constant (NET_SOCK_CFG_FAMILY)
*                   is configured with an appropriate family type value (see 'net_sock.h  CONFIGURATION ERRORS').
*                   The 'else'-conditional code is included for completeness & as an extra precaution in case
*                   'net_sock.h' is incorrectly modified.
*
*               (8) (a) (1) Multiple socket connections with the same local & remote address -- both
*                           addresses & port numbers -- is NOT currently supported.
*
*                       (2) Therefore, when updating a socket connection, it is necessary to search the
*                           connection lists for any other connection with the same local & remote address.
*
*                       (3) Since datagram-type sockets' remote address is NOT required to be static,
*                           datagram-type sockets in a "connected" state MUST search the connection lists
*                           for a connection with the same local & remote address.
*
*                   (b) (1) Also, multiple socket connections with only a local address but the same local
*                           address -- both address & port number -- is NOT currently supported.
*
*                       (2) Therefore, when adding or updating a socket connection with only a local address,
*                           it is necessary to search the connection lists for any other connection with the
*                           same local address.
*
*                       (3) Thus the option for sockets to reuse the same local address is NOT currently
*                           supported even if the socket reuse option (SO_REUSEADDR) is requested.
*********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_BindHandler (NET_SOCK_ID         sock_id,
                                                NET_SOCK_ADDR      *p_addr_local,
                                                NET_SOCK_ADDR_LEN   addr_len,
                                                CPU_BOOLEAN         addr_random_reqd,
                                                NET_SOCK_ADDR      *p_addr_dest,
                                                RTOS_ERR           *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
          NET_SOCK_ADDR_IPv4    *p_addr_ipv4;
          NET_IPv4_ADDR          addr_ip_hostv4;
          NET_IPv4_ADDR          addr_ip_netv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
          NET_SOCK_ADDR_IPv6    *p_addr_ipv6;
          NET_IPv6_ADDR          addr_ip_netv6;
#endif
          NET_IP_ADDRS_QTY       addr_ip_tbl_qty;
          NET_PROTOCOL_TYPE      protocol;
          CPU_BOOLEAN            valid;
          CPU_BOOLEAN            conn_avail;
          CPU_BOOLEAN            addr_over_wr;
          CPU_BOOLEAN            port_random_reqd;
          NET_PORT_NBR           port_nbr_host;
          NET_PORT_NBR           port_nbr_net;
          CPU_INT08U             addr_local[NET_SOCK_ADDR_LEN_MAX];
          CPU_INT08U             addr_remote[NET_SOCK_ADDR_LEN_MAX];
          NET_CONN_ADDR_LEN      addr_len_chk_size;
          NET_CONN_ADDR_LEN      addr_remote_len;
          CPU_INT08U            *p_addr_remote;
          NET_SOCK              *p_sock;
          NET_SOCK_STATE         sock_state;
          NET_CONN_FAMILY        conn_family;
          NET_CONN_PROTOCOL_IX   conn_protocol_ix;
          NET_CONN_ID            conn_id;
          NET_CONN_ID            conn_id_srch;
          NET_CONN_STATE         conn_state;
          NET_IF_NBR             if_nbr;
          CPU_BOOLEAN            addr_avail;
          NET_SOCK_RTN_CODE      rtn_code    = NET_SOCK_BSD_ERR_BIND;


                                                                /* ----------------- HANDLE SOCK TYPE ----------------- */
    p_sock = &NetSock_Tbl[sock_id];
    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:                            /* See Note #4a.                                        */
             switch (p_sock->State) {
                 case NET_SOCK_STATE_FREE:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit;

                 case NET_SOCK_STATE_CLOSED_FAULT:
                      RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
                      goto exit;

                 case NET_SOCK_STATE_CLOSED:
                      conn_avail    = DEF_NO;
                      addr_over_wr  = DEF_NO;
                      p_addr_remote = DEF_NULL;
                      sock_state    = NET_SOCK_STATE_BOUND;
                      break;

                 case NET_SOCK_STATE_BOUND:
                      conn_avail    = DEF_YES;
                      addr_over_wr  = DEF_YES;
                      p_addr_remote = DEF_NULL;
                      sock_state    = NET_SOCK_STATE_BOUND;
                      break;

                 case NET_SOCK_STATE_CONN:
                      conn_avail      =  DEF_YES;
                      addr_over_wr    =  DEF_YES;
                                                                /* Get sock's remote addr.                              */
                      conn_id         =  p_sock->ID_Conn;
                      addr_remote_len =  sizeof(addr_remote);
                      NetConn_AddrRemoteGet(conn_id,
                                           &addr_remote[0],
                                           &addr_remote_len,
                                            p_err);
                      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                           RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                           goto exit;
                      }
                      p_addr_remote = &addr_remote[0];

                      sock_state    =  NET_SOCK_STATE_CONN;
                      break;

                 case NET_SOCK_STATE_NONE:
                 case NET_SOCK_STATE_LISTEN:
                 case NET_SOCK_STATE_CONN_IN_PROGRESS:
                 case NET_SOCK_STATE_CONN_DONE:
                 case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
                 case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_BIND);
             }
             break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:                              /* See Note #4b.                                        */
             switch (p_sock->State) {
                 case NET_SOCK_STATE_FREE:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit;

                 case NET_SOCK_STATE_CLOSED_FAULT:
                      RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
                      goto exit;

                 case NET_SOCK_STATE_CLOSED:
                      conn_avail    =  DEF_NO;
                      addr_over_wr  =  DEF_NO;
                      p_addr_remote =  DEF_NULL;
                      sock_state    =  NET_SOCK_STATE_BOUND;
                      break;

                 case NET_SOCK_STATE_BOUND:                     /* See Note #2d1E1.                                     */
                 case NET_SOCK_STATE_LISTEN:
                 case NET_SOCK_STATE_CONN:                      /* See Note #2d2B.                                      */
                 case NET_SOCK_STATE_CONN_IN_PROGRESS:
                 case NET_SOCK_STATE_CONN_DONE:
                 case NET_SOCK_STATE_CLOSE_IN_PROGRESS:         /* See Note #2d1E2.                                     */
                 case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
                      goto exit;

                 case NET_SOCK_STATE_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_BIND);
             }
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:                                                /* See Note #6a.                                        */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit;
    }


                                                                /* --------------- VALIDATE LOCAL ADDR ---------------- */
    if (addr_random_reqd != DEF_YES) {                          /* If random addr NOT req'd, ...                        */
                                                                /* ... validate local addr (see Note #2b1B).            */
        valid = NetSock_IsValidAddrLocal(p_sock->ProtocolFamily,
                                         p_addr_local,
                                         addr_len,
                                        &if_nbr,
                                         p_err);
        if (valid != DEF_YES) {
            goto exit;
        }

        if (if_nbr == NET_IF_NBR_WILDCARD) {                    /* If the IF is not defined (wildcard addr case), ...   */
            if_nbr  = p_sock->IF_Nbr;                           /* ...set the IF to the one specified by the socket.    */
        }
    }


                                                                /* ----------------- CFG LOCAL ADDR ------------------- */
    switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             if (addr_random_reqd != DEF_YES) {                 /* If random addr NOT req'd, ...                        */
                 p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr_local;/* ... cfg req'd local addr (see Note #3).            */
                 NET_UTIL_VAL_COPY_16(&port_nbr_net,  &p_addr_ipv4->Port);
                 NET_UTIL_VAL_COPY_32(&addr_ip_netv4, &p_addr_ipv4->Addr);
                                                                /* Chk random port nbr req'd (see Note #2b2A).          */
                 port_nbr_host    =  NET_UTIL_NET_TO_HOST_16(port_nbr_net);
                 port_random_reqd = (port_nbr_host == NET_SOCK_PORT_NBR_RANDOM) ? DEF_YES : DEF_NO;

             } else {                                           /* Else cfg random port/this host's addr (see Note #5). */
                 port_random_reqd =  DEF_YES;

                 if_nbr = p_sock->IF_Nbr;                       /* Set the IF to the one specified by the socket.       */
                 if (if_nbr == NET_IF_NBR_NONE) {               /* IF not IF is defined in the socket structure, ...    */
                     if_nbr  = NetIF_GetDflt();                 /* ... get the default IF.                              */
                 }

                 addr_ip_tbl_qty  =  1;
                 addr_avail = NetIPv4_GetAddrHostHandler(if_nbr,
                                                        &addr_ip_hostv4,
                                                        &addr_ip_tbl_qty,
                                                         p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                     goto exit;
                 }
                 if (addr_avail == DEF_NO) {
                     RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);
                     goto exit;
                 }

                 addr_ip_netv4  = NET_UTIL_HOST_TO_NET_32(addr_ip_hostv4);
             }

             switch (p_sock->SockType) {
                 case NET_SOCK_TYPE_STREAM:
                      protocol = NET_PROTOCOL_TYPE_TCP_V4;
                      break;

                 case NET_SOCK_TYPE_DATAGRAM:
                      protocol = NET_PROTOCOL_TYPE_UDP_V4;
                      break;

                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_TYPE, NET_SOCK_BSD_ERR_BIND);
             }
             break;
#endif
#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                                                                /* See Note #7.                                         */
             if (addr_random_reqd != DEF_YES) {                 /* If random addr NOT req'd, ...                        */
                 p_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)p_addr_local;/* ... cfg req'd local addr (see Note #3).            */
                 NET_UTIL_VAL_COPY_16(&port_nbr_net,  &p_addr_ipv6->Port);

                 Mem_Copy(&addr_ip_netv6, &p_addr_ipv6->Addr, NET_IPv6_ADDR_SIZE);
                                                                /* Chk random port nbr req'd (see Note #2b2A).          */
                 port_nbr_host    =  NET_UTIL_NET_TO_HOST_16(port_nbr_net);
                 port_random_reqd = (port_nbr_host == NET_SOCK_PORT_NBR_RANDOM) ? DEF_YES : DEF_NO;

             } else {                                           /* Else cfg random port/this host's addr (see Note #5). */
                 port_random_reqd =  DEF_YES;

                 if_nbr = p_sock->IF_Nbr;                       /* Set the IF to the one specified by the socket.       */
                 if (if_nbr == NET_IF_NBR_NONE) {               /* IF not IF is defined in the socket structure, ...    */
                     if_nbr  = NetIF_GetDflt();                 /* ... get the default IF.                              */
                 }

                 addr_ip_tbl_qty = 1;
                 addr_avail = NetIPv6_GetAddrHostHandler(if_nbr,
                                                        &addr_ip_netv6,
                                                        &addr_ip_tbl_qty,
                                                         p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                     goto exit;
                 }
                 if (addr_avail == DEF_NO) {
                     RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);
                     goto exit;
                 }
             }

             switch (p_sock->SockType) {
                 case NET_SOCK_TYPE_STREAM:
                      protocol = NET_PROTOCOL_TYPE_TCP_V4;
                      break;

                 case NET_SOCK_TYPE_DATAGRAM:
                      protocol = NET_PROTOCOL_TYPE_UDP_V4;
                      break;

                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_TYPE, NET_SOCK_BSD_ERR_BIND);
             }
             break;
#endif

        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;
    }

    if (port_random_reqd == DEF_YES) {                          /* If random port req'd, ...                            */

        port_nbr_host = NetSock_RandomPortNbrGet(protocol);     /* ... get random port nbr (see Note #5a).              */

        port_nbr_net  = NET_UTIL_HOST_TO_NET_16(port_nbr_host);
    }


                                                                /* ------- SRCH FOR LOCAL ADDR IN CONN LIST(S) -------- */
    switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                                                                /* Cfg local addr as net-order sock addr (see Note #3). */
             Mem_Clr(&addr_local, NET_SOCK_ADDR_LEN_MAX);
             NET_UTIL_VAL_COPY_16(&addr_local[NET_SOCK_ADDR_IP_IX_PORT], &port_nbr_net);
             NET_UTIL_VAL_COPY_32(&addr_local[NET_SOCK_ADDR_IP_V4_IX_ADDR], &addr_ip_netv4);

                                                                /* Cfg conn srch.                                       */
             conn_family       = NET_CONN_FAMILY_IP_V4_SOCK;
             addr_len_chk_size = NET_SOCK_ADDR_LEN_IP_V4;
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_UDP:
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V4_UDP;
                      break;

#ifdef  NET_TCP_MODULE_EN
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V4_TCP;
                      break;
#endif

                 case NET_SOCK_PROTOCOL_NONE:
                 default:                                       /* See Note #6b.                                        */
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_BIND);
             }
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
             Mem_Clr(&addr_local, NET_SOCK_ADDR_LEN_MAX);
             NET_UTIL_VAL_COPY_16(&addr_local[NET_SOCK_ADDR_IP_IX_PORT], &port_nbr_net);
             Mem_Copy(&addr_local[NET_SOCK_ADDR_IP_V6_IX_ADDR], &addr_ip_netv6, NET_IPv6_ADDR_SIZE);

                                                                /* Cfg conn srch.                                       */
             conn_family       = NET_CONN_FAMILY_IP_V6_SOCK;
             addr_len_chk_size = NET_SOCK_ADDR_LEN_IP_V6;
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_UDP:
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V6_UDP;
                      break;

#ifdef  NET_TCP_MODULE_EN
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V6_TCP;
                      break;
#endif

                 case NET_SOCK_PROTOCOL_NONE:
                 default:                                       /* See Note #6b.                                        */
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_BIND);
             }
             break;
#endif

        default:
                                                                /* See Note #7.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;
    }

                                                                /* Srch for sock conn with identical ...                */
                                                                /* ... local &/or remote addrs (see Note #8).           */
    conn_id_srch = NetConn_Srch(conn_family,
                                conn_protocol_ix,
                               &addr_local[0],
                                p_addr_remote,
                                addr_len_chk_size,
                                DEF_NULL,
                                DEF_NULL,
                               &conn_state);

    switch (conn_state) {
        case NET_CONN_STATE_NONE:                               /* NO sock with identical local or remote addrs found.  */
        case NET_CONN_STATE_HALF_WILDCARD:
        case NET_CONN_STATE_FULL_WILDCARD:
             break;

        case NET_CONN_STATE_HALF:                               /* If  half- conn'd sock found                ...       */
             if (p_addr_remote != DEF_NULL) {                   /* ... but remote addr avail (see Note #8b2), ...       */
                 break;                                         /* ... allow valid bind.                                */
             }

        case NET_CONN_STATE_FULL:                               /* If  fully conn'd sock found; ...                     */
             if (conn_id_srch == p_sock->ID_Conn) {             /* ... but = sock's conn,       ...                     */
                 rtn_code = NET_SOCK_BSD_ERR_NONE;
                 goto exit;                                     /* ... rtn valid bind;          ...                     */
             }
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrInUseCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_ALREADY_EXISTS);         /* ... else rtn err (see Note #8a).                     */
             goto exit;

        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidAddrCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_BIND);
    }


                                                                /* ----------- ADD LOCAL ADDR TO SOCK CONN ------------ */
    if (conn_avail != DEF_YES) {                                /* If NO conn prev'ly avail, get/cfg sock conn.         */
        conn_id = NetConn_Get(conn_family, conn_protocol_ix, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             goto exit;
        }

        p_sock->ID_Conn = conn_id;                               /* Set sock's conn id.                                  */
        NetConn_ID_AppSet(             conn_id,
                          (NET_CONN_ID)p_sock->ID);

    } else {
        conn_id = p_sock->ID_Conn;                               /* Else get sock's conn id.                             */
    }

                                                                /* Set  sock's local addr.                              */
    NetConn_AddrLocalSet(conn_id,
                         if_nbr,
                         addr_local,
                         addr_len_chk_size,
                         addr_over_wr,
                         p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }


    NetConn_ListUnlink(conn_id);                                /* Unlink sock conn from conn list, if necessary.       */


    NetConn_ListAdd(conn_id);                                   /* Add    sock conn into conn list (see Note #8b).      */

                                                                /* -------------- UPDATE SOCK CONN STATE -------------- */
    p_sock->State = sock_state;

    PP_UNUSED_PARAM(p_addr_dest);

    rtn_code = NET_SOCK_BSD_ERR_NONE;

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                    NetSock_ConnHandlerDatagram()
*
* Description : (1) Connect a datagram-type socket to a remote address :
*
*                   (a) Validate socket connection state
*                   (b) Prepare  socket connection address(s)
*                   (c) Update   socket connection state
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to connect.
*               -------         Argument checked   in NetSock_Conn().
*
*               p_sock          Pointer to socket.
*               ------          Argument validated in NetSock_Conn().
*
*               p_addr_remote   Pointer to socket address structure.
*               -------------   Argument checked   in NetSock_Conn().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE, if NO error(s).
*               NET_SOCK_BSD_ERR_CONN, otherwise.
*
* Note(s)     : (2) (a) For datagram-type sockets, the remote address does NOT require any connection.  The
*                       pseudo-connection provides a remote address to allow datagram-type sockets to use
*                       stream-type sockets' send & receive functions -- NetSock_RxData() & NetSock_TxData().
*
*                   (b) In addition, the remote address is NOT required to be static -- even if the socket
*                       is in a "connected" state.  In other words, any datagram-type socket may "connect"
*                       to different remote addresses on each or any separate socket operation.
*********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_ConnHandlerDatagram (NET_SOCK_ID     sock_id,
                                                        NET_SOCK       *p_sock,
                                                        NET_SOCK_ADDR  *p_addr_remote,
                                                        RTOS_ERR       *p_err)
{
    CPU_BOOLEAN        addr_remote_validate;
    NET_SOCK_RTN_CODE  rtn_code = NET_SOCK_BSD_ERR_CONN;


                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;

        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_BOUND:
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
             addr_remote_validate = DEF_YES;
#else
             addr_remote_validate = DEF_NO;
#endif
             break;

        case NET_SOCK_STATE_CONN:                               /* See Note #2b.                                        */
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             addr_remote_validate = DEF_YES;
             break;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_CONN);
    }


                                                                /* ------------ PREPARE SOCK CONN ADDR(S) ------------- */
    NetSock_ConnHandlerAddr(sock_id, p_sock, p_addr_remote, addr_remote_validate, DEF_YES, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }

                                                                /* -------------- UPDATE SOCK CONN STATE -------------- */
    p_sock->State = NET_SOCK_STATE_CONN;

    rtn_code = NET_SOCK_BSD_ERR_NONE;

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                     NetSock_ConnHandlerStream()
*
* Description : (1) Connect a stream-type socket to a remote address :
*
*                   (a) Validate socket connection state                                See Note #4
*                   (b) Prepare  socket connection address(s)
*                   (c) Initiate transport layer connection :
*                       (1) Get      transport connection
*                       (2) Transmit transport connection request
*                       (3) Wait on  transport connection to connect
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to connect.
*               -------         Argument checked   in NetSock_Conn().
*
*               p_sock          Pointer to socket.
*               ------          Argument validated in NetSock_Conn().
*
*               p_addr_remote   Pointer to socket address structure.
*               -------------   Argument checked   in NetSock_Conn().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE, if NO error(s).
*               NET_SOCK_BSD_ERR_CONN, otherwise.
*
* Note(s)     : (2) (a) For stream-type sockets, the remote address MUST be static.  In other words,
*                       a stream-type socket MUST be connected to & use the same remote address for
*                       ALL socket operations.
*
*                   (b) In addition, the socket MUST be connected to the remote address PRIOR to any
*                       data transmit or receive operation.
*
*                   (c) Stream-type sockets may connect to remote addresses from the following states :
*
*                       (1) CLOSED
*                       (2) BOUND                                          See Note #2d
*                       (3) LISTEN                                         See Note #2d
*
*                   (d) Stream-type sockets MUST be bound to a valid local address that is NOT a
*                       protocol's wildcard address.
*
*               (3) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (4) Socket descriptor write availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor write handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also  Note #1a,
*                            'NetSock_SelDescHandlerWrStream()   Note #3',
*                          & 'NetSock_SelDescHandlerErrStream()  Note #3'.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_SOCK_RTN_CODE  NetSock_ConnHandlerStream (NET_SOCK_ID     sock_id,
                                                      NET_SOCK       *p_sock,
                                                      NET_SOCK_ADDR  *p_addr_remote,
                                                      RTOS_ERR       *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR      addr_srcv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_IPv6_ADDR      addr_srcv6;
    CPU_BOOLEAN        is_addr_wildcard;
#endif
    NET_CONN_ADDR_LEN  addr_len;
    CPU_INT08U         addr_local[NET_SOCK_ADDR_LEN_MAX];
    NET_CONN_ID        conn_id;
    NET_CONN_ID        conn_id_transport;
    NET_SOCK_RTN_CODE  rtn_code   = NET_SOCK_BSD_ERR_CONN;


                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;


        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;


        case NET_SOCK_STATE_CLOSED:                             /* See Note #2c1.                                       */
             break;


        case NET_SOCK_STATE_BOUND:                              /* Chk valid local addr (see Note #2d).                 */
        case NET_SOCK_STATE_LISTEN:
             conn_id  = p_sock->ID_Conn;
             addr_len = sizeof(addr_local);
             NetConn_AddrLocalGet(conn_id,
                                 &addr_local[0],
                                 &addr_len,
                                  p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                 goto exit;
             }

            switch (p_sock->ProtocolFamily) {
                case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
#ifdef  NET_IPv4_MODULE_EN
                     NET_UTIL_VAL_COPY_GET_NET_32(&addr_srcv4, &addr_local[NET_SOCK_ADDR_IP_V4_IX_ADDR]);
                                                                /* If wildcard addr, ...                                */
                     if (addr_srcv4 == NET_SOCK_ADDR_IP_V4_WILDCARD) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_ADDR_SRC);  /* ... rtn invalid addr.                     */
                         goto exit;
                     }
#else
                     RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_CONN);
#endif
                     break;

                case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
#ifdef  NET_IPv6_MODULE_EN
                     Mem_Copy(&addr_srcv6, &addr_local[NET_SOCK_ADDR_IP_V6_IX_ADDR], NET_IPv6_ADDR_SIZE);

                     is_addr_wildcard = Mem_Cmp(&addr_srcv6, &NetIPv6_AddrWildcard, NET_IPv6_ADDR_SIZE);

                     if (is_addr_wildcard == DEF_YES) {         /* If wildcard addr, ...                                */
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_ADDR_SRC); /* ... rtn invalid addr.                      */
                         goto exit;
                     }
#else
                     RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_CONN);
#endif
                     break;

                default:
                     NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                     RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_CONN);
             }
             break;


        case NET_SOCK_STATE_CONN_IN_PROGRESS:
             rtn_code = NetSock_ConnHandlerStreamWait(sock_id, p_sock, p_err);
             goto exit;


        case NET_SOCK_STATE_CONN_DONE:
             p_sock->State = NET_SOCK_STATE_CONN;
             goto exit;


        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit;


        case NET_SOCK_STATE_NONE:
        default:
             PP_UNUSED_PARAM(conn_id);                          /* Prevent possible 'variable unused' warning.          */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_CONN);
    }


                                                                /* ------------ PREPARE SOCK CONN ADDR(S) ------------- */
    NetSock_ConnHandlerAddr(sock_id, p_sock, p_addr_remote, DEF_NO, DEF_NO, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

                                                                /* ---------------- GET TRANSPORT CONN ---------------- */
    conn_id_transport = NetSock_GetConnTransport(p_sock, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

                                                                /* ------- SET TRANSPORT CONNECTION PARAMETERS -------- */
    if (p_sock->RxQ_SizeCfgd != NET_TCP_DFLT_RX_WIN_SIZE_OCTET) {
        NetTCP_ConnCfgRxWinSizeHandler(conn_id_transport, p_sock->RxQ_SizeCfgd, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            goto exit;
        }
    }

    if (p_sock->TxQ_SizeCfgd != NET_TCP_DFLT_TX_WIN_SIZE_OCTET) {
        NetTCP_ConnCfgTxWinSizeHandler(conn_id_transport, p_sock->TxQ_SizeCfgd, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            goto exit;
        }
    }


                                                                /* --------------- INIT TRANSPORT CONN ---------------- */
    switch (p_addr_remote->AddrFamily) {
        case NET_SOCK_ADDR_FAMILY_IP_V4:
#ifdef  NET_IPv4_MODULE_EN
             NetTCP_TxConnReq(conn_id_transport, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                 goto exit;
             }
#else
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_CONN);
#endif
             break;

        case NET_SOCK_ADDR_FAMILY_IP_V6:
#ifdef  NET_IPv6_MODULE_EN
             NetTCP_TxConnReq(conn_id_transport, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                 goto exit;
             }
#else
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_CONN);
#endif
             break;

        default:                                                /* See Note #3.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_CONN);
        }


                                                                /* -------------- WAIT ON TRANSPORT CONN -------------- */
    rtn_code = NetSock_ConnHandlerStreamWait(sock_id, p_sock, p_err);

exit:
    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                   NetSock_ConnHandlerStreamWait()
*
* Description :  (1) Wait for a  stream-type socket to connect to a remote address :
*
*                    (a) Wait on stream-type socket to connect
*                    (b) Update socket connection state
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to connect.
*               -------     Argument checked   in NetSock_Conn().
*
*               p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Conn().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE, if NO error(s).
*               NET_SOCK_BSD_ERR_CONN, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_SOCK_RTN_CODE  NetSock_ConnHandlerStreamWait (NET_SOCK_ID   sock_id,
                                                          NET_SOCK     *p_sock,
                                                          RTOS_ERR     *p_err)
{
    NET_SOCK_STATE     sock_state;
    NET_SOCK_RTN_CODE  rtn_code   = NET_SOCK_BSD_ERR_CONN;
    CPU_BOOLEAN        block;
    CPU_BOOLEAN        secure;


                                                                /* ---------------- WAIT ON SOCK CONN ----------------- */
    block         = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    sock_state    = p_sock->State;
    secure        = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
    p_sock->State = NET_SOCK_STATE_CONN_IN_PROGRESS;
    if ((block  != DEF_YES) &&
        (secure != DEF_YES)){                                   /* If non-blocking sock conn and non-secure...         */
         rtn_code = NET_SOCK_BSD_ERR_NONE;
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_OP_IN_PROGRESS);     /* ... rtn not-yet-conn'd err.                          */
         goto exit;
    }

                                                                /* ---------------- WAIT ON SOCK CONN ----------------- */
    sock_state = p_sock->State;
    Net_GlobalLockRelease();
    NetSock_ConnReqWait(p_sock, p_err);
    Net_GlobalLockAcquire((void *)NetSock_ConnHandlerStreamWait);

    switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_TIMEOUT:
             rtn_code = NET_SOCK_BSD_ERR_CONN;
             goto exit;                                         /* Rtn err from NetSock_ConnReqWait().                  */

        default:
             p_sock->State = sock_state;
             rtn_code = NET_SOCK_BSD_ERR_CONN;
             goto exit;
    }

                                                                /* -------------- UPDATE SOCK CONN STATE -------------- */
    p_sock->State = NET_SOCK_STATE_CONN;

    PP_UNUSED_PARAM(sock_id);

    rtn_code = NET_SOCK_BSD_ERR_NONE;

exit:
    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_ConnHandlerAddr()
*
* Description : (1) Connect a socket to a remote address :
*
*                   (a) Validate remote address for socket connection
*                   (b) Prepare  socket for remote connection :
*                       (1) Bind to local address, if necessary
*                   (c) Add remote address into socket connection
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to connect.
*               -------         Argument checked   in NetSock_Conn().
*
*               p_sock          Pointer to socket.
*               ------          Argument validated in NetSock_Conn().
*
*               p_addr_remote   Pointer to socket address structure.
*               -------------   Argument checked   in NetSock_Conn().
*
*               addr_validate   Validate remote address :
*               -------------
*                                   DEF_NO                      Do NOT validate  remote address.
*                                   DEF_YES                            Validate  remote address.
*
*                               Argument validated in NetSock_ConnHandlerDatagram(),
*                                                     NetSock_ConnHandlerStream().
*
*               addr_over_wr    Allow remote address overwrite :
*               ------------
*                                   DEF_NO                      Do NOT overwrite remote address.
*                                   DEF_YES                            Overwrite remote address.
*
*                               Argument validated in NetSock_ConnHandlerDatagram(),
*                                                     NetSock_ConnHandlerStream().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetSock_ConnHandlerAddr (NET_SOCK_ID     sock_id,
                                       NET_SOCK       *p_sock,
                                       NET_SOCK_ADDR  *p_addr_remote,
                                       CPU_BOOLEAN     addr_validate,
                                       CPU_BOOLEAN     addr_over_wr,
                                       RTOS_ERR       *p_err)
{

                                                                /* ---------- VALIDATE SOCK CONN REMOTE ADDR ---------- */
    if (addr_validate == DEF_YES) {
        NetSock_ConnHandlerAddrRemoteValidate(p_sock, p_addr_remote, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             goto exit;
        }
    }

                                                                /* ----------- PREPARE SOCK FOR REMOTE CONN ----------- */
    if (p_sock->State == NET_SOCK_STATE_CLOSED) {               /* If sock closed, bind to local addr.                  */
        NetSock_ConnHandlerAddrLocalBind(sock_id, p_addr_remote, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             goto exit;
        }
    }
                                                                /* ----------- ADD REMOTE ADDR TO SOCK CONN ----------- */
                                                                /* Set sock's remote addr.                              */
    NetSock_ConnHandlerAddrRemoteSet(p_sock, p_addr_remote, addr_over_wr, p_err);

exit:
    return;
}


/*
*********************************************************************************************************
*                                 NetSock_ConnHandlerAddrLocalBind()
*
* Description : (1) Bind a connecting socket to a local address :
*
*                   (a) Configure local address to :
*                       (1) Specific host address for remote address, if available
*                       (2) Default  host address,                    otherwise
*
*                   (b) Bind to   local address
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to connect.
*               -------         Argument checked in NetSock_Conn().
*
*               p_addr_remote   Pointer to socket address structure.
*               -------------   Argument checked in NetSock_Conn().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) Socket address structure 'AddrFamily' member MUST be configured in host-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (3) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*********************************************************************************************************
*/

static  void  NetSock_ConnHandlerAddrLocalBind (NET_SOCK_ID     sock_id,
                                                NET_SOCK_ADDR  *p_addr_remote,
                                                RTOS_ERR       *p_err)
{
           NET_IF_NBR           if_nbr;
#ifdef  NET_IPv4_MODULE_EN
           NET_SOCK_ADDR_IPv4  *p_addr_ipv4;
           NET_SOCK_ADDR_IPv4   addr_ipv4_local;
           NET_IPv4_ADDR        addr_ipv4_remote;
           NET_IPv4_ADDR        addr_ipv4_host;
#endif
#ifdef NET_IPv6_MODULE_EN
    const  NET_IPv6_ADDR_OBJ   *p_addr_ipv6_obj;
           NET_SOCK_ADDR_IPv6  *p_addr_ipv6;
           NET_SOCK_ADDR_IPv6   addr_ipv6_local;
           CPU_BOOLEAN          addr_ipv6_unspecified;
#endif
           NET_SOCK_ADDR       *p_addr_local;
           NET_SOCK_ADDR_LEN    addr_len;
           CPU_BOOLEAN          addr_random;


                                                                    /* ---------------- CFG LOCAL ADDR ---------------- */
    switch (p_addr_remote->AddrFamily) {
 #ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_ADDR_FAMILY_IP_V4:

             p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr_remote;
             NET_UTIL_VAL_COPY_GET_NET_32(&addr_ipv4_remote, &p_addr_ipv4->Addr);

             addr_ipv4_host =  NetIPv4_GetAddrSrcHandler(&if_nbr, addr_ipv4_remote, p_err);
             if (addr_ipv4_host != NET_IPv4_ADDR_NONE) {
                 p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)&addr_ipv4_local;
                 NET_UTIL_VAL_SET_HOST_16    (&p_addr_ipv4->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V4);
                 NET_UTIL_VAL_SET_NET_16     (&p_addr_ipv4->Port,       NET_SOCK_PORT_NBR_RANDOM);
                 NET_UTIL_VAL_COPY_SET_NET_32(&p_addr_ipv4->Addr,      &addr_ipv4_host);
                 Mem_Clr(&p_addr_ipv4->Unused[0],
                          NET_SOCK_ADDR_IPv4_NBR_OCTETS_UNUSED);

                 p_addr_local = (NET_SOCK_ADDR   *)&addr_ipv4_local;
                 addr_len     = (NET_SOCK_ADDR_LEN) sizeof(addr_ipv4_local);
                 addr_random  =  DEF_NO;

             } else {
                 p_addr_local = DEF_NULL;
                 addr_len     = 0u;
                 addr_random  = DEF_YES;
             }
             break;
#endif

#ifdef NET_IPv6_MODULE_EN
        case NET_SOCK_ADDR_FAMILY_IP_V6:

             p_addr_ipv6     = (NET_SOCK_ADDR_IPv6 *)p_addr_remote;
             p_addr_ipv6_obj = NetIPv6_GetAddrSrcHandler(                        &if_nbr,
                                                         (const  NET_IPv6_ADDR *) DEF_NULL,
                                                         (const  NET_IPv6_ADDR *)&p_addr_ipv6->Addr,
                                                                                  DEF_NULL,
                                                                                  p_err);
             if (p_addr_ipv6_obj == DEF_NULL) {
                  RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_ADDR_SRC);
                  return;
             }

             addr_ipv6_unspecified = NetIPv6_IsAddrUnspecified(&p_addr_ipv6_obj->AddrHost);
             if (addr_ipv6_unspecified == DEF_NO) {
                 p_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)&addr_ipv6_local;
                 NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv6->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V6);
                 NET_UTIL_VAL_SET_NET_16 (&p_addr_ipv6->Port,       NET_SOCK_PORT_NBR_RANDOM);

                 Mem_Copy(&p_addr_ipv6->Addr, &p_addr_ipv6_obj->AddrHost, NET_IPv6_ADDR_SIZE);

                 p_addr_ipv6->FlowInfo = 0u;
                 p_addr_ipv6->ScopeID  = 0u;

                 p_addr_local = (NET_SOCK_ADDR   *)&addr_ipv6_local;
                 addr_len     = (NET_SOCK_ADDR_LEN) sizeof(addr_ipv6_local);
                 addr_random  =  DEF_NO;
             } else {
                 p_addr_local = DEF_NULL;
                 addr_len     = 0u;
                 addr_random  = DEF_YES;
             }
             break;
#endif

        default:                                                    /* See Note #3.                                     */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }

                                                                    /* -------------- BIND TO LOCAL ADDR -------------- */
   (void)NetSock_BindHandler(sock_id,
                             p_addr_local,
                             addr_len,
                             addr_random,
                             p_addr_remote,
                             p_err);
}


/*
*********************************************************************************************************
*                               NetSock_ConnHandlerAddrRemoteValidate()
*
* Description : (1) Validate socket remote address :
*
*                   (a) Validate socket connection                                  See Note #5c
*                   (b) Search for other socket connection(s) with same
*                           local/remote addresses                                  See Note #5
*
*
* Argument(s) : p_sock          Pointer to socket.
*               ------          Argument validated in NetSock_Conn().
*
*               p_addr_remote   Pointer to socket address structure.
*               -------------   Argument checked   in NetSock_Conn().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) Socket connection addresses MUST be maintained in network-order.
*
*                   (b) However, since the port number & address are copied from a network-order socket
*                       address structure directly into a network-order multi-octet array, they do NOT
*                       need to be converted from host-order to network-order.
*
*               (3) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (4) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'Protocol' is incorrectly modified.
*
*               (5) (a) (1) Multiple socket connections with the same local & remote address -- both
*                           addresses & port numbers -- is NOT currently supported.
*
*                       (2) Therefore, when updating a socket connection, it is necessary to search the
*                           connection lists for any other connection with the same local & remote address.
*
*                       (3) Since datagram-type sockets' remote address is NOT required to be static,
*                           datagram-type sockets in a "connected" state MUST search the connection lists
*                           for a connection with the same local & remote address.
*
*                           See also 'NetSock_ConnHandlerDatagram()  Note #2b'.
*
*                   (b) (1) Also, multiple socket connections with only a local address but the same local
*                           address -- both address & port number -- is NOT currently supported.
*
*                       (2) Therefore, when adding or updating a socket connection with only a local address,
*                           it is necessary to search the connection lists for any other connection with the
*                           same local address.
*
*                       (3) Thus the option for sockets to reuse the same local address is NOT currently
*                           supported even if the socket reuse option (SO_REUSEADDR) is requested.
*
*                           See 'net_sock.c  Note #1d1'.
*
*                   See also 'NetSock_BindHandler()  Note #8'.
*********************************************************************************************************
*/

static  void  NetSock_ConnHandlerAddrRemoteValidate (NET_SOCK       *p_sock,
                                                     NET_SOCK_ADDR  *p_addr_remote,
                                                     RTOS_ERR       *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4    *p_addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6    *p_addr_ipv6;
#endif
    CPU_INT08U             addr_remote[NET_SOCK_ADDR_LEN_MAX];
    CPU_INT08U             addr_local[NET_SOCK_ADDR_LEN_MAX];
    NET_CONN_ADDR_LEN      addr_local_len;
    NET_CONN_ID            conn_id;
    NET_CONN_ID            conn_id_srch;
    NET_CONN_FAMILY        conn_family;
    NET_CONN_PROTOCOL_IX   conn_protocol_ix;
    NET_CONN_STATE         conn_state;


                                                                /* ---------------- VALIDATE SOCK CONN ---------------- */
    conn_id = p_sock->ID_Conn;
    if (conn_id == NET_CONN_ID_NONE) {                          /* If NO sock conn, rtn no err (see Note #5c).          */
        goto exit;
    }


                                                                /* --------------- CFG SOCK REMOTE ADDR --------------- */
    switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr_remote;   /* Cfg remote addr as net-order sock addr (see Note #2).*/
             NET_UTIL_VAL_COPY_16(&addr_remote[NET_SOCK_ADDR_IP_IX_PORT], &p_addr_ipv4->Port);
             NET_UTIL_VAL_COPY_32(&addr_remote[NET_SOCK_ADDR_IP_V4_IX_ADDR], &p_addr_ipv4->Addr);
             break;
#endif
#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
             p_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)p_addr_remote;   /* Cfg remote addr as net-order sock addr (see Note #2).*/
             NET_UTIL_VAL_COPY_16(&addr_remote[NET_SOCK_ADDR_IP_IX_PORT], &p_addr_ipv6->Port);
             Mem_Copy(&addr_remote[NET_SOCK_ADDR_IP_V6_IX_ADDR], &p_addr_ipv6->Addr, NET_IPv6_ADDR_SIZE);
             break;
#endif

        default:                                                /* See Note #3.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }


                                                                /* ---- SRCH FOR LOCAL/REMOTE CONN IN CONN LIST(S) ---- */
                                                                /* Get local  addr from sock conn.                      */
    addr_local_len = sizeof(addr_local);
    NetConn_AddrLocalGet(conn_id,
                        &addr_local[0],
                        &addr_local_len,
                         p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }


                                                                /* Cfg conn srch.                                       */
    switch (p_sock->Protocol) {
        case NET_SOCK_PROTOCOL_UDP:
             switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
                 case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                      conn_family      = NET_CONN_FAMILY_IP_V4_SOCK;
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V4_UDP;
                      break;
#endif
#ifdef  NET_IPv6_MODULE_EN
                 case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                      conn_family      = NET_CONN_FAMILY_IP_V6_SOCK;
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V6_UDP;
                      break;
#endif

                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
             }
             break;


#ifdef  NET_TCP_MODULE_EN
        case NET_SOCK_PROTOCOL_TCP:
             switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
                 case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                      conn_family      = NET_CONN_FAMILY_IP_V4_SOCK;
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V4_TCP;
                      break;
#endif
#ifdef  NET_IPv6_MODULE_EN
                 case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                      conn_family      = NET_CONN_FAMILY_IP_V6_SOCK;
                      conn_protocol_ix = NET_CONN_PROTOCOL_IX_IP_V6_TCP;
                      break;
#endif

                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
             }
             break;
#endif

        case NET_SOCK_PROTOCOL_NONE:
        default:                                                /* See Note #4.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }


                                                                /* Srch for sock conn with identical local/remote addrs.*/
    conn_id_srch = NetConn_Srch(conn_family,
                                conn_protocol_ix,
                               &addr_local[0],
                               &addr_remote[0],
                                NET_SOCK_ADDR_LEN_MAX,
                                DEF_NULL,
                                DEF_NULL,
                               &conn_state);

    if (conn_state == NET_CONN_STATE_FULL) {                    /* If local/remote addrs already conn'd, ...            */
        if (conn_id_srch != p_sock->ID_Conn) {                  /* ... but != sock's conn,                ...           */
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidConnInUseCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_ALREADY_EXISTS);          /* ... rtn err (see Note #5).                           */
            goto exit;
        }
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                 NetSock_ConnHandlerAddrRemoteSet()
*
* Description : Set socket's remote address.
*
* Argument(s) : p_sock          Pointer to socket.
*               ------          Argument validated in NetSock_Conn().
*
*               p_addr_remote   Pointer to socket address structure.
*               -------------   Argument checked   in NetSock_ConnHandlerAddrRemoteValidate().
*
*               addr_over_wr    Allow remote address overwrite :
*               ------------
*                                   DEF_NO                      Do NOT overwrite remote address.
*                                   DEF_YES                            Overwrite remote address.
*
*                                   Argument validated in NetSock_ConnHandlerDatagram(),
*                                                         NetSock_ConnHandlerStream().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (1) (a) Socket connection addresses MUST be maintained in network-order.
*
*                   (b) However, since the port number & address are copied from a network-order socket
*                       address structure directly into a network-order multi-octet array, they do NOT
*                       need to be converted from host-order to network-order.
*
*               (2) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*********************************************************************************************************
*/

static  void  NetSock_ConnHandlerAddrRemoteSet (NET_SOCK       *p_sock,
                                                NET_SOCK_ADDR  *p_addr_remote,
                                                CPU_BOOLEAN     addr_over_wr,
                                                RTOS_ERR       *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4  *p_addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6  *p_addr_ipv6;
#endif
    CPU_INT08U           addr_remote[NET_SOCK_ADDR_LEN_MAX];
    NET_CONN_ID          conn_id;

                                                                /* --------------- CFG SOCK REMOTE ADDR --------------- */
    Mem_Clr(&addr_remote, NET_SOCK_ADDR_LEN_MAX);
    switch (p_addr_remote->AddrFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_FAMILY_IP_V4:
             p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr_remote;   /* Cfg remote addr as net-order sock addr (see Note #1).*/
             NET_UTIL_VAL_COPY_16(&addr_remote[NET_SOCK_ADDR_IP_IX_PORT], &p_addr_ipv4->Port);
             NET_UTIL_VAL_COPY_32(&addr_remote[NET_SOCK_ADDR_IP_V4_IX_ADDR], &p_addr_ipv4->Addr);
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_FAMILY_IP_V6:
             p_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)p_addr_remote;   /* Cfg remote addr as net-order sock addr (see Note #1).*/
             NET_UTIL_VAL_COPY_16(&addr_remote[NET_SOCK_ADDR_IP_IX_PORT], &p_addr_ipv6->Port);
             Mem_Copy(&addr_remote[NET_SOCK_ADDR_IP_V6_IX_ADDR], &p_addr_ipv6->Addr, NET_IPv6_ADDR_SIZE);
             break;
#endif

        default:                                                /* See Note #2.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
             goto exit;
    }

                                                                /* --------------- SET SOCK REMOTE ADDR --------------- */
    conn_id = p_sock->ID_Conn;
    NetConn_AddrRemoteSet(conn_id,
                         &addr_remote[0],
                          NET_SOCK_ADDR_LEN_MAX,
                          addr_over_wr,
                          p_err);

exit:
    return;
}


/*
*********************************************************************************************************
*                                     NetSock_ConnAcceptQ_Init()
*
* Description : Initialize a stream-type socket's connection accept queue.
*
* Argument(s) : p_sock          Pointer to a socket.
*               ------          Argument validated in NetSock_Listen().
*
*               sock_q_size     Maximum number of connection requests to accept & queue on listen socket.
*
*                                   NET_SOCK_Q_SIZE_NONE                    NO custom configuration for socket's
*                                                                               connection accept queue maximum
*                                                                               size; configure to default maximum :
*                                                                               NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX.
*
*                                <= NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX     Custom configure socket's connection
*                                                                               accept queue maximum size.
*
* Return(s)   : none.
*
* Note(s)     : (1) Some socket controls were previously initialized in NetSock_Clr() when the socket
*                   was allocated.  These socket controls do NOT need to be re-initialized but are
*                   shown for completeness.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_ConnAcceptQ_Init (NET_SOCK         *p_sock,
                                        NET_SOCK_Q_SIZE   sock_q_size)
{
    NET_SOCK_Q_SIZE  accept_q_size;


    if (sock_q_size != NET_SOCK_Q_SIZE_NONE) {                      /* If     conn accept Q size cfg'd, ..              */
        if (sock_q_size > 0) {                                      /* .. lim conn accept Q size;       ..              */
            accept_q_size = (sock_q_size > NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX) ? NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX
                                                                                : sock_q_size;
        } else {
            accept_q_size =  NET_SOCK_Q_SIZE_MIN;
        }
    } else {                                                        /* .. else cfg to dflt max size.                    */
        accept_q_size = NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX;
    }

    p_sock->ConnAcceptQ_SizeMax = accept_q_size;                     /* Cfg listen sock conn accept Q max size.          */

                                                                    /* Init conn accept Q ctrls.                        */
#if 0                                                               /* Init'd in NetSock_Clr() [see Note #1].           */
    p_sock->ConnAcceptQ_SizeCur = 0u;
#endif
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_ConnAcceptQ_Clr()
*
* Description : Clear a stream-type socket's connection accept queue.
*
* Argument(s) : p_sock       Pointer to a socket.
*               ------       Argument validated in NetSock_ConnSignalAccept(),
*                                                  NetSock_FreeHandler().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_ConnAcceptQ_Clr (NET_SOCK  *p_sock)
{
    CPU_BOOLEAN  done;


                                                                    /* -------------- VALIDATE SOCK TYPE -------------- */
    if (p_sock->SockType != NET_SOCK_TYPE_STREAM) {
        return;
    }

                                                                    /* -------------- CLR CONN ACCEPT Q --------------- */
    done = DEF_NO;
    while (done == DEF_NO) {                                        /* Clr sock's conn accept Q ...                     */
        SLIST_MEMBER           *p_node;
        NET_SOCK_ACCEPT_Q_OBJ  *p_obj = DEF_NULL;
        RTOS_ERR                local_err;


        RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

        p_node = SList_Pop(&p_sock->ConnAcceptQ_Ptr);

        if (p_node == DEF_NULL) {
            done = DEF_YES;
        } else {
            p_obj = SLIST_ENTRY(p_node, NET_SOCK_ACCEPT_Q_OBJ, ListNode);

            NetConn_CloseFromApp(p_obj->ConnID, DEF_YES);
            p_sock->ConnChildQ_SizeCur--;

            Mem_DynPoolBlkFree(&NetSock_AcceptQ_ObjPool, p_obj, &local_err);
            RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
        }
    }
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_ConnAcceptQ_IsAvail()
*
* Description : Check if socket's connection accept queue is available to queue a new connection.
*
* Argument(s) : p_sock       Pointer to a socket.
*               ------       Argument validated in NetSock_ListenQ_IsAvail().
*
* Return(s)   : DEF_YES, if socket connection accept queue is available to queue a new connection.
*               DEF_NO,  otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN  NetSock_ConnAcceptQ_IsAvail (NET_SOCK  *p_sock)
{
    NET_SOCK_Q_SIZE  conn_q_size_cur;
    NET_SOCK_Q_SIZE  conn_q_size_max;
    CPU_BOOLEAN      q_avail;


                                                                    /* --------- CHK SOCK CONN ACCEPT Q AVAIL --------- */
    if (p_sock->ConnChildQ_SizeMax == NET_SOCK_Q_SIZE_UNLIMITED) {
        conn_q_size_cur = p_sock->ConnAcceptQ_SizeCur;
        conn_q_size_max = p_sock->ConnAcceptQ_SizeMax;
    } else {
        conn_q_size_cur = p_sock->ConnChildQ_SizeCur + p_sock->ConnAcceptQ_SizeCur;
        conn_q_size_max = p_sock->ConnChildQ_SizeMax;
    }

    q_avail = (conn_q_size_cur >= conn_q_size_max) ? DEF_NO : DEF_YES;

    return (q_avail);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_ConnAcceptQ_IsRdy()
*
* Description : Check if socket's connection accept queue is ready with any available queued connection(s).
*
* Argument(s) : p_sock       Pointer to a socket.
*
* Return(s)   : DEF_YES, if socket connection accept queue has any available queued connection(s).
*               DEF_NO,  otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN  NetSock_ConnAcceptQ_IsRdy (NET_SOCK  *p_sock)
{
    NET_SOCK_ACCEPT_Q_OBJ  *p_obj  = DEF_NULL;
    CPU_BOOLEAN             is_rdy = DEF_NO;


                                                                /* ---------- CHK SOCK CONN ACCEPT Q RDY ---------- */
    SLIST_FOR_EACH_ENTRY(&p_sock->ConnAcceptQ_Ptr, p_obj, NET_SOCK_ACCEPT_Q_OBJ, ListNode) {
        if (p_obj->IsRdy == DEF_YES) {
            is_rdy = DEF_YES;
            break;
        }
    }

    return (is_rdy);
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_ConnAcceptQ_ConnID_Add()
*
* Description : Add a connection handle identifier into a stream-type socket's connection accept queue.
*
*               (1) A stream-type socket's connection accept queue is a FIFO Q implemented as a circular
*                   ring array :
*
*                   (a) Sockets' 'ConnAcceptQ_HeadIx' points to the next available connection handle
*                       identifier to accept.
*
*                   (b) Sockets' 'ConnAcceptQ_TailIx' points to the next available queue entry to insert
*                       a connection handle identifier.
*
*                   (c) Sockets' 'ConnAcceptQ_HeadIx'/'ConnAcceptQ_TailIx' advance :
*
*                       (1) By increment;
*                       (2) Reset to minimum index value when maximum index value reached.
*
*                           (A) Although a specific maximum array-size/index-value is configured for
*                               each socket connection accept queue ('ConnAcceptQ_SizeMax'), the global
*                               maximum array-size/index-value ('NET_SOCK_CFG_CONN_ACCEPT_Q_SIZE_MAX')
*                               is used as the maximum index value.  Although this uses the entire
*                               queue array (not just a subset) for adding & removing connection handle
*                               identifiers, it eliminates the need to redundantly validate the socket's
*                               configured connection accept queue maximum array-size/index-value.
*
*
*                                  Index to next available          Index to next available entry
*                               connection handle identifier         to insert accept connection
*                                      in accept queue                    handle identifier
*                                      (see Note #1a)                      (see Note #1b)
*
*                                             |                                   |
*                                             |                                   |
*                                             v                                   v
*                              -------------------------------------------------------------
*                              |     |     |     |     |     |     |     |     |     |     |
*                              |     |     |     |     |     |     |     |     |     |     |
*                              |     |     |     |     |     |     |     |     |     |     |
*                              -------------------------------------------------------------
*
*                                                       ---------->
*                                                 FIFO indices advance by
*                                                increment (see Note #1c1)
*
*                              |                                                           |
*                              |<----------------- Circular Ring FIFO Q ------------------>|
*                              |                      (see Note #1)                        |
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_ConnSignalAccept().
*
*               conn_id     Handle identifier of network connection to insert into connection accept queue.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_ConnAcceptQ_ConnID_Add (NET_SOCK     *p_sock,
                                              NET_CONN_ID   conn_id,
                                              RTOS_ERR     *p_err)
{
    NET_SOCK_ACCEPT_Q_OBJ  *p_obj = DEF_NULL;


                                                                            /* ---------- VALIDATE NBR USED ----------- */
                                                                            /* Chk sock max conn accept Q lim.          */
    if (p_sock->ConnAcceptQ_SizeCur >= p_sock->ConnAcceptQ_SizeMax) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_OVF);
        return;
    }

                                                                            /* ---- ADD CONN ID INTO CONN ACCEPT Q ---- */
    p_obj = (NET_SOCK_ACCEPT_Q_OBJ *)Mem_DynPoolBlkGet(&NetSock_AcceptQ_ObjPool, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    p_obj->ConnID = conn_id;
    p_obj->IsRdy  = DEF_NO;

    SList_PushBack(&p_sock->ConnAcceptQ_Ptr, &p_obj->ListNode);

    p_sock->ConnAcceptQ_SizeCur++;
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_ConnAcceptQ_ConnID_Get()
*
* Description : Get a connection handle identifier from a stream-type socket's connection accept queue.
*
*               (1) A stream-type socket's connection accept queue is a FIFO Q implemented as a circular
*                   ring array :
*
*                   (a) Sockets' 'ConnAcceptQ_HeadIx' points to the next available connection handle
*                       identifier to accept.
*
*                   See also 'NetSock_ConnAcceptQ_ConnID_Add()  Note #1'.
*
*
* Argument(s) : p_sock       Pointer to a socket.
*               ------       Argument validated in NetSock_Accept().
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Accept connection handle identifier, if NO error(s).
*               NET_CONN_ID_NONE, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_CONN_ID  NetSock_ConnAcceptQ_ConnID_Get (NET_SOCK  *p_sock,
                                                     RTOS_ERR  *p_err)
{
    NET_SOCK_ACCEPT_Q_OBJ  *p_obj   = DEF_NULL;
    NET_CONN_ID             conn_id = NET_CONN_ID_NONE;
    CPU_BOOLEAN             found   = DEF_NO;


                                                                            /* ---------- VALIDATE SOCK TYPE ---------- */
    if (p_sock->SockType != NET_SOCK_TYPE_STREAM) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
        return (NET_CONN_ID_NONE);
    }
                                                                            /* ---------- VALIDATE NBR USED ----------- */
    if (p_sock->ConnAcceptQ_SizeCur < 1) {
        NET_CTR_ERR_INC(Net_ErrCtrs.Sock.ConnAcceptQ_NoneAvailCtr);
        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);
        return (NET_CONN_ID_NONE);
    }

                                                                            /* ---- GET CONN ID FROM CONN ACCEPT Q ---- */
    SLIST_FOR_EACH_ENTRY(&p_sock->ConnAcceptQ_Ptr, p_obj, NET_SOCK_ACCEPT_Q_OBJ, ListNode) {
        if (p_obj->IsRdy == DEF_YES) {
            conn_id = p_obj->ConnID;
            found   = DEF_YES;
            break;
        }
    }

    if (found == DEF_NO) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);
        return (NET_CONN_ID_NONE);
    }

    SList_Rem(&p_sock->ConnAcceptQ_Ptr, &p_obj->ListNode);

    Mem_DynPoolBlkFree(&NetSock_AcceptQ_ObjPool, p_obj, p_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_CONN_ID_NONE);

    p_sock->ConnAcceptQ_SizeCur--;

    return (conn_id);
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_ConnAcceptQ_ConnID_Srch()
*
* Description : Search for a connection handle identifier in a stream-type socket's connection accept queue.
*
* Argument(s) : p_sock      Pointer to a socket.
*
*               conn_id     Handle identifier of network connection to search for in connection accept queue.
*
* Return(s)   : Pointer to the connection handle identifier found in socket's connection accept queue.
*               DEF_NULL,  otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_SOCK_ACCEPT_Q_OBJ  *NetSock_ConnAcceptQ_ConnID_Srch (NET_SOCK     *p_sock,
                                                                 NET_CONN_ID   conn_id)
{
    NET_SOCK_ACCEPT_Q_OBJ  *p_obj = DEF_NULL;
    CPU_BOOLEAN             found = DEF_NO;


    SLIST_FOR_EACH_ENTRY(&p_sock->ConnAcceptQ_Ptr, p_obj, NET_SOCK_ACCEPT_Q_OBJ, ListNode) {
        if (p_obj->ConnID == conn_id) {
            found = DEF_YES;
            break;
        }
    }

    if (found == DEF_NO) {
        p_obj = DEF_NULL;
    }

    return (p_obj);
}
#endif


/*
*********************************************************************************************************
*                                 NetSock_ConnAcceptQ_ConnID_Remove()
*
* Description : Remove a connection handle identifier from a stream-type socket's connection accept queue.
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_FreeConnFromSock().
*
*               conn_id     Handle identifier of network connection to remove from connection accept queue.
*
* Return(s)   : DEF_YES, connection handle identifier found & successfully removed from socket's connection
*                        accept queue.
*               DEF_NO,  otherwise.
*
* Note(s)     : (1) Assumes queue indices valid.
*
*               (2) If ALL connection handle identifiers in queue searched & queue tail index NOT found,
*                   tail index MUST be invalid -- outside the range of table indices.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN  NetSock_ConnAcceptQ_ConnID_Remove (NET_SOCK     *p_sock,
                                                        NET_CONN_ID   conn_id)
{
    NET_SOCK_ACCEPT_Q_OBJ  *p_obj = DEF_NULL;
    CPU_BOOLEAN             found = DEF_NO;
    RTOS_ERR                local_err;


                                                                            /* ---------- VALIDATE SOCK TYPE ---------- */
    if (p_sock->SockType != NET_SOCK_TYPE_STREAM) {
        return (DEF_NO);
    }
                                                                            /* ----- VALIDATE CONN ACCEPT Q SIZE ------ */
    if (p_sock->ConnAcceptQ_SizeCur < 1) {
        return (DEF_NO);
    }

                                                                            /* ---- SRCH CONN ID IN CONN ACCEPT Q ----- */
    SLIST_FOR_EACH_ENTRY(&p_sock->ConnAcceptQ_Ptr, p_obj, NET_SOCK_ACCEPT_Q_OBJ, ListNode) {
        if (p_obj->ConnID == conn_id) {
            found = DEF_YES;
            break;
        }
    }

    if (found != DEF_YES) {                                                 /* If conn id NOT found, exit remove.       */
        return (DEF_NO);
    }

                                                                            /* -- REMOVE CONN ID FROM CONN ACCEPT Q --- */
    SList_Rem(&p_sock->ConnAcceptQ_Ptr, &p_obj->ListNode);

    Mem_DynPoolBlkFree(&NetSock_AcceptQ_ObjPool, p_obj, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NO);

    p_sock->ConnAcceptQ_SizeCur--;                                          /* Dec conn accept Q cur size.              */

    return (DEF_YES);
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_RxDataHandler()
*
* Description : (1) Receive data from a socket :
*
*                   (a) Validate receive data buffer                                    See Note #2
*                   (b) Validate receive flags                                          See Note #3
*                   (c) Acquire  network lock
*                   (d) Validate socket  used
*                   (e) Receive  socket  data
*                   (f) Release  network lock
*
*
* Argument(s) : sock_id             Socket descriptor/handle identifier of socket to receive data.
*
*               p_data_buf           Pointer to an application data buffer that will receive the socket's received
*                                       data.
*
*               data_buf_len        Size of the   application data buffer (in octets) [see Note #2].
*
*               flags               Flags to select receive options (see Note #3); bit-field flags logically OR'd :
*
*                                       NET_SOCK_FLAG_NONE              No socket flags selected.
*                                       NET_SOCK_FLAG_RX_DATA_PEEK      Receive socket data without consuming
*                                                                           the socket data; i.e. socket data
*                                                                           NOT removed from application receive
*                                                                           queue(s).
*                                       NET_SOCK_FLAG_RX_NO_BLOCK       Receive socket data without blocking.
*
*               p_addr_remote       Pointer to an address buffer that will receive the socket address structure
*               -------------           with the received data's remote address (see Note #4), if NO error(s).
*
*                                   Argument checked     in NetSock_RxDataFrom();
*                                            set to NULL in NetSock_RxData().
*
*               p_addr_len          Pointer to a variable, if available, to ... :
*               ----------
*                                       (a) Pass the size of the address buffer pointed to by 'p_addr_remote'.
*                                       (b) (1) Return the actual size of socket address structure with the
*                                                   received data's remote address, if NO error(s);
*                                           (2) Return 0,                           otherwise.
*
*                                   Argument checked     in NetSock_RxDataFrom();
*                                            set to NULL in NetSock_RxData().
*
*                                   See also Note #5.
*
*               p_ip_opts_buf        Pointer to buffer to receive possible IP options (see Note #6a), if NO error(s).
*
*               ip_opts_buf_len     Size of IP options receive buffer (in octets)    [see Note #6b].
*
*               p_ip_opts_len        Pointer to variable that will receive the return size of any received IP options,
*                                       if NO error(s).
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of positive data octets received, if NO error(s)              [see Note #7a].
*
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED,       if socket connection closed (see Note #7b).
*
*               NET_SOCK_BSD_ERR_RX,                     otherwise                   (see Note #7c1).
*
* Note(s)     : (2) (a) (1) (A) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                               single, complete datagram transmitted MUST be received as a single, complete
*                               datagram.
*
*                           (B) IEEE Std 1003.1, 2004 Edition, Section 'recv() : DESCRIPTION' summarizes
*                               that "for message-based sockets, such as SOCK_DGRAM ... the entire message
*                               shall be read in a single operation.  If a message is too long to fit in
*                               the supplied buffer, and MSG_PEEK is not set in the flags argument, the
*                               excess bytes shall be discarded".
*
*                       (2) Thus if the socket's type is datagram & the receive data buffer size is
*                           NOT large enough for the received data, the receive data buffer is maximally
*                           filled with receive data but the remaining data octets are discarded &
*                           NET_SOCK_ERR_INVALID_DATA_SIZE error is returned.
*
*                   (b) (1) (A) (1) Stream-type sockets transmit & receive all data octets in one or more
*                                   non-distinct packets.  In other words, the application data is NOT
*                                   bounded by any specific packet(s); rather, it is contiguous & sequenced
*                                   from one packet to the next.
*
*                               (2) IEEE Std 1003.1, 2004 Edition, Section 'recv() : DESCRIPTION' summarizes
*                                   that "for stream-based sockets, such as SOCK_STREAM, message boundaries
*                                   shall be ignored.  In this case, data shall be returned to the user as
*                                   soon as it becomes available, and no data shall be discarded".
*
*                           (B) Thus if the socket's type is stream & the receive data buffer size is NOT
*                               large enough for the received data, the receive data buffer is maximally
*                               filled with receive data & the remaining data octets remain queued for
*                               later application-socket receives.
*
*                       (2) Thus it is typical -- but NOT absolutely required -- that a single application
*                           task ONLY receive or request to receive data from a stream-type socket.
*
*                   See also 'NetSock_RxDataHandlerDatagram()  Note #2',
*                          & 'NetSock_RxDataHandlerStream()    Note #2'.
*
*               (3) Only some socket receive flag options are implemented.  If other flag options
*                   are  requested, NetSock_RxDataHandler() aborts & returns appropriate error codes so
*                   that requested flag options are NOT silently ignored.
*
*               (4) (a) Socket address structure 'AddrFamily' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (5) (a) Since 'p_addr_len' argument is both an input & output argument (see 'Argument(s) :
*                       p_addr_len'), ...
*
*                       (1) Its input value SHOULD be validated prior to use; ...
*                       (2) While its output value MUST be initially configured to return a default value
*                           PRIOR to all other validation or function handling in case of any error(s).
*
*                   (b) However, if 'p_addr_len' is available, it SHOULD already be validated & initialized
*                       by previous NetSock_RxData() function(s).
*
*                       See also 'NetSock_RxDataHandlerDatagram()  Note #4b'
*                              & 'NetSock_RxDataHandlerStream()    Note #4b'.
*
*               (6) (a) If ...
*
*                       (1) NO IP options were received
*                             OR
*                       (2) NO IP options receive buffer is provided
*                             OR
*                       (3) IP options receive buffer NOT large enough for the received IP options
*
*                       ... then NO IP options are returned & any received IP options are silently discarded.
*
*                   (b) The IP options receive buffer size SHOULD be large enough to receive the maximum
*                       IP options size, NET_IPv4_HDR_OPT_SIZE_MAX.
*
*                   (c) Received IP options should be provided/decoded via appropriate IP layer API.
*
*                   See also Note #10.
*
*               (7) IEEE Std 1003.1, 2004 Edition, Section 'recv() : RETURN VALUE' states that :
*
*                   (a) "Upon successful completion, recv() shall return the length of the message in bytes."
*
*                   (b) "If no messages are available to be received and the peer has performed an orderly
*                        shutdown, recv() shall return 0."
*
*                   (c) (1) "Otherwise, -1 shall be returned" ...
*                       (2) "and 'errno' set to indicate the error."
*                           'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   See also 'NetSock_RxDataHandlerDatagram()  Note #7'
*                          & 'NetSock_RxDataHandlerStream()    Note #7'.
*
*               (8) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*
*                   However, these pointed-to variables SHOULD already be validated & initialized by
*                   previous NetSock_RxData() function(s).
*
*               (9) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'SockType' is incorrectly modified.
*
*              (10) IP options arguments may NOT be necessary (remove if unnecessary).
*********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_RxDataHandler (NET_SOCK_ID          sock_id,
                                                  void                *p_data_buf,
                                                  CPU_INT16U           data_buf_len,
                                                  NET_SOCK_API_FLAGS   flags,
                                                  NET_SOCK_ADDR       *p_addr_remote,
                                                  NET_SOCK_ADDR_LEN   *p_addr_len,
                                                  void                *p_ip_opts_buf,
                                                  CPU_INT08U           ip_opts_buf_len,
                                                  CPU_INT08U          *p_ip_opts_len,
                                                  RTOS_ERR            *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    NET_SOCK_FLAGS      flag_mask;
    CPU_BOOLEAN         is_used;
#endif
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    CPU_BOOLEAN         secure;
#endif
    NET_SOCK           *p_sock;
    NET_SOCK_RTN_CODE   rtn_code = NET_SOCK_BSD_ERR_RX;


    RTOS_ASSERT_DBG_ERR_SET((p_data_buf != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_RX);
    RTOS_ASSERT_DBG_ERR_SET((data_buf_len >= 1u), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_RX);

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* ----------------- VALIDATE RX FLAGS ---------------- */
    flag_mask = NET_SOCK_FLAG_NONE         |
                NET_SOCK_FLAG_RX_DATA_PEEK |
                NET_SOCK_FLAG_RX_NO_BLOCK;
                                                                /* If any invalid flags req'd, rtn err (see Note #3).   */
    if (((NET_SOCK_FLAGS)flags & (NET_SOCK_FLAGS)~flag_mask) != NET_SOCK_FLAG_NONE) {
        NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFlagsCtr);
        RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_RX);
    }

                                                                /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit;
    }
#endif

                                                                /* ------------------- RX SOCK DATA ------------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             rtn_code = NetSock_RxDataHandlerDatagram(sock_id,
                                                      p_sock,
                                                      p_data_buf,
                                                      data_buf_len,
                                                      flags,
                                                      p_addr_remote,
                                                      p_addr_len,
                                                      p_ip_opts_buf,
                                                      ip_opts_buf_len,
                                                      p_ip_opts_len,
                                                      p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                 goto exit;
             }
             break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
#ifdef  NET_SECURE_MODULE_EN
             secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
#else
             secure = DEF_NO;
#endif
             if (secure  != DEF_YES) {
                 rtn_code = NetSock_RxDataHandlerStream(sock_id,
                                                        p_sock,
                                                        p_data_buf,
                                                        data_buf_len,
                                                        flags,
                                                        p_addr_remote,
                                                        p_addr_len,
                                                        p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                     goto exit;
                 }

             } else {
#ifdef  NET_SECURE_MODULE_EN                               /* If sock secure, rx data via secure handler.          */
                 rtn_code = NetSecure_SockRxDataHandler(p_sock,
                                                        p_data_buf,
                                                        data_buf_len,
                                                        p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                      goto exit;
                 }
#else
                 RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_RX);
#endif
             }
             break;
#endif


        case NET_SOCK_TYPE_NONE:
        default:                                                /* See Note #9.                                         */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;
    }

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                   NetSock_RxDataHandlerDatagram()
*
* Description : (1) Receive data from a datagram-type socket :
*
*                   (a) Validate socket connection state                                See Note #13
*                   (b) Wait on socket receive queue for packet buffer(s)
*                   (c) Get remote host address                                         See Notes #4 & #5
*                   (d) Configure socket transmit :
*                       (1) Configure socket flags
*                   (e) Receive socket data from appropriate transport layer
*                   (f) Free    socket receive packet buffer(s)
*                   (g) Return  socket data received
*
*
* Argument(s) : sock_id             Socket descriptor/handle identifier of socket to receive data.
*               -------             Argument checked   in NetSock_RxDataHandler().
*
*               p_sock              Pointer to a socket.
*               ------              Argument validated in NetSock_RxDataHandler().
*
*               p_data_buf          Pointer to an application data buffer that will receive the socket's received
*               ----------              data.
*
*                                   Argument checked   in NetSock_RxDataHandler().
*
*               data_buf_len        Size of the   application data buffer (in octets) [see Note #2b].
*               ------------        Argument checked   in NetSock_RxDataHandler().
*
*               flags               Flags to select receive options; bit-field flags logically OR'd :
*               -----
*                                       NET_SOCK_FLAG_NONE              No socket flags selected.
*                                       NET_SOCK_FLAG_RX_DATA_PEEK      Receive socket data without consuming
*                                                                           the socket data; i.e. socket data
*                                                                           NOT removed from application receive
*                                                                           queue(s).
*                                       NET_SOCK_FLAG_RX_NO_BLOCK       Receive socket data without blocking
*                                                                           (see Note #3).
*
*                                   Argument checked   in NetSock_RxDataHandler().
*
*               p_addr_remote       Pointer to an address buffer that will receive the socket address structure
*               -------------           with the received data's remote address (see Notes #4 & #5), if NO error(s).
*
*                                   Argument checked     in NetSock_RxDataFrom();
*                                            set to NULL in NetSock_RxData().
*
*               p_addr_len          Pointer to a variable to ... :
*               ----------
*                                       (a) Pass the size of the address buffer pointed to by 'p_addr_remote'.
*                                       (b) (1) Return the actual size of socket address structure with the
*                                                   received data's remote address, if NO error(s);
*                                           (2) Return 0,                           otherwise.
*
*                                   Argument checked     in NetSock_RxDataFrom();
*                                            set to NULL in NetSock_RxData().
*
*                                   See also Note #4b.
*
*               p_ip_opts_buf       Pointer to buffer to receive possible IP options (see Note #6a), if NO error(s).
*
*               ip_opts_buf_len     Size of IP options receive buffer (in octets)    [see Note #6b].
*
*               p_ip_opts_len       Pointer to variable that will receive the return size of any received IP options,
*                                       if NO error(s).
*
*               p_err               Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of positive data octets received, if NO error(s) [see Note #7a].
*
*               NET_SOCK_BSD_ERR_RX,                     otherwise      (see Note #7c).
*
* Note(s)     : (2) (a) (1) Datagram-type sockets transmit & receive all data atomically -- i.e. every single,
*                           complete datagram transmitted MUST be received as a single, complete datagram.
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'recv() : DESCRIPTION' summarizes that
*                           "for message-based sockets, such as SOCK_DGRAM ... the entire message shall be
*                           read in a single operation.  If a message is too long to fit in the supplied
*                           buffer, and MSG_PEEK is not set in the flags argument, the excess bytes shall
*                           be discarded".
*
*                   (b) Thus if the socket's type is datagram & the receive data buffer size is NOT large
*                       enough for the received data, the receive data buffer is maximally filled with receive
*                       data but the remaining data octets are discarded & NET_SOCK_ERR_INVALID_DATA_SIZE
*                       error is returned.
*
*                   See also 'NetSock_RxDataHandler()  Note #2a'.
*
*               (3) If 'flags' argument set to 'NET_SOCK_FLAG_RX_NO_BLOCK'; socket receive does NOT block,
*                   regardless if the socket is configured to block.
*
*               (4) (a) If a pointer to remote address buffer is provided, it is assumed that the remote
*                       address buffer has been previously validated for the remote address to be returned.
*
*                   (b) If a pointer to remote address buffer is provided, it is assumed that a pointer to
*                       an address length buffer is also available & has been previously validated.
*
*                   (c) The remote address is obtained from the first packet buffer.  In other words, if
*                       multiple packet buffers are received for a fragmented datagram, the remote address
*                       is obtained from the first fragment of the datagram.
*
*               (5) (a) Socket address structure 'AddrFamily' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (6) (a) If ...
*
*                       (1) NO IP options were received
*                             OR
*                       (2) NO IP options receive buffer is provided
*                             OR
*                       (3) IP options receive buffer NOT large enough for the received IP options
*
*                       ... then NO IP options are returned & any received IP options are silently discarded.
*
*                   (b) The IP options receive buffer size SHOULD be large enough to receive the maximum
*                       IP options size, NET_IPv4_HDR_OPT_SIZE_MAX.
*
*                   (c) Received IP options should be provided/decoded via appropriate IP layer API.
*
*                   See also Note #14.
*
*               (7) IEEE Std 1003.1, 2004 Edition, Section 'recv() : RETURN VALUE' states that :
*
*                   (a) "Upon successful completion, recv() shall return the length of the message in bytes."
*
*                   (b) "If no messages are available to be received and the peer has performed an orderly
*                        shutdown, recv() shall return 0."
*
*                       (1) Since the socket receive return value of '0' is reserved for socket connection
*                           closes; NO socket receive -- fault or non-fault -- should ever return '0' octets
*                           received.
*
*                       (2) However, since NO actual connections are implemented for datagram-type sockets
*                           (see 'NetSock_ConnHandlerDatagram()  Note #2a'), NO actual socket connections
*                           can be closed on datagram-type sockets.  Therefore, datagram-type socket
*                           receives MUST NEVER return '0'.
*
*                   (c) (1) "Otherwise, -1 shall be returned" ...
*                       (2) "and 'errno' set to indicate the error."
*                           'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   See also 'NetSock_RxDataHandler()  Note #7'.
*
*               (8) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*
*                   However, these pointed-to variables SHOULD already be validated & initialized by
*                   previous NetSock_RxData() function(s).
*
*               (9) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*              (10) Default case already invalidated in NetSock_Open().  However, the default case
*                   is included as an extra precaution in case 'Protocol' is incorrectly modified.
*
*              (11) (a) Transport layer SHOULD typically free &/or discard packet buffer(s) after
*                       receiving application data.
*
*                   (b) However, if received packet buffer(s) NOT consumed AND any socket receive
*                       queue error(s) occur; socket receive MUST free/discard packet buffer(s).
*
*              (12) On ANY error(s), network resources MUST be appropriately freed :
*
*                   (a) NetSock_RxDataHandlerDatagram() assumes that datagram-type sockets have been
*                       previously validated as configured by caller function(s).  Therefore, on any
*                       internal socket connection error(s), the socket MUST be closed.
*
*              (13) Socket descriptor read availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor read handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also  Note #1a,
*                            'NetSock_SelDescHandlerRdDatagram()   Note #3',
*                          & 'NetSock_SelDescHandlerErrDatagram()  Note #3'.
*
*              (14) IP options arguments may NOT be necessary (remove if unnecessary).
*********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_RxDataHandlerDatagram (NET_SOCK_ID          sock_id,
                                                          NET_SOCK            *p_sock,
                                                          void                *p_data_buf,
                                                          CPU_INT16U           data_buf_len,
                                                          NET_SOCK_API_FLAGS   flags,
                                                          NET_SOCK_ADDR       *p_addr_remote,
                                                          NET_SOCK_ADDR_LEN   *p_addr_len,
                                                          void                *p_ip_opts_buf,
                                                          CPU_INT08U           ip_opts_buf_len,
                                                          CPU_INT08U          *p_ip_opts_len,
                                                          RTOS_ERR            *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_SOCK_ADDR_IPv4  *p_addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_SOCK_ADDR_IPv6  *p_addr_ipv6;
#endif
    CPU_BOOLEAN          no_block;
    CPU_BOOLEAN          block;
    CPU_BOOLEAN          peek;
    NET_BUF             *p_buf_head;
    NET_BUF             *p_buf_head_next;
    NET_BUF_HDR         *p_buf_head_hdr;
    NET_BUF_HDR         *p_buf_head_next_hdr;
    NET_FLAGS            flags_transport;
    CPU_INT16U           data_len_tot;
    NET_SOCK_RTN_CODE    rtn_code  = NET_SOCK_BSD_ERR_RX;


                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;

        case NET_SOCK_STATE_CLOSED:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
             goto exit;

        case NET_SOCK_STATE_BOUND:                              /* If sock NOT conn'd to remote addr       ...          */
             if (p_addr_remote == DEF_NULL) {                   /* ... & remote addr rtn buf NOT provided, ...          */
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidOpCtr);
                 RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);      /* ... rtn invalid op err.                              */
                 goto exit;
             }
             break;

        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             p_sock->State = NET_SOCK_STATE_CONN;
             break;

        case NET_SOCK_STATE_CONN:
             break;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_RX);
    }


                                                                /* ---------------- WAIT ON SOCK RX Q ----------------- */
    no_block = DEF_BIT_IS_SET((NET_SOCK_FLAGS)flags, NET_SOCK_FLAG_RX_NO_BLOCK);
    if (no_block == DEF_YES) {                                  /* If 'No Block' flag set,         ...                  */
        block = DEF_NO;                                         /* ... do NOT block (see Note #3); ...                  */
    } else {                                                    /* ... else chk sock's no-block flag.                   */
        block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    }

    if (block != DEF_YES) {                                     /* If non-blocking sock rx  ...                         */
        if (p_sock->RxQ_Head == DEF_NULL) {                     /* ... & no rx'd data pkts, ...                         */
            RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_BLOCK);             /* ... rtn  rx Q empty err.                             */
            goto exit;
        }
    }

    Net_GlobalLockRelease();
    NetSock_RxQ_Wait(p_sock, p_err);
    Net_GlobalLockAcquire((void *)NetSock_RxDataHandlerDatagram);

    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

                                                                /* Cfg rx pkt buf ptrs.                                 */
    p_buf_head      =  p_sock->RxQ_Head;
    p_buf_head_hdr  = &p_buf_head->Hdr;
    p_buf_head_next =  p_buf_head_hdr->NextPrimListPtr;


                                                                /* ----------------- GET REMOTE ADDR ------------------ */
    if (p_addr_remote != DEF_NULL) {                            /* If remote addr buf avail, ...                        */
                                                                /* ... rtn datagram src addr (see Note #4).             */
        if (DEF_BIT_IS_CLR(p_buf_head_hdr->Flags, NET_BUF_FLAG_IPv6_FRAME)) {
#ifdef  NET_IPv4_MODULE_EN
            p_addr_ipv4  = (NET_SOCK_ADDR_IPv4 *)p_addr_remote; /* Cfg remote addr struct    (see Note #5).             */
            NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv4->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V4);
            NET_UTIL_VAL_COPY_SET_NET_16(&p_addr_ipv4->Port,  &p_buf_head_hdr->TransportPortSrc);
            NET_UTIL_VAL_COPY_SET_NET_32(&p_addr_ipv4->Addr,  &p_buf_head_hdr->IP_AddrSrc     );
            Mem_Clr(&p_addr_ipv4->Unused[0],
                     NET_SOCK_ADDR_IPv4_NBR_OCTETS_UNUSED);

           *p_addr_len = (NET_SOCK_ADDR_LEN )NET_SOCK_ADDR_IPv4_SIZE; /* See Note  #4b.                                 */
#endif

        } else {
#ifdef  NET_IPv6_MODULE_EN
            p_addr_ipv6  = (NET_SOCK_ADDR_IPv6 *)p_addr_remote; /* Cfg remote addr struct    (see Note #5).             */
            NET_UTIL_VAL_SET_HOST_16(&p_addr_ipv6->AddrFamily, NET_SOCK_ADDR_FAMILY_IP_V6);
            NET_UTIL_VAL_COPY_SET_NET_16(&p_addr_ipv6->Port,  &p_buf_head_hdr->TransportPortSrc);
            p_addr_ipv6->Addr = p_buf_head_hdr->IPv6_AddrSrc;

           *p_addr_len = (NET_SOCK_ADDR_LEN )NET_SOCK_ADDR_IPv6_SIZE; /* See Note  #4b.                                 */
#endif
       }
    }

                                                                /* ------------------- CFG SOCK RX -------------------- */
                                                                /* Cfg sock rx flags.                                   */
    peek = DEF_BIT_IS_SET((NET_SOCK_FLAGS)flags, NET_SOCK_FLAG_RX_DATA_PEEK);


                                                                /* ------------------- RX SOCK DATA ------------------- */
    data_len_tot = 0u;

    switch (p_sock->Protocol) {                                 /* Rx app data from transport layer rx.                 */
        case NET_SOCK_PROTOCOL_UDP:
                                                                /* Cfg transport rx flags.                              */
             flags_transport = NET_UDP_FLAG_NONE;
             if (peek == DEF_YES) {
                 DEF_BIT_SET(flags_transport, NET_UDP_FLAG_RX_DATA_PEEK);
             }

             data_len_tot = NetUDP_RxAppData(p_buf_head,
                                             p_data_buf,
                                             data_buf_len,
                                             flags_transport,
                                             p_ip_opts_buf,
                                             ip_opts_buf_len,
                                             p_ip_opts_len,
                                             p_err);
             switch (RTOS_ERR_CODE_GET(*p_err)) {
                 case RTOS_ERR_NONE:
                 case RTOS_ERR_WOULD_OVF:
                      break;

                 default:
                      goto exit;
             }
             break;


        case NET_SOCK_PROTOCOL_NONE:
        default:                                                /* See Notes #10 & #12a.                                */
             PP_UNUSED_PARAM(flags_transport);                  /* Prevent possible 'variable unused' warnings.         */
             PP_UNUSED_PARAM(peek);
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_RX);
    }


                                                                /* ---------------- FREE SOCK RX PKTS ----------------- */
    if (peek == DEF_YES) {                                      /* Signal sock rx Q to negate non-consuming peek.       */
        NetSock_RxQ_Signal(p_sock);
    }

    if (peek != DEF_YES) {                                      /* If peek opt NOT req'd, pkt buf(s) consumed : ...     */
        if (p_buf_head_next != DEF_NULL) {                      /* ... if rem'ing rx Q non-empty,               ...     */
                                                                /* ... unlink from prev'ly q'd pkt buf(s);      ...     */
            p_buf_head_next_hdr                  = &p_buf_head_next->Hdr;
            p_buf_head_next_hdr->PrevPrimListPtr =  DEF_NULL;
        }
                                                                /* ... & set new sock rx Q head.                        */
        p_sock->RxQ_Head = p_buf_head_next;
        if (p_sock->RxQ_Head == DEF_NULL) {                     /* If head now  points to NULL, ..                      */
            p_sock->RxQ_Tail  = DEF_NULL;                       /* .. tail also points to NULL.                         */
        }

        if (p_sock->RxQ_SizeCur >  (NET_SOCK_DATA_SIZE)data_len_tot) { /* If cur rx Q size >  tot data len, ..          */
            p_sock->RxQ_SizeCur -= (NET_SOCK_DATA_SIZE)data_len_tot; /* .. dec rx Q size by tot data len.               */
        } else {                                                /* Else lim to min rx Q size.                           */
            p_sock->RxQ_SizeCur  = 0u;
        }
    }

    rtn_code = (NET_SOCK_RTN_CODE)data_len_tot;

    PP_UNUSED_PARAM(sock_id);

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                       NetSock_TxDataHandler()
*
* Description : (1) Transmit data through a socket :
*
*                   (a) Validate transmit data                                          See Note #2
*                   (b) Validate transmit flags                                         See Note #3
*                   (c) Acquire  network  lock
*                   (d) Validate socket   used
*                   (e) Validate remote   address                                       See Note #4
*                   (f) Transmit socket   data
*                   (g) Release  network  lock
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to transmit data.
*
*               p_data          Pointer to application data to transmit.
*
*               data_len        Length of  application data to transmit (in octets) [see Note #2].
*
*               flags           Flags to select transmit options (see Note #3); bit-field flags logically OR'd :
*
*                                   NET_SOCK_FLAG_NONE              No socket flags selected.
*                                   NET_SOCK_FLAG_TX_NO_BLOCK       Transmit socket data without blocking.
*
*               p_addr_remote   Pointer to destination address buffer (see Note #4).
*
*               addr_len        Length of  destination address buffer (in octets).
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of positive data octets transmitted, if NO error(s)              [see Note #5a1].
*
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED,          if socket connection closed (see Note #5b).
*
*               NET_SOCK_BSD_ERR_TX,                        otherwise                   (see Note #5a2A).
*
* Note(s)     : (2) (a) (1) (A) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                               single, complete datagram transmitted MUST be received as a single, complete
*                               datagram.  Thus each call to transmit data MUST be transmitted in a single,
*                               complete datagram.
*
*                           (B) (1) IEEE Std 1003.1, 2004 Edition, Section 'send() : DESCRIPTION' states
*                                   that "if the message is too long to pass through the underlying protocol,
*                                   send() shall fail and no data shall be transmitted".
*
*                               (2) Since IP transmit fragmentation is NOT currently supported (see 'net_ip.h
*                                   Note #1d'), if the socket's type is datagram & the requested transmit
*                                   data length is greater than the socket/transport layer MTU, then NO data
*                                   is transmitted & NET_SOCK_ERR_INVALID_DATA_SIZE error is returned.
*
*                       (2) (A) (1) Stream-type sockets transmit & receive all data octets in one or more
*                                   non-distinct packets.  In other words, the application data is NOT
*                                   bounded by any specific packet(s); rather, it is contiguous & sequenced
*                                   from one packet to the next.
*
*                               (2) Thus if the socket's type is stream & the socket's transmit data queue(s)
*                                   are NOT large enough for the transmitted data, the  transmit data queue(s)
*                                   are maximally filled with transmit data & the remaining data octets are
*                                   discarded but may be re-transmitted by later application-socket transmits.
*
*                               (3) Therefore, NO stream-type socket transmit data length should be "too long
*                                   to pass through the underlying protocol" & cause the socket transmit to
*                                   "fail ... [with] no data ... transmitted" (see Note #2a1B1).
*
*                           (B) Thus it is typical -- but NOT absolutely required -- that a single application
*                               task ONLY transmit or request to transmit data to a stream-type socket.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*                   See also 'NetSock_TxDataHandlerDatagram()  Note #2',
*                          & 'NetSock_TxDataHandlerStream()    Note #3'.
*
*               (3) Only some socket transmit flag options are implemented.  If other flag options are
*                   requested, NetSock_TxData() handler function(s) abort & return appropriate error codes
*                   so that requested flag options are NOT silently ignored.
*
*               (4) (a) Socket address structure 'AddrFamily' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*
*                   See also 'net_sock.h  NETWORK SOCKET ADDRESS DATA TYPES  Note #2'.
*
*               (5) (a) IEEE Std 1003.1, 2004 Edition, Section 'send() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, send() shall return the number of bytes sent."
*
*                           (A) Section 'send() : DESCRIPTION' elaborates that "successful completion
*                               of a call to sendto() does not guarantee delivery of the message".
*
*                           (B) (1) Thus applications SHOULD verify the actual returned number of data
*                                   octets transmitted &/or prepared for transmission.
*
*                               (2) In addition, applications MAY desire verification of receipt &/or
*                                   acknowledgement of transmitted data to the remote host -- either
*                                   inherently by the transport layer or explicitly by the application.
*
*                       (2) (A) "Otherwise, -1 shall be returned" ...
*                               (1) Section 'send() : DESCRIPTION' elaborates that "a return value of
*                                   -1 indicates only locally-detected errors".
*
*                           (B) "and 'errno' set to indicate the error."
*                               'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   (b) Although NO socket send() specification states to return '0' when the socket's
*                       connection is closed, it seems reasonable to return '0' since it is possible for the
*                       socket connection to be close()'d or shutdown() by the remote host.
*
*                       (1) Since the socket transmit return value of '0' is reserved for socket connection
*                           closes; NO socket transmit -- fault or non-fault -- should ever return '0' octets
*                           transmitted.
*
*                   See also 'NetSock_TxDataHandlerDatagram()  Note #6'
*                          & 'NetSock_TxDataHandlerStream()    Note #5'.
*
*               (6) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'SockType' is incorrectly modified.
**********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_TxDataHandler (NET_SOCK_ID          sock_id,
                                                  void                *p_data,
                                                  CPU_INT16U           data_len,
                                                  NET_SOCK_API_FLAGS   flags,
                                                  NET_SOCK_ADDR       *p_addr_remote,
                                                  NET_SOCK_ADDR_LEN    addr_len,
                                                  RTOS_ERR            *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
   CPU_BOOLEAN         valid;
   CPU_BOOLEAN         is_used;
   NET_SOCK_FLAGS      flag_mask;
#endif
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
   CPU_BOOLEAN         secure;
#endif
   NET_SOCK           *p_sock;
   NET_SOCK_RTN_CODE   rtn_code = NET_SOCK_BSD_ERR_TX;


   RTOS_ASSERT_DBG_ERR_SET((p_data != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, NET_SOCK_BSD_ERR_TX);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* ---------------- VALIDATE TX FLAGS ----------------- */
   flag_mask = NET_SOCK_FLAG_NONE       |
               NET_SOCK_FLAG_TX_NO_BLOCK;
                                                                /* If any invalid flags req'd, rtn err (see Note #3).   */
   if ((flags & ~flag_mask) != NET_SOCK_FLAG_NONE) {
       NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFlagsCtr);
       RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_TX);
   }
#endif

                                                                /* ----------------- ACQUIRE NET LOCK ----------------- */
   Net_GlobalLockAcquire((void *)NetSock_TxDataHandler);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit_release;
    }
#endif


   p_sock = &NetSock_Tbl[sock_id];

#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* --------------- VALIDATE REMOTE ADDR --------------- */
   if (p_addr_remote != DEF_NULL) {
       valid = NetSock_IsValidAddrRemote(p_addr_remote, addr_len, p_sock, p_err);
       if (valid != DEF_YES) {
           goto exit_release;
       }
   }
#else
   PP_UNUSED_PARAM(addr_len);                                   /* Prevent 'variable unused' compiler warning.          */
#endif


                                                                /* ------------------- TX SOCK DATA ------------------- */
   switch (p_sock->SockType) {
       case NET_SOCK_TYPE_DATAGRAM:
            rtn_code = NetSock_TxDataHandlerDatagram(sock_id,
                                                     p_sock,
                                                     p_data,
                                                     data_len,
                                                     flags,
                                                     p_addr_remote,
                                                     p_err);
            if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                goto exit_release;
            }
            break;


       case NET_SOCK_TYPE_STREAM:
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
#ifdef  NET_SECURE_MODULE_EN
            secure = DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE);
#else
            secure = DEF_NO;
#endif
            if (secure != DEF_YES) {
                rtn_code = NetSock_TxDataHandlerStream(sock_id,
                                                       p_sock,
                                                       p_data,
                                                       data_len,
                                                       flags,
                                                       p_err);
                if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                    goto exit_release;
                }
            } else {
#ifdef  NET_SECURE_MODULE_EN                               /* If sock secure, tx data via secure handler.          */
                rtn_code = NetSecure_SockTxDataHandler(p_sock,
                                                       p_data,
                                                       data_len,
                                                       p_err);
                if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                    goto exit_release;
                }
#else
                Net_GlobalLockRelease();
                RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_TX);
#endif
            }
#else
            Net_GlobalLockRelease();
            RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_TX);
#endif
            break;



       case NET_SOCK_TYPE_NONE:
       default:                                                 /* See Note #6.                                         */
            NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
            goto exit_release;
   }


exit_release:
                                                                /* ----------------- RELEASE NET LOCK ----------------- */
   Net_GlobalLockRelease();

   return (rtn_code);
}


/*
*********************************************************************************************************
*                                   NetSock_TxDataHandlerDatagram()
*
* Description : (1) Transmit data through a datagram-type socket :
*
*                   (a) Validate  socket connection state                               See Note #10
*                   (b) Configure socket transmit :
*                       (1) Configure source/destination addresses for transmit
*                       (2) Configure socket flags
*                   (c) Transmit  socket data via appropriate transport layer
*                   (d) Return    socket data transmitted length
*
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to receive data.
*               -------         Argument checked   in NetSock_TxDataHandler().
*
*               p_sock          Pointer to a socket.
*               ------          Argument validated in NetSock_TxDataHandler().
*
*               p_data          Pointer to application data.
*               ------          Argument checked   in NetSock_TxDataHandler().
*
*               data_len        Length of  application data (in octets) [see Note #2].
*               --------        Argument checked   in NetSock_TxDataHandler().
*
*               flags           Flags to select transmit options; bit-field flags logically OR'd :
*               -----
*                                   NET_SOCK_FLAG_NONE              No socket flags selected.
*                                   NET_SOCK_FLAG_TX_NO_BLOCK       Transmit socket data without blocking
*                                                                       (see Note #3).
*
*                               Argument checked   in NetSock_TxDataHandler().
*
*               p_addr_remote   Pointer to destination address buffer (see Note #4).
*               -------------   Argument checked   in NetSock_TxDataHandler().
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of positive data octets transmitted, if NO error(s)              [see Note #6a1].
*
*               NET_SOCK_BSD_RTN_CODE_CONN_CLOSED,          if socket connection closed (see Note #6b).
*
*               NET_SOCK_BSD_ERR_TX,                        otherwise                   (see Note #6a2A).
*
* Note(s)     : (2) (a) (1) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                           single, complete datagram transmitted MUST be received as a single, complete
*                           datagram.  Thus each call to transmit data MUST be transmitted in a single,
*                           complete datagram.
*
*                       (2) (A) IEEE Std 1003.1, 2004 Edition, Section 'send() : DESCRIPTION' states that
*                               "if the message is too long to pass through the underlying protocol, send()
*                               shall fail and no data shall be transmitted".
*
*                           (B) Since IP transmit fragmentation is NOT currently supported (see 'net_ip.h
*                               Note #1d'), if the socket's type is datagram & the requested transmit
*                               data length is greater than the socket/transport layer MTU, then NO data
*                               is transmitted & NET_SOCK_ERR_INVALID_DATA_SIZE error is returned.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*                   See also 'NetSock_TxDataHandler()  Note #2a'.
*
*               (3) If 'flags' argument set to 'NET_SOCK_FLAG_TX_NO_BLOCK'; socket transmit does NOT configure
*                   the transport layer transmit to block, regardless if the socket is configured to block.
*
*               (4) If a pointer to remote address buffer is provided, it is assumed that the remote
*                   address buffer & remote address buffer length have been previously validated.
*
*               (5) (a) A socket's local address MUST be available in order to transmit.
*
*                   (b) If a protocol's wildcard address is specified in the socket connection's
*                       local address structure, use the default host address on the default interface
*                       when transmitting packets.
*
*               (6) (a) IEEE Std 1003.1, 2004 Edition, Section 'send() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, send() shall return the number of bytes sent."
*
*                           (A) Section 'send() : DESCRIPTION' elaborates that "successful completion
*                               of a call to sendto() does not guarantee delivery of the message".
*
*                           (B) (1) Thus applications SHOULD verify the actual returned number of data
*                                   octets transmitted &/or prepared for transmission.
*
*                               (2) In addition, applications MAY desire verification of receipt &/or
*                                   acknowledgement of transmitted data to the remote host -- either
*                                   inherently by the transport layer or explicitly by the application.
*
*                       (2) (A) "Otherwise, -1 shall be returned" ...
*                               (1) Section 'send() : DESCRIPTION' elaborates that "a return value of
*                                   -1 indicates only locally-detected errors".
*
*                           (B) "and 'errno' set to indicate the error."
*                               'errno' NOT currently supported (see 'net_bsd.h  Note #1b').
*
*                   (b) Although NO socket send() specification states to return '0' when the socket's
*                       connection is closed, it seems reasonable to return '0' since it is possible for the
*                       socket connection to be close()'d or shutdown() by the remote host.
*
*                       (1) Since the socket transmit return value of '0' is reserved for socket connection
*                           closes; NO socket transmit -- fault or non-fault -- should ever return '0' octets
*                           transmitted.
*
*                       (2) However, since NO actual connections are implemented for datagram-type sockets
*                           (see 'NetSock_ConnHandlerDatagram()  Note #2a'), NO actual socket connections
*                           can be closed on datagram-type sockets.  Therefore, datagram-type socket
*                           transmits MUST NEVER return '0'.
*
*                   See also 'NetSock_TxDataHandler()  Note #5'.
*
*               (7) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (8) Default case already invalidated in NetSock_Open().  However, the default case
*                   is included as an extra precaution in case 'Protocol' is incorrectly modified.
*
*               (9) On ANY error(s), network resources MUST be appropriately freed :
*
*                   (a) NetSock_TxDataHandlerDatagram() assumes that internal socket configuration
*                       has been previously validated by caller function(s).  Therefore, on any
*                       internal socket configuration error(s), the socket MUST be closed.
*
*                   (b) NetSock_TxDataHandlerDatagram() assumes that any internal socket connection
*                       error(s) on datagram-type sockets may NOT be critical &/or may be transitory;
*                       thus NO network or socket resource(s) are closed/freed.
*
*                   (c) Since transport layer error(s) may NOT be critical &/or may be transitory, NO
*                       network or socket resource(s) are closed/freed.
*
*              (10) Socket descriptor write availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor write handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also  Note #1a,
*                            'NetSock_SelDescHandlerWrDatagram()   Note #3',
*                          & 'NetSock_SelDescHandlerErrDatagram()  Note #3'.
*
*              (11) (a) RFC #1122, Section 4.1.4 states that "an application-layer program MUST be able
*                       to set the TTL and TOS values as well as IP options for sending a ... datagram,
*                       and these values must be passed transparently to the IP layer".
*
*                   (b) IP transmit options currently NOT implemented
*
*              (12) 'sock_id' may NOT be necessary but is included for consistency.
*********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_TxDataHandlerDatagram (NET_SOCK_ID          sock_id,
                                                          NET_SOCK            *p_sock,
                                                          void                *p_data,
                                                          CPU_INT16U           data_len,
                                                          NET_SOCK_API_FLAGS   flags,
                                                          NET_SOCK_ADDR       *p_addr_remote,
                                                          RTOS_ERR            *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
           NET_SOCK_ADDR_IPv4      *p_addr_ipv4;
           NET_IPv4_ADDR_OBJ       *p_addr_obj_ipv4;
           NET_IPv4_ADDR            src_addrv4;
           NET_IPv4_ADDR            src_addr_hostv4;
           NET_IPv4_ADDR            dest_addrv4;
#if 0
           CPU_BOOLEAN              addr_initv4;
#endif
           NET_IPv4_TOS             TOS;
           NET_IPv4_TTL             TTL;
           NET_IPv4_FLAGS           flags_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
           NET_SOCK_ADDR_IPv6      *p_addr_ipv6;
    const  NET_IPv6_ADDR_OBJ          *p_ipv6_addrs;
           NET_IPv6_ADDR            src_addrv6;
           NET_IPv6_ADDR            dest_addrv6;
           CPU_BOOLEAN              is_addr_wildcard;
           NET_IPv6_TRAFFIC_CLASS   traffic_class;
           NET_IPv6_FLOW_LABEL      flow_label;
           NET_IPv6_HOP_LIM         hop_lim;
           NET_IPv6_FLAGS           flags_ipv6;
#endif
#ifdef  NET_MCAST_TX_MODULE_EN
           CPU_BOOLEAN             addr_dest_multicast = DEF_NO;
#endif
           CPU_BOOLEAN              no_block;
           CPU_BOOLEAN              block;
           NET_CONN_ID              conn_id;
           NET_IF_NBR               if_nbr;
           CPU_INT08U               addr_local[NET_SOCK_ADDR_LEN_MAX];
           CPU_INT08U               addr_remote[NET_SOCK_ADDR_LEN_MAX];
           NET_CONN_ADDR_LEN        addr_len;
           NET_PORT_NBR             src_port;
           NET_PORT_NBR             dest_port;
           NET_FLAGS                flags_transport;
           CPU_INT16U               data_len_tot = 0;
           NET_SOCK_RTN_CODE        rtn_code     = NET_SOCK_BSD_ERR_TX;


    PP_UNUSED_PARAM(sock_id);                                   /* Prevent 'variable unused' warning (see Note #12).    */

                                                                /* ----------------- VALIDATE TX DATA ----------------- */
    if (data_len > p_sock->TxQ_SizeCfgd) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_WOULD_OVF);
        goto exit;
    }

                                                                /* ------------- VALIDATE SOCK CONN STATE ------------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NotUsedCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);
             goto exit;

        case NET_SOCK_STATE_CLOSED:                             /* If sock closed, bind to random local addr.           */
            (void)NetSock_BindHandler(sock_id,
                                      DEF_NULL,
                                      0,
                                      DEF_YES,
                                      p_addr_remote,
                                      p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  goto exit;
             }
             break;

        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_CONN:
             break;

        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
             p_sock->State = NET_SOCK_STATE_CONN;
             break;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidStateCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, NET_SOCK_BSD_ERR_TX);
    }


                                                                /* -------------- CFG TX SRC/DEST ADDRS --------------- */
    conn_id  = p_sock->ID_Conn;

                                                                /* Cfg remote/dest addr.                                */
    if (p_addr_remote == DEF_NULL) {                            /* If remote addr NOT provided, ...                     */
                                                                /* ... get sock conn's remote addr.                     */
        addr_len = sizeof(addr_remote);
        NetConn_AddrRemoteGet(conn_id,
                             &addr_remote[0],
                             &addr_len,
                              p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
             goto exit;
        }


        switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
            case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                 NET_UTIL_VAL_COPY_GET_NET_16(&dest_port,   &addr_remote[NET_SOCK_ADDR_IP_IX_PORT]);
                 NET_UTIL_VAL_COPY_GET_NET_32(&dest_addrv4, &addr_remote[NET_SOCK_ADDR_IP_V4_IX_ADDR]);
                 break;
#endif

#ifdef  NET_IPv6_MODULE_EN
            case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                 NET_UTIL_VAL_COPY_GET_NET_16(&dest_port,   &addr_remote[NET_SOCK_ADDR_IP_IX_PORT]);
                 Mem_Copy(&dest_addrv6,
                          &addr_remote[NET_SOCK_ADDR_IP_V6_IX_ADDR],
                           NET_IPv6_ADDR_SIZE);
                 break;
#endif

            default:
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                 RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_TX);
        }


    } else {                                                    /* Else cfg remote addr (see Note #4).                  */

        switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
            case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
                 p_addr_ipv4 = (NET_SOCK_ADDR_IPv4 *)p_addr_remote;
                 NET_UTIL_VAL_COPY_GET_NET_16(&dest_port,   &p_addr_ipv4->Port);
                 NET_UTIL_VAL_COPY_GET_NET_32(&dest_addrv4, &p_addr_ipv4->Addr);

#ifdef  NET_MCAST_TX_MODULE_EN
                 addr_dest_multicast = NetIPv4_IsAddrMulticast(dest_addrv4);
#endif
                 break;
#endif

#ifdef  NET_IPv6_MODULE_EN
            case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
                 p_addr_ipv6  = (NET_SOCK_ADDR_IPv6 *)p_addr_remote;
                 NET_UTIL_VAL_COPY_GET_NET_16(&dest_port, &p_addr_ipv6->Port);
                 dest_addrv6 = p_addr_ipv6->Addr;

#ifdef  NET_MCAST_TX_MODULE_EN
                 addr_dest_multicast = NetIPv6_IsAddrMcast(&dest_addrv6);
#endif
                 break;
#endif

            default:
                 NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                 RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_TX);
        }


    }

                                                                /* Cfg local/src addr (see Note #5).                    */
    addr_len = sizeof(addr_local);
    NetConn_AddrLocalGet(conn_id,
                        &addr_local[0],
                        &addr_len,
                         p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }


    if_nbr = p_sock->IF_Nbr;                            /* Set the IF to the one specified by the socket.       */
    if (if_nbr == NET_IF_NBR_NONE) {                    /* IF not IF is defined in the socket structure, ...    */
        if_nbr  = NetIF_GetDflt();                      /* ... get the default IF.                              */

    }

    switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             NET_UTIL_VAL_COPY_GET_NET_16(&src_port,   &addr_local[NET_SOCK_ADDR_IP_IX_PORT]);
             NET_UTIL_VAL_COPY_GET_NET_32(&src_addrv4, &addr_local[NET_SOCK_ADDR_IP_V4_IX_ADDR]);

             if (src_addrv4 == NET_SOCK_ADDR_IP_V4_WILDCARD) {  /* If wildcard addr,                                ... */
                                                                /* .... check if it's for address configuration (DHCP)  */
                 p_addr_obj_ipv4 = NetIPv4_GetAddrObjCfgdOnIF(if_nbr, src_addrv4);
                 if (p_addr_obj_ipv4 != DEF_NULL) {
                     if (p_addr_obj_ipv4->CfgMode != NET_IP_ADDR_CFG_MODE_DYN_INIT) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_ADDR_SRC);
                         goto exit;
                     }
                 } else {
                     src_addr_hostv4 = NetIPv4_GetAddrSrcHandler(&if_nbr, dest_addrv4, p_err);
                     if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                          goto exit;
                     }
                     if (src_addr_hostv4 == NET_IPv4_ADDR_NONE) {
                         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_ADDR_SRC);
                         goto exit;
                     }
                     src_addrv4  = src_addr_hostv4;             /* ... else subst cfg'd host addr for src addr.         */
                 }
             }
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
             NET_UTIL_VAL_COPY_GET_NET_16(&src_port, &addr_local[NET_SOCK_ADDR_IP_IX_PORT]);
             Mem_Copy(&src_addrv6,
                      &addr_local[NET_SOCK_ADDR_IP_V6_IX_ADDR],
                       NET_IPv6_ADDR_SIZE);

             is_addr_wildcard = Mem_Cmp(&src_addrv6, &NetIPv6_AddrWildcard, NET_IPv6_ADDR_SIZE);

             if (is_addr_wildcard) {                            /* If wildcard addr,                                ... */

                 p_ipv6_addrs = NetIPv6_GetAddrSrcHandler(                        &if_nbr,
                                                          (const  NET_IPv6_ADDR *)&src_addrv6,
                                                          (const  NET_IPv6_ADDR *)&dest_addrv6,
                                                                                   DEF_NULL,
                                                                                   p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                      goto exit;
                 }
                 if (p_ipv6_addrs == DEF_NULL) {
                     RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_ADDR_SRC);
                     goto exit;                                 /* No cfgd address to use as src addr.                  */
                 }
                                                                /* ... subst cfg'd host addr for remote addr, if avail; */
                 Mem_Copy(&src_addrv6, &p_ipv6_addrs->AddrHost, NET_IPv6_ADDR_SIZE);

             }
             break;
#endif

        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_TX);
    }


                                                                /* ------------------- CFG SOCK TX -------------------- */
                                                                /* Cfg sock tx flags.                                   */
    no_block = DEF_BIT_IS_SET((NET_SOCK_FLAGS)flags, NET_SOCK_FLAG_TX_NO_BLOCK);
    if (no_block == DEF_YES) {                                  /* If 'No Block' flag set,         ...                  */
        block = DEF_NO;                                         /* ... do NOT block (see Note #3); ...                  */
    } else {                                                    /* ... else chk sock's no-block flag.                   */
        block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
    }


                                                                /* ------------------- TX SOCK DATA ------------------- */
    data_len_tot = 0u;

    switch (p_sock->Protocol) {                                  /* Tx app data via transport layer tx.                  */
        case NET_SOCK_PROTOCOL_UDP:
                                                                /* Cfg transport tx flags.                              */
             flags_transport = NET_UDP_FLAG_NONE;
             if (block == DEF_YES) {
                 DEF_BIT_SET(flags_transport, NET_UDP_FLAG_TX_BLOCK);
             }

             switch (p_sock->ProtocolFamily) {
#ifdef  NET_IPv4_MODULE_EN
                 case NET_SOCK_PROTOCOL_FAMILY_IP_V4:

                                                                /* Prepare IP params.                                   */
                    NetConn_IPv4TxParamsGet(conn_id, &flags_ipv4, &TOS, &TTL);

#ifdef  NET_MCAST_TX_MODULE_EN
                     if (addr_dest_multicast == DEF_YES) {      /* If      multicast dest addr, ...                     */
                                                                /* ... get multicast TTL.                               */
                         TTL = NetConn_IPv4TxTTL_MulticastGet(conn_id);
                     }
#endif

                                                                /* See Note #11b.                                       */
                      data_len_tot = NetUDP_TxAppDataHandlerIPv4(if_nbr,
                                                                 p_data,
                                                                 data_len,
                                                                 src_addrv4,
                                                                 src_port,
                                                                 dest_addrv4,
                                                                 dest_port,
                                                                 TOS,
                                                                 TTL,
                                                                 flags_transport,
                                                                 flags_ipv4,
                                                                 DEF_NULL,
                                                                 p_err);
                      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                           goto exit;
                      }
                      break;
#endif

#ifdef  NET_IPv6_MODULE_EN
                 case NET_SOCK_PROTOCOL_FAMILY_IP_V6:

                                                                /* Prepare IP params.                                   */
                    NetConn_IPv6TxParamsGet(conn_id,
                                            &traffic_class,
                                            &flow_label,
                                            &hop_lim,
                                            &flags_ipv6);

#ifdef  NET_MCAST_TX_MODULE_EN
                     if (addr_dest_multicast == DEF_YES) {                      /* If      multicast dest addr, ...     */
                         hop_lim = NetConn_IPv6TxHopLimMcastGet(conn_id);       /* ... get multicast TTL.               */
                     }
#endif

                                                                /* See Note #11b.                                       */
                      data_len_tot = NetUDP_TxAppDataHandlerIPv6(p_data,
                                                                 data_len,
                                                                &src_addrv6,
                                                                 src_port,
                                                                &dest_addrv6,
                                                                 dest_port,
                                                                 traffic_class,
                                                                 flow_label,
                                                                 hop_lim,
                                                                 flags_transport,
                                                                 p_err);
                      if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                           goto exit;
                      }
                      break;
#endif

                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidFamilyCtr);
                      RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_TX);
             }
             break;


        case NET_SOCK_PROTOCOL_NONE:
        default:                                                /* See Notes #8 & #9a.                                  */
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, NET_SOCK_BSD_ERR_TX);
    }


                                                                /* -------------- RTN TX'D SOCK DATA LEN -------------- */
    if (data_len_tot <= NET_SOCK_DATA_SIZE_MIN) {               /* If tx'd data len < 1, ...                            */
        RTOS_ERR_SET(*p_err, RTOS_ERR_FAIL);                        /* ... rtn invalid tx data size err (see Note #6b1).    */
        rtn_code = NET_SOCK_BSD_ERR_TX;
        goto exit;
    }

    rtn_code = (NET_SOCK_RTN_CODE)data_len_tot;

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                       NetSock_SelDescHandler()
*
* Description : Handle socket descriptor for operation(s) :
*
* Argument(s) : sock_nbr_max        Maximum number of socket descriptors/handle identifiers in the socket.
*
*               p_sock_desc_rd      Pointer to the read set of socket descriptors/handle identifiers.
*
*               p_sock_desc_wr      Pointer to the write set of socket descriptors/handle identifiers.
*
*               p_sock_desc_err     Pointer to the error set of socket descriptors/handle identifiers.
*
*               p_err               Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of socket descriptors with available resources &/or operations, if any.
*               NET_SOCK_BSD_RTN_CODE_TIMEOUT, on timeout.
*               NET_SOCK_BSD_ERR_SEL, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  NET_SOCK_QTY  NetSock_SelDescHandler (NET_SOCK_QTY    sock_nbr_max,
                                              NET_SOCK_DESC  *p_sock_desc_rd,
                                              NET_SOCK_DESC  *p_sock_desc_wr,
                                              NET_SOCK_DESC  *p_sock_desc_err,
                                              RTOS_ERR       *p_err)
{
    NET_SOCK_ID    sock_id;
    NET_SOCK_DESC  sock_desc_rtn_rd;
    NET_SOCK_DESC  sock_desc_rtn_wr;
    NET_SOCK_DESC  sock_desc_rtn_err;
    NET_SOCK_QTY   sock_nbr_rdy      = 0u;
    CPU_BOOLEAN    sock_desc_used;
    CPU_BOOLEAN    sock_rdy;
    RTOS_ERR       local_err;


    if (p_sock_desc_rd != DEF_NULL) {                           /* If avail, chk rd sock desc's (see Note #3b2A1).      */
                                                                /* Copy    req'd rd sock desc's (see Note #3c2A1).      */
        NET_SOCK_DESC_COPY(&sock_desc_rtn_rd, p_sock_desc_rd);
                                                                /* Chk ALL avail rd sock desc's (see Note #3b1).        */
        for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {
             sock_desc_used = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_rd);
             if (sock_desc_used != 0) {                         /* If rd sock desc req'd, ...                           */
                                                                /* ... chk/cfg for rdy rd op(s) [see Note #3b2A1].      */
                 RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                 sock_rdy = NetSock_SelDescHandlerRd(sock_id, &local_err);
                 switch (RTOS_ERR_CODE_GET(local_err)) {
                     case RTOS_ERR_NONE:
                     case RTOS_ERR_NET_CONN_CLOSED_FAULT:
                     case RTOS_ERR_NET_INVALID_CONN:
                     case RTOS_ERR_INVALID_STATE:
                          if (sock_rdy != DEF_YES) {            /* If sock NOT rdy, ...                                 */
                                                                /* ... clr from rtn rd sock desc's (see #3b2B1b2B).     */
                              NET_SOCK_DESC_CLR(sock_id, &sock_desc_rtn_rd);
                          } else {                              /* Else inc nbr rdy    sock desc's (see #3b2B1a2).      */
                              sock_nbr_rdy++;
                          }
                          break;

                     case RTOS_ERR_NOT_INIT:
                     case RTOS_ERR_INVALID_TYPE:                /* See Notes #3a2A1 & #3d1.                             */
                     case RTOS_ERR_INVALID_HANDLE:
                     default:
                          RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(local_err));
                          goto exit_err;
                 }
             }
        }

    } else {                                                    /* Else if NO  rd sock desc's avail, ...                */
        NET_SOCK_DESC_INIT(&sock_desc_rtn_rd);                  /* ... clr rtn rd sock desc's.                          */
    }


    if (p_sock_desc_wr != DEF_NULL) {                           /* If avail, chk wr sock desc's (see Note #3b2A2).      */
                                                                /* Copy    req'd wr sock desc's (see Note #3c2A1).      */
        NET_SOCK_DESC_COPY(&sock_desc_rtn_wr, p_sock_desc_wr);
                                                                /* Chk ALL avail wr sock desc's (see Note #3b1).        */
        for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {
             sock_desc_used = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_wr);
             if (sock_desc_used != 0) {                         /* If wr sock desc req'd, ...                           */
                                                                /* ... chk/cfg for rdy wr op(s) [see Note #3b2A2].      */
                 RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                 sock_rdy = NetSock_SelDescHandlerWr(sock_id, &local_err);
                 switch (RTOS_ERR_CODE_GET(local_err)) {
                     case RTOS_ERR_NONE:
                     case RTOS_ERR_NET_CONN_CLOSED_FAULT:
                     case RTOS_ERR_NET_INVALID_CONN:
                     case RTOS_ERR_INVALID_STATE:
                          if (sock_rdy != DEF_YES) {            /* If sock NOT rdy, ...                                 */
                                                                /* ... clr from rtn wr sock desc's (see #3b2B1b2B).     */
                              NET_SOCK_DESC_CLR(sock_id, &sock_desc_rtn_wr);
                          } else {                              /* Else inc nbr rdy    sock desc's (see #3b2B1a2).      */
                              sock_nbr_rdy++;
                          }
                          break;

                     case RTOS_ERR_NOT_INIT:
                     case RTOS_ERR_INVALID_HANDLE:              /* See Notes #3a2A1 & #3d1.                             */
                     case RTOS_ERR_INVALID_TYPE:
                     default:
                          RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(local_err));
                          goto exit_err;
                 }
             }
        }

    } else {                                                    /* Else if NO  wr sock desc's avail, ...                */
        NET_SOCK_DESC_INIT(&sock_desc_rtn_wr);                  /* ... clr rtn wr sock desc's.                          */
    }


    if (p_sock_desc_err != DEF_NULL) {                          /* If avail, chk err sock desc's (see Note #3b2A3).     */
                                                                /* Copy    req'd err sock desc's (see Note #3c2A1).     */
        NET_SOCK_DESC_COPY(&sock_desc_rtn_err, p_sock_desc_err);
                                                                /* Chk ALL avail err sock desc's (see Note #3b1).       */
        for (sock_id = 0; sock_id < sock_nbr_max; sock_id++) {
             sock_desc_used = NET_SOCK_DESC_IS_SET(sock_id, p_sock_desc_err);
             if (sock_desc_used != 0) {                         /* If err sock desc req'd, ...                          */
                                                                /* ... chk/cfg for avail err(s)  [see Note #3b2A3].     */
                 RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                 sock_rdy = NetSock_SelDescHandlerErr(sock_id, &local_err);
                 switch (RTOS_ERR_CODE_GET(local_err)) {
                     case RTOS_ERR_NONE:
                     case RTOS_ERR_NET_CONN_CLOSED_FAULT:
                     case RTOS_ERR_NET_INVALID_CONN:
                     case RTOS_ERR_INVALID_STATE:
                          if (sock_rdy != DEF_YES) {            /* If sock NOT rdy, ...                                 */
                                                                /* ... clr from rtn wr sock desc's (see #3b2B1b2B).     */
                              NET_SOCK_DESC_CLR(sock_id, &sock_desc_rtn_err);
                          } else {                              /* Else inc nbr rdy    sock desc's (see #3b2B1a2).      */
                              sock_nbr_rdy++;
                          }
                          break;

                     case RTOS_ERR_NOT_INIT:
                     case RTOS_ERR_INVALID_HANDLE:              /* See Notes #3a2A1 & #3d1.                             */
                     case RTOS_ERR_INVALID_TYPE:
                     default:
                          RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(local_err));
                          goto exit_err;
                 }
             }
        }

    } else {                                                    /* Else if NO  err sock desc's avail, ...               */
        NET_SOCK_DESC_INIT(&sock_desc_rtn_err);                 /* ... clr rtn err sock desc's.                         */
    }

    NET_SOCK_DESC_COPY(p_sock_desc_rd,  &sock_desc_rtn_rd);
    NET_SOCK_DESC_COPY(p_sock_desc_wr,  &sock_desc_rtn_wr);
    NET_SOCK_DESC_COPY(p_sock_desc_err, &sock_desc_rtn_err);

    return (sock_nbr_rdy);

exit_err:
    return (0u);

}
#endif

/*
*********************************************************************************************************
*                                     NetSock_SelDescHandlerRd()
*
* Description : (1) Handle socket descriptor for read operation(s) :
*
*                   (a) Check socket for available read operation(s) :
*                       (1) Read data                                                   See Note #2b1
*                       (2) Read connection closed                                      See Note #2b2
*                       (3) Read connection(s) available                                See Note #2b3
*                       (4) Socket error(s)                                             See Note #2b4
*
*                   (b) Configure socket event table to wait on appropriate read
*                           operation(s), if necessary
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to check for available read operation(s).
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if socket has any available read operation(s) [see Notes #2b1 & #2b3]; OR ...
*
*                        if socket's read connection is closed         [see Note  #2b2];        OR ...
*
*                        if socket has any available socket error(s)   [see Note  #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the 'readfds' ... parameter
*                   ... to see whether ... [any] descriptors are ready for reading" :
*
*                   (a) "A descriptor shall be considered ready for reading when a call to an input function
*                        ... would not block, whether or not the function would transfer data successfully.
*                        (The function might return data, an end-of-file indication, or an error other than
*                        one indicating that it is blocked, and in each of these cases the descriptor shall
*                        be considered ready for reading.)" :
*
*                       (1) "If the socket is currently listening, then it shall be marked as readable if
*                            an incoming connection request has been received, and a call to the accept()
*                            function shall complete without blocking."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Pages 164-165 states that "a socket is ready for reading if any of the
*                       following ... conditions is true" :
*
*                       (1) "A read operation on the socket will not block and will return a value greater
*                            than 0 (i.e., the data that is ready to be read)."
*
*                       (2) "The read half of the connection is closed (i.e., a TCP connection that has
*                            received a FIN).  A read operation ... will not block and will return 0 (i.e.,
*                            EOF)."
*
*                       (3) "The socket is a listening socket and the number of completed connections is
*                            nonzero.  An accept() on the listening socket will ... not block."
*
*                       (4) "A socket error is pending.  A read operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A1'.
*
*               (3) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'SockType' is incorrectly modified.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_SelDescHandlerRd (NET_SOCK_ID   sock_id,
                                               RTOS_ERR     *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif
    CPU_BOOLEAN   sock_rdy  = DEF_YES;
    NET_SOCK     *p_sock;


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                            /* -------------- VALIDATE SOCK USED -------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit;                                                  /* Rtn sock err (see Note #2b4).                    */
    }
#endif

                                                                    /* ------------- HANDLE SOCK DESC RD -------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             sock_rdy = NetSock_SelDescHandlerRdDatagram(p_sock, p_err);
             break;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             sock_rdy = NetSock_SelDescHandlerRdStream(p_sock, p_err);
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:                                                    /* See Note #3.                                     */
             sock_rdy  = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);           /* Rtn sock err (see Note #2b4).                    */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                 NetSock_SelDescHandlerRdDatagram()
*
* Description : (1) Handle datagram-type socket descriptor for read operation(s) :
*
*                   (a) Check datagram-type socket for available read operation(s) :
*                       (1) Read data                                                   See Note #2b1
*                       (2) Socket error(s)                                             See Note #2b4
*
*                   (b) Configure socket event table to wait on appropriate read
*                           operation(s) :
*
*                       (1) Socket receive data
*
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of datagram-type socket to
*               -------         check for available read operation(s).
*
*                               Argument checked   in NetSock_SelDescHandlerRd().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if datagram-type socket has any available read operation(s) [see Note #2b1]; OR ...
*
*                        if datagram-type socket has any available socket error(s)   [see Note #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the 'readfds' ... parameter
*                   ... to see whether ... [any] descriptors are ready for reading" :
*
*                   (a) "A descriptor shall be considered ready for reading when a call to an input function
*                        ... would not block, whether or not the function would transfer data successfully.
*                        (The function might return data, an end-of-file indication, or an error other than
*                        one indicating that it is blocked, and in each of these cases the descriptor shall
*                        be considered ready for reading.)"
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Pages 164-165 states that "a socket is ready for reading if any of the
*                       following ... conditions is true" :
*
*                       (1) "A read operation on the socket will not block and will return a value greater
*                            than 0 (i.e., the data that is ready to be read)."
*
*                       (4) "A socket error is pending.  A read operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A1'.
*
*               (3) Socket descriptor read availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor read handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also 'NetSock_RxDataHandlerDatagram()  Note #13'.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_SelDescHandlerRdDatagram (NET_SOCK  *p_sock,
                                                       RTOS_ERR  *p_err)
{
    CPU_BOOLEAN  sock_rdy = DEF_YES;


                                                                /* ----------- HANDLE DATAGRAM SOCK DESC RD ----------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);     /* Rtn sock err (see Note #2b4).                        */
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);  /* Rtn sock err (see Note #2b4).                    */
             goto exit;

        case NET_SOCK_STATE_CLOSED:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);      /* Rtn sock err (see Note #2b4).                        */
             goto exit;

        case NET_SOCK_STATE_BOUND:                              /* Chk non-conn'd datagram socks ...                    */
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN:                               /* ... &   conn'd datagram socks ...                    */
        case NET_SOCK_STATE_CONN_DONE:
                                                                /* ... for rx data avail (see Note #2b1).               */
             sock_rdy = NetSock_IsAvailRxDatagram(p_sock, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  sock_rdy = DEF_YES;
                  goto exit;
             }
             break;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_LISTEN:                             /* Listen  datagram socks NOT allowed.                  */
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                  /* Closing datagram socks NOT allowed.                  */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);      /* Rtn sock err (see Note #2b4).                        */
             goto exit;
    }


exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_SelDescHandlerRdStream()
*
* Description : (1) Handle stream-type socket descriptor for read operation(s) :
*
*                   (a) Check stream-type socket for available read operation(s) :
*                       (1) Read data                                                   See Note #2b1
*                       (2) Read connection closed                                      See Note #2b2
*                       (3) Read connection(s) available                                See Note #2b3
*                       (4) Socket error(s)                                             See Note #2b4
*
*                   (b) Configure socket event table to wait on appropriate read
*                           operation(s) :
*
*                       (1) Socket connection's transport layer receive operation(s)
*                       (2) Socket accept connection(s) available
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of stream-type socket to
*               -------                     check for available read operation(s).
*
*                                       Argument checked   in NetSock_SelDescHandlerRd().
*
*               p_sock                  Pointer to a socket.
*               ------                  Argument validated in NetSock_SelDescHandlerRd().
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if stream-type socket has any available read operation(s) [see Notes #2b1 & #2b3]; OR ...
*
*                        if stream-type socket's read connection is closed         [see Note  #2b2];        OR ...
*
*                        if stream-type socket has any available socket error(s)   [see Note  #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the 'readfds' ... parameter
*                   ... to see whether ... [any] descriptors are ready for reading" :
*
*                   (a) "A descriptor shall be considered ready for reading when a call to an input function
*                        ... would not block, whether or not the function would transfer data successfully.
*                        (The function might return data, an end-of-file indication, or an error other than
*                        one indicating that it is blocked, and in each of these cases the descriptor shall
*                        be considered ready for reading.)" :
*
*                       (1) "If the socket is currently listening, then it shall be marked as readable if
*                            an incoming connection request has been received, and a call to the accept()
*                            function shall complete without blocking."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Pages 164-165 states that "a socket is ready for reading if any of the
*                       following ... conditions is true" :
*
*                       (1) "A read operation on the socket will not block and will return a value greater
*                            than 0 (i.e., the data that is ready to be read)."
*
*                       (2) "The read half of the connection is closed (i.e., a TCP connection that has
*                            received a FIN).  A read operation ... will not block and will return 0 (i.e.,
*                            EOF)."
*
*                       (3) "The socket is a listening socket and the number of completed connections is
*                            nonzero.  An accept() on the listening socket will ... not block."
*
*                       (4) "A socket error is pending.  A read operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A1'.
*
*               (3) Socket descriptor read availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor read handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also 'NetSock_RxDataHandlerStream()  Note #11'
*                          & 'NetSock_Accept()               Note #10'.
*********************************************************************************************************
*/

#if ((NET_SOCK_CFG_SEL_EN         == DEF_ENABLED) && \
     defined(NET_SOCK_TYPE_STREAM_MODULE_EN))
static  CPU_BOOLEAN  NetSock_SelDescHandlerRdStream (NET_SOCK  *p_sock,
                                                     RTOS_ERR  *p_err)
{
    NET_CONN_ID  conn_id;
    NET_CONN_ID  conn_id_transport;
    CPU_BOOLEAN  sock_rdy          = DEF_YES;


                                                                /* ------------ HANDLE STREAM SOCK DESC RD ------------ */
                                                                /* Get transport conn id.                               */
    conn_id           = p_sock->ID_Conn;
    conn_id_transport = NetConn_ID_TransportGet(conn_id);

    if (conn_id_transport == NET_CONN_ID_NONE) {
         sock_rdy = DEF_YES;
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
         goto exit;                                             /* Rtn sock err (see Note #2b4).                        */
    }


    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);     /* Rtn sock err (see Note #2b4).                        */
             goto exit;


        case NET_SOCK_STATE_CLOSED_FAULT:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);  /* Rtn sock err (see Note #2b4).                    */
             goto exit;


        case NET_SOCK_STATE_CLOSED:                             /* Closed     stream socks NOT allowed.                 */
        case NET_SOCK_STATE_BOUND:                              /* Non-conn'd stream socks NOT allowed.                 */
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);      /* Rtn sock err (see Note #2b4).                        */
             goto exit;


        case NET_SOCK_STATE_LISTEN:                             /* Chk listen stream socks ...                          */
             sock_rdy = NetSock_ConnAcceptQ_IsRdy(p_sock);      /* ... for any avail conn (see Note #2b3).              */
             break;


        case NET_SOCK_STATE_CONN_IN_PROGRESS:                   /* Rtn conn-in-progress stream socks ...                */
             sock_rdy = DEF_NO;                                 /* ... as NOT rdy.                                      */
             break;                                             /* Cfg sock event to wait on transport rx.              */


        case NET_SOCK_STATE_CONN:                               /* Chk   conn'd  stream socks             ...           */
        case NET_SOCK_STATE_CONN_DONE:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                  /* ... & closing stream socks;            ...           */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
                                                                /* ... for rx data avail  (see Note #2b1) ...           */
                                                                /* ... OR  rx conn closed (see Note #2b2).              */
             sock_rdy = NetSock_IsAvailRxStream(p_sock, conn_id_transport, p_err);
             switch (RTOS_ERR_CODE_GET(*p_err)) {
                 case RTOS_ERR_NONE:
                      break;

                 case RTOS_ERR_INVALID_HANDLE:
                      sock_rdy = DEF_YES;
                      goto exit;                                /* Rtn sock err (see Note #2b4).                        */

                 default:
                      sock_rdy = DEF_YES;
                      RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);  /* Rtn sock err (see Note #2b4).                */
                      goto exit;
             }
             break;


        case NET_SOCK_STATE_NONE:
        default:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);      /* Rtn sock err (see Note #2b4).                        */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_SelDescHandlerWr()
*
* Description : (1) Handle socket descriptor for write operation(s) :
*
*                   (a) Check socket for available write operation(s) :
*                       (1) Write data                                                  See Note #2b1
*                       (2) Write connection closed                                     See Note #2b2
*                       (3) Write connection(s) available                               See Note #2b3
*                       (4) Socket error(s)                                             See Note #2b4
*
*                   (b) Configure socket event table to wait on appropriate write
*                           operation(s), if necessary
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of socket to check for
*                                           available write operation(s).
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if socket has any available write operation(s) [see Notes #2b1 & #2b3]; OR ...
*
*                        if socket's write connection is closed         [see Note  #2b2];        OR ...
*
*                        if socket has any available socket error(s)    [see Note  #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'writefds' ...
*                   parameter ... to see whether ... [any] descriptors ... are ready for writing" :
*
*                   (a) "A descriptor shall be considered ready for writing when a call to an output function
*                        ... would not block, whether or not the function would transfer data successfully" :
*
*                       (1) "If a non-blocking call to the connect() function has been made for a socket, and
*                            the connection attempt has either succeeded or failed leaving a pending error,
*                            the socket shall be marked as writable."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket is ready for writing if any of the
*                       following ... conditions is true" :
*
*                       (1) "A write operation will not block and will return a positive value (e.g., the
*                            number of bytes accepted by the transport layer)."
*
*                       (2) "The write half of the connection is closed."
*
*                       (3) "A socket using a non-blocking connect() has completed the connection, or the
*                            connect() has failed."
*
*                       (4) "A socket error is pending.  A write operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A2'.
*
*               (3) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'SockType' is incorrectly modified.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_SelDescHandlerWr (NET_SOCK_ID   sock_id,
                                               RTOS_ERR     *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif
    CPU_BOOLEAN   sock_rdy = DEF_YES;
    NET_SOCK     *p_sock;


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
        goto exit;
    }
#endif

                                                                /* --------------- HANDLE SOCK DESC WR ---------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             sock_rdy = NetSock_SelDescHandlerWrDatagram(p_sock, p_err);
             break;

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             sock_rdy = NetSock_SelDescHandlerWrStream(p_sock, p_err);
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:                                                /* See Note #3.                                         */
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);       /* Rtn sock err (see Note #2b4).                        */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                 NetSock_SelDescHandlerWrDatagram()
*
* Description : (1) Handle datagram-type socket descriptor for write operation(s) :
*
*                   (a) Check datagram-type socket for available write operation(s) :
*                       (1) Write data                                                  See Note #2b1
*                       (2) Socket error(s)                                             See Note #2b4
*
*                   (b) Configure socket event table to wait on appropriate write
*                           operation(s) :
*
*                       (1) Socket transmit data
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of datagram-type socket to
*               -------                     check for available write operation(s).
*
*                                       Argument checked   in NetSock_SelDescHandlerWr().
*
*               p_sock                  Pointer to a socket.
*               ------                  Argument validated in NetSock_SelDescHandlerWr().
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if datagram-type socket has any available write operation(s) [see Note #2b1]; OR ...
*
*                        if datagram-type socket has any available socket error(s)    [see Note #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'writefds' ...
*                   parameter ... to see whether ... [any] descriptors ... are ready for writing" :
*
*                   (a) "A descriptor shall be considered ready for writing when a call to an output function
*                        ... would not block, whether or not the function would transfer data successfully."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket is ready for writing if any of the
*                       following ... conditions is true" :
*
*                       (1) "A write operation will not block and will return a positive value (e.g., the
*                            number of bytes accepted by the transport layer)."
*
*                       (4) "A socket error is pending.  A write operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A2'.
*
*               (3) Socket descriptor write availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor write handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also 'NetSock_TxDataHandlerDatagram()  Note #10'.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_SelDescHandlerWrDatagram (NET_SOCK  *p_sock,
                                                       RTOS_ERR  *p_err)
{
    CPU_BOOLEAN  sock_rdy = DEF_YES;


                                                                    /* --------- HANDLE DATAGRAM SOCK DESC WR --------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);         /* Rtn sock err (see Note #2b4).                    */
             goto exit;


        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);  /* Rtn sock err (see Note #2b4).                    */
             goto exit;


        case NET_SOCK_STATE_CLOSED:                                 /* Chk non-conn'd datagram socks ...                */
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN:                                   /* ... &   conn'd datagram socks ...                */
        case NET_SOCK_STATE_CONN_DONE:
                                                                    /* ... if rdy for tx (see Note #2b1).               */
             sock_rdy = NetSock_IsRdyTxDatagram(p_sock, p_err);
             switch (RTOS_ERR_CODE_GET(*p_err)) {
                 case RTOS_ERR_NONE:
                      break;

                 case RTOS_ERR_INVALID_HANDLE:
                      goto exit;                                    /* Rtn sock err (see Note #2b4).                    */

                 default:
                      RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN); /* Rtn sock err (see Note #2b4).                 */
                      goto exit;
             }
             break;


        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_LISTEN:                                 /* Listen  datagram socks NOT allowed.              */
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                      /* Closing datagram socks NOT allowed.              */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);          /* Rtn sock err (see Note #2b4).                    */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_SelDescHandlerWrStream()
*
* Description : (1) Handle stream-type socket descriptor for write operation(s) :
*
*                   (a) Check stream-type socket for available write operation(s) :
*                       (1) Write data                                                  See Note #2b1
*                       (2) Write connection closed                                     See Note #2b2
*                       (3) Write connection(s) available                               See Note #2b3
*                       (4) Socket error(s)                                             See Note #2b4
*
*                   (b) Configure socket event table to wait on appropriate write
*                           operation(s) :
*
*                       (1) Socket connection's transport layer transmit operation(s)
*                       (2) Socket connection(s) complete
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of stream-type socket to
*               -------                     check for available write operation(s).
*
*                                       Argument checked   in NetSock_SelDescHandlerWr().
*
*               p_sock                  Pointer to a socket.
*               ------                  Argument validated in NetSock_SelDescHandlerWr().
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if stream-type socket has any available write operation(s) [see Notes #2b1 & #2b3]; OR ...
*
*                        if stream-type socket's write connection is closed         [see Note  #2b2];        OR ...
*
*                        if stream-type socket has any available socket error(s)    [see Note  #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'writefds' ...
*                   parameter ... to see whether ... [any] descriptors ... are ready for writing" :
*
*                   (a) "A descriptor shall be considered ready for writing when a call to an output function
*                        ... would not block, whether or not the function would transfer data successfully" :
*
*                       (1) "If a non-blocking call to the connect() function has been made for a socket, and
*                            the connection attempt has either succeeded or failed leaving a pending error,
*                            the socket shall be marked as writable."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket is ready for writing if any of the
*                       following ... conditions is true" :
*
*                       (1) "A write operation will not block and will return a positive value (e.g., the
*                            number of bytes accepted by the transport layer)."
*
*                       (2) "The write half of the connection is closed."
*
*                       (3) "A socket using a non-blocking connect() has completed the connection, or the
*                            connect() has failed."
*
*                       (4) "A socket error is pending.  A write operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A2'.
*
*               (3) Socket descriptor write availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor write handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also 'NetSock_TxDataHandlerStream()  Note #8'
*                          & 'NetSock_ConnHandlerStream()    Note #4'.
*********************************************************************************************************
*/

#if ((NET_SOCK_CFG_SEL_EN         == DEF_ENABLED)) && \
     defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
static  CPU_BOOLEAN  NetSock_SelDescHandlerWrStream (NET_SOCK  *p_sock,
                                                     RTOS_ERR  *p_err)
{
    NET_CONN_ID  conn_id;
    NET_CONN_ID  conn_id_transport;
    CPU_BOOLEAN  sock_rdy          = DEF_YES;


                                                                    /* ---------- HANDLE STREAM SOCK DESC WR ---------- */
                                                                    /* Get transport conn id.                           */
    conn_id           = p_sock->ID_Conn;
    conn_id_transport = NetConn_ID_TransportGet(conn_id);

    if  (conn_id_transport == NET_CONN_ID_NONE) {
         sock_rdy = DEF_YES;
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
         goto exit;                                                 /* Rtn sock err (see Note #2b4).                    */
    }


    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);         /* Rtn sock err (see Note #2b4).                    */
             goto exit;


        case NET_SOCK_STATE_CLOSED_FAULT:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);  /* Rtn sock err (see Note #2b4).                    */
             goto exit;


        case NET_SOCK_STATE_CLOSED:                                 /* Closed     stream socks NOT allowed.             */
        case NET_SOCK_STATE_BOUND:                                  /* Non-conn'd stream socks NOT allowed.             */
        case NET_SOCK_STATE_LISTEN:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);          /* Rtn sock err (see Note #2b4).                    */
             return (DEF_YES);


        case NET_SOCK_STATE_CONN_IN_PROGRESS:                       /* Rtn conn-in-progress stream socks ...            */
             sock_rdy = DEF_NO;                                     /* ... as NOT rdy.                                  */
             break;


        case NET_SOCK_STATE_CONN_DONE:                              /* Rtn conn-completed stream socks ...              */
             sock_rdy = DEF_YES;                                    /* ... as     rdy (see Note #2b3).                  */
             break;


        case NET_SOCK_STATE_CONN:                                   /* Chk conn'd stream socks               ...        */
                                                                    /* ... if rdy for tx     (see Note #2b1) ...        */
                                                                    /* ... OR tx conn closed (see Note #2b2).           */
             sock_rdy = NetSock_IsRdyTxStream(p_sock, conn_id_transport, p_err);
             switch (RTOS_ERR_CODE_GET(*p_err)) {
                 case RTOS_ERR_NONE:
                      break;

                 case RTOS_ERR_INVALID_HANDLE:
                      sock_rdy = DEF_YES;
                      goto exit;                                    /* Rtn sock err (see Note #2b4).                    */

                 default:
                      sock_rdy = DEF_YES;
                      RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN); /* Rtn sock err (see Note #2b4).                 */
                      goto exit;
             }
             break;


        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                      /* Rtn closing stream socks ...                     */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             sock_rdy = DEF_YES;                                    /* ... as rdy   (see Note #2b2).                    */
             break;


        case NET_SOCK_STATE_NONE:
        default:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);          /* Rtn sock err (see Note #2b4).                    */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_SelDescHandlerErr()
*
* Description : (1) Handle socket descriptor for error(s) &/or exception(s) :
*
*                   (a) Check socket for available error(s) &/or exception(s) :
*                       (1) Socket error(s)                                         See Notes #2a2 & #2a3
*
*                   (b) Configure socket event table to wait on appropriate error
*                           operation(s), if necessary
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of socket to check for
*                                           available error(s).
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err                   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if socket has any available socket error(s) [see Note #2a].
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'errorfds' ...
*                   parameter ... to see whether ... [any] descriptors ... have an exceptional condition
*                   pending" :
*
*                   (a) "A file descriptor ... shall be considered to have an exceptional condition pending
*                        ... as noted below" :
*
*                       (1) (A) "A socket ... receive operation ... [that] would return out-of-band data
*                                without blocking."
*                           (B) "A socket ... [with] out-of-band data ... present in the receive queue."
*
*                       (2) "If a socket has a pending error."
*
*                       (3) "Other circumstances under which a socket may be considered to have an exceptional
*                            condition pending are protocol-specific and implementation-defined."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket has an exception condition pending if ...
*                       any of the following ... conditions is true" :
*
*                       (1) (A) "Out-of-band data for the socket" is currently available; ...
*                           (B) "Or the socket is still at the out-of-band mark."
*
*                   (c) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states "that when an error occurs on a socket, it is [also]
*                       marked as both readable and writeable by select()".
*
*                   See also 'NetSock_Sel()  Note #3b2A3'.
*
*               (3) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'SockType' is incorrectly modified.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_SelDescHandlerErr (NET_SOCK_ID  sock_id,
                                                RTOS_ERR    *p_err)
{
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif
    CPU_BOOLEAN   sock_rdy  = DEF_YES;
    NET_SOCK     *p_sock;


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
         goto exit;
    }
#endif

                                                                /* --------------- HANDLE SOCK DESC ERR --------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_DATAGRAM:
             sock_rdy = NetSock_SelDescHandlerErrDatagram(p_sock, p_err);
             break;


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             sock_rdy = NetSock_SelDescHandlerErrStream(p_sock, p_err);
             break;
#endif

        case NET_SOCK_TYPE_NONE:
        default:                                                /* See Note #3.                                         */
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);       /* Rtn sock err (see Note #2a3).                        */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                 NetSock_SelDescHandlerErrDatagram()
*
* Description : (1) Handle datagram-type socket descriptor for error(s) &/or exception(s) :
*
*                   (a) Check datagram-type socket for available error(s) &/or exception(s) :
*
*                       (1) Socket error(s)                                         See Notes #2a2 & #2a3
*
*                   (b) Configure socket event table to wait on appropriate error
*                           operation(s), if necessary
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of datagram-type socket to
*               -------                     check for available error(s).
*
*                                       Argument checked   in NetSock_SelDescHandlerErr().
*
*               p_sock                  Pointer to a socket.
*               ------                  Argument validated in NetSock_SelDescHandlerErr().
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err                   Pointer to variable that will receive the return error code from this function.
*
*
* Return(s)   : DEF_YES, if datagram-type socket has any available socket error(s) [see Note #2a].
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'errorfds' ...
*                   parameter ... to see whether ... [any] descriptors ... have an exceptional condition
*                   pending" :
*
*                   (a) "A file descriptor ... shall be considered to have an exceptional condition pending
*                        ... as noted below" :
*
*                       (2) "If a socket has a pending error."
*
*                       (3) "Other circumstances under which a socket may be considered to have an exceptional
*                            condition pending are protocol-specific and implementation-defined."
*
*                   See also 'NetSock_Sel()  Note #3b2A3'.
*
*               (3) Socket descriptor error availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor error handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also 'NetSock_RxDataHandlerDatagram()  Note #13'
*                          & 'NetSock_TxDataHandlerDatagram()  Note #10'.
*
*               (4) Since datagram-type sockets typically never wait on transmit operations, no socket
*                   event need be configured to wait on datagram-type socket transmit errors.
*
*                   See also 'NetSock_IsRdyTxDatagram()  Note #3'.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_SelDescHandlerErrDatagram (NET_SOCK  *p_sock,
                                                        RTOS_ERR  *p_err)
{
    CPU_BOOLEAN  sock_rdy = DEF_YES;


                                                                    /* -------- HANDLE DATAGRAM SOCK DESC ERR --------- */
    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);         /* Rtn sock err (see Note #2a3).                    */
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT);  /* Rtn sock err (see Note #2a2).                    */
             goto exit;

        case NET_SOCK_STATE_CLOSED:                                 /* Rtn closed     datagram socks, ...               */
        case NET_SOCK_STATE_BOUND:                                  /* ... non-conn'd datagram socks, ...               */
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN:                                   /* ... &   conn'd datagram socks  ...               */
        case NET_SOCK_STATE_CONN_DONE:
             sock_rdy = DEF_NO;                                     /* ... as NOT rdy w/ any err(s).                    */
             break;

        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_LISTEN:                                 /* Listen  datagram socks NOT allowed.              */
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                      /* Closing datagram socks NOT allowed.              */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        default:
             sock_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);          /* Rtn sock err (see Note #2a3).                    */
             goto exit;
    }


exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                  NetSock_SelDescHandlerErrStream()
*
* Description : (1) Handle stream-type socket descriptor for error(s) &/or exception(s) :
*
*                   (a) Check stream-type socket for available error(s) &/or exception(s) :
*
*                       (1) Out-of-band data                                        See Note  #2b1
*                       (2) Socket error(s)                                         See Notes #2a2 & #2a3
*
*                   (b) Configure socket event table to wait on appropriate error
*                           operation(s), if necessary
*
*
* Description : (1) Check stream-type socket for available error(s) :
*
*
*
* Argument(s) : sock_id                 Socket descriptor/handle identifier of stream-type socket to
*               -------                     check for available error(s).
*
*                                       Argument checked   in NetSock_SelDescHandlerErr().
*
*               p_sock                  Pointer to a socket.
*               ------                  Argument validated in NetSock_SelDescHandlerErr().
*
*               p_sock_event_tbl        Pointer to a socket event table to configure socket events
*               ----------------            to wait on.
*
*                                       Argument validated in NetSock_Sel().
*
*               p_sock_event_nbr_cfgd   Pointer to the number of configured socket events.
*               ---------------------   Argument validated in NetSock_Sel().
*
*               p_err                   Pointer to variable that will receive the return error code from this function.
*
*
* Return(s)   : DEF_YES, if stream-type socket has any available socket error(s) [see Note #2a].
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'errorfds' ...
*                   parameter ... to see whether ... [any] descriptors ... have an exceptional condition
*                   pending" :
*
*                   (a) "A file descriptor ... shall be considered to have an exceptional condition pending
*                        ... as noted below" :
*
*                       (1) (A) "A socket ... receive operation ... [that] would return out-of-band data
*                                without blocking."
*                           (B) "A socket ... [with] out-of-band data ... present in the receive queue."
*
*                           Out-of-band data NOT supported (see 'net_tcp.h  Note #1b').
*
*                       (2) "If a socket has a pending error."
*
*                       (3) "Other circumstances under which a socket may be considered to have an exceptional
*                            condition pending are protocol-specific and implementation-defined."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket has an exception condition pending if ...
*                       any of the following ... conditions is true" :
*
*                       (1) (A) "Out-of-band data for the socket" is currently available; ...
*                           (B) "Or the socket is still at the out-of-band mark."
*
*                           Out-of-band data NOT supported (see 'net_tcp.h  Note #1b').
*
*                   See also 'NetSock_Sel()  Note #3b2A3'.
*
*               (3) Socket descriptor error availability determined by other socket handler function(s).
*                   For correct interoperability between socket descriptor error handler functionality &
*                   all other appropriate socket handler function(s); ANY modification to any of these
*                   functions MUST be appropriately synchronized.
*
*                   See also 'NetSock_RxDataHandlerStream()  Note #11',
*                            'NetSock_TxDataHandlerStream()  Note #8' ,
*                            'NetSock_Accept()               Note #10',
*                          & 'NetSock_ConnHandlerStream()    Note #4'.
*********************************************************************************************************
*/

#if ((NET_SOCK_CFG_SEL_EN         == DEF_ENABLED)) && \
     defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
static  CPU_BOOLEAN  NetSock_SelDescHandlerErrStream (NET_SOCK  *p_sock,
                                                      RTOS_ERR  *p_err)
{
    NET_CONN_ID  conn_id;
    NET_CONN_ID  conn_id_transport;
    CPU_BOOLEAN  sock_rdy = DEF_YES;


                                                                /* ----------- HANDLE STREAM SOCK DESC ERR ------------ */
                                                                /* Get transport conn id.                               */
    conn_id           = p_sock->ID_Conn;
    conn_id_transport = NetConn_ID_TransportGet(conn_id);

    if  (conn_id_transport == NET_CONN_ID_NONE) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
         goto exit;                                             /* Rtn sock err (see Note #2a3).                        */
    }


    switch (p_sock->State) {
        case NET_SOCK_STATE_FREE:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);     /* Rtn sock err (see Note #2a3).                        */
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             RTOS_ERR_SET(*p_err, RTOS_ERR_NET_CONN_CLOSED_FAULT); /* Rtn sock err (see Note #2a2).                     */
             goto exit;

        case NET_SOCK_STATE_CLOSED:                             /* Rtn closed       stream socks ...                    */
        case NET_SOCK_STATE_BOUND:                              /* ... & non-conn'd stream socks ...                    */
             sock_rdy = DEF_NO;                                 /* ... as NOT rdy w/ any err(s).                        */
             break;

        case NET_SOCK_STATE_LISTEN:                             /* Rtn listen stream socks ...                          */
             sock_rdy = DEF_NO;                                 /* ... as NOT rdy w/ any err(s).                        */
             break;

        case NET_SOCK_STATE_CONN_IN_PROGRESS:                   /* Rtn conn-in-progress stream socks ...                */
             sock_rdy = DEF_NO;                                 /* ... as NOT rdy w/ any err(s).                        */
             break;

        case NET_SOCK_STATE_CONN:                               /* Rtn conn'd stream socks ...                          */
        case NET_SOCK_STATE_CONN_DONE:
             sock_rdy = DEF_NO;                                 /* ... as NOT rdy w/ any err(s).                        */
             break;

        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:                  /* Rtn closing stream socks ...                         */
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
             sock_rdy = DEF_NO;                                 /* ... as NOT rdy w/ any err(s).                        */
             break;

        case NET_SOCK_STATE_NONE:
        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);      /* Rtn sock err (see Note #2a3).                        */
             goto exit;
    }

exit:
    return (sock_rdy);
}
#endif


/*
*********************************************************************************************************
*                                    NetSock_IsAvailRxDatagram()
*
* Description : (1) Check datagram-type socket for available receive operation(s) :
*
*                   (a) Receive data                                                    See Note #2b1
*                   (b) Socket error(s)                                                 See Note #2b4
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_SelDescHandlerRd().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if datagram-type socket has any available receive data    [see Note #2b1]; OR ...
*
*                        if datagram-type socket has any available socket error(s) [see Note #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the 'readfds' ... parameter
*                   ... to see whether ... [any] descriptors are ready for reading" :
*
*                   (a) "A descriptor shall be considered ready for reading when a call to an input function
*                        ... would not block, whether or not the function would transfer data successfully.
*                        (The function might return data, an end-of-file indication, or an error other than
*                        one indicating that it is blocked, and in each of these cases the descriptor shall
*                        be considered ready for reading.)"
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Pages 164-165 states that "a socket is ready for reading if any of the
*                       following ... conditions is true" :
*
*                       (1) "A read operation on the socket will not block and will return a value greater
*                            than 0 (i.e., the data that is ready to be read)."
*
*                       (4) "A socket error is pending.  A read operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A1'.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_IsAvailRxDatagram (NET_SOCK  *p_sock,
                                                RTOS_ERR  *p_err)
{
    CPU_BOOLEAN  rx_avail = DEF_YES;


    PP_UNUSED_PARAM(p_err);

                                                                /* ---------------- CHK SOCK RX AVAIL ----------------- */
    rx_avail = (p_sock->RxQ_Head != DEF_NULL) ? DEF_YES : DEF_NO;

    return (rx_avail);
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_IsAvailRxStream()
*
* Description : (1) Check stream-type socket for available receive operation(s) :
*
*                   (a) Receive data                                                    See Note #2b1
*                   (b) Receive connection closed                                       See Note #2b2
*                   (c) Socket  error(s)                                                See Note #2b4
*
*
* Argument(s) : p_sock              Pointer to a socket.
*               ------              Argument validated in NetSock_SelDescHandlerRd().
*
*               conn_id_transport   Connection's transport layer handle identifier.
*               -----------------   Argument checked   in NetSock_SelDescHandlerRdStream().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if stream-type socket has any available receive data    [see Note #2b1]; OR ...
*
*                        if stream-type socket's receive connection is closed    [see Note #2b2]; OR ...
*
*                        if stream-type socket has any available socket error(s) [see Note #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the 'readfds' ... parameter
*                   ... to see whether ... [any] descriptors are ready for reading" :
*
*                   (a) "A descriptor shall be considered ready for reading when a call to an input function
*                        ... would not block, whether or not the function would transfer data successfully.
*                        (The function might return data, an end-of-file indication, or an error other than
*                        one indicating that it is blocked, and in each of these cases the descriptor shall
*                        be considered ready for reading.)"
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Pages 164-165 states that "a socket is ready for reading if any of the
*                       following ... conditions is true" :
*
*                       (1) "A read operation on the socket will not block and will return a value greater
*                            than 0 (i.e., the data that is ready to be read)."
*
*                       (2) "The read half of the connection is closed (i.e., a TCP connection that has
*                            received a FIN).  A read operation ... will not block and will return 0 (i.e.,
*                            EOF)."
*
*                       (4) "A socket error is pending.  A read operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A1'.
*
*               (3) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'Protocol' is incorrectly modified.
*********************************************************************************************************
*/

#if ((NET_SOCK_CFG_SEL_EN         == DEF_ENABLED)) && \
     defined(NET_SOCK_TYPE_STREAM_MODULE_EN)
static  CPU_BOOLEAN  NetSock_IsAvailRxStream (NET_SOCK     *p_sock,
                                              NET_CONN_ID   conn_id_transport,
                                              RTOS_ERR     *p_err)
{
    CPU_BOOLEAN  rx_avail  = DEF_YES;
#ifdef  NET_SECURE_MODULE_EN
    RTOS_ERR     local_err;
#endif


                                                                /* ---------------- CHK SOCK RX AVAIL ----------------- */

    switch (p_sock->Protocol) {                                 /* Chk rx avail from transport layer.                   */
        case NET_SOCK_PROTOCOL_TCP:
             rx_avail = NetTCP_ConnIsAvailRx((NET_TCP_CONN_ID)conn_id_transport,
                                                              p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                 rx_avail = DEF_YES;
                 goto exit;
             }

#ifdef  NET_SECURE_MODULE_EN
             if ((DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE)) &&
                 (rx_avail == DEF_NO)) {
                  RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                  rx_avail = NetSecure_SockRxIsDataPending(p_sock, &local_err);
             }
#endif
             break;

        default:                                                /* See Note #3.                                         */
             rx_avail = DEF_YES;                                /* Prevent possible 'variable unused' warning.          */
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);     /* Rtn sock err (see Note #2b4).                        */
             goto exit;
    }


exit:
    return (rx_avail);
}
#endif


/*
*********************************************************************************************************
*                                      NetSock_IsRdyTxDatagram()
*
* Description : (1) Check datagram-type socket ready for transmit operation(s) :
*
*                   (a) Transmit data                                                   See Note #2b1
*                   (b) Socket error(s)                                                 See Note #2b4
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_SelDescHandlerWr().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if datagram-type socket is ready to transmit data         [see Note #2b1]; OR ...
*
*                        if datagram-type socket has any available socket error(s) [see Note #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'writefds' ...
*                   parameter ... to see whether ... [any] descriptors ... are ready for writing" :
*
*                   (a) "A descriptor shall be considered ready for writing when a call to an output function
*                        ... would not block, whether or not the function would transfer data successfully."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket is ready for writing if any of the
*                       following ... conditions is true" :
*
*                       (1) "A write operation will not block and will return a positive value (e.g., the
*                            number of bytes accepted by the transport layer)."
*
*                       (4) "A socket error is pending.  A write operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A2'.
*
*               (3) Datagram-type sockets are typically always available for transmit operations.
*********************************************************************************************************
*/

#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
static  CPU_BOOLEAN  NetSock_IsRdyTxDatagram (NET_SOCK  *p_sock,
                                              RTOS_ERR  *p_err)
{
    CPU_BOOLEAN  tx_rdy;


                                                                /* Prevent possible 'variable unused' warning.          */
    PP_UNUSED_PARAM(p_sock);
    PP_UNUSED_PARAM(p_err);

                                                                /* ----------------- CHK SOCK TX RDY ------------------ */
    tx_rdy = DEF_YES;                                           /* See Note #3.                                         */

    return (tx_rdy);
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_IsRdyTxStream()
*
* Description : (1) Check stream-type socket ready for transmit operation(s) :
*
*                   (a) Transmit data                                                   See Note #2b1
*                   (b) Transmit connection closed                                      See Note #2b2
*                   (c) Socket error(s)                                                 See Note #2b4
*
*
* Argument(s) : p_sock              Pointer to a socket.
*               ------              Argument validated in NetSock_SelDescHandlerWr().
*
*               conn_id_transport   Connection's transport layer handle identifier.
*               -----------------   Argument checked   in NetSock_SelDescHandlerWrStream().
*
*               p_err               Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_YES, if stream-type socket is ready to transmit data         [see Note #2b1]; OR ...
*
*                        if stream-type socket's transmit connection is closed   [see Note #2b2]; OR ...
*
*                        if stream-type socket has any available socket error(s) [see Note #2b4].
*
*               DEF_NO,  otherwise.
*
* Note(s)     : (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that "the select()
*                   function shall ... examine the file descriptor[s] ... passed in the ... 'writefds' ...
*                   parameter ... to see whether ... [any] descriptors ... are ready for writing" :
*
*                   (a) "A descriptor shall be considered ready for writing when a call to an output function
*                        ... would not block, whether or not the function would transfer data successfully."
*
*                   (b) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition, 6th Printing,
*                       Section 6.3, Page 165 states that "a socket is ready for writing if any of the
*                       following ... conditions is true" :
*
*                       (1) "A write operation will not block and will return a positive value (e.g., the
*                            number of bytes accepted by the transport layer)."
*
*                       (2) "The write half of the connection is closed."
*
*                       (4) "A socket error is pending.  A write operation on the socket will not block and
*                            will return an error (-1) with 'errno' set to the specific error condition."
*
*                   See also 'NetSock_Sel()  Note #3b2A2'.
*
*               (3) Default case already invalidated in NetSock_Open().  However, the default case is
*                   included as an extra precaution in case 'Protocol' is incorrectly modified.
*********************************************************************************************************
*/

#if ((NET_SOCK_CFG_SEL_EN == DEF_ENABLED) && \
     defined(NET_SOCK_TYPE_STREAM_MODULE_EN))
static  CPU_BOOLEAN  NetSock_IsRdyTxStream (NET_SOCK     *p_sock,
                                            NET_CONN_ID   conn_id_transport,
                                            RTOS_ERR     *p_err)
{
    CPU_BOOLEAN  tx_rdy;


                                                                /* ----------------- CHK SOCK TX RDY ------------------ */
    switch (p_sock->Protocol) {                                 /* Chk tx rdy status from transport layer.              */
        case NET_SOCK_PROTOCOL_TCP:
             tx_rdy = NetTCP_ConnIsRdyTx((NET_TCP_CONN_ID)conn_id_transport,
                                                          p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  tx_rdy = DEF_YES;
                  goto exit;
             }
             break;

        case NET_SOCK_PROTOCOL_NONE:
        default:                                                /* See Note #3.                                         */
             tx_rdy = DEF_YES;
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);     /* Rtn sock err (see Note #2b4).                        */
             goto exit;
    }


exit:
    return (tx_rdy);
}
#endif


/*
*********************************************************************************************************
*                                            NetSock_Get()
*
* Description : (1) Allocate & initialize a socket :
*
*                   (a) Get      a socket
*                   (b) Validate   socket
*                   (c) Initialize socket
*                   (d) Update     socket pool statistics
*                   (e) Return pointer to socket
*                         OR
*                       Null pointer & error code, on failure
*
*               (2) The socket pool is implemented as a stack :
*
*                   (a) 'NetSock_PoolPtr' points to the head of the socket pool.
*
*                   (b) Sockets' 'NextPtr's link each socket to form  the socket pool stack.
*
*                   (c) Sockets are inserted & removed at the head of the socket pool stack.
*
*
*                                        Sockets are
*                                     inserted & removed
*                                        at the head
*                                       (see Note #2c)
*
*                                             |                 NextPtr
*                                             |             (see Note #2b)
*                                             v                    |
*                                                                  |
*                                          -------       -------   v   -------       -------
*                         Socket Pool ---->|     |------>|     |------>|     |------>|     |
*                           Pointer        |     |       |     |       |     |       |     |
*                                          |     |       |     |       |     |       |     |
*                        (see Note #2a)    -------       -------       -------       -------
*
*                                          |                                               |
*                                          |<----------- Pool of Free Sockets ------------>|
*                                          |                (see Note #2)                  |
*
*
* Argument(s) : p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Pointer to socket, if NO error(s).
*               Pointer to NULL,   otherwise.
*
* Note(s)     : (3) (a) Socket pool is accessed by 'NetSock_PoolPtr' during execution of
*
*                       (1) NetSock_Init()
*                       (2) NetSock_Get()
*                       (3) NetSock_Free()
*
*                   (b) Since the primary tasks of the network protocol suite are prevented from running
*                       concurrently (see 'net.h  Note #3'), it is NOT necessary to protect the shared
*                       resources of the socket pool since no asynchronous access from other network
*                       tasks is possible.
*********************************************************************************************************
*/

static  NET_SOCK  *NetSock_Get (RTOS_ERR  *p_err)
{
    NET_SOCK  *p_sock    = DEF_NULL;
    RTOS_ERR   local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                                                                /* --------------------- GET SOCK --------------------- */
    if (NetSock_PoolPtr != DEF_NULL) {                          /* If sock pool NOT empty, get sock from pool.          */
        p_sock           = NetSock_PoolPtr;
        NetSock_PoolPtr  = p_sock->NextPtr;

    } else {                                                    /* If none avail, rtn err.                              */
        p_sock = DEF_NULL;
        NET_CTR_ERR_INC(Net_ErrCtrs.Sock.NoneAvailCtr);
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
        goto exit;
    }

                                                                /* -------------------- INIT SOCK --------------------- */
    NetSock_Clr(p_sock);
    DEF_BIT_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_USED);        /* Set sock as used.                                    */
    p_sock->State = NET_SOCK_STATE_CLOSED;

#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
                                                                /* -------------- UPDATE SOCK POOL STATS -------------- */
    NetStat_PoolEntryUsedInc(&NetSock_PoolStat, &local_err);
#endif

    PP_UNUSED_PARAM(local_err);

exit:
    return (p_sock);                                            /* -------------------- RTN SOCK ---------------------- */
}


/*
*********************************************************************************************************
*                                     NetSock_GetConnTransport()
*
* Description : (1) Allocate a transport layer connection :
*
*                   (a) Get a transport connection
*                   (b) Set connection handle identifiers
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Listen(),
*                                                 NetSock_ConnHandlerStream().
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Transport connection handler identifer, if NO error(s).
*               NET_CONN_ID_NONE, otherwise.
*
* Note(s)     : (2) The 'NET_SOCK_CFG_FAMILY' pre-processor 'else'-conditional code will never be
*                   compiled/linked since 'net_sock.h' ensures that the family type configuration
*                   constant (NET_SOCK_CFG_FAMILY) is configured with an appropriate family type
*                   value (see 'net_sock.h  CONFIGURATION ERRORS').  The 'else'-conditional code
*                   is included for completeness & as an extra precaution in case 'net_sock.h' is
*                   incorrectly modified.
*
*               (3) On ANY error(s) after the transport connection is allocated, the transport connection
*                   MUST be freed.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  NET_CONN_ID  NetSock_GetConnTransport (NET_SOCK  *p_sock,
                                               RTOS_ERR  *p_err)
{
    NET_CONN_ID  conn_id_transport = NET_CONN_ID_NONE;
    NET_CONN_ID  conn_id           = NET_CONN_ID_NONE;
    CPU_BOOLEAN  is_used           = DEF_NO;


                                                                /* --------------- RETRIEVE CONNECTION ---------------- */
    conn_id  = p_sock->ID_Conn;

    is_used = NetConn_IsUsed(conn_id);
    if (is_used != DEF_YES) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
        conn_id_transport = NET_CONN_ID_NONE;
        goto exit;
    }

                                                                /* ---------------- GET TRANSPORT CONN ---------------- */
    conn_id_transport =  NetTCP_ConnGet(&NetSock_AppPostRx,
                                        &NetSock_AppPostTx,
                                         p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         conn_id_transport = NET_CONN_ID_NONE;
         goto exit;
    }

                                                                /* ------------------- SET CONN IDs ------------------- */
                                                                /* Set conn's transport id.                             */
    NetConn_ID_TransportSet(conn_id, conn_id_transport);

                                                                /* Set transport's conn id.                             */
    NetTCP_ConnSetID_Conn((NET_TCP_CONN_ID)conn_id_transport,
                                           conn_id);

exit:
    return (conn_id_transport);
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_CloseHandler()
*
* Description : (1) Close a socket for valid socket close operations :
*
*                   (a) Close socket
*                   (b) Free  socket
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Close(),
*                                                 NetSock_CloseHandlerStream(),
*                                                 NetSock_CloseSockFromClose(),
*                                                 various.
*
*               close_conn              Indicate whether to close network   connection (see Note #2) :
*
*                                           DEF_YES                    Close network   connection.
*                                           DEF_NO              Do NOT close network   connection.
*
*               close_conn_transport    Indicate whether to close transport connection (see Note #2) :
*
*                                           DEF_YES                    Close transport connection.
*                                           DEF_NO              Do NOT close transport connection.
*
* Return(s)   : none.
*
* Note(s)     : (2) Except when closed from the network connection layer, a socket SHOULD close its
*                   network connection(s).
*
*               (3) On any socket close handler error(s), socket already discarded in close handle.
*********************************************************************************************************
*/

static  void  NetSock_CloseHandler (NET_SOCK     *p_sock,
                                    CPU_BOOLEAN   close_conn,
                                    CPU_BOOLEAN   close_conn_transport)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

                                                                /* ------------------- CLOSE SOCK --------------------- */
    NetSock_CloseSockHandler(p_sock, close_conn, close_conn_transport, &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {        /* See Note #3.                                         */
        goto exit;
    }
                                                                /* -------------------- FREE SOCK --------------------- */
    NetSock_Free(p_sock);

exit:
    return;
}


/*
*********************************************************************************************************
*                                     NetSock_CloseSockHandler()
*
* Description : (1) Close a socket :
*
*                   (a) Close socket secure session                         See Note #5
*                   (b) Free  socket
*                   (c) Close socket connection(s)                          See Note #2
*                   (d) Clear socket connection handle identifier           See Note #2b2
*
*
* Argument(s) : p_sock                  Pointer to a socket.
*               ------                  Argument validated in NetSock_CloseHandler(),
*                                                             NetSock_CloseSockHandler().
*
*               close_conn              Indicate whether to close network   connection (see Note #2a) :
*
*                                           DEF_YES                    Close network   connection.
*                                           DEF_NO              Do NOT close network   connection.
*
*               close_conn_transport    Indicate whether to close transport connection (see Note #2a) :
*
*                                           DEF_YES                    Close transport connection.
*                                           DEF_NO              Do NOT close transport connection.
*
*               p_err                   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) When a socket closes, specified socket network connection(s) MAY be closed
*                       &/or network connection handle identifier(s) MAY be cleared :
*
*                       (1) Network   connection
*                       (2) Transport connection
*
*                           (A) A socket may maintain its network connection despite closing the transport
*                               connection, but a socket MUST NOT close the network connection but expect
*                               to maintain the transport connection.
*
*                           (B) Network connection(s) MAY be closed even if socket close fails (see also
*                               Note #4).
*
*                   (b) Socket's network connection handle identifier MUST be :
*
*                       (1) Obtained PRIOR to any socket free operation
*                       (2) Cleared  after :
*                           (A) Socket connection(s) closed
*                           (B) Socket free handler error(s) handled
*                                 (see 'NetSock_FreeHandler()  Note #3')
*
*               (3) #### To prevent closing a socket already closed via previous socket close,
*                   NetSock_CloseSockHandler() checks the socket's 'USED' flag BEFORE closing
*                   the socket.
*
*                   This prevention is only best-effort since any invalid duplicate socket closes
*                   MAY be asynchronous to potentially valid socket opens.  Thus the invalid socket
*                   close(s) MAY corrupt the socket's valid operation(s).
*
*                   However, since the primary tasks of the network protocol suite are prevented
*                   from running concurrently (see 'net.h  Note #3'), it is NOT necessary to protect
*                   socket resources from possible corruption since no asynchronous access from
*                   other network tasks is possible.
*
*               (4) On any socket close handler error(s), socket already discarded in close handler
*                   (see 'NetSock_FreeHandler()  Note #3a').
*
*                   See also 'NetSock_Close()  Note #2'.
*
*               (5) If the socket is secure, secure session MUST be closed before closing the socket.
*********************************************************************************************************
*/

static  void  NetSock_CloseSockHandler (NET_SOCK     *p_sock,
                                        CPU_BOOLEAN   close_conn,
                                        CPU_BOOLEAN   close_conn_transport,
                                        RTOS_ERR     *p_err)
{
    NET_CONN_ID  conn_id;


    switch (p_sock->State) {
        case NET_SOCK_STATE_CLOSED:
        case NET_SOCK_STATE_CLOSE_IN_PROGRESS:
        case NET_SOCK_STATE_CLOSING_DATA_AVAIL:
        case NET_SOCK_STATE_BOUND:
        case NET_SOCK_STATE_LISTEN:
        case NET_SOCK_STATE_CONN:
        case NET_SOCK_STATE_CONN_IN_PROGRESS:
        case NET_SOCK_STATE_CONN_DONE:
        default:
             break;
                                                                /* Sock NOT re-closed if already free/closed.           */
        case NET_SOCK_STATE_NONE:
        case NET_SOCK_STATE_FREE:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
             goto exit;

        case NET_SOCK_STATE_CLOSED_FAULT:
             goto exit;
    }

                                                                /* -------------------- FREE SOCK --------------------- */
    conn_id = p_sock->ID_Conn;                                  /* Get sock's net conn id  (see Note #2b1).             */
    NetSock_FreeHandler(p_sock);

                                                                /* ------------------ CLOSE CONN(S) ------------------- */
    if (close_conn == DEF_YES) {                                /* If req'd, close conn(s) [see Note #2a].              */
        NetConn_CloseFromApp(conn_id, close_conn_transport);
    }

                                                                /* Chk free err(s) AFTER conn close(s) [see Note #2aB]. */
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {           /* On  free err(s), abort sock close   (see Note #4).   */
        goto exit;
    }

                                                                /* ------------------ CLR SOCK CONN ------------------- */
    p_sock->ID_Conn = NET_CONN_ID_NONE;                         /* Clr sock's net conn id (see Note #2b2).              */

exit:
    return;
}


/*
*********************************************************************************************************
*                                    NetSock_CloseSockFromClose()
*
* Description : (1) Close a socket due to socket close/closing faults :
*
*                   (a) Update socket close statistic(s)
*                   (b) Close  socket
*
*
*               (2) (a) (1) MUST ONLY be called by socket closing functions (see Note #2b).
*
*                       (2) Close socket connection for the following socket close/closing faults :
*
*                           (A) Socket close parameters are corrupted :
*                               (1) Type
*                               (2) State
*
*                   (b) Frees socket resources, closes socket connection(s), returns socket to CLOSED
*                       state AND frees the socket since the close was initiated by the application.
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Close(),
*                                                 NetSock_CloseHandlerStream().
*
* Return(s)   : none.
*
* Note(s)     : (3) A socket closing with fault(s) SHOULD close its network connection(s).
*
*               (4) If socket close handler fails, socket already discarded in close handler.
*********************************************************************************************************
*/

static  void  NetSock_CloseSockFromClose (NET_SOCK  *p_sock)
{
                                                                /* ---------------- UPDATE CLOSE STATS ---------------- */
    NET_CTR_ERR_INC(Net_ErrCtrs.Sock.CloseCtr);
                                                                /* -------------------- CLOSE SOCK -------------------- */
    NetSock_CloseHandler(p_sock, DEF_YES, DEF_YES);             /* See Note #3.                                         */
}


/*
*********************************************************************************************************
*                                         NetSock_CloseConn()
*
* Description : (1) Close a socket's network connection(s) :
*
*                   (a) Remove connection handle identifier from socket's connection accept queue
*                   (b) Close  network connection(s)
*
*               (2) Closes socket network connection(s) ONLY; does NOT free or close socket(s).
*
*
* Argument(s) : conn_id     Handle identifier of network connection to close.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_CloseConn (NET_CONN_ID  conn_id)
{
    NetSock_CloseConnFree(conn_id);
    NetConn_CloseFromApp(conn_id, DEF_YES);                     /* See Note #1b.                                        */
}
#endif


/*
*********************************************************************************************************
*                                       NetSock_CloseConnFree()
*
* Description : Close/free network connection from socket.
*
* Argument(s) : conn_id     Handle identifier of network connection.
*
* Return(s)   : none.
*
* Note(s)     : (1) Network connections are de-referenced from cloned socket application connections.
*********************************************************************************************************
*/

#ifdef NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_CloseConnFree (NET_CONN_ID  conn_id)
{
    NET_SOCK_ID  sock_id;


                                                                /* ------------------- GET SOCK ID -------------------- */
    sock_id = (NET_SOCK_ID)NetConn_ID_AppCloneGet(conn_id);     /* Get clone sock app conn (see Note #1).               */

                                                                /* --------------- FREE CONN FROM SOCK ---------------- */
    NetSock_FreeConnFromSock(sock_id, conn_id);
}
#endif


/*
*********************************************************************************************************
*                                           NetSock_Free()
*
* Description : (1) Free a socket :
*
*                   (a) Remove connnection id from parent socket accept queue
*                   (b) Clear  socket controls
*                   (b) Free   socket back to socket pool
*                   (c) Update socket pool statistics
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Close(),
*                                                 NetSock_CloseHandler(),
*                                                 NetSock_CloseHandlerStream().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetSock_Free (NET_SOCK  *p_sock)
{
    RTOS_ERR   local_err;
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NET_SOCK  *p_sock_parent;
#endif


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    if (p_sock->ID_SockParent != NET_SOCK_ID_NONE) {
                                                                /* ----- REMOVE CONN ID FROM PARENT CONN ACCEPT Q ----- */
        p_sock_parent = &NetSock_Tbl[p_sock->ID_SockParent];
        p_sock_parent->ConnChildQ_SizeCur--;                    /* Dec conn accept Q cur size.                          */
    }

    p_sock->ID_SockParent = NET_SOCK_ID_NONE;
#endif

                                                                /* --------------------- CLR SOCK --------------------- */
#ifdef  NET_SECURE_MODULE_EN
    if (DEF_BIT_IS_SET(p_sock->Flags, NET_SOCK_FLAG_SOCK_SECURE)) {
        NetSecure_SockClose(p_sock, &local_err);                /* Clr secure session.                                  */
    }
#endif

    p_sock->State = NET_SOCK_STATE_FREE;                        /* Set sock as freed/NOT used.                          */
    DEF_BIT_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_USED);

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
    NetSock_Clr(p_sock);
#endif

                                                                /* -------------------- FREE SOCK --------------------- */
    p_sock->NextPtr  = NetSock_PoolPtr;
    NetSock_PoolPtr = p_sock;

#if (NET_STAT_POOL_SOCK_EN == DEF_ENABLED)
                                                                /* -------------- UPDATE SOCK POOL STATS -------------- */
    NetStat_PoolEntryUsedDec(&NetSock_PoolStat, &local_err);
#endif

    PP_UNUSED_PARAM(local_err);
}


/*
*********************************************************************************************************
*                                        NetSock_FreeHandler()
*
* Description : (1) Free a socket :
*
*                   (a) Free  socket address                                    See Note #2
*                   (b) Clear socket connection
*                   (c) Free  socket packet buffer queues
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument checked in NetSock_CloseSockHandler().
*
* Return(s)   : none.
*
* Note(s)     : (2) Since connection address information is required to free the socket address,
*                   the socket address MUST be freed PRIOR to the network connection.
*
*               (3) (a) On ANY free socket connection handler error, socket is discarded & caller
*                       function MUST NOT further handle or re-discard the socket.
*
*                   (b) ALL network resources linked to the socket MUST be freed PRIOR to socket
*                       free or discard so that no network resources are lost.
*
*                   (c) Error code returned by 'p_err' for a socket discard refers to the last
*                       free socket error ONLY.
*
*               (4) See 'net_sock.h  NETWORK SOCKET DATA TYPE  Note #2'.
*********************************************************************************************************
*/

static  void  NetSock_FreeHandler (NET_SOCK  *p_sock)
{
                                                                /* ------------------ CLR SOCK CONN ------------------- */
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN

    NetSock_ConnReqAbort(p_sock);                               /* Abort wait on sock conn req.                         */

    NetSock_ConnReqClr(p_sock);

    NetSock_ConnAcceptQ_Abort(p_sock);                          /* Abort wait on sock conn accept Q.                    */

    NetSock_ConnAcceptQ_Clr(p_sock);                            /* Clr sock conn accept Q.                              */

    NetSock_ConnAcceptQ_SemClr(p_sock);

    NetSock_ConnCloseAbort(p_sock);                             /* Abort wait on sock conn close.                       */

    NetSock_ConnCloseClr(p_sock);
#endif

                                                                /* FREE SOCK Q's                                        */
    NetSock_FreeBufQ(&p_sock->RxQ_Head, &p_sock->RxQ_Tail);
#if 0                                                           /* See Note #4.                                         */
    NetSock_FreeBufQ(&p_sock->TxQ_Head, &p_sock->TxQ_Tail);
#endif

    NetSock_RxQ_Abort(p_sock);                                  /* Abort wait on sock rx Q.                             */

    NetSock_RxQ_Clr(p_sock);                                    /* Clr sock rx Q.                                       */
}


/*
*********************************************************************************************************
*                                         NetSock_FreeBufQ()
*
* Description : Free a socket's buffer queue.
*
* Argument(s) : p_buf_q_head     Pointer to a socket buffer queue's head pointer.
*
*               p_buf_q_tail     Pointer to a socket buffer queue's tail pointer.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetSock_FreeBufQ (NET_BUF  **p_buf_q_head,
                                NET_BUF  **p_buf_q_tail)
{
    NET_BUF  *p_buf_q;

                                                                /* Free buf Q.                                          */
    p_buf_q = *p_buf_q_head;
   (void)NetBuf_FreeBufQ_PrimList(p_buf_q, DEF_NULL);
                                                                /* Clr  buf Q ptrs to NULL.                             */
   *p_buf_q_head = DEF_NULL;
   *p_buf_q_tail = DEF_NULL;
}


/*
*********************************************************************************************************
*                                            NetSock_Clr()
*
* Description : Clear socket controls.
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument validated in NetSock_Init(),
*                                                 NetSock_Get(),
*                                                 NetSock_Free().
*
* Return(s)   : none.
*
* Note(s)     : (1) See 'net_sock.h  NETWORK SOCKET DATA TYPE  Note #2'.
*********************************************************************************************************
*/

static  void  NetSock_Clr (NET_SOCK  *p_sock)
{
    p_sock->NextPtr          =  DEF_NULL;
    p_sock->RxQ_Head         =  DEF_NULL;
    p_sock->RxQ_Tail         =  DEF_NULL;
#if 0                                                           /* See Note #1.                                         */
    p_sock->TxQ_Head         =  DEF_NULL;
    p_sock->TxQ_Tail         =  DEF_NULL;
#endif

    p_sock->RxQ_SizeCfgd     =  NET_SOCK_CFG_RX_Q_SIZE_OCTET;
    p_sock->RxQ_SizeCur      =  0;
    p_sock->TxQ_SizeCfgd     =  NET_SOCK_CFG_TX_Q_SIZE_OCTET;
#if 0                                                           /* See Note #1.                                         */
    p_sock->TxQ_SizeCur      =  0;
#endif

    p_sock->ID_Conn          =  NET_CONN_ID_NONE;
    p_sock->ProtocolFamily   =  NET_SOCK_PROTOCOL_FAMILY_NONE;
    p_sock->Protocol         =  NET_SOCK_PROTOCOL_NONE;
    p_sock->SockType         =  NET_SOCK_TYPE_NONE;

#ifdef  NET_SECURE_MODULE_EN
    p_sock->SecureSession    =  DEF_NULL;
#endif

    p_sock->State            =  NET_SOCK_STATE_FREE;
    p_sock->Flags            =  NET_SOCK_FLAG_SOCK_NONE;

#if (NET_SOCK_DFLT_NO_BLOCK_EN == DEF_ENABLED)
    DEF_BIT_SET(p_sock->Flags,  NET_SOCK_FLAG_SOCK_NO_BLOCK);
#endif


#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
                                                                /* ------------ CLR SOCK CONN ACCEPT Q VALS ----------- */
    p_sock->ConnAcceptQ_SizeMax = NET_SOCK_Q_SIZE_NONE;
    p_sock->ConnAcceptQ_SizeCur = 0u;

    p_sock->ConnChildQ_SizeMax  = NET_SOCK_Q_SIZE_UNLIMITED;
    p_sock->ConnChildQ_SizeCur  = 0u;

    SList_Init(&p_sock->ConnAcceptQ_Ptr);
#endif

                                                                /* ------------ CFG SOCK DFLT TIMEOUT VALS ------------ */
    NetSock_RxQ_TimeoutDflt(p_sock);

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
    NetSock_ConnReqTimeoutDflt(p_sock);
    NetSock_ConnAcceptQ_TimeoutDflt(p_sock);
    NetSock_ConnCloseTimeoutDflt(p_sock);
#endif
}


/*
*********************************************************************************************************
*                                           NetSock_Copy()
*
* Description : Copy a socket.
*
* Argument(s) : p_sock_dest      Pointer to socket to receive socket copy.
*               -----------      Argument validated in NetSock_Accept().
*
*               p_sock_src       Pointer to socket to copy.
*               ----------       Argument validated in NetSock_Accept().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  void  NetSock_Copy (NET_SOCK  *p_sock_dest,
                            NET_SOCK  *p_sock_src)
{
    p_sock_dest->ProtocolFamily = p_sock_src->ProtocolFamily;
    p_sock_dest->Protocol       = p_sock_src->Protocol;
    p_sock_dest->SockType       = p_sock_src->SockType;

    p_sock_dest->ID_Conn        = p_sock_src->ID_Conn;

    p_sock_dest->State          = p_sock_src->State;

    p_sock_dest->Flags          = p_sock_src->Flags;
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_RandomPortNbrGet()
*
* Description : Get a random port number from the random port number queue.
*
*               (1) Random port number queue is a FIFO Q implemented as a circular ring array :
*
*                   (a) 'NetSock_RandomPortNbrQ_HeadIx' points to the next available port number.
*
*                   (b) 'NetSock_RandomPortNbrQ_TailIx' points to the next available queue entry
*                       to insert freed port numbers.
*
*                   (c) 'NetSock_RandomPortNbrQ_HeadIx'/'NetSock_RandomPortNbrQ_TailIx' advance :
*
*                       (1) By increment;
*                       (2) Reset to minimum index value when maximum index value reached.
*
*
*                                  Index to next available
*                                 port number in random port     Index to next available
*                                       number queue            entry to free port number
*                                      (see Note #1a)                (see Note #1b)
*
*                                             |                             |
*                                             |                             |
*                                             v                             v
*                              -------------------------------------------------------
*                              |     |     |     |     |     |     |     |     |     |
*                              |     |     |     |     |     |     |     |     |     |
*                              |     |     |     |     |     |     |     |     |     |
*                              -------------------------------------------------------
*
*                                                    ---------->
*                                               FIFO indices advance by
*                                              increment (see Note #1c1)
*
*                              |                                                     |
*                              |<-------------- Circular Ring FIFO Q --------------->|
*                              |                   (see Note #1)                     |
*
*
* Argument(s) : protocol        Network protocol type.
*
* Return(s)   : Random port number,     if NO error(s).
*               NET_SOCK_PORT_NBR_NONE, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  NET_PORT_NBR  NetSock_RandomPortNbrGet (NET_PROTOCOL_TYPE   protocol)
{
    CPU_BOOLEAN   used     = DEF_YES;
    NET_PORT_NBR  port_nbr = NetSock_RandomPortNbrCur;



    while (used != DEF_NO) {
        used = NetConn_IsPortUsed(port_nbr, protocol);

        if (used == DEF_YES) {
            NetSock_RandomPortNbrCur = (NET_PORT_NBR)NetUtil_RandomRangeGet(NET_SOCK_PORT_NBR_RANDOM_MIN,
                                                                            NET_SOCK_PORT_NBR_RANDOM_MAX);
            port_nbr                 =  NetSock_RandomPortNbrCur;
        }
    }

    NetSock_RandomPortNbrCur++;

    return (port_nbr);
}


/*
*********************************************************************************************************
*                                          NetSock_OpGetSock()
*
* Description : Get the specified socket option from the p_sock socket.
*
* Argument(s) : p_sock       Pointer to socket to get the option from.
*
*               opt          Socket option to get the   value.
*
*               p_opt_val    Pointer to a socket option value buffer.
*
*               p_opt_len    Pointer to a socket option value buffer length.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : NET_SOCK_BSD_ERR_NONE,    if NO error(s).
*               NET_SOCK_BSD_ERR_OPT_GET, otherwise.
*
* Note(s)     : (1) The supported options are:
*
*                   (a) Level NET_SOCK_PROTOCOL_SOCK:
*
*                       Option name                     Returned data type    Option decription
*                       -----------------------------   ------------------    ------------------
*                       NET_SOCK_OPT_SOCK_TYPE          NET_SOCK_TYPE         Socket type:
*                                                                                 NET_SOCK_TYPE_STREAM
*                                                                                 NET_SOCK_TYPE_DATAGRAM
*
*                       NET_SOCK_OPT_SOCK_KEEP_ALIVE    CPU_BOOLEAN           Socket keep-alive status:
*                                                                                 DEF_ENABLED
*                                                                                 DEF_DISABLED
*
*                       NET_SOCK_OPT_SOCK_ACCEPT_CONN   CPU_BOOLEAN           Socket is in listen state:
*                                                                                 DEF_YES
*                                                                                 DEF_NO
*
*                       NET_SOCK_OPT_SOCK_TX_BUF_SIZE   NET_TCP_WIN_SIZE      TCP connection transmit windows size  value
*                       NET_SOCK_OPT_SOCK_RX_BUF_SIZE   NET_TCP_WIN_SIZE      TCP connection receive  windows size  value
*                       NET_SOCK_OPT_SOCK_TX_TIMEOUT    CPU_INT32U            TCP connection transmit queue timeout value
*                       NET_SOCK_OPT_SOCK_RX_TIMEOUT    CPU_INT32U            TCP connection receive  queue timeout value
*
*               (2) NetLock must be aquired before calling this function.
*********************************************************************************************************
*/

static  NET_SOCK_RTN_CODE  NetSock_OpGetSock (NET_SOCK           *p_sock,
                                              NET_SOCK_OPT_NAME   opt_name,
                                              void               *p_opt_val,
                                              NET_SOCK_OPT_LEN   *p_opt_len,
                                              RTOS_ERR           *p_err)
{
#ifdef  NET_TCP_MODULE_EN
    NET_TCP_CONN       *p_conn;
    NET_CONN_ID         conn_id;
    NET_CONN_ID         conn_id_transport;
    CPU_INT32U          timeout_ms;
    CPU_BOOLEAN         sock_listen;
    CPU_BOOLEAN         is_used;
#endif


    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_STREAM:
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
             switch (p_sock->Protocol) {
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_id = p_sock->ID_Conn;

                      is_used = NetConn_IsUsed(conn_id);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          return (NET_SOCK_BSD_ERR_OPT_GET);
                      }

                      conn_id_transport = NetConn_ID_TransportGet(conn_id);

                      is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          return (NET_SOCK_BSD_ERR_OPT_GET);
                      }

                      switch (opt_name) {
                          case NET_SOCK_OPT_SOCK_TYPE:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(NET_SOCK_TYPE)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                              *p_opt_len = sizeof(NET_SOCK_TYPE);

                               Mem_Copy(p_opt_val,
                                       &p_sock->SockType,
                                       *p_opt_len);
                               break;


                          case NET_SOCK_OPT_SOCK_TX_BUF_SIZE:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(NET_TCP_WIN_SIZE)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                              *p_opt_len =  sizeof(NET_TCP_WIN_SIZE);
                               p_conn    = &NetTCP_ConnTbl[conn_id_transport];

                               Mem_Copy(p_opt_val,
                                       &p_conn->TxWinSizeCfgd,
                                       *p_opt_len);
                               break;


                          case NET_SOCK_OPT_SOCK_RX_BUF_SIZE:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(NET_TCP_WIN_SIZE)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                              *p_opt_len =  sizeof(NET_TCP_WIN_SIZE);
                               p_conn    = &NetTCP_ConnTbl[conn_id_transport];

                               Mem_Copy(p_opt_val,
                                       &p_conn->RxWinSizeCfgd,
                                       *p_opt_len);
                               break;


                          case NET_SOCK_OPT_SOCK_TX_TIMEOUT:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(CPU_INT32U)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                              *p_opt_len  =  sizeof(CPU_INT32U);
                               timeout_ms = NetTCP_TxQ_TimeoutGet_ms(conn_id_transport);

                               Mem_Copy(p_opt_val,
                                       &timeout_ms,
                                       *p_opt_len);
                               break;


                          case NET_SOCK_OPT_SOCK_RX_TIMEOUT:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(CPU_INT32U)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                              *p_opt_len  = sizeof(CPU_INT32U);
                               timeout_ms = NetTCP_RxQ_TimeoutGet_ms(conn_id_transport);

                               Mem_Copy(p_opt_val,
                                       &timeout_ms,
                                       *p_opt_len);
                               break;


                          case NET_SOCK_OPT_SOCK_ACCEPT_CONN:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(CPU_BOOLEAN)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                               sock_listen = (p_sock->State == NET_SOCK_STATE_LISTEN);
                              *p_opt_len   =  sizeof(CPU_BOOLEAN);

                               Mem_Copy(p_opt_val,
                                       &sock_listen,
                                       *p_opt_len);
                               break;


                          case NET_SOCK_OPT_SOCK_KEEP_ALIVE:
                               RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(CPU_BOOLEAN)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                               conn_id = p_sock->ID_Conn;

                               is_used = NetConn_IsUsed(conn_id);
                               if (is_used != DEF_YES) {
                                   RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                                   return (NET_SOCK_BSD_ERR_OPT_GET);
                               }

                               conn_id_transport = NetConn_ID_TransportGet(conn_id);

                               p_conn    = &NetTCP_ConnTbl[conn_id_transport];
                              *p_opt_len =  sizeof(CPU_BOOLEAN);

                               Mem_Copy(p_opt_val,
                                       &p_conn->TxKeepAliveEn,
                                       *p_opt_len);
                               break;


                          default:
                               RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
                      }
                      break;


                 default:
                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
             }
#else
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_NOT_AVAIL, NET_SOCK_BSD_ERR_OPT_GET);
#endif
             break;


        case NET_SOCK_TYPE_DATAGRAM:
             switch (opt_name) {
                 case NET_SOCK_OPT_SOCK_TYPE:
                      RTOS_ASSERT_DBG_ERR_SET((*p_opt_len >= (CPU_INT32S)sizeof(NET_SOCK_TYPE)), *p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);

                     *p_opt_len = sizeof(NET_SOCK_TYPE);

                      Mem_Copy(p_opt_val,
                              &p_sock->SockType,
                              *p_opt_len);
                      break;

                 default:
                      RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
             }
             break;


        default:
             RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_INVALID_ARG, NET_SOCK_BSD_ERR_OPT_GET);
    }

    return (NET_SOCK_BSD_ERR_NONE);
}


/*
*********************************************************************************************************
*                                     NetSock_CfgTxNagleEnHandler()
*
* Description : (1) Configure Scoket's TCP connection's transmit Nagle algorithm enable.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of the socket to configure the Nagle
*               -------     TCP transmission algorithm enable.
*
*               nagle_en    Desired state the Nagle algorithm enable.
*
*               p_err        Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_OK,   If the Nagle algorithm was successfully set,
*               DEF_FAIL, Otherwise.
*
* Note(s)     : (2) NetSock_CfgTxNagleEnHandler() MUST be called with the global network lock already acquired.
*
*               (3) RFC #1122, Section 4.2.3.4 also states that "a TCP SHOULD implement the Nagle
*                   Algorithm ... However, there MUST be a way for an application to disable the
*                   Nagle algorithm on an individual connection".
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
static  CPU_BOOLEAN  NetSock_CfgTxNagleEnHandler (NET_SOCK_ID   sock_id,
                                                  CPU_BOOLEAN   nagle_en,
                                                  RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
    NET_CONN_ID   conn_id;
    NET_CONN_ID   conn_id_transport;
    CPU_BOOLEAN   is_used;
    CPU_BOOLEAN   rtn_val  = DEF_FAIL;


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)                        /* ---------------- VALIDATE SOCK USED ---------------- */
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
         goto exit;
    }
#endif


                                                                /* --------------- CFG TCP TX NAGLE EN ---------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
        case NET_SOCK_TYPE_STREAM:
             switch (p_sock->Protocol) {
#ifdef  NET_TCP_MODULE_EN
                 case NET_SOCK_PROTOCOL_TCP:
                      conn_id = p_sock->ID_Conn;

                      is_used = NetConn_IsUsed(conn_id);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit;
                      }

                      conn_id_transport = NetConn_ID_TransportGet(conn_id);

                      is_used = NetTCP_ConnIsUsed((NET_TCP_CONN_ID)conn_id_transport);
                      if (is_used != DEF_YES) {
                          RTOS_ERR_SET(*p_err, RTOS_ERR_NET_INVALID_CONN);
                          goto exit;
                      }

                     (void)NetTCP_ConnCfgTxNagleEnHandler((NET_TCP_CONN_ID)conn_id_transport,
                                                                           nagle_en);
                      break;
#endif

                 case NET_SOCK_PROTOCOL_NONE:
                 default:
                      NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidProtocolCtr);
                      RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
                      goto exit;
             }
             break;
#endif


        case NET_SOCK_TYPE_NONE:
        case NET_SOCK_TYPE_DATAGRAM:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit;
    }

    PP_UNUSED_PARAM(conn_id);                                   /* Prevent possible 'variable unused' warnings.         */
    PP_UNUSED_PARAM(conn_id_transport);

    rtn_val = DEF_OK;

exit:
    return (rtn_val);
}
#endif


/*
*********************************************************************************************************
*                                     NetSock_CfgIF_Handler()
*
* Description : Configure the interface that must be used by the socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of the socket to configure the interface
*               -------     number.
*
*               if_nbr      Interface number to bind to the socket.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : DEF_OK,   If the interface number was successfully set
*               DEF_FAIL, Otherwise.
*
* Note(s)     : NetSock_CfgIF_Handler() is called by network protocol suite function(s)
*               & MUST be called with the global network lock already acquired.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetSock_CfgIF_Handler (NET_SOCK_ID   sock_id,
                                            NET_IF_NBR    if_nbr,
                                            RTOS_ERR     *p_err)
{
    NET_SOCK     *p_sock;
    CPU_BOOLEAN   rtn_val = DEF_FAIL;
#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_BOOLEAN   is_used;
#endif


    RTOS_ASSERT_DBG_ERR_SET((if_nbr <= NET_IF_CFG_MAX_NBR_IF), *p_err, RTOS_ERR_INVALID_ARG, DEF_FAIL);


#if (RTOS_ARG_CHK_EXT_EN == DEF_ENABLED)
    is_used = NetSock_IsUsed(sock_id);
    if (is_used != DEF_YES) {
         RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_HANDLE);
         goto exit;
    }
#endif

                                                                /* ---------------- CFG SOCKET IF NBR ----------------- */
    p_sock = &NetSock_Tbl[sock_id];

    switch (p_sock->SockType) {
        case NET_SOCK_TYPE_STREAM:
        case NET_SOCK_TYPE_DATAGRAM:
             p_sock->IF_Nbr = if_nbr;
             break;

        case NET_SOCK_TYPE_NONE:
        default:
             NET_CTR_ERR_INC(Net_ErrCtrs.Sock.InvalidSockTypeCtr);
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit;
    }

    rtn_val = DEF_OK;

exit:
    return (rtn_val);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL */

