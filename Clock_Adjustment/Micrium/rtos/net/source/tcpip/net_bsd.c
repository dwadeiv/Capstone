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
*                                            BSD 4.x LAYER
*
* File : net_bsd.c
*********************************************************************************************************
* Note(s) : (1) Supports BSD 4.x Layer API with the following restrictions/constraints :
*
*               (a) ONLY supports the following BSD layer functionality :
*                   (1) BSD sockets
*
*               (b) Return variable 'errno' NOT currently supported
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

#include  <rtos/net/include/net_cfg_net.h>
#include  <rtos/net/include/net_bsd.h>
#include  <rtos/net/include/net_sock.h>
#include  <rtos/net/include/net_ascii.h>
#include  <rtos/net/include/net_util.h>
#include  <rtos/net/source/util/net_dict_priv.h>
#include  <rtos/net/include/net_app.h>
#include  <rtos/net/include/net_bsd.h>

#include  "net_sock_priv.h"
#include  "net_ascii_priv.h"

#ifdef  NET_DNS_CLIENT_MODULE_EN
#include  <rtos/net/include/dns_client.h>
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                                  (NET)
#define  RTOS_MODULE_CUR                               RTOS_CFG_MODULE_NET


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*
* Note(s) : (1) BSD 4.x global variables are required only for applications that call BSD 4.x functions.
*
*               See also 'MODULE  Note #1b'
*                      & 'STANDARD BSD 4.x FUNCTION PROTOTYPES  Note #1'.
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_BSD_SERVICE_FTP_DATA                      20
#define  NET_BSD_SERVICE_FTP                           21
#define  NET_BSD_SERVICE_TELNET                        23
#define  NET_BSD_SERVICE_SMTP                          25
#define  NET_BSD_SERVICE_DNS                           53
#define  NET_BSD_SERVICE_BOOTPS                        67
#define  NET_BSD_SERVICE_BOOTPC                        68
#define  NET_BSD_SERVICE_TFTP                          69
#define  NET_BSD_SERVICE_DNS                           53
#define  NET_BSD_SERVICE_HTTP                          80
#define  NET_BSD_SERVICE_NTP                          123
#define  NET_BSD_SERVICE_SNMP                         161
#define  NET_BSD_SERVICE_HTTPS                        443
#define  NET_BSD_SERVICE_SMTPS                        465


#define  NET_BSD_SERVICE_FTP_DATA_STR               "ftp_data"
#define  NET_BSD_SERVICE_FTP_STR                    "ftp"
#define  NET_BSD_SERVICE_TELNET_STR                 "telnet"
#define  NET_BSD_SERVICE_SMTP_STR                   "smtp"
#define  NET_BSD_SERVICE_DNS_STR                    "dns"
#define  NET_BSD_SERVICE_BOOTPS_STR                 "bootps"
#define  NET_BSD_SERVICE_BOOTPC_STR                 "bootpc"
#define  NET_BSD_SERVICE_TFTP_STR                   "tftp"
#define  NET_BSD_SERVICE_HTTP_STR                   "http"
#define  NET_BSD_SERVICE_SNMP_STR                   "snmp"
#define  NET_BSD_SERVICE_NTP_STR                    "ntp"
#define  NET_BSD_SERVICE_HTTPS_STR                  "https"
#define  NET_BSD_SERVICE_SMTPS_STR                  "smtps"



/*
*********************************************************************************************************
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  NET_IPv4_MODULE_EN
                                                                /* See Note #1.                                 */
static  CPU_CHAR   NetBSD_IP_to_Str_Array[NET_ASCII_LEN_MAX_ADDR_IPv4];
#endif

static  MEM_DYN_POOL  NetBSD_AddrInfoPool;
static  MEM_DYN_POOL  NetBSD_SockAddrPool;

const  NET_DICT  NetBSD_ServicesStrTbl[] = {
    { NET_BSD_SERVICE_FTP_DATA, NET_BSD_SERVICE_FTP_DATA_STR, (sizeof(NET_BSD_SERVICE_FTP_DATA_STR) - 1) },
    { NET_BSD_SERVICE_FTP,      NET_BSD_SERVICE_FTP_STR,      (sizeof(NET_BSD_SERVICE_FTP_STR)      - 1) },
    { NET_BSD_SERVICE_TELNET,   NET_BSD_SERVICE_TELNET_STR,   (sizeof(NET_BSD_SERVICE_TELNET_STR)   - 1) },
    { NET_BSD_SERVICE_SMTP,     NET_BSD_SERVICE_SMTP_STR,     (sizeof(NET_BSD_SERVICE_SMTP_STR)     - 1) },
    { NET_BSD_SERVICE_DNS,      NET_BSD_SERVICE_DNS_STR,      (sizeof(NET_BSD_SERVICE_DNS_STR)      - 1) },
    { NET_BSD_SERVICE_BOOTPS,   NET_BSD_SERVICE_BOOTPS_STR,   (sizeof(NET_BSD_SERVICE_BOOTPS_STR)   - 1) },
    { NET_BSD_SERVICE_BOOTPC,   NET_BSD_SERVICE_BOOTPC_STR,   (sizeof(NET_BSD_SERVICE_BOOTPC_STR)   - 1) },
    { NET_BSD_SERVICE_TFTP,     NET_BSD_SERVICE_TFTP_STR,     (sizeof(NET_BSD_SERVICE_TFTP_STR)     - 1) },
    { NET_BSD_SERVICE_HTTP,     NET_BSD_SERVICE_HTTP_STR,     (sizeof(NET_BSD_SERVICE_HTTP_STR)     - 1) },
    { NET_BSD_SERVICE_SNMP,     NET_BSD_SERVICE_SNMP_STR,     (sizeof(NET_BSD_SERVICE_SNMP_STR)     - 1) },
    { NET_BSD_SERVICE_NTP,      NET_BSD_SERVICE_NTP_STR,      (sizeof(NET_BSD_SERVICE_NTP_STR)      - 1) },
    { NET_BSD_SERVICE_HTTPS,    NET_BSD_SERVICE_HTTPS_STR,    (sizeof(NET_BSD_SERVICE_HTTPS_STR)    - 1) },
    { NET_BSD_SERVICE_SMTPS,    NET_BSD_SERVICE_SMTPS_STR,    (sizeof(NET_BSD_SERVICE_SMTPS_STR)    - 1) },
};


typedef  enum   net_bsd_sock_protocol {
    NET_BSD_SOCK_PROTOCOL_UNKNOWN,
    NET_BSD_SOCK_PROTOCOL_UDP,
    NET_BSD_SOCK_PROTOCOL_TCP,
    NET_BSD_SOCK_PROTOCOL_TCP_UDP,
    NET_BSD_SOCK_PROTOCOL_UDP_TCP,
} NET_BSD_SOCK_PROTOCOL;


typedef  struct net_bsd_service_proto {
    NET_PORT_NBR           Port;
    NET_BSD_SOCK_PROTOCOL  Protocol;
} NET_BSD_SERVICE_PROTOCOL;




const  NET_BSD_SERVICE_PROTOCOL  NetBSD_ServicesProtocolTbl[] = {
        { NET_BSD_SERVICE_FTP_DATA, NET_BSD_SOCK_PROTOCOL_TCP     },
        { NET_BSD_SERVICE_FTP,      NET_BSD_SOCK_PROTOCOL_TCP     },
        { NET_BSD_SERVICE_TELNET,   NET_BSD_SOCK_PROTOCOL_TCP     },
        { NET_BSD_SERVICE_SMTP,     NET_BSD_SOCK_PROTOCOL_TCP     },
        { NET_BSD_SERVICE_DNS,      NET_BSD_SOCK_PROTOCOL_UDP_TCP },
        { NET_BSD_SERVICE_BOOTPS,   NET_BSD_SOCK_PROTOCOL_UDP_TCP },
        { NET_BSD_SERVICE_BOOTPC,   NET_BSD_SOCK_PROTOCOL_UDP_TCP },
        { NET_BSD_SERVICE_TFTP,     NET_BSD_SOCK_PROTOCOL_UDP     },
        { NET_BSD_SERVICE_HTTP,     NET_BSD_SOCK_PROTOCOL_TCP_UDP },
        { NET_BSD_SERVICE_SNMP,     NET_BSD_SOCK_PROTOCOL_TCP_UDP },
        { NET_BSD_SERVICE_NTP,      NET_BSD_SOCK_PROTOCOL_TCP_UDP },
        { NET_BSD_SERVICE_HTTPS,    NET_BSD_SOCK_PROTOCOL_TCP_UDP },
        { NET_BSD_SERVICE_SMTPS,    NET_BSD_SOCK_PROTOCOL_TCP     },
};

#ifdef  NET_IPv6_MODULE_EN
const  struct  in6_addr  in6addr_any                  = NET_IPv6_ADDR_ANY_INIT;
const  struct  in6_addr  in6addr_loopback             = IN6ADDR_LOOPBACK_INIT;
const  struct  in6_addr  in6addr_linklocal_allnodes   = IN6ADDR_LINKLOCAL_ALLNODES_INIT;
const  struct  in6_addr  in6addr_linklocal_allrouters = IN6ADDR_LINKLOCAL_ALLROUTERS_INIT;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     LOCAL FUNCTION PROTOTYPTES
*********************************************************************************************************
*********************************************************************************************************
*/

static  struct  addrinfo    *NetBSD_AddrInfoGet    (struct  addrinfo             **pp_head,
                                                    struct  addrinfo             **pp_tail);

static          void         NetBSD_AddrInfoFree   (struct  addrinfo              *p_addrinfo);

static          CPU_BOOLEAN  NetBSD_AddrInfoSet    (struct  addrinfo              *p_addrinfo,
                                                            NET_SOCK_ADDR_FAMILY   family,
                                                            NET_PORT_NBR           port_nbr,
                                                            CPU_INT08U            *p_addr,
                                                            NET_IP_ADDR_LEN        addr_len,
                                                            NET_SOCK_PROTOCOL      protocol,
                                                            CPU_CHAR              *p_canonname);

static          void         NetBSD_AddrCfgValidate(        CPU_BOOLEAN           *p_ipv4_cfgd,
                                                            CPU_BOOLEAN           *p_ipv6_cfgd);


/*
*********************************************************************************************************
*                                     STANDARD BSD 4.x FUNCTIONS
*
* Note(s) : (1) BSD 4.x function definitions are required only for applications that call BSD 4.x functions.
*
*               See 'net_bsd.h  MODULE  Note #1b3'
*                 & 'net_bsd.h  STANDARD BSD 4.x FUNCTION PROTOTYPES  Note #1'.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             NetBSD_Init()
*
* Description : Initialize BSD module.
*
* Argument(s) : p_err   Pointer to variable that will hold the return error code from this function.
*
* Return(s)   : none.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetBSD_Init (RTOS_ERR  *p_err)
{
    RTOS_ERR  err;


    Mem_DynPoolCreate("BSD AddrInfo Pool",
                      &NetBSD_AddrInfoPool,
                       DEF_NULL,
                       sizeof(struct addrinfo),
                       sizeof(CPU_SIZE_T),
                       0,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_ALLOC);
        goto exit;
    }


    Mem_DynPoolCreate("BSD AddrInfo Pool",
                      &NetBSD_SockAddrPool,
                       DEF_NULL,
                       sizeof(struct sockaddr),
                       sizeof(CPU_SIZE_T),
                       0,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_ALLOC);
        goto exit;
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                              socket()
*
* Description : Create a socket.
*
* Argument(s) : protocol_family     Socket protocol family :
*
*                                       PF_INET     Internet Protocol version 4 (IPv4).
*                                       PF_INET6    Internet Protocol version 6 (IPv6).
*
*               sock_type           Socket type :
*
*                                       SOCK_DGRAM              Datagram-type socket.
*                                       SOCK_STREAM             Stream  -type socket.
*
*               protocol            Socket protocol :
*
*                                       0                       Default protocol for socket type.
*                                       IPPROTO_UDP             User Datagram        Protocol (UDP).
*                                       IPPROTO_TCP             Transmission Control Protocol (TCP).
*
* Return(s)   : Socket descriptor/handle identifier, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int  socket (int  protocol_family,
             int  sock_type,
             int  protocol)
{
    int       rtn_code;
    RTOS_ERR  local_err;



    rtn_code = (int)NetSock_Open((NET_SOCK_PROTOCOL_FAMILY)protocol_family,
                                 (NET_SOCK_TYPE           )sock_type,
                                 (NET_SOCK_PROTOCOL       )protocol,
                                                          &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                               close()
*
* Description : Close a socket.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to close.
*
* Return(s)   :  0, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : (1) Once an application closes its socket, NO further operations on the socket are allowed
*                   & the application MUST NOT continue to access the socket.
*********************************************************************************************************
*/

int  close (int  sock_id)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_Close(sock_id,
                                 &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                               bind()
*
* Description : Bind a socket to a local address.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to bind to a local address.
*
*               p_addr_local    Pointer to socket address structure.
*
*               addr_len        Length  of socket address structure (in octets).
*
* Return(s)   :  0, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : (1) (a) Socket address structure 'sa_family' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*********************************************************************************************************
*/

int  bind (        int         sock_id,
           struct  sockaddr   *p_addr_local,
                   socklen_t   addr_len)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_Bind(                 sock_id,
                                 (NET_SOCK_ADDR *)p_addr_local,
                                                  addr_len,
                                                 &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                              connect()
*
* Description : Connect a socket to a remote server.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to connect.
*
*               p_addr_remote   Pointer to socket address structure (see Note #1).
*
*               addr_len        Length  of socket address structure (in octets).
*
* Return(s)   :  0, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : (1) (a) Socket address structure 'sa_family' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*********************************************************************************************************
*/

int  connect (        int         sock_id,
              struct  sockaddr   *p_addr_remote,
                      socklen_t   addr_len)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_Conn(                 sock_id,
                                 (NET_SOCK_ADDR *)p_addr_remote,
                                                  addr_len,
                                                 &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                              listen()
*
* Description : Set socket to listen for connection requests.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to listen.
*
*               sock_q_size     Number of connection requests to queue on listen socket.
*
* Return(s)   :  0, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
int  listen (int  sock_id,
             int  sock_q_size)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_Listen(sock_id,
                                   sock_q_size,
                                  &local_err);

    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                              accept()
*
* Description : Get a new socket accepted from a socket set to listen for connection requests.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of listen socket.
*
*               p_addr_remote   Pointer to an address buffer that will receive the socket address structure
*                               of the accepted socket's remote address, if NO error(s).
*
*               p_addr_len      Pointer to a variable to pass the size of the address buffer pointed to
*                               by 'p_addr_remote' and return the actual size of socket address
*                               structure with the accepted socket's remote address, if NO error(s);
*                               Return 0, otherwise.
*
* Return(s)   : Socket descriptor/handle identifier of new accepted socket, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : (1) (a) Socket address structure 'sa_family' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*********************************************************************************************************
*/
#ifdef  NET_SOCK_TYPE_STREAM_MODULE_EN
int  accept (        int         sock_id,
             struct  sockaddr   *p_addr_remote,
                     socklen_t  *p_addr_len)
{
    int                 rtn_code;
    NET_SOCK_ADDR_LEN   addr_len;
    RTOS_ERR            local_err;


    addr_len   = (NET_SOCK_ADDR_LEN)*p_addr_len;
    rtn_code   = (int)NetSock_Accept((NET_SOCK_ID        ) sock_id,
                                     (NET_SOCK_ADDR     *) p_addr_remote,
                                     (NET_SOCK_ADDR_LEN *)&addr_len,
                                                          &local_err);

   *p_addr_len = (socklen_t)addr_len;

    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                             recvfrom()
*
* Description : Receive data from a socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to receive data.
*
*               p_data_buf      Pointer to an application data buffer that will receive the socket's received
*                               data.
*
*               data_buf_len    Size of the   application data buffer (in octets).
*
*               flags           Flags to select receive options; bit-field flags logically OR'd :
*
*                                   0                           No socket flags selected.
*                                   MSG_PEEK                    Receive socket data without consuming the socket data.
*                                   MSG_DONTWAIT                Receive socket data without blocking.
*
*               p_addr_remote   Pointer to an address buffer that will receive the socket address structure
*                               with the received data's remote address, if NO error(s).
*
*               p_addr_len      Pointer to a variable to pass the size of the address buffer pointed to
*                               by 'p_addr_remote' and return the actual size of socket address structure
*                               with the received data's remote address, if NO error(s). Return 0, otherwise.
*
* Return(s)   : Number of positive data octets received, if NO error(s).
*                0,  if socket connection closed.
*               -1,  otherwise.
*
* Note(s)     : (1) (a) (1) Datagram-type sockets transmit & receive all data atomically -- i.e. every
*                           single, complete datagram transmitted MUST be received as a single, complete
*                           datagram.
*
*                       (2) Thus if the socket's type is datagram & the receive data buffer size is
*                           NOT large enough for the received data, the receive data buffer is maximally
*                           filled with receive data but the remaining data octets are discarded &
*                           NET_SOCK_ERR_INVALID_DATA_SIZE error is returned.
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
*               (2) Only some socket receive flag options are implemented.  If other flag options are requested,
*                   socket receive handler function(s) abort & return appropriate error codes so that requested
*                   flag options are NOT silently ignored.
*
*               (3) (a) Socket address structure 'sa_family' member returned in host-order & SHOULD NOT
*                       be converted to network-order.
*
*                   (b) Socket address structure addresses returned in network-order & SHOULD be converted
*                       from network-order to host-order.
*
*               (4) IEEE Std 1003.1, 2004 Edition, Section 'recvfrom() : RETURN VALUE' states that :
*
*                   (a) "Upon successful completion, recvfrom() shall return the length of the message in
*                        bytes."
*
*                   (b) "If no messages are available to be received and the peer has performed an orderly
*                        shutdown, recvfrom() shall return 0."
*
*                   (c) (1) "Otherwise, [-1 shall be returned]" ...
*                       (2) "and 'errno' set to indicate the error."
*                           'errno' NOT currently supported.
*********************************************************************************************************
*/

ssize_t  recvfrom (        int         sock_id,
                           void       *p_data_buf,
                          _size_t      data_buf_len,
                           int         flags,
                   struct  sockaddr   *p_addr_remote,
                           socklen_t  *p_addr_len)
{
    ssize_t             rtn_code;
    NET_SOCK_ADDR_LEN   addr_len;
    RTOS_ERR            local_err;


    if (data_buf_len > DEF_INT_16U_MAX_VAL) {
        return (NET_BSD_ERR_DFLT);
    }

    addr_len   = (NET_SOCK_ADDR_LEN)*p_addr_len;
    rtn_code   = (ssize_t)NetSock_RxDataFrom((NET_SOCK_ID        ) sock_id,
                                             (void              *) p_data_buf,
                                             (CPU_INT16U         ) data_buf_len,
                                             (NET_SOCK_API_FLAGS ) flags,
                                             (NET_SOCK_ADDR     *) p_addr_remote,
                                             (NET_SOCK_ADDR_LEN *)&addr_len,
                                                                   DEF_NULL,
                                             (CPU_INT08U         ) 0u,
                                                                   DEF_NULL,
                                                                  &local_err);

   *p_addr_len = (socklen_t)addr_len;

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                               recv()
*
* Description : Receive data from a socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to receive data.
*
*               p_data_buf      Pointer to an application data buffer that will receive the socket's received
*                                   data.
*
*               data_buf_len    Size of the application data buffer (in octets).
*
*               flags           Flags to select receive options; bit-field flags logically OR'd :
*
*                                   0               No socket flags selected.
*                                   MSG_PEEK        Receive socket data without consuming the socket data.
*                                   MSG_DONTWAIT    Receive socket data without blocking.
*
* Return(s)   : Number of positive data octets received, if NO error(s).
*                0, if socket connection closed.
*               -1, otherwise.
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
*               (2) Only some socket receive flag options are implemented. If other flag options are requested,
*                   socket receive handler function(s) abort & return appropriate error codes so that requested
*                   flag options are NOT silently ignored.
*
*               (3) IEEE Std 1003.1, 2004 Edition, Section 'recv() : RETURN VALUE' states that :
*
*                   (a) "Upon successful completion, recv() shall return the length of the message in bytes."
*
*                   (b) "If no messages are available to be received and the peer has performed an orderly
*                        shutdown, recv() shall return 0."
*
*                   (c) (1) "Otherwise, -1 shall be returned" ...
*                       (2) "and 'errno' set to indicate the error."
*                           'errno' NOT currently supported.
*********************************************************************************************************
*/

ssize_t  recv (int      sock_id,
               void    *p_data_buf,
              _size_t   data_buf_len,
               int      flags)
{
    ssize_t   rtn_code;
    RTOS_ERR  local_err;


    if (data_buf_len > DEF_INT_16U_MAX_VAL) {
        return (NET_BSD_ERR_DFLT);
    }

    rtn_code = (ssize_t)NetSock_RxData((NET_SOCK_ID       ) sock_id,
                                       (void             *) p_data_buf,
                                       (CPU_INT16U        ) data_buf_len,
                                       (NET_SOCK_API_FLAGS) flags,
                                                           &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                              sendto()
*
* Description : Send data through a socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to send data.
*
*               p_data          Pointer to application data to send.
*
*               data_len        Length of  application data to send (in octets).
*
*               flags           Flags to select send options; bit-field flags logically OR'd :
*
*                                   0               No socket flags selected.
*                                   MSG_DONTWAIT    Send socket data without blocking.
*
*               p_addr_remote   Pointer to destination address buffer;
*                                   required for datagram sockets, optional for stream sockets.
*
*               addr_len        Length of  destination address buffer (in octets).
*
* Return(s)   : Number of positive data octets sent, if NO error(s).
*                0, if socket connection closed.
*               -1, otherwise.
*
* Note(s)     : (1) (a) (1) (A) Datagram-type sockets send & receive all data atomically -- i.e. every single,
*                               complete datagram  sent MUST be received as a single, complete datagram.
*                               Thus each call to send data MUST be transmitted in a single, complete datagram.
*
*                           (B) Since IP transmit fragmentation is NOT currently supported, if the socket's
*                               type is datagram & the requested send data length is greater than the
*                               socket/transport layer MTU, then NO data is sent & RTOS_ERR_WOULD_OVF
*                               error is returned.
*
*                       (2) (A) (1) Stream-type sockets send & receive all data octets in one or more non-
*                                   distinct packets.  In other words, the application data is NOT bounded
*                                   by any specific packet(s); rather, it is contiguous & sequenced from
*                                   one packet to the next.
*
*                               (2) Thus if the socket's type is stream & the socket's send data queue(s) are
*                                   NOT large enough for the send data, the send data queue(s) are maximally
*                                   filled with send data & the remaining data octets are discarded but may be
*                                   re-sent by later application-socket sends.
*
*                               (3) Therefore, NO stream-type socket send data length should be "too long to
*                                   pass through the underlying protocol" & cause the socket send to "fail ...
*                                   [with] no data ... transmitted".
*
*                           (B) Thus it is typical -- but NOT absolutely required -- that a single application
*                               task ONLY send or request to send data to a stream-type socket.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*               (2) Only some socket send flag options are implemented.  If other flag options are requested,
*                   socket send handler function(s) abort & return appropriate error codes so that requested
*                   flag options are NOT silently ignored.
*
*               (3) (a) Socket address structure 'sa_family' member MUST be configured in host-order &
*                       MUST NOT be converted to/from network-order.
*
*                   (b) Socket address structure addresses MUST be configured/converted from host-order
*                       to network-order.
*
*               (4) (a) IEEE Std 1003.1, 2004 Edition, Section 'sendto() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, sendto() shall return the number of bytes sent."
*
*                           (A) Section 'sendto() : DESCRIPTION' elaborates that "successful completion
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
*                               (1) Section 'sendto() : DESCRIPTION' elaborates that "a return value of
*                                   -1 indicates only locally-detected errors".
*
*                           (B) "and 'errno' set to indicate the error."
*                               'errno' NOT currently supported.
*
*                   (b) Although NO socket send() specification states to return '0' when the socket's
*                       connection is closed, it seems reasonable to return '0' since it is possible for the
*                       socket connection to be close()'d or shutdown() by the remote host.
*********************************************************************************************************
*/

ssize_t  sendto (        int         sock_id,
                         void       *p_data,
                        _size_t      data_len,
                         int         flags,
                 struct  sockaddr   *p_addr_remote,
                         socklen_t   addr_len)
{
    ssize_t   rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (ssize_t)NetSock_TxDataTo((NET_SOCK_ID       ) sock_id,
                                         (void             *) p_data,
                                         (CPU_INT16U        ) data_len,
                                         (NET_SOCK_API_FLAGS) flags,
                                         (NET_SOCK_ADDR    *) p_addr_remote,
                                         (NET_SOCK_ADDR_LEN ) addr_len,
                                                             &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                               send()
*
* Description : Send data through a socket.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to send data.
*
*               p_data          Pointer to application data to send.
*
*               data_len        Length of  application data to send (in octets).
*
*               flags           Flags to select send options; bit-field flags logically OR'd :
*
*                                   0               No socket flags selected.
*                                   MSG_DONTWAIT    Send socket data without blocking.
*
* Return(s)   : Number of positive data octets sent, if NO error(s).
*                0, if socket connection closed.
*               -1, otherwise.
*
* Note(s)     : (1) (a) (1) (A) Datagram-type sockets send & receive all data atomically -- i.e. every single,
*                               complete datagram  sent MUST be received as a single, complete datagram.
*                               Thus each call to send data MUST be transmitted in a single, complete datagram.
*
*                           (B) (1) IEEE Std 1003.1, 2004 Edition, Section 'send() : DESCRIPTION' states that
*                                   "if the message is too long to pass through the underlying protocol, send()
*                                   shall fail and no data shall be transmitted".
*
*                               (2) Since IP transmit fragmentation is NOT currently supported (see 'net_ip.h
*                                   Note #1d'), if the socket's type is datagram & the requested send data
*                                   length is greater than the socket/transport layer MTU, then NO data is
*                                   sent & NET_SOCK_ERR_INVALID_DATA_SIZE error is returned.
*
*                       (2) (A) (1) Stream-type sockets send & receive all data octets in one or more non-
*                                   distinct packets.  In other words, the application data is NOT bounded
*                                   by any specific packet(s); rather, it is contiguous & sequenced from
*                                   one packet to the next.
*
*                               (2) Thus if the socket's type is stream & the socket's send data queue(s) are
*                                   NOT large enough for the send data, the send data queue(s) are maximally
*                                   filled with send data & the remaining data octets are discarded but may be
*                                   re-sent by later application-socket sends.
*
*                               (3) Therefore, NO stream-type socket send data length should be "too long to
*                                   pass through the underlying protocol" & cause the socket send to "fail ...
*                                   [with] no data ... transmitted".
*
*                           (B) Thus it is typical -- but NOT absolutely required -- that a single application
*                               task ONLY send or request to send data to a stream-type socket.
*
*                   (b) 'data_len' of 0 octets NOT allowed.
*
*               (2) Only some socket send flag options are implemented.  If other flag options are requested,
*                   socket send handler function(s) abort & return appropriate error codes so that requested
*                   flag options are NOT silently ignored.
*
*               (3) (a) IEEE Std 1003.1, 2004 Edition, Section 'send() : RETURN VALUE' states that :
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
*                               'errno' NOT currently supported.
*
*                   (b) Although NO socket send() specification states to return '0' when the socket's
*                       connection is closed, it seems reasonable to return '0' since it is possible for the
*                       socket connection to be close()'d or shutdown() by the remote host.
*********************************************************************************************************
*/

ssize_t  send (int      sock_id,
               void    *p_data,
              _size_t   data_len,
               int      flags)
{
    ssize_t   rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (ssize_t)NetSock_TxData((NET_SOCK_ID       ) sock_id,
                                       (void             *) p_data,
                                       (CPU_INT16U        ) data_len,
                                       (NET_SOCK_API_FLAGS) flags,
                                                           &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                              select()
*
* Description : Check multiple file descriptors for available resources &/or operations.
*
* Argument(s) : desc_nbr_max    Maximum number of file descriptors in the file descriptor sets.
*
*               p_desc_rd       Pointer to a set of file descriptors to :
*
*                                   (a) Check for available read operation(s).
*
*                                   (b) (1) Return the actual file descriptors ready for available
*                                               read  operation(s), if NO error(s);
*                                       (2) Return the initial, non-modified set of file descriptors,
*                                               on any error(s);
*                                       (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                               if any timeout expires.
*
*               p_desc_wr       Pointer to a set of file descriptors to :
*
*                                   (a) Check for available write operation(s).
*
*                                   (b) (1) Return the actual file descriptors ready for available
*                                               write operation(s), if NO error(s);
*                                       (2) Return the initial, non-modified set of file descriptors,
*                                               on any error(s);
*                                       (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                               if any timeout expires .
*
*               p_desc_err      Pointer to a set of file descriptors to :
*
*                                   (a) Check for any error(s) &/or exception(s).
*
*                                   (b) (1) Return the actual file descriptors flagged with any error(s)
*                                               &/or exception(s), if NO non-descriptor-related error(s);
*                                       (2) Return the initial, non-modified set of file descriptors,
*                                               on any error(s);
*                                       (3) Return a null-valued (i.e. zero-cleared) descriptor set,
*                                               if any timeout expires.
*
*               p_timeout        Pointer to a timeout.
*
* Return(s)   : Number of file descriptors with available resources &/or operations, if any.
*                0, on timeout.
*               -1, otherwise.
*
* Note(s)     : (1) (a) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that :
*
*                       (1) (A) "select() ... shall support" the following file descriptor types :
*
*                               (1) "regular files,"
*                               (2) "terminal and pseudo-terminal devices,"
*                               (3) "STREAMS-based files,"
*                               (4) "FIFOs,"
*                               (5) "pipes,"
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
*                       (1) (A) "The 'nfds' argument ('desc_nbr_max') specifies the range of descriptors to
*                                be tested.  The first 'nfds' descriptors shall be checked in each set; that
*                                is, the descriptors from zero through nfds-1 in the descriptor sets shall
*                                be examined."
*
*                           (B) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                               6th Printing, Section 6.3, Page 163 states that "the ['nfds'] argument"
*                               specifies :
*
*                               (1) "the number of descriptors," ...
*                               (2) "not the largest value."
*
*                       (2) "The select() function shall ... examine the file descriptor sets whose addresses
*                            are passed in the 'readfds' ('p_desc_rd'), 'writefds' ('p_desc_wr'), and 'errorfds'
*                            ('p_desc_err') parameters to see whether some of their descriptors are ready for
*                            reading, are ready for writing, or have an exceptional condition pending,
*                            respectively."
*
*                           (A) (1) (a) "If the 'readfds' argument ('p_desc_rd') is not a null pointer, it
*                                        points to an object of type 'fd_set' that on input specifies the
*                                        file descriptors to be checked for being ready to read, and on
*                                        output indicates which file descriptors are ready to read."
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
*                               (2) (a) "If the 'writefds' argument ('p_desc_wr') is not a null pointer, it
*                                        points to an object of type 'fd_set' that on input specifies the
*                                        file descriptors to be checked for being ready to write, and on
*                                        output indicates which file descriptors are ready to write."
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
*                               (3) (a) "If the 'errorfds' argument ('p_desc_err') is not a null pointer, it
*                                        points to an object of type 'fd_set' that on input specifies the file
*                                        descriptors to be checked for error conditions pending, and on output
*                                        indicates which file descriptors have error conditions pending."
*
*                                   (b) "A file descriptor ... shall be considered to have an exceptional
*                                        condition pending ... as noted below" :
*
*                                       (2) "If a socket has a pending error."
*
*                                       (3) "Other circumstances under which a socket may be considered to
*                                            have an exceptional condition pending are protocol-specific
*                                            and implementation-defined."
*
*                                   (d) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                                       6th Printing, Section 6.3, Page 165 states "that when an error occurs on
*                                       a socket, it is [also] marked as both readable and writeable by select()".
*
*                           (B) (1) (a) "Upon successful completion, ... select() ... shall" :
*
*                                       (1) "modify the objects pointed to by the 'readfds' ('p_desc_rd'),
*                                            'writefds' ('p_desc_wr'), and 'errorfds' ('p_desc_err') arguments
*                                            to indicate which file descriptors are ready for reading, ready
*                                            for writing, or have an error condition pending, respectively," ...
*
*                                       (2) "and shall return the total number of ready descriptors in all
*                                            the output sets."
*
*                                   (b) (1) "For each file descriptor less than nfds ('desc_nbr_max'), the
*                                            corresponding bit shall be set on successful completion" :
*
*                                           (A) "if it was set on input" ...
*                                           (B) "and the associated condition is true for that file descriptor."
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
*                                            ('timeval')."
*
*                                   (b) (1) (A) "If the 'readfds' ('p_desc_rd'), 'writefds' ('p_desc_wr'), and
*                                                'errorfds' ('p_desc_err') arguments are"                         ...
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
*                           (B) (1) "For the select() function, the timeout period is given ... in an argument
*                                   ('p_timeout') of type struct 'timeval'" ... :
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
*                   (c) (1) (A) IEEE Std 1003.1, 2004 Edition, Section 'select() : RETURN VALUE' states that :
*
*                               (1) "Upon successful completion, ... select() ... shall return the total
*                                    number of bits set in the bit masks."
*
*                               (2) (a) "Otherwise, -1 shall be returned," ...
*                                   (b) "and 'errno' shall be set to indicate the error."
*                                       'errno' NOT currently supported (see 'net_bsd.c  Note #1b').
*
*                           (B) Stevens/Fenner/Rudoff, UNIX Network Programming, Volume 1, 3rd Edition,
*                               6th Printing, Section 6.3, Page 161 states that BSD select() function
*                               "returns ... 0 on timeout".
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'select() : DESCRIPTION' states that :
*
*                           (A) "On failure, the objects pointed to by the 'readfds' ('p_desc_rd'), 'writefds'
*                                ('p_desc_wr'), and 'errorfds' ('p_desc_err') arguments shall not be modified."
*
*                           (B) "If the 'timeout' interval expires without the specified condition being
*                                true for any of the specified file descriptors, the objects pointed to
*                                by the 'readfds' ('p_desc_rd'), 'writefds' ('p_desc_wr'), and 'errorfds'
*                                ('p_desc_err') arguments shall have all bits set to 0."
*
*                   (d) IEEE Std 1003.1, 2004 Edition, Section 'select() : ERRORS' states that "under the
*                       following conditions, ... select() shall fail and set 'errno' to" :
*
*                       (1) "[EBADF] - One or more of the file descriptor sets specified a file descriptor
*                            that is not a valid open file descriptor."
*
*                       (2) "[EINVAL]" -
*
*                           (A) "The 'nfds' argument ('desc_nbr_max') is" :
*                               (1) "less than 0 or" ...
*                               (2) "greater than FD_SETSIZE."
*
*                           (B) "An invalid timeout interval was specified."
*
*                           'errno' NOT currently supported.
*********************************************************************************************************
*/

#if    (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
int  select (        int       desc_nbr_max,
             struct  fd_set   *p_desc_rd,
             struct  fd_set   *p_desc_wr,
             struct  fd_set   *p_desc_err,
             struct  timeval  *p_timeout)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_Sel((NET_SOCK_QTY      ) desc_nbr_max,
                                (NET_SOCK_DESC    *) p_desc_rd,
                                (NET_SOCK_DESC    *) p_desc_wr,
                                (NET_SOCK_DESC    *) p_desc_err,
                                (NET_SOCK_TIMEOUT *) p_timeout,
                                                    &local_err);

    return (rtn_code);
}
#endif


/*
*********************************************************************************************************
*                                             inet_addr()
*
* Description : Convert an IPv4 address in ASCII dotted-decimal notation to a network protocol IPv4 address
*                   in network-order.
*
* Argument(s) : p_addr      Pointer to an ASCII string that contains a dotted-decimal IPv4 address (see Note #2).
*
* Return(s)   : Network-order IPv4 address represented by ASCII string, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : (1) RFC #1983 states that "dotted decimal notation ... refers [to] IP addresses of the
*                   form A.B.C.D; where each letter represents, in decimal, one byte of a four byte IP
*                   address".
*
*                   In other words, the dotted-decimal notation separates four decimal octet values by
*                   the dot, or period, character ('.').  Each decimal value represents one octet of
*                   the IP address starting with the most significant octet in network-order.
*
*                       IP Address Examples : 192.168.1.64 = 0xC0A80140
*
*               (2) The dotted-decimal ASCII string MUST :
*
*                   (a) Include ONLY decimal values & the dot, or period, character ('.') ; ALL other
*                       characters trapped as invalid, including any leading or trailing characters.
*
*                   (b) Include UP TO four decimal values separated by UP TO three dot characters
*                       & MUST be terminated with the NULL character.
*
*                   (c) Ensure that each decimal value does NOT exceed the maximum octet value (i.e. 255).
*
*                   (d) Ensure that each decimal value does NOT include leading zeros.
*********************************************************************************************************
*/

#ifdef  NET_IPv4_MODULE_EN
in_addr_t  inet_addr (char  *p_addr)
{
    in_addr_t   addr;
    RTOS_ERR    local_err;


    addr = (in_addr_t)NetASCII_Str_to_IPv4((CPU_CHAR *)p_addr,
                                                      &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        addr = (in_addr_t)NET_BSD_ERR_DFLT;
    }

    addr =  NET_UTIL_HOST_TO_NET_32(addr);

    return (addr);
}
#endif


/*
*********************************************************************************************************
*                                             inet_ntoa()
*
* Description : Convert a network protocol IPv4 address into a dotted-decimal notation ASCII string.
*
* Argument(s) : addr        IPv4 address.
*
* Return(s)   : Pointer to ASCII string of converted IPv4 address, if NO error(s).
*               Pointer to NULL, otherwise.
*
* Note(s)     : (1) RFC #1983 states that "dotted decimal notation ... refers [to] IP addresses of the
*                   form A.B.C.D; where each letter represents, in decimal, one byte of a four byte IP
*                   address".
*
*                   In other words, the dotted-decimal notation separates four decimal octet values by
*                   the dot, or period, character ('.').  Each decimal value represents one octet of
*                   the IP address starting with the most significant octet in network-order.
*
*                       IP Address Examples : 192.168.1.64 = 0xC0A80140
*
*               (2) IEEE Std 1003.1, 2004 Edition, Section 'inet_ntoa() : DESCRIPTION' states that
*                   "inet_ntoa() ... need not be reentrant ... [and] is not required to be thread-safe".
*
*                   Since the character string is returned in a single, global character string array,
*                   this conversion function is NOT re-entrant.
*********************************************************************************************************
*/

#ifdef  NET_IPv4_MODULE_EN
char  *inet_ntoa (struct  in_addr  addr)
{
    in_addr_t   addr_ip;
    RTOS_ERR    local_err;


    addr_ip = addr.s_addr;
    addr_ip = NET_UTIL_NET_TO_HOST_32(addr_ip);

    NetASCII_IPv4_to_Str((NET_IPv4_ADDR) addr_ip,
                         (CPU_CHAR    *)&NetBSD_IP_to_Str_Array[0],
                         (CPU_BOOLEAN  ) DEF_NO,
                                        &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
        return (DEF_NULL);
    }

    return ((char *)&NetBSD_IP_to_Str_Array[0]);
}
#endif


/*
*********************************************************************************************************
*                                             inet_aton()
*
* Description : Convert an IPv4 address in ASCII dotted-decimal notation to a network protocol IPv4 address
*               in network-order.
*
* Argument(s) : p_addr_in   Pointer to an ASCII string that contains a dotted-decimal IPv4 address.
*
*               p_addr      Pointer to an IPv4 address.
*
* Return(s)   : 1  if the supplied address is valid,
*               0, otherwise.
*
* Note(s)     : (1) RFC #1983 states that "dotted decimal notation ... refers [to] IP addresses of the
*                   form A.B.C.D; where each letter represents, in decimal, one byte of a four byte IP
*                   address".
*
*                   In other words, the dotted-decimal notation separates four decimal octet values by
*                   the dot, or period, character ('.').  Each decimal value represents one octet of
*                   the IP address starting with the most significant octet in network-order.
*
*                       IP Address Examples : 192.168.1.64 = 0xC0A80140
*
*               (2) IEEE Std 1003.1, 2004 Edition - inet_addr, inet_ntoa - IPv4 address manipulation:
*
*                   (a) Values specified using IPv4 dotted decimal notation take one of the following forms:
*
*                       (1) a.b.c.d - When four parts are specified, each shall be interpreted
*                                     as a byte of data and assigned, from left to right,
*                                     to the four bytes of an Internet address.
*
*                       (2) a.b.c   - When a three-part address is specified, the last part shall
*                                     be interpreted as a 16-bit quantity and placed in the
*                                     rightmost two bytes of the network address. This makes
*                                     the three-part address format convenient for specifying
*                                     Class B network addresses as "128.net.host".
*
*                       (3) a.b     - When a two-part address is supplied, the last part shall be
*                                     interpreted as a 24-bit quantity and placed in the
*                                     rightmost three bytes of the network address. This makes
*                                     the two-part address format convenient for specifying
*                                     Class A network addresses as "net.host".
*
*                       (4) a       - When only one part is given, the value shall be stored
*                                     directly in the network address without any byte rearrangement.
*
*               (3) The dotted-decimal ASCII string MUST :
*
*                   (a) Include ONLY decimal values & the dot, or period, character ('.') ; ALL other
*                       characters trapped as invalid, including any leading or trailing characters.
*
*                   (b) Include UP TO four decimal values separated by UP TO three dot characters
*                       & MUST be terminated with the NULL character.
*
*                   (c) Ensure that each decimal value does NOT exceed the maximum value for its form:
*
*                       (1) a.b.c.d - 255.255.255.255
*                       (2) a.b.c   - 255.255.65535
*                       (3) a.b     - 255.16777215
*                       (4) a       - 4294967295
*
*                   (d) Ensure that each decimal value does NOT include leading zeros.
*********************************************************************************************************
*/
#ifdef  NET_IPv4_MODULE_EN
int  inet_aton(       char     *p_addr_in,
               struct in_addr  *p_addr)
{
    in_addr_t    addr;
    CPU_INT08U   pdot_nbr;
    RTOS_ERR     local_err;


    addr = (in_addr_t)NetASCII_Str_to_IPv4_Handler(p_addr_in,
                                                  &pdot_nbr,
                                                  &local_err);

    if ((RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE)                 ||
        (pdot_nbr                      > NET_ASCII_NBR_MAX_DOT_ADDR_IP)) {

        addr = (in_addr_t)NET_BSD_ERR_NONE;
        p_addr->s_addr = addr;

        return (DEF_FAIL);
    }

    addr =  NET_UTIL_HOST_TO_NET_32(addr);
    p_addr->s_addr = addr;

    return (DEF_OK);
}
#endif


/*
*********************************************************************************************************
*                                            setsockopt()
*
* Description : Set socket option.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to set the option.
*
*               protocol    Protocol level at which the option resides.
*
*               opt_name    Name of the single socket option to set.
*
*               p_opt_val   Pointer to the socket option value to set.
*
*               opt_len     Option length.
*
* Return(s)   :  0, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int  setsockopt (int         sock_id,
                 int         protocol,
                 int         opt_name,
                 void       *p_opt_val,
                 socklen_t   opt_len)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_OptSet((NET_SOCK_ID      ) sock_id,
                                   (NET_SOCK_PROTOCOL) protocol,
                                   (NET_SOCK_OPT_NAME) opt_name,
                                   (void            *) p_opt_val,
                                   (NET_SOCK_OPT_LEN ) opt_len,
                                                      &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                            getsockopt()
*
* Description : Get socket option.
*
* Argument(s) : sock_id     Socket descriptor/handle identifier of socket to get the option.
*
*               protocol    Protocol level at which the option resides.
*
*               opt_name    Name of the single socket option to get.
*
*               p_opt_val   Pointer to the socket option value to get.
*
*               opt_len     Option length.
*
* Return(s)   :  0, if NO error(s).
*               -1, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int  getsockopt (int         sock_id,
                 int         protocol,
                 int         opt_name,
                 void       *p_opt_val,
                 socklen_t  *p_opt_len)
{
    int       rtn_code;
    RTOS_ERR  local_err;


    rtn_code = (int)NetSock_OptGet((NET_SOCK_ID       ) sock_id,
                                   (NET_SOCK_PROTOCOL ) protocol,
                                   (NET_SOCK_OPT_NAME ) opt_name,
                                   (void             *) p_opt_val,
                                   (NET_SOCK_OPT_LEN *) p_opt_len,
                                                       &local_err);

    return (rtn_code);
}


/*
*********************************************************************************************************
*                                             getaddrinfo()
*
* Description : Converts human-readable text strings representing hostnames or IP addresses into a
*               dynamically allocated linked list of struct addrinfo structures.
*
* Argument(s) : p_node_name     A pointer to a string that contains a host (node) name or a numeric host
*                               address string. For the Internet protocol, the numeric host address string
*                               is a dotted-decimal IPv4 address or an IPv6 hex address.
*
*               p_service_name  A pointer to a string that contains either a service name or port number
*                               represented as a string.
*
*               p_hints         A pointer to an addrinfo structure that provides hints about the type of
*                               socket the caller supports.
*
*               res             A pointer to a linked list of one or more addrinfo structures that contains
*                               response information about the host.
*
* Return(s)   : 0, if no error,
*
*               Most nonzero error codes returned map to the set of errors outlined by Internet Engineering
*               Task Force (IETF) recommendations:
*
*                   EAI_ADDRFAMILY
*                   EAI_AGAIN
*                   EAI_BADFLAGS
*                   EAI_FAIL
*                   EAI_FAMILY
*                   EAI_MEMORY
*                   EAI_NONAME
*                   EAI_OVERFLOW
*                   EAI_SERVICE
*                   EAI_SOCKTYPE
*                   EAI_SYSTEM
*
* Note(s)     : none.
*********************************************************************************************************
*/

int  getaddrinfo (const          char       *p_node_name,
                  const          char       *p_service_name,
                  const  struct  addrinfo   *p_hints,
                         struct  addrinfo  **pp_res)
{
#ifdef  NET_DNS_CLIENT_MODULE_EN
           NET_HOST_IP_ADDR       *p_hosts_addr      = DEF_NULL;
#endif
    struct addrinfo               *p_addrinfo;
    struct addrinfo               *p_addrinfo_head   = DEF_NULL;
    struct addrinfo               *p_addrinfo_tail   = DEF_NULL;
           NET_IP_ADDR_FAMILY      hints_addr_family = NET_IP_ADDR_FAMILY_UNKNOWN;
           NET_BSD_SOCK_PROTOCOL   hints_protocol    = NET_BSD_SOCK_PROTOCOL_UNKNOWN;
           NET_PORT_NBR            service_port      = NET_PORT_NBR_NONE;
           CPU_BOOLEAN             service_port_num  = DEF_NO;
           NET_BSD_SOCK_PROTOCOL   service_protocol  = NET_BSD_SOCK_PROTOCOL_UNKNOWN;
           NET_SOCK_PROTOCOL       protocol;
           CPU_CHAR               *p_canonname       = DEF_NULL;
           CPU_BOOLEAN             valid_cfdg        = DEF_NO;
           CPU_BOOLEAN             host_num          = DEF_NO;
           CPU_BOOLEAN             wildcard          = DEF_NO;
           CPU_BOOLEAN             result;
           int                     rtn_val           = 0;
           CPU_BOOLEAN             set_ipv4;
           CPU_BOOLEAN             set_ipv6;
#ifndef  NET_DNS_CLIENT_MODULE_EN
           NET_IP_ADDR_FAMILY      ip_family;
           RTOS_ERR                err_net;
#endif


    PP_UNUSED_PARAM(pp_res);

#ifdef  NET_IPv4_MODULE_EN
    set_ipv4 = DEF_YES;
#else
    set_ipv4 = DEF_NO;
#endif

#ifdef  NET_IPv6_MODULE_EN
    set_ipv6 = DEF_YES;
#else
    set_ipv6 = DEF_NO;
#endif



    if ((p_node_name    == DEF_NULL) &&
        (p_service_name == DEF_NULL)) {
        rtn_val = EAI_NONAME;
        goto exit;
    }


    if (p_hints != DEF_NULL) {
        set_ipv4 = DEF_NO;
        set_ipv6 = DEF_NO;

        switch (p_hints->ai_socktype) {
            case 0:                                                 /* When ai_socktype is zero the caller will accept  */
                 hints_protocol = NET_BSD_SOCK_PROTOCOL_UNKNOWN;    /* any socket type.                                 */
                 break;

#ifdef  NET_TCP_MODULE_EN
            case SOCK_STREAM:
                 hints_protocol = NET_BSD_SOCK_PROTOCOL_TCP;
                 break;
#endif

            case SOCK_DGRAM:
                 hints_protocol = NET_BSD_SOCK_PROTOCOL_UDP;
                 break;

            default:
                 rtn_val = EAI_SOCKTYPE;
                 goto exit;
        }


        switch (p_hints->ai_family) {
            case AF_UNSPEC:                                         /* When ai_family is set to AF_UNSPEC, it means     */
                 hints_addr_family = NET_IP_ADDR_FAMILY_UNKNOWN;    /* the caller will accept any address family        */
                 break;                                             /* supported by the operating system.               */

#ifdef  NET_IPv4_MODULE_EN
            case AF_INET:
                 hints_addr_family = NET_IP_ADDR_FAMILY_IPv4;
                 break;
#endif

#ifdef  NET_IPv6_MODULE_EN
            case AF_INET6:
                 hints_addr_family = NET_IP_ADDR_FAMILY_IPv6;
                 break;
#endif

            default:
                 rtn_val = EAI_FAMILY;
                 goto exit;
        }


        switch (hints_addr_family) {
#ifdef  NET_IPv4_MODULE_EN
            case NET_IP_ADDR_FAMILY_IPv4:
                 set_ipv6 = DEF_NO;
                 break;
#endif

#ifdef  NET_IPv6_MODULE_EN
            case NET_IP_ADDR_FAMILY_IPv6:
                 set_ipv4 = DEF_NO;
                 break;
#endif

            case NET_IP_ADDR_FAMILY_UNKNOWN:
                 break;

            default:
                 rtn_val = EAI_SYSTEM;
                 goto exit;
        }


        wildcard         = DEF_BIT_IS_SET(p_hints->ai_flags, AI_PASSIVE);
                                                                /* If the AI_PASSIVE flag is specified in               */
                                                                /* hints.ai_flags, and node is NULL, then the returned  */
                                                                /* socket addresses will be suitable for bind(2)ing a   */
                                                                /* socket that will accept(2) connections.              */

        host_num         = DEF_BIT_IS_SET(p_hints->ai_flags, AI_NUMERICHOST);
                                                                /* If hints.ai_flags contains the AI_NUMERICHOST flag,  */
                                                                /* then node must be a numerical network address. The   */
                                                                /* AI_NUMERICHOST flag suppresses any potentially       */
                                                                /* lengthy network host address lookups.                */

        service_port_num = DEF_BIT_IS_SET(p_hints->ai_flags, AI_NUMERICSERV);
                                                                /* If AI_NUMERICSERV is specified in hints.ai_flags     */
                                                                /* and service is not NULL, then service must point to  */
                                                                /* a string containing a numeric port number.  This     */
                                                                /* flag is used to inhibit the invocation of a name     */
                                                                /* resolution service in cases where it is known not to */
                                                                /* be required.                                         */

        valid_cfdg       = DEF_BIT_IS_SET(p_hints->ai_flags, AI_ADDRCONFIG);
                                                                /* If the AI_ADDRCONFIG bit is set, IPv4 addresses      */
                                                                /* shall be returned only if an IPv4 address is         */
                                                                /* configured on the local system, and IPv6 addresses   */
                                                                /* shall be returned only if an IPv6 address is         */
                                                                /* configured on the local system.                      */
        if (valid_cfdg == DEF_YES) {
            NetBSD_AddrCfgValidate(&set_ipv4, &set_ipv6);
        }
    }   /* if (hints != DEF_NULL) */




    if (p_service_name != DEF_NULL) {                                  /* service sets the port in each returned address       */
                                                                /* structure.  If this argument is a service name , it  */
                                                                /* is translated to the corresponding port number.      */
        CPU_SIZE_T  len;
        CPU_INT32U  i;
        CPU_INT32U  obj_ctn = sizeof(NetBSD_ServicesProtocolTbl) / sizeof(NET_BSD_SERVICE_PROTOCOL);


        if (service_port_num == DEF_NO) {
            len = Str_Len_N(p_service_name, 255);
            service_port = NetDict_KeyGet(NetBSD_ServicesStrTbl, sizeof(NetBSD_ServicesStrTbl), p_service_name, DEF_NO, len);
            if (service_port >= NET_PORT_NBR_MAX) {
                service_port_num = DEF_YES;                     /* service argument can also be specified as a decimal  */
                                                                /* number, which is simply converted to binary.         */
            }
        }


        if (service_port_num == DEF_YES) {
            CPU_INT32U  val;

            val = Str_ParseNbr_Int32U(p_service_name, DEF_NULL, DEF_NBR_BASE_DEC);
            if ((val < NET_PORT_NBR_MIN) ||
                (val > NET_PORT_NBR_MAX)) {
                rtn_val = EAI_NONAME;
                goto exit;
            }

            service_port = (NET_PORT_NBR)val;
        }


        for (i = 0; i < obj_ctn; i++) {
            const NET_BSD_SERVICE_PROTOCOL  *p_obj = &NetBSD_ServicesProtocolTbl[i];

            if (service_port == p_obj->Port) {
                service_protocol = p_obj->Protocol;
                break;
            }
        }

        if (service_protocol == NET_BSD_SOCK_PROTOCOL_UNKNOWN) {
            rtn_val = EAI_SERVICE;
            goto exit;
        }
    } else {                                                    /* If service is NULL, then the port number of the      */
                                                                /* returned socket addresses will be left uninitialized */
    }


    switch (hints_protocol) {
        case NET_BSD_SOCK_PROTOCOL_UNKNOWN:
            switch (service_protocol) {
                case NET_BSD_SOCK_PROTOCOL_UNKNOWN:
                     protocol = NET_SOCK_PROTOCOL_NONE;
                     break;

                case NET_BSD_SOCK_PROTOCOL_UDP:
                case NET_BSD_SOCK_PROTOCOL_UDP_TCP:
                     protocol = NET_SOCK_PROTOCOL_UDP;
                     break;

                case NET_BSD_SOCK_PROTOCOL_TCP:
                case NET_BSD_SOCK_PROTOCOL_TCP_UDP:
                     protocol = NET_SOCK_PROTOCOL_TCP;
                     break;

                default:
                     rtn_val = EAI_SERVICE;
                     goto exit;
            }
            break;

        case NET_BSD_SOCK_PROTOCOL_UDP:
        case NET_BSD_SOCK_PROTOCOL_UDP_TCP:
             protocol = NET_SOCK_PROTOCOL_UDP;
             break;

        case NET_BSD_SOCK_PROTOCOL_TCP:
        case NET_BSD_SOCK_PROTOCOL_TCP_UDP:
             protocol = NET_SOCK_PROTOCOL_TCP;
             break;

        default:
             rtn_val = EAI_SERVICE;
             goto exit;
    }



    if (p_node_name == DEF_NULL) {
        if (set_ipv4 == DEF_YES) {
#ifdef  NET_IPv4_MODULE_EN
            if (wildcard == DEF_YES) {
                NET_IPv4_ADDR  addr = INADDR_ANY;
                                                                /* If the AI_PASSIVE flag is specified in               */
                                                                /* hints.ai_flags, and node is NULL, then the returned  */
                                                                /* socket addresses will be suitable for binding a      */
                                                                /* socket that will accept(2) connections.  The         */
                                                                /* returned socket address will contain the "wildcard   */
                                                                /* address" (INADDR_ANY for IPv4 addresses,             */
                                                                /* IN6ADDR_ANY_INIT for IPv6 address).                  */

                p_addrinfo = NetBSD_AddrInfoGet(&p_addrinfo_head, &p_addrinfo_tail);
                if (p_addrinfo == DEF_NULL) {
                    rtn_val = EAI_MEMORY;
                    goto exit_error;
                }


                result = NetBSD_AddrInfoSet(p_addrinfo,
                                            NET_SOCK_ADDR_FAMILY_IP_V4,
                                            service_port,
                             (CPU_INT08U *)&addr,
                                            sizeof(addr),
                                            protocol,
                                            p_canonname);
                if (result != DEF_OK) {
                    rtn_val = EAI_SYSTEM;
                    goto exit_error;
                }

            } else {
                NET_IPv4_ADDR  addr = INADDR_LOOPBACK;
                                                                /* If the AI_PASSIVE flag is not set in hints.ai_flags, */
                                                                /* then the returned socket addresses will be suitable  */
                                                                /* for use with connect(2), sendto(2), or sendmsg(2).   */
                                                                /* If node is NULL, then the network address will be    */
                                                                /* set to the loopback interface address (              */
                                                                /* INADDR_LOOPBACK for IPv4 addresses,                  */
                                                                /* IN6ADDR_LOOPBACK_INIT for IPv6 address); this is     */
                                                                /* used by applications that intend to communicate with */
                                                                /* peers running on the same host.                      */

                p_addrinfo = NetBSD_AddrInfoGet(&p_addrinfo_head, &p_addrinfo_tail);
                if (p_addrinfo == DEF_NULL) {
                    rtn_val = EAI_MEMORY;
                    goto exit_error;
                }

                result = NetBSD_AddrInfoSet(p_addrinfo,
                                            NET_SOCK_ADDR_FAMILY_IP_V4,
                                            service_port,
                             (CPU_INT08U *)&addr,
                                            sizeof(INADDR_LOOPBACK),
                                            protocol,
                                            p_canonname);
                if (result != DEF_OK) {
                    rtn_val = EAI_SYSTEM;
                    goto exit_error;
                }
            }
#endif
        } /* if (ipv4_set == DEF_YES) */



        if (set_ipv6 == DEF_YES) {
#ifdef  NET_IPv6_MODULE_EN
            if (wildcard == DEF_YES) {
                                                                /* If the AI_PASSIVE flag is specified in               */
                                                                /* hints.ai_flags, and node is NULL, then the returned  */
                                                                /* socket addresses will be suitable for binding a      */
                                                                /* socket that will accept(2) connections.  The         */
                                                                /* returned socket address will contain the "wildcard   */
                                                                /* address" (INADDR_ANY for IPv4 addresses,             */
                                                                /* IN6ADDR_ANY_INIT for IPv6 address).                  */
                p_addrinfo = NetBSD_AddrInfoGet(&p_addrinfo_head, &p_addrinfo_tail);
                if (p_addrinfo == DEF_NULL) {
                    rtn_val = EAI_MEMORY;
                    goto exit_error;
                }


                result = NetBSD_AddrInfoSet(p_addrinfo,
                                            NET_SOCK_ADDR_FAMILY_IP_V6,
                                            service_port,
                             (CPU_INT08U *)&in6addr_any,
                                            sizeof(in6addr_any),
                                            protocol,
                                            p_canonname);
                if (result != DEF_OK) {
                    rtn_val = EAI_SYSTEM;
                    goto exit_error;
                }

            } else {

                p_addrinfo = NetBSD_AddrInfoGet(&p_addrinfo_head, &p_addrinfo_tail);
                if (p_addrinfo == DEF_NULL) {
                    rtn_val = EAI_MEMORY;
                    goto exit_error;
                }

                result = NetBSD_AddrInfoSet(p_addrinfo,
                                            NET_SOCK_ADDR_FAMILY_IP_V6,
                                            service_port,
                             (CPU_INT08U *)&in6addr_loopback,
                                            sizeof(in6addr_loopback),
                                            protocol,
                                            p_canonname);
                if (result != DEF_OK) {
                    rtn_val = EAI_SYSTEM;
                    goto exit_error;
                }
            }
#endif
        } /* if (ipv6_set == DEF_YES) */


    } else { /* if (node == DEF_NULL) */
#ifdef  NET_DNS_CLIENT_MODULE_EN
        NET_HOST_IP_ADDR  *p_hosts_addr_cur;
        CPU_INT08U         host_addr_nbr;
        DNSc_FLAGS         dns_flags = DNSc_FLAG_NONE;
        DNSc_STATUS        status;
        RTOS_ERR           err_dns;


        if ((set_ipv4 == DEF_YES) &&
            (set_ipv6 == DEF_NO)) {
            DEF_BIT_SET(dns_flags, DNSc_FLAG_IPv4_ONLY);
        } else if ((set_ipv4 == DEF_YES) &&
                   (set_ipv6 == DEF_NO )) {
            DEF_BIT_SET(dns_flags, DNSc_FLAG_IPv6_ONLY);
        }

        status = DNSc_GetHostAddrs((CPU_CHAR *)p_node_name,
                                              &p_hosts_addr,
                                              &host_addr_nbr,
                                               dns_flags,
                                               DEF_NULL,
                                              &err_dns);
        switch (status) {
            case DNSc_STATUS_RESOLVED:
                 p_hosts_addr_cur = p_hosts_addr;
                 while (p_hosts_addr_cur != DEF_NULL) {
                     NET_SOCK_ADDR_FAMILY sock_family;


                     p_addrinfo = NetBSD_AddrInfoGet(&p_addrinfo_head, &p_addrinfo_tail);
                     if (p_addrinfo == DEF_NULL) {
                         rtn_val = EAI_MEMORY;
                         goto exit_error;
                     }

                     switch (p_hosts_addr_cur->AddrObj.AddrLen) {
                         case NET_IPv4_ADDR_LEN:
                              sock_family = NET_SOCK_ADDR_FAMILY_IP_V4;
                              break;

                         case NET_IPv6_ADDR_LEN:
                              sock_family = NET_SOCK_ADDR_FAMILY_IP_V6;
                              break;
                     }

                     result = NetBSD_AddrInfoSet(p_addrinfo,
                                                 sock_family,
                                                 service_port,
                                  (CPU_INT08U *)&p_hosts_addr_cur->AddrObj.Addr,
                                                 p_hosts_addr_cur->AddrObj.AddrLen,
                                                 protocol,
                                                 p_canonname);
                     if (result != DEF_OK) {
                         rtn_val = EAI_SYSTEM;
                         goto exit_error;
                     }

                     p_hosts_addr_cur = p_hosts_addr_cur->NextPtr;
                 }

                *pp_res = p_addrinfo_head;
                 DNSc_FreeHostAddrs(p_hosts_addr);
                 break;

            case DNSc_STATUS_PENDING:
                 rtn_val = EAI_AGAIN;
                 goto exit;

            case DNSc_STATUS_FAILED:
                 rtn_val = EAI_AGAIN;
                 goto exit;

            case DNSc_STATUS_UNKNOWN:
            case DNSc_STATUS_NONE:
            default:
                rtn_val = EAI_SYSTEM;
                goto exit;
        }

       (void)&host_num;                                         /* Prevent variable unused. Numeric conversion is done  */
                                                                /* by DNSc_GetHostAddrs as first step                   */

#else
        if (host_num == DEF_NO) {
            rtn_val = EAI_SYSTEM;
            goto exit_error;
        }

        p_addrinfo = NetBSD_AddrInfoGet(&p_addrinfo_head, &p_addrinfo_tail);
        if (p_addrinfo == DEF_NULL) {
            rtn_val = EAI_MEMORY;
            goto exit_error;
        }


        ip_family = NetASCII_Str_to_IP((CPU_CHAR *)p_node_name,
                                                   p_addrinfo->ai_addr,
                                                   sizeof(struct  sockaddr),
                                                  &err_net);
        if (RTOS_ERR_CODE_GET(err_net) == RTOS_ERR_NONE) {
            NET_SOCK_ADDR_FAMILY  sock_family;
            NET_IP_ADDR_LEN       addr_len;


            if (RTOS_ERR_CODE_GET(err_net) != RTOS_ERR_NONE) {
                rtn_val = EAI_SYSTEM;
                goto exit_error;
            }

            switch (ip_family) {
                case NET_IP_ADDR_FAMILY_IPv4:
                     sock_family = NET_SOCK_ADDR_FAMILY_IP_V4;
                     addr_len    = NET_IPv4_ADDR_LEN;
                     break;

                case NET_IP_ADDR_FAMILY_IPv6:
                     sock_family = NET_SOCK_ADDR_FAMILY_IP_V6;
                     addr_len    = NET_IPv6_ADDR_LEN;
                     break;

                default:
                    break;
            }


            result = NetBSD_AddrInfoSet(p_addrinfo,
                                        sock_family,
                                        service_port,
                          (CPU_INT08U *)p_addrinfo->ai_addr,
                                        addr_len,
                                        protocol,
                                        p_canonname);
            if (result != DEF_OK) {
                rtn_val = EAI_SYSTEM;
                goto exit_error;
            }
        }
#endif
    }


    goto exit;


exit_error:
#ifdef  NET_DNS_CLIENT_MODULE_EN
    DNSc_FreeHostAddrs(p_hosts_addr);
#endif
    NetBSD_AddrInfoFree(p_addrinfo_head);

exit:
    return (rtn_val);
}


/*
*********************************************************************************************************
*                                            freeaddrinfo()
*
* Description : Frees addrinfo structures information that getaddrinfo has allocated.
*
* Argument(s) : res     A pointer to the addrinfo structure or linked list of addrinfo structures to be freed.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void freeaddrinfo(struct addrinfo *res)
{
    NetBSD_AddrInfoFree(res);
}


/*
*********************************************************************************************************
*                                              inet_ntop()
*
* Description : Converts an IPv4 or IPv6 Internet network address into a string in Internet standard format.
*
* Argument(s) : af      Address family:
*
*                           AF_INET     Ipv4 Address Family
*                           AF_INET6    Ipv6 Address Family
*
*               src     A pointer to the IP address in network byte to convert to a string.
*
*               dst     A pointer to a buffer in which to store the NULL-terminated string representation of the IP address.
*
*               size    Length, in characters, of the buffer pointed to by dst.
*
* Return(s)   : Pointer to a buffer containing the string representation of IP address in standard format , If no error occurs.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

const  char  *inet_ntop (      int        af,
                         const void      *src,
                               char      *dst,
                               socklen_t  size)
{
    char      *p_rtn = DEF_NULL;
    RTOS_ERR   err;

    switch (af) {
#ifdef  NET_IPv4_MODULE_EN
        case AF_INET:
             if (size < INET_ADDRSTRLEN) {
                 goto exit;
             }
             NetASCII_IPv4_to_Str(*((NET_IPv4_ADDR *)src), dst, DEF_NO, &err);
             if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
                 goto exit;
             }
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case AF_INET6:
            if (size < INET6_ADDRSTRLEN) {
                goto exit;
            }
            NetASCII_IPv6_to_Str((NET_IPv6_ADDR *)src, dst, DEF_NO, DEF_NO, &err);
            if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
                goto exit;
            }
            break;
#endif

        default:
            goto exit;
    }

    p_rtn = dst;

exit:
    return ((const char *)p_rtn);
}


/*
*********************************************************************************************************
*                                              inet_pton()
*
* Description : Converts an IPv4 or IPv6 Internet network address in its standard text presentation form
*               into its numeric binary form.
*
* Argument(s) : af      Address family:
*
*                           AF_INET     Ipv4 Address Family
*                           AF_INET6    Ipv6 Address Family
*
*               src     A pointer to the NULL-terminated string that contains the text representation of
*                       the IP address to convert to numeric binary form.
*
*               dst     A pointer to a buffer that will receive the numeric binary representation of the
*                       IP address.
*
* Return(s)   : 1, if no error.
*
*               0, if src does not contain a character string representing a valid network address in
*                  the specified address family.
*
*               -1, if af does not contain a valid address family.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int  inet_pton (      int    af,
                const char  *src,
                      void  *dst)
{

    int       rtn = 0;
    RTOS_ERR  err;

#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR  *p_addr_ipv4 = (NET_IPv4_ADDR  *)dst;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_IPv6_ADDR  *p_addr_ipv6 = (NET_IPv6_ADDR  *)dst;
#endif

    switch (af) {
#ifdef  NET_IPv4_MODULE_EN
        case AF_INET:
            *p_addr_ipv4 = NetASCII_Str_to_IPv4((CPU_CHAR *)src, &err);
             if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
                 goto exit;
             }
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case AF_INET6:

            *p_addr_ipv6 = NetASCII_Str_to_IPv6((CPU_CHAR *)src, &err);
             if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
                 goto exit;
             }
             break;
#endif

        default:
            rtn = -1;
            goto exit;
    }

    rtn = 1;

exit:
    return (rtn);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         NetBSD_AddrInfoGet()
*
* Description : Allocate and link to a list an addrinfo structure
*
* Argument(s) : pp_head     Pointer to the head pointer list
*
*               pp_tail     Pointer to the tail pointer list
*
* Return(s)   : Pointer to the allocated addrinfo structure.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  struct  addrinfo *NetBSD_AddrInfoGet (struct  addrinfo  **pp_head,
                                              struct  addrinfo  **pp_tail)
{
    struct  addrinfo  *p_addrinfo;
    struct  sockaddr  *p_sockaddr;
    struct  addrinfo  *p_addrinfo_rtn = DEF_NULL;
            RTOS_ERR   err;


    p_addrinfo = (struct  addrinfo  *)Mem_DynPoolBlkGet(&NetBSD_AddrInfoPool, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }


    p_sockaddr = (struct  sockaddr  *)Mem_DynPoolBlkGet(&NetBSD_SockAddrPool, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        NetBSD_AddrInfoFree(p_addrinfo);
        goto exit;
    }


    p_addrinfo->ai_addr = p_sockaddr;
    p_addrinfo_rtn      = p_addrinfo;

    if (*pp_head == DEF_NULL) {
        *pp_head = p_addrinfo_rtn;
    }

    if (*pp_tail != DEF_NULL) {
      (*pp_tail)->ai_next = p_addrinfo_rtn;
    }

    *pp_tail = p_addrinfo_rtn;

exit:
    return (p_addrinfo_rtn);
}



/*
*********************************************************************************************************
*                                         NetBSD_AddrInfoFree()
*
* Description : Free an addrinfo structure
*
* Argument(s) : p_addrinfo  Pointer to the addrinfo structure to free.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void NetBSD_AddrInfoFree (struct  addrinfo  *p_addrinfo)
{
    struct  addrinfo  *p_blk = p_addrinfo;
            RTOS_ERR   err;


    while (p_blk != DEF_NULL) {

        if (p_blk->ai_addr != DEF_NULL) {
            Mem_DynPoolBlkFree(&NetBSD_SockAddrPool, p_blk->ai_addr, &err);
            p_blk->ai_addr = DEF_NULL;
        }

        Mem_DynPoolBlkFree(&NetBSD_AddrInfoPool, p_blk, &err);


        p_blk = p_blk->ai_next;

       (void)&err;
    }
}



/*
*********************************************************************************************************
*                                         NetBSD_AddrInfoSet()
*
* Description : Set addrinfo's field.
*
* Argument(s) : p_addrinfo      Pointer to the addrinfo structure to be filled.
*
*               family          Socket family
*
*               port_nbr        Port number
*
*               p_addr          Pointer to the IP address
*
*               addr_len        IP address length
*
*               protocol        Socket protocol
*
*               p_canonname     $$$$ Add description for 'p_canonname'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : getaddrinfo().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetBSD_AddrInfoSet (struct  addrinfo              *p_addrinfo,
                                                 NET_SOCK_ADDR_FAMILY   family,
                                                 NET_PORT_NBR           port_nbr,
                                                 CPU_INT08U            *p_addr,
                                                 NET_IP_ADDR_LEN        addr_len,
                                                 NET_SOCK_PROTOCOL      protocol,
                                                 CPU_CHAR              *p_canonname)
{
    CPU_BOOLEAN  result = DEF_FAIL;
    RTOS_ERR     err;


    NetApp_SetSockAddr((NET_SOCK_ADDR *)p_addrinfo->ai_addr,
                                        family,
                                        port_nbr,
                                        p_addr,
                                        addr_len,
                                       &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    p_addrinfo->ai_family    = family;
    p_addrinfo->ai_addrlen   = addr_len;
    p_addrinfo->ai_protocol  = (int)protocol;
    p_addrinfo->ai_canonname = p_canonname;
    p_addrinfo->ai_protocol  = protocol;
    result                   = DEF_OK;

exit:
    return (result);
}



/*
*********************************************************************************************************
*                                       NetBSD_AddrCfgValidate()
*
* Description : Validate that IPs are configured on interface(s).
*
* Argument(s) : p_ipv4_cfgd     Pointer to a variable that will receive the IPv4 address configuration status
*
*               p_ipv6_cfgd     Pointer to a variable that will receive the IPv6 address configuration status
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  NetBSD_AddrCfgValidate  (CPU_BOOLEAN  *p_ipv4_cfgd,
                                       CPU_BOOLEAN  *p_ipv6_cfgd)
{
    NET_IF_NBR   if_nbr_ix;
    NET_IF_NBR   if_nbr_cfgd;
    NET_IF_NBR   if_nbr_base;
    RTOS_ERR     err;


    if_nbr_base  = NetIF_GetNbrBaseCfgd();
    if_nbr_cfgd  = NetIF_GetExtAvailCtr(&err);
    if_nbr_cfgd -= if_nbr_base;



#ifdef  NET_IPv4_MODULE_EN
     if (*p_ipv4_cfgd == DEF_YES) {
         if_nbr_ix = if_nbr_base;

         for (if_nbr_ix = 0; if_nbr_ix <= if_nbr_cfgd; if_nbr_ix++) {
             *p_ipv4_cfgd = NetIPv4_IsAddrsCfgdOnIF(if_nbr_ix, &err);
             if (*p_ipv4_cfgd == DEF_YES) {
                 break;
             }
         }
     }
#else
    *p_ipv4_cfgd = DEF_NO;
#endif


#ifdef  NET_IPv6_MODULE_EN
    if (*p_ipv4_cfgd == DEF_YES) {
        if_nbr_ix = if_nbr_base;

        for (if_nbr_ix = 0; if_nbr_ix <= if_nbr_cfgd; if_nbr_ix++) {
            *p_ipv6_cfgd = NetIPv6_IsAddrsCfgdOnIF(if_nbr_ix, &err);
             if (*p_ipv6_cfgd == DEF_YES) {
                 break;
             }
        }
    }
#else
   *p_ipv6_cfgd = DEF_NO;
#endif
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL */

