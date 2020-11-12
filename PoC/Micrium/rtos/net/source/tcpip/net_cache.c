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
*                                  NETWORK ADDRESS CACHE MANAGEMENT
*
* File : net_cache.c
*********************************************************************************************************
* Note(s) : (1) Address cache management module ONLY required for network interfaces that require
*               network-address-to-hardware-address bindings (see RFC #826 'Abstract').
*
*           (2) Supports Address Resolution Protocol as described in RFC #826 with the following
*               restrictions/constraints :
*
*               (a) ONLY supports the following hardware types :
*                   (1) 48-bit Ethernet
*
*               (b) ONLY supports the following protocol types :
*                   (1)  32-bit IPv4
*                   (2) 128-bit IPv6
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
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_cfg_net.h>

#ifdef  NET_CACHE_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  NET_IPv4_MODULE_EN
#include  "net_arp_priv.h"
#endif
#ifdef  NET_IPv6_MODULE_EN
#include  "net_ndp_priv.h"
#endif

#include  "net_cache_priv.h"
#include  "net_type_priv.h"
#include  "net_stat_priv.h"
#include  "net_tmr_priv.h"
#include  "net_if_priv.h"

#include  <rtos/common/include/lib_utils.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>


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
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


#ifdef  NET_ARP_MODULE_EN
NET_CACHE_ADDR_ARP   NetCache_AddrARP_Tbl[NET_ARP_CFG_CACHE_NBR];
NET_CACHE_ADDR_ARP  *NetCache_AddrARP_PoolPtr;           /* Ptr to pool of free ARP caches.              */
#if (NET_STAT_POOL_ARP_EN == DEF_ENABLED)
NET_STAT_POOL        NetCache_AddrARP_PoolStat;
#endif
#endif

#ifdef  NET_NDP_MODULE_EN
NET_CACHE_ADDR_NDP   NetCache_AddrNDP_Tbl[NET_NDP_CFG_CACHE_NBR];
NET_CACHE_ADDR_NDP  *NetCache_AddrNDP_PoolPtr;           /* Ptr to pool of free NDP caches.              */
#if (NET_STAT_POOL_NDP_EN == DEF_ENABLED)
NET_STAT_POOL        NetCache_AddrNDP_PoolStat;
#endif
#endif

NET_CACHE_ADDR      *NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_MAX];
NET_CACHE_ADDR      *NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_MAX];

extern  CPU_INT16U  NetARP_CacheAccessedTh_nbr;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  NET_CACHE_ADDR  *NetCache_AddrGet (NET_CACHE_TYPE   cache_type,
                                           RTOS_ERR        *p_err);

static  void             NetCache_AddrFree(NET_CACHE_ADDR  *pcache,
                                           CPU_BOOLEAN      tmr_free);

static  void             NetCache_Unlink   (NET_CACHE_ADDR  *pcache);

static  void             NetCache_Clr      (NET_CACHE_ADDR  *pcache);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           NetCache_Init()
*
* Description : (1) Initialize address cache:
*
*                   (a) Demultiplex  parent cache type
*                   (b) Set  address        cache type
*                   (c) Set hardware address type
*                   (d) Set hardware address length
*                   (e) Set protocol address length
*                   (f) Set protocol address length
*                   (g) Free address cache to cache pool
*
* Argument(s) : pcache_parent   Pointer on the parent   cache to be associated with the address cache.
*
*               pcache_child    Pointer on the address cache to be initialized.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Note(s)     : (2) Cache pool MUST be initialized PRIOR to initializing the pool with pointers
*                   to caches.
*
*               (3) Each NDP cache addr type are initialized to NET_CACHE_TYPE_NDP but will be modified to
*                   their respective NDP type when used.
*
*                   See also 'net_cache.h  NETWORK CACHE TYPE DEFINES'.
*********************************************************************************************************
*/

void  NetCache_Init (NET_CACHE_ADDR  *pcache_parent,
                     NET_CACHE_ADDR  *pcache_addr,
                     RTOS_ERR        *p_err)
{

    switch (pcache_parent->Type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
                                                                /* Init each ARP addr cache type--NEVER modify.         */
             pcache_addr->Type             = NET_CACHE_TYPE_ARP;

                                                                /* Init each ARP HW       type/addr len--NEVER modify.  */
             pcache_addr->AddrHW_Type      = NET_ADDR_HW_TYPE_802x;
             pcache_addr->AddrHW_Len       = NET_IF_HW_ADDR_LEN_MAX;

                                                                /* Init each ARP protocol type/addr len--NEVER modify.  */
             pcache_addr->AddrProtocolType = NET_PROTOCOL_TYPE_IP_V4;
             pcache_addr->AddrProtocolLen  = NET_IPv4_ADDR_SIZE;

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
             NetCache_Clr((NET_CACHE_ADDR *)pcache_addr);
#endif
                                                                /* Free ARP cache to cache pool (see Note #2).          */
             pcache_addr->NextPtr          = (NET_CACHE_ADDR     *)NetCache_AddrARP_PoolPtr;
             NetCache_AddrARP_PoolPtr      = (NET_CACHE_ADDR_ARP *)pcache_addr;
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
                                                                /* Init each NDP addr cache type (see Note #3.)         */
             pcache_addr->Type             = NET_CACHE_TYPE_NDP;

                                                                /* Init each NDP HW       type/addr len--NEVER modify.  */
             pcache_addr->AddrHW_Type      = NET_ADDR_HW_TYPE_802x;
             pcache_addr->AddrHW_Len       = NET_IF_HW_ADDR_LEN_MAX;

                                                                /* Init each NDP protocol type/addr len--NEVER modify.  */
             pcache_addr->AddrProtocolType = NET_PROTOCOL_TYPE_IP_V6;
             pcache_addr->AddrProtocolLen  = NET_IPv6_ADDR_SIZE;

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
             NetCache_Clr((NET_CACHE_ADDR *)pcache_addr);
#endif
                                                                /* Free NDP cache to cache pool (see Note #2).          */
             pcache_addr->NextPtr          = (NET_CACHE_ADDR     *)NetCache_AddrNDP_PoolPtr;
             NetCache_AddrNDP_PoolPtr      = (NET_CACHE_ADDR_NDP *)pcache_addr;
             break;
#endif

        default:
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
            goto exit;
    }

    pcache_addr->AddrHW_Valid            = DEF_NO;
    pcache_addr->AddrProtocolValid       = DEF_NO;
    pcache_addr->AddrProtocolSenderValid = DEF_NO;
                                                                /* Set ptr to parent cache.                             */
    pcache_addr->ParentPtr               = (void *)pcache_parent;


exit:
    return;
}


/*
*********************************************************************************************************
*                                      NetCache_CfgAccessedTh()
*
* Description : Configure cache access promotion threshold.
*
* Argument(s) : cache_type      Cache type:
*
*                                   NET_CACHE_TYPE_ARP           ARP          cache type
*                                   NET_CACHE_TYPE_NDP           NDP neighbor cache type
*
*               nbr_access      Desired number of cache accesses before cache is promoted.
*
* Return(s)   : DEF_OK,   cache access promotion threshold configured.
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) 'NetARP_CacheAccessedTh_nbr' & 'NetNDP_CacheAccessedTh_nbr'  MUST ALWAYS be accessed
*                    exclusively in critical sections.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetCache_CfgAccessedTh (NET_CACHE_TYPE  cache_type,
                                     CPU_INT16U      nbr_access)
{
    CPU_SR_ALLOC();


    switch (cache_type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
#if (NET_ARP_CACHE_ACCESSED_TH_MIN > DEF_INT_16U_MIN_VAL)
             if (nbr_access < NET_ARP_CACHE_ACCESSED_TH_MIN) {
                 return (DEF_FAIL);
             }
#endif
#if (NET_ARP_CACHE_ACCESSED_TH_MAX < DEF_INT_16U_MAX_VAL)
             if (nbr_access > NET_ARP_CACHE_ACCESSED_TH_MAX) {
                 return (DEF_FAIL);
             }
#endif

             CPU_CRITICAL_ENTER();
             NetARP_CacheAccessedTh_nbr = nbr_access;
             CPU_CRITICAL_EXIT();
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
#if (NET_NDP_CACHE_ACCESSED_TH_MIN > DEF_INT_16U_MIN_VAL)
             if (nbr_access < NET_NDP_CACHE_ACCESSED_TH_MIN) {
                 return (DEF_FAIL);
             }
#endif
#if (NET_NDP_CACHE_ACCESSED_TH_MAX < DEF_INT_16U_MAX_VAL)
             if (nbr_access > NET_NDP_CACHE_ACCESSED_TH_MAX) {
                 return (DEF_FAIL);
             }
#endif

             CPU_CRITICAL_ENTER();
             NetNDP_CacheAccessedTh_nbr = nbr_access;
             CPU_CRITICAL_EXIT();
             break;
#endif

        default:
             return (DEF_FAIL);

    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         NetCache_CfgAddrs()
*
* Description : (1) Configure a cache :
*
*                   (a) Get cache from cache pool
*                   (b) Get cache timer
*                   (c) Configure cache :
*                       (1) Configure interface number
*                       (2) Configure cache addresses :
*                           (A) Hardware address
*                           (B) Protocol address(s)
*                       (3) Configure cache controls
*
*
* Argument(s) : if_nbr                  Interface number for this cache entry.
*               ------                  Argument validated in NetCache_AddResolved
*                                                             NetARP_ProbeAddrOnNet(),
*                                                             NetARP_CacheAddPend(),
*                                                             NetNDP_CacheAddPend().
*
*               cache_type              Cache type:
*
*                                           NET_CACHE_TYPE_ARP     ARP          cache type
*                                           NET_CACHE_TYPE_NDP     NDP neighbor cache type
*
*               paddr_hw                Pointer to hardware address        (see Note #2b).
*
*               addr_hw_len             Hardware address length.
*
*               paddr_protocol          Pointer to protocol address        (see Note #2c).
*               --------------          Argument checked in   NetCache_AddResolved
*                                                             NetARP_ProbeAddrOnNet(),
*                                                             NetARP_CacheAddPend(),
*                                                             NetNDP_CacheAddPend().
*
*               paddr_protocol_sender   Pointer to sender protocol address (see note #2a).
*
*               addr_protocol_len       Protocol address length.
*
*               timeout_fnct            Pointer to timeout function.
*
*               timeout_ms              Timeout value (in milliseconds).
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) (a) If 'paddr_protocol_sender' available, MUST point to a valid protocol address, in
*                           network-order, configured on interface number 'if_nbr'.
*
*                   (b) If 'paddr_hw'  available, MUST point to a valid hardware address, in
*                           network-order.
*
*                   (c) 'paddr_protocol' MUST point to a valid protocol address in network-order.
*
*                   See also 'net_arp.c NetARP_CacheHandler() Note #2e3' &
*                            'net_ndp.c NetNDP_CacheHandler() Note #2e3'.
*
*               (3) On ANY error(s), network resources MUST be appropriately freed.
*
*               (4) During ARP cache initialization, some cache controls were previously initialized
*                   in NetCache_AddrGet() when the cache was allocated from the cache pool.  These cache
*                   controls do NOT need to be re-initialized but are shown for completeness.
*********************************************************************************************************
*/
NET_CACHE_ADDR  *NetCache_CfgAddrs (NET_CACHE_TYPE       cache_type,
                                    NET_IF_NBR           if_nbr,
                                    CPU_INT08U          *p_addr_hw,
                                    NET_CACHE_ADDR_LEN   addr_hw_len,
                                    CPU_INT08U          *p_addr_protocol,
                                    CPU_INT08U          *p_addr_protocol_sender,
                                    NET_CACHE_ADDR_LEN   addr_protocol_len,
                                    CPU_BOOLEAN          timer_en,
                                    CPU_FNCT_PTR         timeout_fnct,
                                    CPU_INT32U           timeout_ms,
                                    RTOS_ERR            *p_err)
{
#ifdef  NET_ARP_MODULE_EN
    NET_CACHE_ADDR_ARP      *p_cache_addr_arp;
    NET_ARP_CACHE           *p_cache_arp;
#endif
#ifdef  NET_NDP_MODULE_EN
    NET_CACHE_ADDR_NDP      *p_cache_addr_ndp;
    NET_NDP_NEIGHBOR_CACHE  *p_cache_ndp;
#endif
    NET_CACHE_ADDR          *p_cache_addr  = DEF_NULL;


    PP_UNUSED_PARAM(addr_hw_len);                               /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(addr_protocol_len);


    switch (cache_type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
                                                                    /* ---------------- GET ARP CACHE ----------------- */
             p_cache_addr_arp = (NET_CACHE_ADDR_ARP *)NetCache_AddrGet(cache_type, p_err);

             if (p_cache_addr_arp == DEF_NULL) {
                 goto exit;                                         /* Rtn err from NetCache_AddrGet().                 */
             }

             if (timer_en == DEF_ENABLED) {
                                                                    /* -------------- GET ARP CACHE TMR --------------- */
                 p_cache_arp = (NET_ARP_CACHE *)p_cache_addr_arp->ParentPtr;

                 p_cache_arp->TmrPtr = NetTmr_Get(timeout_fnct,
                                                  p_cache_arp,
                                                  timeout_ms,
                                                  NET_TMR_OPT_NONE,
                                                  p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {  /* If tmr unavail, ...                              */
                                                                    /* ... free ARP cache (see Note #3).                */
                      NetCache_AddrFree((NET_CACHE_ADDR *)p_cache_addr_arp, DEF_NO);
                      goto exit;
                 }
             }

                                                                    /* ---------------- CFG ARP CACHE ----------------- */
                                                                    /* Cfg ARP cache addr(s).                           */
             if (p_addr_hw != DEF_NULL) {                           /* If hw addr avail (see Note #2b), ...             */
                 Mem_Copy(&p_cache_addr_arp->AddrHW[0],             /* ... copy into ARP cache.                         */
                           p_addr_hw,
                           NET_IF_HW_ADDR_LEN_MAX);

                 p_cache_addr_arp->AddrHW_Valid = DEF_YES;
             }

             if (p_addr_protocol_sender != DEF_NULL) {              /* If sender protocol addr avail (see Note #2a), ...*/
                 Mem_Copy(&p_cache_addr_arp->AddrProtocolSender[0],
                           p_addr_protocol_sender,                  /* ... copy into ARP cache.                         */
                           NET_IPv4_ADDR_SIZE);

                 p_cache_addr_arp->AddrProtocolSenderValid = DEF_YES;
             }

             Mem_Copy(&p_cache_addr_arp->AddrProtocol[0],           /* Copy protocol addr into ARP cache (see Note #2c).*/
                       p_addr_protocol,
                       NET_IPv4_ADDR_SIZE);

             p_cache_addr_arp->AddrProtocolValid = DEF_YES;

                                                                    /* Cfg ARP cache ctrl(s).                           */
             p_cache_addr_arp->IF_Nbr         = if_nbr;

#if 0                                                               /* Init'd in NetCache_AddrGet() [see Note #4].      */
             p_cache_addr_arp->AccessedCtr    = 0u;
             p_cache_addr_arp->ReqAttemptsCtr = 0u;

             p_cache_addr_arp->TxQ_Head       = DEF_NULL;
             p_cache_addr_arp->TxQ_Tail       = DEF_NULL;
                                                                    /* Cfg'd  in NetCache_Insert().                     */
             p_cache_addr_arp->PrevPtr        = DEF_NULL;
             p_cache_addr_arp->NextPtr        = DEF_NULL;
#endif

             p_cache_addr = (NET_CACHE_ADDR *)p_cache_addr_arp;
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
                                                                    /* ---------------- GET NDP CACHE ----------------- */
             p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)NetCache_AddrGet(cache_type, p_err);

             if (p_cache_addr_ndp == DEF_NULL) {
                 goto exit;                                         /* Rtn err from NetCache_AddrGet().                 */
             }

             if (timer_en == DEF_ENABLED) {
                                                                    /* -------------- GET NDP CACHE TMR --------------- */
                 p_cache_ndp = (NET_NDP_NEIGHBOR_CACHE *)p_cache_addr_ndp->ParentPtr;

                 p_cache_ndp->TmrPtr = NetTmr_Get(timeout_fnct,
                                                  p_cache_ndp,
                                                  timeout_ms,
                                                  NET_TMR_OPT_NONE,
                                                  p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {  /* If tmr unavail, ...                              */
                                                                    /* ... free NDP cache (see Note #3).                */
                      NetCache_AddrFree((NET_CACHE_ADDR *)p_cache_addr_ndp, DEF_NO);
                      goto exit;
                 }
             }
                                                                    /* ---------------- CFG NDP CACHE ----------------- */
                                                                    /* Cfg NDP cache addr(s).                           */
             if (p_addr_hw != DEF_NULL) {                           /* If hw addr avail (see Note #2b), ...             */
                 Mem_Copy(&p_cache_addr_ndp->AddrHW[0],             /* ... copy into NDP cache.                         */
                           p_addr_hw,
                           NET_IF_HW_ADDR_LEN_MAX);

                 p_cache_addr_ndp->AddrHW_Valid = DEF_YES;
             }

             if (p_addr_protocol_sender != DEF_NULL) {              /* If sender protocol addr avail (see Note #2a), ...*/
                 Mem_Copy(&p_cache_addr_ndp->AddrProtocolSender[0],
                           p_addr_protocol_sender,                  /* ... copy into NDP cache.                         */
                           NET_IPv6_ADDR_SIZE);

                 p_cache_addr_ndp->AddrProtocolSenderValid = DEF_YES;
             }

             Mem_Copy(&p_cache_addr_ndp->AddrProtocol[0],           /* Copy protocol addr into NDP cache (see Note #2c).*/
                       p_addr_protocol,
                       NET_IPv6_ADDR_SIZE);

             p_cache_addr_ndp->AddrProtocolValid = DEF_YES;

                                                                    /* Cfg NDP cache ctrl(s).                           */
             p_cache_addr_ndp->IF_Nbr         = if_nbr;

#if 0                                                               /* Init'd in NetNDP_CacheGet() [see Note #4].       */
             p_cache_addr_ndp->AccessedCtr    = 0u;
             p_cache_addr_ndp->ReqAttemptsCtr = 0u;

             p_cache_addr_ndp->TxQ_Head       = DEF_NULL;
             p_cache_addr_ndp->TxQ_Tail       = DEF_NULL;

                                                                /* Cfg'd  in NetNDP_CacheInsert().                      */
             p_cache_addr_ndp->PrevPtr        = DEF_NULL;
             p_cache_addr_ndp->NextPtr        = DEF_NULL;
#endif

             p_cache_addr = (NET_CACHE_ADDR *)p_cache_addr_ndp;
             break;
#endif

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, DEF_NULL);
    }


exit:
    return (p_cache_addr);
}


/*
*********************************************************************************************************
*                                         NetCache_AddrSrch()
*
* Description : Search cache list for cache with specific protocol address.
*
*               (1) (a) Cache List resolves protocol-address-to-hardware-address bindings based on the
*                       following cache fields :
*
*                           (A) Some fields are configured at compile time
*                                   (see 'net_arp.h  ARP CACHE  Note #3').
*
*                       (1) Cache    Type                Should be configured at compile time (see Note #1aA)
*                       (2) Hardware Type                Should be configured at compile time (see Note #1aA)
*                       (3) Hardware Address Length      Should be configured at compile time (see Note #1aA)
*                       (4) Protocol Type                Should be configured at compile time (see Note #1aA)
*                       (5) Protocol Address Length      Should be configured at compile time (see Note #1aA)
*                       (6) Protocol Address             Should be generated  at run     time
*
*                   (b) Caches are linked to form Cache List.
*
*                       (1) In the diagram below, ... :
*
*                           (A) The top horizontal row represents the list of caches.
*
*                           (B) (1) 'NetCache_???_ListHead' points to the head of the Cache List;
*                               (2) 'NetCache_???_ListTail' points to the tail of the Cache List.
*
*                           (C) Caches' 'PrevPtr' & 'NextPtr' doubly-link each cache to form the Cache List.
*
*                       (2) Caches in the 'PENDING' state are pending hardware address resolution by an
*                           ARP Reply.  While in the 'PENDING' state, ALL transmit packet buffers are enqueued
*                           for later transmission when the corresponding ARP Reply is received.
*
*                           In the diagram below, ... :
*
*                           (A) (1) ARP caches' 'TxQ_Head' points to the head of the pending transmit packet queue;
*                               (2) ARP caches' 'TxQ_Tail' points to the tail of the pending transmit packet queue.
*
*                           (B) Buffer's 'PrevSecListPtr' & 'NextSecListPtr' link each buffer in a pending transmit
*                               packet queue.
*
*                       (3) (A) For any ARP cache lookup, all ARP caches are searched in order to find the
*                               ARP cache with the appropriate hardware address--i.e. the ARP cache with the
*                               corresponding protocol address (see Note #1a5).
*
*                           (B) To expedite faster ARP cache lookup for recently added (or recently promoted)
*                               ARP caches :
*
*                               (1) (a) (1) ARP caches are added at (or promoted to); ...
*                                       (2) ARP caches are searched starting at       ...
*                                   (b) ... the head of the ARP Cache List.
*
*                               (2) (a) As ARP caches are added into the list, older ARP caches migrate to the
*                                       tail of the ARP Cache List.  Once an ARP cache expires or is discarded,
*                                       it is removed from the ARP Cache List.
*
*                                   (b) Also if NO ARP cache is available & a new ARP cache is needed, then
*                                       the oldest ARP cache at the tail of the ARP Cache List is removed for
*                                       allocation.
*
*
*                                          |                                               |
*                                          |<-------------- List of Caches --------------->|
*                                          |               (see Note #1b1A)                |
*
*                                        New caches                              Oldest cache in
*                                     inserted at head                             Cache List
*                                    (see Note #1b3B1b)                         (see Note #1b3B2)
*
*                                             |             NextPtr                     |
*                                             |        (see Note #1b1C)                 |
*                                             v                   |                     v
*                                                                 |
*                           Head of        -------       -------  v    -------       -------   (see Note #1b1B2)
*                          Cache List ---->|     |------>|     |------>|     |------>|     |
*                                          |     |       |     |       |     |       |     |        Tail of
*                                          |     |<------|     |<------|     |<------|     |<----  Cache List
*                     (see Note #1b1B1)    -------       -------  ^    -------       -------
*                                            | |                  |      | |
*                                            | |                  |      | |
*                                            | ------       PrevPtr      | ------
*                            TxQ_Head  --->  |      |  (see Note #1b1C)  |      | <---  TxQ_Tail
*                        (see Note #1b2A1)   v      |                    v      |   (see Note #1b2A2)
*               ---                        -------  |                  -------  |
*                ^                         |     |  |                  |     |  |
*                |                         |     |  |                  |     |  |
*                |                         |     |  |                  |     |  |
*                |                         |     |  |                  |     |  |
*                |                         -------  |                  -------  |
*                |                           | ^    |                    | ^    |
*                |       NextSecListPtr ---> | |    |                    | | <----- PrevSecListPtr
*                |      (see Note #1b2B)     v |    |                    v |    |  (see Note #1b2B)
*                |                         -------  |                  -------  |
*                                          |     |  |                  |     |<--
*        Buffers pending on                |     |  |                  |     |
*          cache resolution                |     |  |                  -------
*         (see Note #1b2)                  |     |  |
*                                          -------  |
*                |                           | ^    |
*                |                           | |    |
*                |                           v |    |
*                |                         -------  |
*                |                         |     |<--
*                |                         |     |
*                |                         |     |
*                v                         |     |
*               ---                        -------
*
*
* Argument(s) : cache_type          Cache type:
*
*                                       NET_CACHE_TYPE_ARP     ARP          cache type
*                                       NET_CACHE_TYPE_NDP     NDP neighbor cache type
*
*               paddr_hw            Pointer to variable that will receive the hardware address (see Note #3).
*
*               addr_hw_len         Hardware address length.
*
*               paddr_protocol      Pointer to protocol address (see Note #2).
*               --------------      Argument checked in NetARP_CacheHandler(),
*                                                       NetNDP_CacheHandler(),
*                                                       NetARP_ProbeAddrOnNet().
*
*               addr_protocol_len   Protocol address length.
*
* Return(s)   : Pointer to cache with specific protocol address, if found.
*               Pointer to NULL, otherwise.
*
* Note(s)     : (2) 'paddr_protocol' MUST point to a protocol address in network-order.
*
*                   See also 'net_arp.c NetARP_CacheHandler() Note #2e3' &
*                            'net_ndp.c NetNDP_CacheHandler() Note #2e3'.
*
*               (3) The hardware address is returned in network-order; i.e. the pointer to the hardware
*                   address points to the highest-order octet.
*********************************************************************************************************
*/

NET_CACHE_ADDR  *NetCache_AddrSrch (NET_CACHE_TYPE       cache_type,
                                    NET_IF_NBR           if_nbr,
                                    CPU_INT08U          *p_addr_protocol,
                                    NET_CACHE_ADDR_LEN   addr_protocol_len)
{
#ifdef  NET_ARP_MODULE_EN
    NET_CACHE_ADDR_ARP  *p_cache_addr_arp = DEF_NULL;
#endif
#ifdef  NET_NDP_MODULE_EN
    NET_CACHE_ADDR_NDP  *p_cache_addr_ndp = DEF_NULL;
#endif
    NET_CACHE_ADDR      *p_cache          = DEF_NULL;
    NET_CACHE_ADDR      *p_cache_next     = DEF_NULL;
    CPU_INT08U          *p_cache_addr     = DEF_NULL;
    CPU_INT16U           th               = 0;
    CPU_BOOLEAN          found            = DEF_NO;
    CPU_SR_ALLOC();


    PP_UNUSED_PARAM(addr_protocol_len);                         /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(if_nbr);


    switch (cache_type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
             p_cache_addr_arp = (NET_CACHE_ADDR_ARP *)NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP];
             while ((p_cache_addr_arp != DEF_NULL) &&           /* Srch    ARP Cache List ...               */
                    (found            == DEF_NO)) {             /* ... until cache found.                   */

                 p_cache_next = (NET_CACHE_ADDR *) p_cache_addr_arp->NextPtr;
                 p_cache_addr = (CPU_INT08U     *)&p_cache_addr_arp->AddrProtocol[0];

                                                                            /* Cmp ARP cache protocol addr.             */
                 found =  Mem_Cmp(p_addr_protocol,
                                  p_cache_addr,
                                  NET_IPv4_ADDR_SIZE);

                 if (found != DEF_YES) {                                    /* If NOT found, ..                         */
                     p_cache_addr_arp = (NET_CACHE_ADDR_ARP *)p_cache_next; /* .. adv to next ARP cache.                */

                 } else {                                                   /* Else rtn found NDP cache.                */
                     p_cache = (NET_CACHE_ADDR *)p_cache_addr_arp;

                     p_cache_addr_arp->AccessedCtr++;
                     CPU_CRITICAL_ENTER();
                     th = NetARP_CacheAccessedTh_nbr;
                     CPU_CRITICAL_EXIT();
                     if (p_cache->AccessedCtr > th) {                       /* If ARP cache accessed > th, & ..         */
                         p_cache->AccessedCtr = 0u;
                                                                            /* .. ARP cache NOT @ list head, ..         */
                         if (p_cache != NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP]) {
                             NetCache_Unlink(p_cache);
                             NetCache_Insert(p_cache);                      /* .. promote ARP cache to list head.       */
                         }
                     }
                 }
             }
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
             p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP];
             while ((p_cache_addr_ndp != DEF_NULL) &&           /* Srch    NDP Cache List ...               */
                    (found            == DEF_NO)) {             /* ... until cache found.                   */

                 p_cache_next = (NET_CACHE_ADDR *) p_cache_addr_ndp->NextPtr;
                 p_cache_addr = (CPU_INT08U     *)&p_cache_addr_ndp->AddrProtocol[0];

                 if (p_cache_addr_ndp->Type != cache_type) {
                     p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)p_cache_next;
                     continue;
                 }
                                                                            /* Cmp NDP cache protocol addr.             */
                 if (p_cache_addr_ndp->IF_Nbr == if_nbr){
                     found =  Mem_Cmp(p_addr_protocol,
                                      p_cache_addr,
                                      NET_IPv6_ADDR_SIZE);
                 }

                 if (found != DEF_YES) {                                    /* If NOT found, ..                         */
                     p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)p_cache_next; /* .. adv to next NDP cache.                */

                 } else {                                                   /* Else rtn found NDP cache.                */
                     p_cache = (NET_CACHE_ADDR *)p_cache_addr_ndp;

                     p_cache_addr_ndp->AccessedCtr++;
                     CPU_CRITICAL_ENTER();
                     th = NetNDP_CacheAccessedTh_nbr;
                     CPU_CRITICAL_EXIT();
                     if (p_cache->AccessedCtr > th) {                       /* If NDP cache accessed > th, & ..         */
                         p_cache->AccessedCtr = 0u;
                                                                            /* .. NDP cache NOT @ list head, ..         */
                         if (p_cache != NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP]) {
                             NetCache_Unlink(p_cache);
                             NetCache_Insert(p_cache);                      /* .. promote NDP cache to list head.       */
                         }
                     }
                 }
             }
             break;
#endif

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_TYPE, DEF_NULL);
    }


    return (p_cache);
}


/*
*********************************************************************************************************
*                                        NetCache_AddResolved()
*
* Description : (1) Add a 'RESOLVED' cache into the Cache List :
*
*                   (a) Configure cache :
*                       (1) Get default-configured cache
*                       (2) Cache state
*                   (b) Insert    cache into Cache List
*
*
* Argument(s) : if_nbr          Interface number for this cache entry.
*
*               paddr_hw        Pointer to hardware address (see Note #2a).
*               --------        Argument checked in caller(s)
*
*               paddr_protocol  Pointer to protocol address (see Note #2b).
*               --------------  Argument checked in caller(s)
*
*               cache_type      Cache type:
*
*                                   NET_CACHE_TYPE_ARP     ARP          cache type
*                                   NET_CACHE_TYPE_NDP     NDP neighbor cache type
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : none.
*
* Note(s)     : (2) Addresses MUST be in network-order :
*
*                   (a) 'paddr_hw'       MUST point to valid hardware address in network-order.
*                   (b) 'paddr_protocol' MUST point to valid protocol address in network-order.
*
*                   See also 'net_arp.c NetARP_CacheHandler() Note #2e3' &
*                            'net_ndp.c NetNDP_CacheHandler() Note #2e3'.
*********************************************************************************************************
*/

void  NetCache_AddResolved (NET_IF_NBR       if_nbr,
                            CPU_INT08U      *paddr_hw,
                            CPU_INT08U      *paddr_protocol,
                            NET_CACHE_TYPE   cache_type,
                            CPU_FNCT_PTR     fnct,
                            CPU_INT32U       timeout_ms,
                            RTOS_ERR        *p_err)
{
#ifdef  NET_ARP_MODULE_EN
    NET_CACHE_ADDR_ARP      *p_cache_addr_arp;
    NET_ARP_CACHE           *p_cache_arp;
#endif
#ifdef  NET_NDP_MODULE_EN
    NET_CACHE_ADDR_NDP      *p_cache_addr_ndp;
    NET_NDP_NEIGHBOR_CACHE  *p_cache_ndp;
#endif


    switch (cache_type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
             p_cache_addr_arp = (NET_CACHE_ADDR_ARP *)NetCache_CfgAddrs(NET_CACHE_TYPE_ARP,
                                                                        if_nbr,
                                                                        paddr_hw,
                                                                        NET_IF_HW_ADDR_LEN_MAX,
                                                                        paddr_protocol,
                                                                        0,
                                                                        NET_IPv4_ADDR_SIZE,
                                                                        DEF_YES,
                                                                        fnct,
                                                                        timeout_ms,
                                                                        p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  goto exit;
             }

             p_cache_arp = (NET_ARP_CACHE *)((NET_CACHE_ADDR *)p_cache_addr_arp)->ParentPtr;

             DEF_BIT_SET(p_cache_arp->Flags, NET_CACHE_FLAG_USED);
             p_cache_arp->ReqAttemptsCtr = 0;
             p_cache_arp->State          = NET_ARP_CACHE_STATE_RESOLVED;

                                                                /* ------- INSERT ARP CACHE INTO ARP CACHE LIST ------- */
             NetCache_Insert((NET_CACHE_ADDR *)p_cache_addr_arp);
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
             p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)NetCache_CfgAddrs(NET_CACHE_TYPE_NDP,
                                                                        if_nbr,
                                                                        paddr_hw,
                                                                        NET_IF_HW_ADDR_LEN_MAX,
                                                                        paddr_protocol,
                                                                        0,
                                                                        NET_IPv6_ADDR_SIZE,
                                                                        DEF_YES,
                                                                        fnct,
                                                                        timeout_ms,
                                                                        p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  goto exit;
             }

             p_cache_ndp = (NET_NDP_NEIGHBOR_CACHE *)p_cache_addr_ndp->ParentPtr;

             p_cache_ndp->State = NET_NDP_CACHE_STATE_REACHABLE;

                                                                         /* ------- INSERT NDP CACHE INTO NDP CACHE LIST ------- */
             NetCache_Insert((NET_CACHE_ADDR *)p_cache_addr_ndp);
             break;
#endif

        default:
             RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_TYPE);
             goto exit;
    }


exit:
    return;
}


/*
*********************************************************************************************************
*                                          NetCache_Insert()
*
* Description : Insert a cache into the Cache List.
*
* Argument(s) : p_cache      Pointer to a cache.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetCache_Insert (NET_CACHE_ADDR  *p_cache)
{
    switch (p_cache->Type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:                                /* ---------------- CFG ARP CACHE PTRS ---------------- */
             p_cache->PrevPtr = DEF_NULL;
             p_cache->NextPtr = NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP];

                                                                /* ------- INSERT ARP CACHE INTO ARP CACHE LIST ------- */
                                                                /* If list NOT empty, insert before head.               */
             if (NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP] != DEF_NULL) {
                 NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP]->PrevPtr = p_cache;
             } else {                                           /* Else add first ARP cache to list.                    */
                 NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_ARP]          = p_cache;
             }
                                                                /* Insert ARP cache @ list head.                        */
             NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP] = p_cache;
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:                                /* ---------------- CFG NDP CACHE PTRS ---------------- */
             p_cache->PrevPtr = DEF_NULL;
             p_cache->NextPtr = NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP];

                                                                /* ------- INSERT NDP CACHE INTO NDP CACHE LIST ------- */
                                                                /* If list NOT empty, insert before head.               */
             if (NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP] != DEF_NULL) {
                 NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP]->PrevPtr = p_cache;
             } else {                                           /* Else add first NDP cache to list.                    */
                 NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_NDP] = p_cache;
             }
                                                                /* Insert NDP cache @ list head.                        */
             NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP]     = p_cache;
             break;
#endif

        default:
             return;
    }
}


/*
*********************************************************************************************************
*                                          NetCache_Remove()
*
* Description : (1) Remove a cache from the Cache List :
*
*                   (a) Remove cache from    Cache List
*                   (b) Free   cache back to cache pool
*
*
* Argument(s) : p_cache     Pointer to a cache.
*               ------      Argument checked in caller(s)
*
*               tmr_free    Indicate whether to free network timer :
*
*                               DEF_YES                Free network timer for cache.
*                               DEF_NO          Do NOT free network timer for cache
*                                                     [Freed by NetTmr_Handler()].
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetCache_Remove (NET_CACHE_ADDR  *p_cache,
                       CPU_BOOLEAN      tmr_free)
{
                                                                /* ----------- REMOVE CACHE FROM CACHE LIST ----------- */
    NetCache_Unlink(p_cache);
                                                                /* -------------------- FREE CACHE -------------------- */
    NetCache_AddrFree(p_cache, tmr_free);
}


/*
*********************************************************************************************************
*                                        NetCache_UnlinkBuf()
*
* Description : Unlink a network buffer from a cache's transmit queue.
*
* Argument(s) : p_buf       Pointer to network buffer enqueued in a cache transmit buffer queue.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetCache_UnlinkBuf (NET_BUF  *p_buf)
{
    NET_BUF             *p_buf_prev;
    NET_BUF             *p_buf_next;
    NET_BUF_HDR         *p_buf_hdr;
    NET_BUF_HDR         *p_buf_hdr_prev;
    NET_BUF_HDR         *p_buf_hdr_next;
    NET_CACHE_ADDR      *p_cache;


    p_buf_hdr = &p_buf->Hdr;
    p_cache   = (NET_CACHE_ADDR *)p_buf_hdr->UnlinkObjPtr;


                                                                /* ------------ UNLINK BUF FROM CACHE TX Q ------------ */
    p_buf_prev = p_buf_hdr->PrevSecListPtr;
    p_buf_next = p_buf_hdr->NextSecListPtr;
                                                                /* Point prev cache pending tx Q buf to next buf.       */
    if (p_buf_prev != DEF_NULL) {
        p_buf_hdr_prev                 = &p_buf_prev->Hdr;
        p_buf_hdr_prev->NextSecListPtr =  p_buf_next;
    } else {
        p_cache->TxQ_Head              =  p_buf_next;
    }
                                                                /* Point next cache pending tx Q buf to prev buf.       */
    if (p_buf_next != DEF_NULL) {
        p_buf_hdr_next                 = &p_buf_next->Hdr;
        p_buf_hdr_next->PrevSecListPtr =  p_buf_prev;
    } else {
        p_cache->TxQ_Tail              =  p_buf_prev;
    }


                                                                /* -------------- CLR BUF'S UNLINK CTRLS -------------- */
    p_buf_hdr->PrevSecListPtr = DEF_NULL;                       /* Clr pending tx Q ptrs.                               */
    p_buf_hdr->NextSecListPtr = DEF_NULL;

    p_buf_hdr->UnlinkFnctPtr  = DEF_NULL;                       /* Clr unlink ptrs.                                     */
    p_buf_hdr->UnlinkObjPtr   = DEF_NULL;
}


/*
*********************************************************************************************************
*                                        NetCache_TxPktHandler()
*
* Description : (1) Transmit packet buffers from cache transmit buffer queue :
*
*                   (a) Resolve  packet  buffer(s)' hardware address(s)
*                   (b) Update   packet  buffer(s)' unlink/reference values
*                   (c) Transmit packet  buffer(s)
*
*
* Argument(s) : pbuf_q      Pointer to network buffer(s) to transmit.
*
*               paddr_hw    Pointer to sender's hardware address (see Note #2).
*               --------    Argument checked in caller(s)
*
* Return(s)   : none.
*
* Note(s)     : (2) Addresses MUST be in network-order.
*
*               (3) RFC #1122, Section 2.3.2.2 states that "the link layer SHOULD" :
*
*                   (a) "Save (rather than discard) ... packets destined to the same unresolved
*                        IP address and" ...
*                   (b) "Transmit the saved packet[s] when the address has been resolved."
*********************************************************************************************************
*/

void  NetCache_TxPktHandler (NET_PROTOCOL_TYPE   proto_type,
                             NET_BUF            *p_buf_q,
                             CPU_INT08U         *p_addr_hw)
{
    NET_BUF                 *p_buf_list;
    NET_BUF                 *p_buf_list_next;
    NET_BUF                 *p_buf;
    NET_BUF                 *p_buf_next;
    NET_BUF_HDR             *p_buf_hdr;
#ifdef  NET_NDP_MODULE_EN
    NET_NDP_NEIGHBOR_CACHE  *p_cache;
    NET_CACHE_ADDR_NDP      *p_cache_addr_ndp;
    CPU_INT08U              *p_addr_protocol;
    CPU_INT08U               cache_state;
    CPU_INT32U               timeout_ms;
#endif
    CPU_INT08U              *p_buf_addr_hw;
#ifdef  NET_NDP_MODULE_EN
    NET_IF_NBR               if_nbr;
    CPU_SR_ALLOC();
#endif
    RTOS_ERR                 local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    p_buf_hdr = &p_buf_q->Hdr;

    p_buf_list = p_buf_q;
    while (p_buf_list  != DEF_NULL) {                           /* Handle ALL buf lists in Q.                           */
        p_buf_hdr       = &p_buf_list->Hdr;
        p_buf_list_next =  p_buf_hdr->NextSecListPtr;
        p_buf           =  p_buf_list;

        while (p_buf != DEF_NULL) {                             /* Handle ALL bufs in buf list.                         */
            p_buf_hdr   = &p_buf->Hdr;
            p_buf_next  = p_buf_hdr->NextBufPtr;

            switch (proto_type) {
#ifdef  NET_ARP_MODULE_EN
                case NET_PROTOCOL_TYPE_ARP:
                     p_buf_addr_hw = p_buf_hdr->ARP_AddrHW_Ptr;
                     Mem_Copy(p_buf_addr_hw,                    /* Copy hw addr into pkt buf.                           */
                              p_addr_hw,
                              NET_IF_HW_ADDR_LEN_MAX);
                     break;
#endif
#ifdef  NET_NDP_MODULE_EN
                case NET_PROTOCOL_TYPE_NDP:
                     if_nbr       = p_buf_hdr->IF_Nbr;
                     p_buf_addr_hw = p_buf_hdr->NDP_AddrHW_Ptr;
                     Mem_Copy(p_buf_addr_hw,                    /* Copy hw addr into pkt buf.                           */
                              p_addr_hw,
                              NET_IF_HW_ADDR_LEN_MAX);
                     p_addr_protocol  =  p_buf_hdr->NDP_AddrProtocolPtr;
                     p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)NetCache_AddrSrch(NET_CACHE_TYPE_NDP,
                                                                                if_nbr,
                                                                                p_addr_protocol,
                                                                                NET_IPv6_ADDR_SIZE);

                     if (p_cache_addr_ndp != DEF_NULL) {         /* If NDP cache found, chk state.                       */
                         p_cache = (NET_NDP_NEIGHBOR_CACHE *)p_cache_addr_ndp->ParentPtr;
                         cache_state = p_cache->State;
                         if (cache_state == NET_NDP_CACHE_STATE_STALE) {
                             p_cache->State = NET_NDP_CACHE_STATE_DLY;
                             CPU_CRITICAL_ENTER();
                             timeout_ms = NetNDP_DelayTimeout_ms;
                             CPU_CRITICAL_EXIT();
                             NetTmr_Set(p_cache->TmrPtr,
                                        NetNDP_DelayTimeout,
                                        timeout_ms);
                        }
                     }
                     break;
#endif

                default:
                     break;
            }
                                                                /* Clr buf sec list & unlink ptrs.                      */
            p_buf_hdr->PrevSecListPtr = DEF_NULL;
            p_buf_hdr->NextSecListPtr = DEF_NULL;
            p_buf_hdr->UnlinkFnctPtr  = DEF_NULL;
            p_buf_hdr->UnlinkObjPtr   = DEF_NULL;

            NetIF_Tx(p_buf, &local_err);                        /* Tx pkt to IF (see Note #3b).                         */
            if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
                NET_CTR_ERR_INC(Net_ErrCtrs.Cache.ReTxPktFailCtr);
            }

            p_buf  = p_buf_next;
        }

        p_buf_list = p_buf_list_next;
    }
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
*                                         NetCache_AddrGet()
*
* Description : (1) Allocate & initialize a cache :
*
*                   (a) Get a cache
*                   (b) Validate   cache
*                   (c) Initialize cache
*                   (d) Update cache pool statistics
*                   (e) Return pointer to cache
*                         OR
*                       Null pointer & error code, on failure
*
*               (2) The cache pool is implemented as a stack :
*
*                   (a) Caches' 'PoolPtr's points to the head of the   cache pool.
*
*                   (b) Caches' 'NextPtr's link each cache to form the cache pool stack.
*
*                   (c) Caches are inserted & removed at the head of   cache pool stack.
*
*
*                                         Caches are
*                                     inserted & removed
*                                        at the head
*                                       (see Note #2c)
*
*                                             |                 NextPtr
*                                             |             (see Note #2b)
*                                             v                    |
*                                                                  |
*                                          -------       -------   v   -------       -------
*                              Pool   ---->|     |------>|     |------>|     |------>|     |
*                            Pointer       |     |       |     |       |     |       |     |
*                                          |     |       |     |       |     |       |     |
*                         (see Note #2a)   -------       -------       -------       -------
*
*                                          |                                               |
*                                          |<------------ Pool of Free Caches ------------>|
*                                          |                (see Note #2)                  |
*
*
* Argument(s) : cache_type      Cache type:
*
*                                   NET_CACHE_TYPE_ARP     ARP cache type
*                                   NET_CACHE_TYPE_NDP     NDP neighbor cache type
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Pointer to cache, if NO error(s).
*               Pointer to NULL,  otherwise.
*
* Note(s)     : (3) (a) Cache pool is accessed by 'NetARP_CachePoolPtr' during execution of
*
*                       (1) NetARP_Init()
*                       (2) NetNDP_Init()
*                       (3) NetCache_AddrGet()
*                       (4) NetCache_AddrFree()
*
*                   (b) Since the primary tasks of the network protocol suite are prevented from running
*                       concurrently (see 'net.h  Note #3'), it is NOT necessary to protect the shared
*                       resources of the cache pool since no asynchronous access from other network
*                       tasks is possible.
*
*               (4) 'No cache available' case NOT possible during correct operation of  the cache.
*                   However, the 'else' case is included as an extra precaution in case the cache
*                   list is incorrectly modified &/or corrupted.
*********************************************************************************************************
*/

static  NET_CACHE_ADDR  *NetCache_AddrGet (NET_CACHE_TYPE   cache_type,
                                           RTOS_ERR        *p_err)
{
    NET_CACHE_ADDR      *p_cache          = DEF_NULL;
#ifdef  NET_ARP_MODULE_EN
    NET_CACHE_ADDR_ARP  *p_cache_addr_arp = DEF_NULL;
#endif
#ifdef  NET_NDP_MODULE_EN
    NET_CACHE_ADDR_NDP  *p_cache_addr_ndp = DEF_NULL;
#endif
#if ((NET_STAT_POOL_ARP_EN == DEF_ENABLED) || \
     (NET_STAT_POOL_NDP_EN == DEF_ENABLED))
    RTOS_ERR             local_err;
#endif



    switch (cache_type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:                                        /* -------------- GET ARP CACHE --------------- */
             if (NetCache_AddrARP_PoolPtr != DEF_NULL) {                /* If ARP cache pool NOT empty, ...             */
                                                                        /* ...  get cache from pool.                    */
                 p_cache_addr_arp          = NetCache_AddrARP_PoolPtr;
                 NetCache_AddrARP_PoolPtr  = (NET_CACHE_ADDR_ARP *)p_cache_addr_arp->NextPtr;

             } else {                                                   /* If ARP Cache List NOT empty, ...              */
                                                                        /* ... get ARP cache from list tail.             */
                 NET_CACHE_ADDR  *p_entry = NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_ARP];


                 while (p_entry != DEF_NULL) {

                     if (p_entry->AddrHW_Valid == DEF_YES) {            /* Make sure to remove only a resolved entry.    */
                         break;
                     }

                     p_entry = p_entry->NextPtr;
                 }

                 if (p_entry == DEF_NULL) {
                     NET_CTR_ERR_INC(Net_ErrCtrs.ARP.NoneAvailCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
                     goto exit;
                 }

                 NetCache_Remove(p_entry, DEF_YES);
                 p_entry                   = (NET_CACHE_ADDR     *)NetCache_AddrARP_PoolPtr;
                 NetCache_AddrARP_PoolPtr  = (NET_CACHE_ADDR_ARP *)p_entry->NextPtr;

                 p_cache_addr_arp = (NET_CACHE_ADDR_ARP  *)p_entry;
             }

                                                                        /* ---------------- INIT CACHE ---------------- */
             NetCache_Clr((NET_CACHE_ADDR *)p_cache_addr_arp);
             DEF_BIT_SET(p_cache_addr_arp->Flags, NET_CACHE_FLAG_USED); /* Set cache as used.                           */

#if (NET_STAT_POOL_ARP_EN == DEF_ENABLED)
             RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                        /* --------- UPDATE CACHE POOL STATS ---------- */
             NetStat_PoolEntryUsedInc(&NetCache_AddrARP_PoolStat, &local_err);
#endif

             p_cache = (NET_CACHE_ADDR *)p_cache_addr_arp;
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:                                        /* -------------- GET NDP CACHE --------------- */

             if (NetCache_AddrNDP_PoolPtr != DEF_NULL) {                /* If ARP cache pool NOT empty, ...             */
                                                                        /* ...  get cache from pool.                    */
                 p_cache_addr_ndp          = NetCache_AddrNDP_PoolPtr;
                 NetCache_AddrNDP_PoolPtr  = (NET_CACHE_ADDR_NDP *)p_cache_addr_ndp->NextPtr;

             } else {                                                   /* If ARP Cache List NOT empty, ...              */
                                                                        /* ... get ARP cache from list tail.             */
                 NET_CACHE_ADDR  *p_entry = NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_NDP];


                 while (p_entry != DEF_NULL) {

                     if (p_entry->AddrHW_Valid == DEF_YES) {            /* Make sure to remove only a resolved entry.    */
                         break;
                     }

                     p_entry = p_entry->NextPtr;
                 }

                 if (p_entry == DEF_NULL) {
                     NET_CTR_ERR_INC(Net_ErrCtrs.NDP.NoneAvailCtr);
                     RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
                     goto exit;
                 }

                 NetCache_Remove(p_entry, DEF_YES);
                 p_cache_addr_ndp          = NetCache_AddrNDP_PoolPtr;
                 NetCache_AddrNDP_PoolPtr  = (NET_CACHE_ADDR_NDP *)p_entry->NextPtr;

                 p_cache_addr_ndp = (NET_CACHE_ADDR_NDP  *)p_entry;
             }

                                                                        /* ---------------- INIT CACHE ---------------- */
             NetCache_Clr((NET_CACHE_ADDR *)p_cache_addr_ndp);
             DEF_BIT_SET(p_cache_addr_ndp->Flags, NET_CACHE_FLAG_USED); /* Set cache as used.                           */

#if (NET_STAT_POOL_NDP_EN == DEF_ENABLED)
             RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                        /* --------- UPDATE CACHE POOL STATS ---------- */
             NetStat_PoolEntryUsedInc(&NetCache_AddrNDP_PoolStat, &local_err);
#endif

             p_cache       = (NET_CACHE_ADDR *)p_cache_addr_ndp;
             p_cache->Type = cache_type;
             break;
#endif

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, DEF_NULL);
    }


exit:
    return (p_cache);
}

/*
*********************************************************************************************************
*                                         NetCache_AddrFree()
*
* Description : (1) Free a cache :
*
*                   (a) Free   cache timer
*                   (b) Free   cache buffer queue
*                   (c) Clear  cache controls
*                   (d) Free   cache back to cache pool
*                   (e) Update cache pool statistics
*
*
* Argument(s) : pcache      Pointer to a cache.
*               ------      Argument checked in NetCache_CfgAddrs(),
*                                               NetCache_Remove()
*                                                 by NetCache_AddrGet(),
*                                                    NetARP_CacheHandler(),
*                                                    NetARP_CacheReqTimeout(),
*                                                    NetARP_CacheTimeout(),
*                                                    NetNDP_CacheHandler(),
*                                                    NetNDP_CacheReqTimeout(),
*                                                    NetNDP_CacheTimeout().
*
*               tmr_free    Indicate whether to free network timer :
*
*                               DEF_YES                Free network timer for cache.
*                               DEF_NO          Do NOT free network timer for cache
*                                                     [Freed by  NetTmr_Handler()
*                                                            via NetCache_Remove()].
*
* Return(s)   : none.
*
* Note(s)     : (2) #### To prevent freeing an cache already freed via previous cache free,
*                   NetCache_AddrFree() checks the cache's 'USED' flag BEFORE freeing the cache.
*
*                   This prevention is only best-effort since any invalid duplicate cache frees
*                   MAY be asynchronous to potentially valid cache gets.  Thus the invalid cache
*                   free(s) MAY corrupt the cache's valid operation(s).
*
*                   However, since the primary tasks of the network protocol suite are prevented from
*                   running concurrently (see 'net.h  Note #3'), it is NOT necessary to protect cache
*                   resources from possible corruption since no asynchronous access from other network
*                   tasks is possible.
*
*               (3) When a cache in the 'PENDING' state is freed, it discards its transmit packet
*                   buffer queue.  The discard is performed by the network interface layer since it is
*                   the last layer to initiate transmission for these packet buffers.
*********************************************************************************************************
*/

static  void  NetCache_AddrFree (NET_CACHE_ADDR  *p_cache,
                                 CPU_BOOLEAN      tmr_free)
{
    NET_CTR                 *p_ctr;
    CPU_BOOLEAN              used;
#ifdef  NET_ARP_MODULE_EN
    NET_ARP_CACHE           *p_cache_arp;
#endif
#ifdef  NET_NDP_MODULE_EN
    NET_NDP_NEIGHBOR_CACHE  *p_cache_ndp;
#endif
#if ((NET_STAT_POOL_ARP_EN == DEF_ENABLED) || \
     (NET_STAT_POOL_NDP_EN == DEF_ENABLED))
    RTOS_ERR                 local_err;
#endif


                                                                    /* ------------ VALIDATE ARP CACHE USED ----------- */
    used = DEF_BIT_IS_SET(p_cache->Flags, NET_CACHE_FLAG_USED);
    if (used != DEF_YES) {                                          /* If ARP cache NOT used, ...                       */
        NET_CTR_ERR_INC(Net_ErrCtrs.Cache.NotUsedCtr);
        return;                                                     /* ... rtn but do NOT free (see Note #2).           */
    }


    p_cache->AddrHW_Valid            = DEF_NO;
    p_cache->AddrProtocolValid       = DEF_NO;
    p_cache->AddrProtocolSenderValid = DEF_NO;

    switch (p_cache->Type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
                                                                        /* ------------ FREE ARP CACHE TMR ------------ */
            p_cache_arp = (NET_ARP_CACHE *)p_cache->ParentPtr;

            if (tmr_free == DEF_YES) {
                 if (p_cache_arp->TmrPtr != DEF_NULL) {
                     NetTmr_Free(p_cache_arp->TmrPtr);
                     p_cache_arp->TmrPtr = DEF_NULL;
                 }
             }

                                                                        /* ----------- FREE ARP CACHE BUF Q ----------- */
#if (NET_CTR_CFG_ERR_EN == DEF_ENABLED)
             p_ctr = &Net_ErrCtrs.Cache.TxPktDisCtr;                    /* See Note #3.                                 */
#else
             p_ctr =  DEF_NULL;
#endif
            (void)NetBuf_FreeBufQ_SecList(p_cache->TxQ_Head,
                                          p_ctr,
                                          NetCache_UnlinkBuf);

             p_cache->TxQ_Nbr = 0;

                                                                        /* --------------- CLR ARP CACHE -------------- */
             p_cache_arp->State = NET_ARP_CACHE_STATE_FREE;             /* Set ARP cache as freed/NOT used.             */
             DEF_BIT_CLR(p_cache->Flags, NET_CACHE_FLAG_USED);
#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
             NetCache_Clr(p_cache);
#endif

                                                                        /* -------------- FREE ARP CACHE -------------- */
             p_cache->NextPtr         = (NET_CACHE_ADDR     *)NetCache_AddrARP_PoolPtr;
             NetCache_AddrARP_PoolPtr = (NET_CACHE_ADDR_ARP *)p_cache;

#if (NET_STAT_POOL_ARP_EN == DEF_ENABLED)
             RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                        /* ------- UPDATE ARP CACHE POOL STATS -------- */
             NetStat_PoolEntryUsedDec(&NetCache_AddrARP_PoolStat, &local_err);
#endif
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
                                                                        /* ------------ FREE NDP CACHE TMR ------------ */
             p_cache_ndp = (NET_NDP_NEIGHBOR_CACHE *)p_cache->ParentPtr;

             if (tmr_free == DEF_YES) {
                 if (p_cache_ndp->TmrPtr != DEF_NULL) {
                     NetTmr_Free(p_cache_ndp->TmrPtr);
                     p_cache_ndp->TmrPtr = DEF_NULL;
                 }
             }

                                                                        /* ----------- FREE NDP CACHE BUF Q ----------- */
#if (NET_CTR_CFG_ERR_EN == DEF_ENABLED)
             p_ctr = &Net_ErrCtrs.Cache.TxPktDisCtr;                    /* See Note #3.                                 */
#else
             p_ctr =  DEF_NULL;
#endif
            (void)NetBuf_FreeBufQ_SecList(p_cache->TxQ_Head,
                                          p_ctr,
                                          NetCache_UnlinkBuf);

             p_cache->TxQ_Nbr = 0;

                                                                        /* --------------- CLR NDP CACHE -------------- */
             p_cache_ndp->State = NET_NDP_CACHE_STATE_NONE;             /* Set NDP cache as freed/NOT used.             */
             p_cache_ndp->ReqAttemptsCtr = 0;
             DEF_BIT_CLR(p_cache->Flags, NET_CACHE_FLAG_USED);

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
             NetCache_Clr(p_cache);
#endif

                                                                        /* -------------- FREE NDP CACHE -------------- */
             p_cache->NextPtr         = (NET_CACHE_ADDR     *)NetCache_AddrNDP_PoolPtr;
             NetCache_AddrNDP_PoolPtr = (NET_CACHE_ADDR_NDP *)p_cache;

             p_cache->Type            = NET_CACHE_TYPE_NDP;

#if (NET_STAT_POOL_NDP_EN == DEF_ENABLED)
             RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                                                                        /* ------- UPDATE NDP CACHE POOL STATS -------- */
             NetStat_PoolEntryUsedDec(&NetCache_AddrNDP_PoolStat, &local_err);
#endif
             break;
#endif

        default:
            RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }
}

/*
*********************************************************************************************************
*                                          NetCache_Unlink()
*
* Description : Unlink a cache from the Cache List.
*
* Argument(s) : p_cache     Pointer to a cache.
*               ------      Argument checked in NetCache_AddrSrch();
*                                               NetCache_Remove()
*                                                 by NetCacheGet(),
*                                                    NetARP_CacheHandler(),
*                                                    NetARP_CacheReqTimeout(),
*                                                    NetARP_CacheTimeout(),
*                                                    NetNDP_CacheHandler(),
*                                                    NetNDP_CacheReqTimeout(),
*                                                    NetNDP_CacheTimeout().
*
* Return(s)   : none.
*
* Note(s)     : (1) Since NetCache_Unlink() called ONLY to remove & then re-link or free caches,
*                   it is NOT necessary to clear the entry's previous & next pointers.  However, pointers
*                   cleared to NULL shown for correctness & completeness.
*********************************************************************************************************
*/

static  void  NetCache_Unlink (NET_CACHE_ADDR  *p_cache)
{
    NET_CACHE_ADDR  *p_cache_next;
    NET_CACHE_ADDR  *p_cache_prev;


    p_cache_prev = p_cache->PrevPtr;
    p_cache_next = p_cache->NextPtr;

    switch (p_cache->Type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:                                /* ------- UNLINK ARP CACHE FROM ARP CACHE LIST ------- */
                                                                /* Point prev ARP cache to next ARP cache.              */
             if (p_cache_prev != DEF_NULL) {
                 p_cache_prev->NextPtr                             = p_cache_next;
             } else {
                 NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_ARP] = p_cache_next;
             }
                                                                /* Point next ARP cache to prev ARP cache.              */
             if (p_cache_next != DEF_NULL) {
                 p_cache_next->PrevPtr                             = p_cache_prev;
             } else {
                 NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_ARP] = p_cache_prev;
             }

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)                     /* Clr ARP cache's ptrs (see Note #1).                  */
             p_cache->PrevPtr = DEF_NULL;
             p_cache->NextPtr = DEF_NULL;
#endif
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:                                /* ------- UNLINK NDP CACHE FROM NDP CACHE LIST ------- */
                                                                /* Point prev NDP cache to next NDP cache.              */
             if (p_cache_prev != DEF_NULL) {
                 p_cache_prev->NextPtr                             = p_cache_next;
             } else {
                 NetCache_AddrListHead[NET_CACHE_ADDR_LIST_IX_NDP] = p_cache_next;
             }
                                                                /* Point next NDP cache to prev NDP cache.              */
             if (p_cache_next != DEF_NULL) {
                 p_cache_next->PrevPtr                             = p_cache_prev;
             } else {
                 NetCache_AddrListTail[NET_CACHE_ADDR_LIST_IX_NDP] = p_cache_prev;
             }

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)                     /* Clr NDP cache's ptrs (see Note #1).                  */
             p_cache->PrevPtr = DEF_NULL;
             p_cache->NextPtr = DEF_NULL;
#endif
             break;
#endif

        default:
             return;
    }
}


/*
*********************************************************************************************************
*                                           NetCache_Clr()
*
* Description : Clear cache controls.
*
* Argument(s) : p_cache     Pointer to a cache.
*               ------      Argument validated in NetCache_AddrGet(),
*                                                 NetCache_AddrFree(),
*                                                 NetARP_Init(),
*                                                 NetNDP_Init().
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetCache_Clr (NET_CACHE_ADDR  *p_cache)
{

#ifdef  NET_ARP_MODULE_EN
    NET_CACHE_ADDR_ARP  *p_cache_addr_arp;
#endif
#ifdef  NET_NDP_MODULE_EN
    NET_CACHE_ADDR_NDP  *p_cache_addr_ndp;
#endif


    p_cache->PrevPtr                 = DEF_NULL;
    p_cache->NextPtr                 = DEF_NULL;
    p_cache->TxQ_Head                = DEF_NULL;
    p_cache->TxQ_Tail                = DEF_NULL;
    p_cache->TxQ_Nbr                 = 0;
    p_cache->IF_Nbr                  = NET_IF_NBR_NONE;
    p_cache->AccessedCtr             = 0u;
    p_cache->Flags                   = NET_CACHE_FLAG_NONE;

    p_cache->AddrHW_Valid            = DEF_NO;
    p_cache->AddrProtocolValid       = DEF_NO;
    p_cache->AddrProtocolSenderValid = DEF_NO;


    switch (p_cache->Type) {
#ifdef  NET_ARP_MODULE_EN
        case NET_CACHE_TYPE_ARP:
             p_cache_addr_arp = (NET_CACHE_ADDR_ARP *)p_cache;

             Mem_Clr(&p_cache_addr_arp->AddrHW[0],
                      NET_IF_HW_ADDR_LEN_MAX);
             Mem_Clr(&p_cache_addr_arp->AddrProtocol[0],
                      NET_IPv4_ADDR_SIZE);
             Mem_Clr(&p_cache_addr_arp->AddrProtocolSender[0],
                      NET_IPv4_ADDR_SIZE);
             break;
#endif

#ifdef  NET_NDP_MODULE_EN
        case NET_CACHE_TYPE_NDP:
             p_cache_addr_ndp = (NET_CACHE_ADDR_NDP *)p_cache;

             Mem_Clr(&p_cache_addr_ndp->AddrHW[0],
                      NET_IF_HW_ADDR_LEN_MAX);
             Mem_Clr(&p_cache_addr_ndp->AddrProtocol[0],
                      NET_IPv6_ADDR_SIZE);
             Mem_Clr(&p_cache_addr_ndp->AddrProtocolSender[0],
                      NET_IPv6_ADDR_SIZE);
             break;
#endif

        default:
             return;
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_CACHE_MODULE_EN   */
#endif  /* RTOS_MODULE_NET_AVAIL */

