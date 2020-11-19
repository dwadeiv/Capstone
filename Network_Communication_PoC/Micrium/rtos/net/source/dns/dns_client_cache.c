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
*                                       DNS CLIENT CACHE MODULE
*
* File : dns_client_cache.c
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

#include  <rtos/net/include/net_cfg_net.h>

#ifdef  NET_DNS_CLIENT_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/net/include/net_type.h>

#include  "dns_client_cache_priv.h"
#include  "dns_client_req_priv.h"

#include  <rtos/net/include/dns_client.h>
#include  <rtos/net/include/net_ascii.h>
#include  <rtos/net/include/net_util.h>

#include  "../tcpip/net_if_priv.h"

#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                       (NET, DNS)
#define  RTOS_MODULE_CUR                    RTOS_CFG_MODULE_NET_APP


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

extern  DNSc_DATA  *DNSc_DataPtr;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void              DNScCache_LockAcquire      (void);

static  void              DNScCache_LockRelease      (void);

static  void              DNScCache_HostRemoveHandler(DNSc_HOST_OBJ      *p_host);

static  void              DNScCache_HostObjNameSet   (DNSc_HOST_OBJ      *p_host,
                                                      CPU_CHAR           *p_host_name,
                                                      RTOS_ERR           *p_err);

static  DNSc_HOST_OBJ    *DNScCache_HostSrchByName   (CPU_CHAR           *p_host_name);

static  CPU_BOOLEAN       DNScCache_HostNameCmp      (DNSc_HOST_OBJ      *p_host,
                                                      CPU_CHAR           *p_host_name);

static  DNSc_CACHE_ITEM  *DNScCache_ItemGet          (RTOS_ERR           *p_err);

static  void              DNScCache_ItemFree         (DNSc_CACHE_ITEM    *p_cache);

static  DNSc_HOST_OBJ    *DNScCache_ItemHostGet      (void);

static  void              DNScCache_ItemRelease      (DNSc_CACHE_ITEM    *p_cache);

static  void              DNScCache_ItemRemove       (DNSc_CACHE_ITEM    *p_cache);

static  DNSc_ADDR_ITEM   *DNScCache_AddrItemGet      (RTOS_ERR           *p_err);

static  void              DNScCache_AddrItemFree     (DNSc_ADDR_ITEM     *p_item);

static  void              DNScCache_HostRelease      (DNSc_HOST_OBJ      *p_host);

static  void              DNScCache_HostAddrClr      (DNSc_HOST_OBJ      *p_host);

static  DNSc_STATUS       DNScCache_Resolve          (DNSc_HOST_OBJ      *p_host,
                                                      RTOS_ERR           *p_err);

static  void              DNScCache_Req              (DNSc_HOST_OBJ      *p_host,
                                                      RTOS_ERR           *p_err);

static  DNSc_STATUS       DNScCache_Resp             (DNSc_HOST_OBJ      *p_host,
                                                      RTOS_ERR           *p_err);

static  NET_HOST_IP_ADDR *NetCache_HostAddrsGet      (NET_HOST_IP_ADDR  **p_head,
                                                      NET_HOST_IP_ADDR  **p_tail);

static  void              NetCache_HostAddrsFree     (NET_HOST_IP_ADDR   *p_addrs);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           DNScCache_Init()
*
* Description : Initialize cache module.
*
* Argument(s) : p_cfg       Pointer to DNSc's configuration.
*
*               p_mem_seg   Pointer to memory segment to use for dynamic allocation.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_Init (const  DNSc_CFG  *p_cfg,
                             DNSc_DATA *p_dns_data,
                             MEM_SEG   *p_mem_seg,
                             RTOS_ERR  *p_err)
{
    CPU_SIZE_T  nb_addr;


    p_dns_data->CacheLockHandle = KAL_LockCreate("DNSc Lock",
                                                  KAL_OPT_CREATE_NONE,
                                                  p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

    Mem_DynPoolCreate("DNSc Cache Item Pool",
                      &p_dns_data->CacheItemPool,
                       p_mem_seg,
                       sizeof(DNSc_CACHE_ITEM),
                       sizeof(CPU_ALIGN),
                       1u,
                       p_cfg->CacheEntriesMaxNbr,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

    p_dns_data->CacheItemListHead = DEF_NULL;


    Mem_DynPoolCreate("DNSc Cache Host Obj Pool",
                      &p_dns_data->CacheHostObjPool,
                       p_mem_seg,
                       sizeof(DNSc_HOST_OBJ),
                       sizeof(CPU_ALIGN),
                       1u,
                       p_cfg->CacheEntriesMaxNbr,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

    Mem_DynPoolCreate("DNSc Cache Host Obj Pool",
                      &p_dns_data->CacheHostNamePool,
                       p_mem_seg,
                       p_cfg->HostNameLenMax,
                       sizeof(CPU_ALIGN),
                       1u,
                       p_cfg->CacheEntriesMaxNbr,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

    nb_addr = 0u;
#ifdef  NET_IPv4_MODULE_EN
    if (p_cfg->AddrIPv4MaxPerHost != LIB_MEM_BLK_QTY_UNLIMITED) {
        nb_addr += p_cfg->AddrIPv4MaxPerHost;
    } else {
        nb_addr = LIB_MEM_BLK_QTY_UNLIMITED;
    }
#endif
#ifdef  NET_IPv6_MODULE_EN
    if ((p_cfg->AddrIPv6MaxPerHost != LIB_MEM_BLK_QTY_UNLIMITED) &&
        (nb_addr != LIB_MEM_BLK_QTY_UNLIMITED)) {
        nb_addr += p_cfg->AddrIPv6MaxPerHost;
    } else {
        nb_addr = LIB_MEM_BLK_QTY_UNLIMITED;
    }
#endif
    if ((p_cfg->CacheEntriesMaxNbr != LIB_MEM_BLK_QTY_UNLIMITED) &&
        (nb_addr != LIB_MEM_BLK_QTY_UNLIMITED)) {
        nb_addr *= p_cfg->CacheEntriesMaxNbr;
    } else {
        nb_addr = LIB_MEM_BLK_QTY_UNLIMITED;
    }

    Mem_DynPoolCreate("DNSc Cache Addr Item Pool",
                      &p_dns_data->CacheAddrItemPool,
                       p_mem_seg,
                       sizeof(DNSc_ADDR_ITEM),
                       sizeof(CPU_ALIGN),
                       1u,
                       nb_addr,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

    Mem_DynPoolCreate("DNSc Cache Addr Obj Pool",
                      &p_dns_data->CacheAddrObjPool,
                       p_mem_seg,
                       sizeof(NET_IP_ADDR_OBJ),
                       sizeof(CPU_ALIGN),
                       1u,
                       nb_addr,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit;
    }

    Mem_DynPoolCreate("DNSc Host IP Addr Pool for app",
                      &p_dns_data->CacheHostAddrObjPool,
                       p_mem_seg,
                       sizeof(NET_HOST_IP_ADDR),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                       p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            goto exit;
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                            DNScCache_Clr()
*
* Description : Clear all elements of the cache.
*
* Argument(s) : none.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_Clr (void)
{
    DNSc_CACHE_ITEM  *p_cache = DNSc_DataPtr->CacheItemListHead;
    DNSc_CACHE_ITEM  *p_cache_next;


    DNScCache_LockAcquire();

    while (p_cache != DEF_NULL) {
        p_cache_next = p_cache->NextPtr;
        switch (p_cache->HostPtr->State) {
            case DNSc_STATE_INIT_REQ:
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 break;

            case DNSc_STATE_FREE:
            case DNSc_STATE_RESOLVED:
            case DNSc_STATE_FAILED:
            default:
                 DNScCache_ItemRelease(p_cache);
                 break;
        }

        p_cache = p_cache_next;
    }

    DNScCache_LockRelease();
}


/*
*********************************************************************************************************
*                                        DNScCache_HostInsert()
*
* Description : Add an entry in the cache.
*
* Argument(s) : p_host  Pointer to host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function:
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostInsert (DNSc_HOST_OBJ  *p_host,
                            RTOS_ERR       *p_err)
{
    DNSc_CACHE_ITEM  *p_cache;


    DNScCache_LockAcquire();

    p_cache = DNScCache_ItemGet(p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit_release;
    }

    p_cache->HostPtr = p_host;

    if (DNSc_DataPtr->CacheItemListHead == DEF_NULL) {
        p_cache->NextPtr        = DEF_NULL;
        DNSc_DataPtr->CacheItemListHead  = p_cache;
    } else {
        p_cache->NextPtr        = DNSc_DataPtr->CacheItemListHead;
        DNSc_DataPtr->CacheItemListHead  = p_cache;
    }


exit_release:
    DNScCache_LockRelease();
}



/*
*********************************************************************************************************
*                                      DNScCache_HostSrchRemove()
*
* Description : Search host name in cache and remove it.
*
* Argument(s) : p_host_name     Pointer to a string that contains the host name.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostSrchRemove (CPU_CHAR  *p_host_name,
                                RTOS_ERR  *p_err)
{
    DNSc_HOST_OBJ  *p_host;


    DNScCache_LockAcquire();

    p_host = DNScCache_HostSrchByName(p_host_name);
    if (p_host != DEF_NULL) {
        switch(p_host->State) {
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NET_OP_IN_PROGRESS);
                 goto exit_release;

            case DNSc_STATE_RESOLVED:
            case DNSc_STATE_FAILED:
            default:
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
                 DNScCache_HostRemoveHandler(p_host);
                 goto exit_release;
        }
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);


exit_release:
    DNScCache_LockRelease();
}


/*
*********************************************************************************************************
*                                        DNScCache_HostRemove()
*
* Description : Remove host from the cache.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostRemove (DNSc_HOST_OBJ  *p_host)
{
    DNScCache_LockAcquire();

    DNScCache_HostRemoveHandler(p_host);

    DNScCache_LockRelease();
}


/*
*********************************************************************************************************
*                                           DNScCache_Srch()
*
* Description : Search host in cache and return IP addresses, if found.
*
* Argument(s) : p_host_name         Pointer to a string that contains the host name.
*
*               p_addrs             Pointer to addresses array.
*
*               addr_nbr_max        Number of address the address array can contains.
*
*               p_addr_nbr_rtn      Pointer to a variable that will receive number of addresses copied.
*
*               flags           DNS client flag:
*
*                                   DNSc_FLAG_NONE              By default all IP address can be returned.
*                                   DNSc_FLAG_IPv4_ONLY         Return only IPv4 address(es).
*                                   DNSc_FLAG_IPv6_ONLY         Return only IPv6 address(es).
*
*               p_err               Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_STATUS  DNScCache_Srch (CPU_CHAR         *p_host_name,
                             NET_IP_ADDR_OBJ  *p_addrs,
                             CPU_INT08U        addr_nbr_max,
                             CPU_INT08U       *p_addr_nbr_rtn,
                             DNSc_FLAGS        flags,
                             RTOS_ERR         *p_err)
{
    CPU_INT08U         i       = 0u;
    DNSc_HOST_OBJ     *p_host  = DEF_NULL;
    NET_IP_ADDR_OBJ   *p_addr  = DEF_NULL;
    DNSc_ADDR_ITEM    *p_item  = DEF_NULL;
    DNSc_STATUS        status  = DNSc_STATUS_FAILED;
    CPU_BOOLEAN        no_ipv4 = DEF_BIT_IS_SET(flags, DNSc_FLAG_IPv6_ONLY);
    CPU_BOOLEAN        no_ipv6 = DEF_BIT_IS_SET(flags, DNSc_FLAG_IPv4_ONLY);


   *p_addr_nbr_rtn = 0u;

    DNScCache_LockAcquire();

    p_host = DNScCache_HostSrchByName(p_host_name);
    if (p_host == DEF_NULL) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);
        goto exit_release;
    }

    switch(p_host->State) {
        case DNSc_STATE_TX_REQ_IPv4:
        case DNSc_STATE_RX_RESP_IPv4:
        case DNSc_STATE_TX_REQ_IPv6:
        case DNSc_STATE_RX_RESP_IPv6:
             status = DNSc_STATUS_PENDING;
             goto exit_release;

        case DNSc_STATE_RESOLVED:
             status = DNSc_STATUS_RESOLVED;
             break;

        case DNSc_STATE_FAILED:
        default:
             goto exit_release;
    }

    p_item = p_host->AddrsFirstPtr;

    for (i = 0u; i < p_host->AddrsCount; i++) {                 /* Copy Addresses                                       */
        p_addr = p_item->AddrPtr;
        if (*p_addr_nbr_rtn < addr_nbr_max) {
            CPU_BOOLEAN  add_addr = DEF_YES;


            switch (p_addr->AddrLen) {
                case NET_IPv4_ADDR_SIZE:
                     if (no_ipv4 == DEF_YES) {
                         add_addr = DEF_NO;
                     }
                     break;

                case NET_IPv6_ADDR_SIZE:
                     if (no_ipv6 == DEF_YES) {
                         add_addr = DEF_NO;
                     }
                     break;

                default:
                     add_addr = DEF_NO;
                     break;
            }

            if (add_addr == DEF_YES) {
                p_addrs[*p_addr_nbr_rtn] = *p_addr;
               *p_addr_nbr_rtn += 1u;
            }

            p_item = p_item->NextPtr;

        } else {
            goto exit_release;
        }
    }

    PP_UNUSED_PARAM(p_addr_nbr_rtn);


exit_release:
    DNScCache_LockRelease();

    return (status);
}


/*
*********************************************************************************************************
*                                        DNScCache_HostObjGet()
*
* Description : Get a free host object.
*
* Argument(s) : p_host_name     Pointer to a string that contains the domain name.
*
*               flags           DNS client flag:
*
*                                   DNSc_FLAG_NONE              By default this function is blocking.
*                                   DNSc_FLAG_NO_BLOCK          Don't block (only possible if DNSc's task is enabled).
*                                   DNSc_FLAG_FORCE_CACHE       Take host from the cache, don't send new DNS request.
*                                   DNSc_FLAG_FORCE_RENW       Force DNS request, remove existing entry in the cache.
*
*               p_cfg           Pointer to a request configuration. Should be set to overwrite default DNS configuration
*                               (such as DNS server, request timeout, etc.). Must be set to DEF_NULL to use default
*                               configuration.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Pointer to the host object acquired.
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_HOST_OBJ  *DNScCache_HostObjGet (CPU_CHAR      *p_host_name,
                                      DNSc_FLAGS     flags,
                                      DNSc_REQ_CFG  *p_cfg,
                                      RTOS_ERR      *p_err)
{
#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    KAL_SEM_HANDLE   sem    = KAL_SemHandleNull;
#endif
    DNSc_HOST_OBJ   *p_host = DEF_NULL;


    PP_UNUSED_PARAM(flags);

    DNScCache_LockAcquire();

                                                                /* ---------- ACQUIRE SIGNAL TASK SEMAPHORE ----------- */
#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    if (DEF_BIT_IS_SET(flags, DNSc_FLAG_NO_BLOCK) == DEF_NO) {

        sem = KAL_SemCreate("DNSc Block Task Signal", DEF_NULL, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            goto exit_release;
        }
    }
#endif


    p_host = (DNSc_HOST_OBJ *)Mem_DynPoolBlkGet(&DNSc_DataPtr->CacheHostObjPool, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) == RTOS_ERR_NONE) {
        p_host->NamePtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&DNSc_DataPtr->CacheHostNamePool, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            goto exit_free_host_obj;
        }

    } else {
        p_host = DNScCache_ItemHostGet();
    }

    if (p_host == DEF_NULL) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
        goto exit_release;
    }

    if (p_host->NamePtr == DEF_NULL) {
        p_host->NameLenMax = DNSc_DataPtr->CacheHostNamePool.BlkSize - 1;
    }

    p_host->NameLenMax = DNSc_DataPtr->CacheHostNamePool.BlkSize;
    Mem_Clr(p_host->NamePtr, p_host->NameLenMax);
    p_host->IF_Nbr         = NET_IF_NBR_WILDCARD;
    p_host->AddrsCount     = 0u;
    p_host->AddrsIPv4Count = 0u;
    p_host->AddrsIPv6Count = 0u;
    p_host->AddrsFirstPtr  = DEF_NULL;
    p_host->AddrsEndPtr    = DEF_NULL;

#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    p_host->TaskSignal = sem;
#endif

    p_host->State      = DNSc_STATE_INIT_REQ;
    p_host->ReqCfgPtr  = p_cfg;

    DNScCache_HostObjNameSet(p_host, p_host_name, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        goto exit_release;
    }

    goto exit_release;


exit_free_host_obj:
{
    RTOS_ERR  local_err;

    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheHostObjPool, p_host, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NULL);
}

exit_release:
    DNScCache_LockRelease();

    return (p_host);
}


/*
*********************************************************************************************************
*                                        DNScCache_HostObjFree()
*
* Description : Free a host object.
*
* Argument(s) : p_host  Pointer to the host object to free.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostObjFree (DNSc_HOST_OBJ  *p_host)
{
    RTOS_ERR  local_err;


#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    if (p_host->TaskSignal.SemObjPtr != KAL_SemHandleNull.SemObjPtr) {
        KAL_SemDel(p_host->TaskSignal);
    }

    p_host->TaskSignal = KAL_SemHandleNull;
#endif

    DNScCache_HostAddrClr(p_host);

    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheHostNamePool, p_host->NamePtr, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheHostObjPool, p_host, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}


/*
*********************************************************************************************************
*                                      DNScCache_HostAddrInsert()
*
* Description : Insert address object in the addresses list of the host object.
*
* Argument(s) : p_host  Pointer to the host object.
*
*               p_addr  Pointer to the address object (must be acquired with cache module)
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostAddrInsert (DNSc_HOST_OBJ    *p_host,
                                NET_IP_ADDR_OBJ  *p_addr,
                                RTOS_ERR         *p_err)
{
    DNSc_ADDR_ITEM  *p_item_cur;


    switch (p_addr->AddrLen) {
        case NET_IPv4_ADDR_SIZE:
             if (p_host->AddrsIPv4Count >= DNSc_DataPtr->CfgAddrIPv4MaxPerHost) {
                 if (p_host->AddrsIPv4Count == 0u) {
                     RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_WOULD_OVF, ;);
                 }
                 goto exit;
             }
             break;

        case NET_IPv6_ADDR_SIZE:
             if (p_host->AddrsIPv6Count >= DNSc_DataPtr->CfgAddrIPv6MaxPerHost) {
                 if (p_host->AddrsIPv6Count == 0u) {
                     RTOS_DBG_FAIL_EXEC_ERR(*p_err, RTOS_ERR_WOULD_OVF, ;);
                 }
                 goto exit;
             }
             break;

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;)
    }

    p_item_cur = DNScCache_AddrItemGet(p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }


    p_item_cur->AddrPtr = p_addr;

    if (p_host->AddrsFirstPtr == DEF_NULL) {
        p_host->AddrsFirstPtr = p_item_cur;
        p_host->AddrsEndPtr   = p_item_cur;

    } else {
        p_host->AddrsEndPtr->NextPtr = p_item_cur;
        p_host->AddrsEndPtr          = p_item_cur;
    }

    switch (p_addr->AddrLen) {
        case NET_IPv4_ADDR_SIZE:
             p_host->AddrsIPv4Count++;
             break;


        case NET_IPv6_ADDR_SIZE:
             p_host->AddrsIPv6Count++;
             break;

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }

    p_host->AddrsCount++;


exit:
    return;
}



/*
*********************************************************************************************************
*                                       DNScCache_HostAddrAlloc()
*
* Description : Allocate a Host IP address.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Pointer to the allocated Host IP address structure.
*
* Caller(s)   : DNS_HostAddrsGet().
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_HOST_IP_ADDR  *DNScCache_HostAddrAlloc (RTOS_ERR  *p_err)
{
    NET_HOST_IP_ADDR  *p_addr = DEF_NULL;

    DNScCache_LockAcquire();

    p_addr = NetCache_HostAddrsGet(&p_addr, &p_addr);
    if (p_addr == DEF_NULL) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_ALLOC);
        goto exit_release;
    }

    goto exit_release;

exit_release:
    DNScCache_LockRelease();

    return (p_addr);
}


/*
*********************************************************************************************************
*                                       DNScCache_HostAddrsGet()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_host_name     $$$$ Add description for 'p_host_name'
*
*               p_addrs         $$$$ Add description for 'p_addrs'
*
*               p_addr_nbr_rtn  $$$$ Add description for 'p_addr_nbr_rtn'
*
*               flags           $$$$ Add description for 'flags'
*
*               p_err           $$$$ Add description for 'p_err'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : DNS_HostAddrsGet().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

DNSc_STATUS  DNScCache_HostAddrsGet (CPU_CHAR           *p_host_name,
                                     NET_HOST_IP_ADDR  **p_addrs,
                                     CPU_INT08U         *p_addr_nbr_rtn,
                                     DNSc_FLAGS          flags,
                                     RTOS_ERR           *p_err)
{
    NET_HOST_IP_ADDR  *p_host_addr;
    NET_HOST_IP_ADDR  *p_host_addr_head = DEF_NULL;
    NET_HOST_IP_ADDR  *p_host_addr_tail = DEF_NULL;
    CPU_INT08U         i                = 0u;
    DNSc_HOST_OBJ     *p_host           = DEF_NULL;
    DNSc_ADDR_ITEM    *p_item           = DEF_NULL;
    DNSc_STATUS        status           = DNSc_STATUS_FAILED;
    CPU_BOOLEAN        no_ipv4          = DEF_BIT_IS_SET(flags, DNSc_FLAG_IPv6_ONLY);
    CPU_BOOLEAN        no_ipv6          = DEF_BIT_IS_SET(flags, DNSc_FLAG_IPv4_ONLY);


   *p_addr_nbr_rtn = 0u;

    DNScCache_LockAcquire();

    p_host = DNScCache_HostSrchByName(p_host_name);
    if (p_host != DEF_NULL) {
        switch(p_host->State) {
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 status = DNSc_STATUS_PENDING;
                 RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_STATE);
                 goto exit_release;

            case DNSc_STATE_RESOLVED:
                 status = DNSc_STATUS_RESOLVED;
                 goto exit_found;

            case DNSc_STATE_FAILED:
            default:
                RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
                 goto exit_release;
        }
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);

    goto exit_release;


exit_found:

    p_item = p_host->AddrsFirstPtr;

    for (i = 0u; i < p_host->AddrsCount; i++) {                 /* Copy Addresses                                       */
        NET_IP_ADDR_OBJ  *p_dns_addr = p_item->AddrPtr;
        CPU_BOOLEAN       add_addr   = DEF_YES;

        switch (p_dns_addr->AddrLen) {
            case NET_IPv4_ADDR_SIZE:
                 if (no_ipv4 == DEF_YES) {
                     add_addr = DEF_NO;
                 }
                 break;

            case NET_IPv6_ADDR_SIZE:
                 if (no_ipv6 == DEF_YES) {
                     add_addr = DEF_NO;
                 }
                 break;

            default:
                 add_addr = DEF_NO;
                 break;
        }

        if (add_addr == DEF_YES) {
            p_host_addr = NetCache_HostAddrsGet(&p_host_addr_head, &p_host_addr_tail);
            if (p_host_addr == DEF_NULL) {
                RTOS_ERR_SET(*p_err, RTOS_ERR_ALLOC);
                goto exit_error;
            }

            p_host_addr->AddrObj.AddrLen = p_dns_addr->AddrLen;
            Mem_Copy(&p_host_addr->AddrObj.Addr, &p_dns_addr->Addr, p_dns_addr->AddrLen);

           *p_addr_nbr_rtn += 1;
        }

        p_item = p_item->NextPtr;
    }

   *p_addrs = p_host_addr_head;

    goto exit_release;


exit_error:
    DNScCache_HostAddrsFree(p_host_addr_head);
   *p_addr_nbr_rtn = 0u;

exit_release:
    DNScCache_LockRelease();

    return (status);
}


/*
*********************************************************************************************************
*                                       DNScCache_HostAddrsFree()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_addrs     $$$$ Add description for 'p_addrs'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : DNS_HostAddrsGet(),
*               DNScCache_HostAddrsGet(),
*               DNSc_HostAddrsFree().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void DNScCache_HostAddrsFree (NET_HOST_IP_ADDR  *p_addrs)
{
    DNScCache_LockAcquire();

    NetCache_HostAddrsFree(p_addrs);

    DNScCache_LockRelease();
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrObjGet()
*
* Description : Acquire an address object that can be inserted in host list afterward.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : Pointer to the address object acquired, if no error.
*               DEF_NULL, otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

NET_IP_ADDR_OBJ  *DNScCache_AddrObjGet (RTOS_ERR  *p_err)
{
    NET_IP_ADDR_OBJ  *p_addr = DEF_NULL;


    p_addr = (NET_IP_ADDR_OBJ *)Mem_DynPoolBlkGet(&DNSc_DataPtr->CacheAddrObjPool, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }

    Mem_Clr(p_addr, sizeof(NET_IP_ADDR_OBJ));

exit:
    return (p_addr);
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrObjFree()
*
* Description : Free an address object
*
* Argument(s) : p_addr  Pointer to address object to free.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_AddrObjFree (NET_IP_ADDR_OBJ  *p_addr)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheAddrObjPool, p_addr, &local_err);
    PP_UNUSED_PARAM(local_err);
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrObjSet()
*
* Description : Set address object from IP string.
*
* Argument(s) : p_addr      Pointer to the address object.
*
*               p_str_addr  Pointer to the string that contains the IP address.
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_AddrObjSet (NET_IP_ADDR_OBJ  *p_addr,
                            CPU_CHAR         *p_str_addr,
                            RTOS_ERR         *p_err)
{
    NET_IP_ADDR_FAMILY  ip_addr_family;


    ip_addr_family = NetASCII_Str_to_IP(         p_str_addr,
                                        (void *)&p_addr->Addr,
                                                 sizeof(p_addr->Addr),
                                                 p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    switch (ip_addr_family) {
        case NET_IP_ADDR_FAMILY_IPv4:
             p_addr->AddrLen = NET_IPv4_ADDR_SIZE;
             break;

        case NET_IP_ADDR_FAMILY_IPv6:
             p_addr->AddrLen = NET_IPv6_ADDR_SIZE;
             break;

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }
}


/*
*********************************************************************************************************
*                                        DNScCache_ResolveHost()
*
* Description : Launch resolution of an host.
*
* Argument(s) : p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function:
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_STATUS  DNScCache_ResolveHost (DNSc_HOST_OBJ  *p_host,
                                    RTOS_ERR       *p_err)
{
    DNSc_STATUS  status;


    DNScCache_LockAcquire();

    status = DNScCache_Resolve(p_host, p_err);

    DNScCache_LockRelease();

    return (status);
}


/*
*********************************************************************************************************
*                                        DNScCache_ResolveAll()
*
* Description : Launch resolution on all entries that are pending in the cache.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Number of entries that are completed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16U  DNScCache_ResolveAll (RTOS_ERR  *p_err)
{
    DNSc_CACHE_ITEM  *p_item;
    DNSc_HOST_OBJ    *p_host;
    DNSc_STATUS       status;
    CPU_INT16U        resolved_ctr = 0u;


    DNScCache_LockAcquire();

    p_item = DNSc_DataPtr->CacheItemListHead;

    while (p_item != DEF_NULL) {
        p_host = p_item->HostPtr;

        if (p_host->State != DNSc_STATE_RESOLVED) {
            status = DNScCache_Resolve(p_host, p_err);
            switch (status) {
                case DNSc_STATUS_NONE:
                case DNSc_STATUS_PENDING:
                     break;

                case DNSc_STATUS_RESOLVED:
                case DNSc_STATUS_FAILED:
                default:
#ifdef  DNSc_SIGNAL_TASK_MODULE_EN
                    if (KAL_SEM_HANDLE_IS_NULL(p_host->TaskSignal) != DEF_YES) {
                        RTOS_ERR  local_err;


                        RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
                        KAL_SemPost(p_host->TaskSignal, KAL_OPT_NONE, &local_err);
                        RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, 0u);
                    }
#endif
                    resolved_ctr++;
                    break;
            }
        }

        p_item = p_item->NextPtr;
    }

    DNScCache_LockRelease();

    return (resolved_ctr);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        DNScCache_LockAcquire()
*
* Description : Acquire lock on the cache list.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_LockAcquire (void)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    KAL_LockAcquire(DNSc_DataPtr->CacheLockHandle, KAL_OPT_PEND_NONE, 0, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}


/*
*********************************************************************************************************
*                                        DNScCache_LockRelease()
*
* Description : Release cache list lock.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_LockRelease (void)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    KAL_LockRelease(DNSc_DataPtr->CacheLockHandle, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}


/*
*********************************************************************************************************
*                                     DNScCache_HostRemoveHandler()
*
* Description : Remove host from the cache.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_HostRemoveHandler (DNSc_HOST_OBJ  *p_host)
{
    DNSc_CACHE_ITEM  *p_cache = DNSc_DataPtr->CacheItemListHead;


    while (p_cache != DEF_NULL) {
        if (p_cache->HostPtr == p_host) {
            DNScCache_ItemRelease(p_cache);
            goto exit;
        }
        p_cache = p_cache->NextPtr;
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                      DNScCache_HostObjNameSet()
*
* Description : Set the name in host object.
*
* Argument(s) : p_host          Pointer to the host object to set.
*
*               p_host_name     Pointer to a string that contains the domain name.

*               p_err           Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_HostObjNameSet (DNSc_HOST_OBJ  *p_host,
                                        CPU_CHAR       *p_host_name,
                                        RTOS_ERR       *p_err)
{
     Str_Copy_N(p_host->NamePtr, p_host_name, p_host->NameLenMax);
     RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                      DNScCache_HostSrchByName()
*
* Description : Search for an host in the cache from a host name string.
*
* Argument(s) : p_host_name     Pointer to a string that contains the domain name.
*
* Return(s)   : Pointer to the host object, if found.
*               DEF_NULL, Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_HOST_OBJ  *DNScCache_HostSrchByName (CPU_CHAR  *p_host_name)
{
    DNSc_HOST_OBJ    *p_host  = DEF_NULL;
    DNSc_CACHE_ITEM  *p_cache = DNSc_DataPtr->CacheItemListHead;
    CPU_BOOLEAN       match;


    if (p_cache == DEF_NULL) {
         goto exit;
    }

    while (p_cache != DEF_NULL) {
        p_host = p_cache->HostPtr;
        match  = DNScCache_HostNameCmp(p_host, p_host_name);
        if (match == DEF_YES) {
            goto exit;
        }

        p_cache = p_cache->NextPtr;
    }

    p_host = DEF_NULL;

exit:
    return (p_host);
}


/*
*********************************************************************************************************
*                                        DNScCache_HostNameCmp()
*
* Description : Compare host object name field and a host name hane string
*
* Argument(s) : p_host          Pointer to the host object.
*
*               p_host_name     Pointer to a string that contains the host name.
*
* Return(s)   : DEF_OK, if names match
*               DEF_FAIL, otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  DNScCache_HostNameCmp (DNSc_HOST_OBJ  *p_host,
                                            CPU_CHAR       *p_host_name)
{
    CPU_BOOLEAN  result = DEF_FAIL;
    CPU_INT16S   cmp;


    cmp = Str_Cmp_N(p_host_name, p_host->NamePtr, p_host->NameLenMax);
    if (cmp == 0) {
        result = DEF_OK;
    }

    return (result);
}


/*
*********************************************************************************************************
*                                          DNScCache_ItemGet()
*
* Description : Get an Cache item element (list element)
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : Pointer to the item element.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_CACHE_ITEM  *DNScCache_ItemGet (RTOS_ERR  *p_err)
{
    DNSc_CACHE_ITEM  *p_cache;


    p_cache = (DNSc_CACHE_ITEM *)Mem_DynPoolBlkGet(&DNSc_DataPtr->CacheItemPool, p_err);

    return (p_cache);
}


/*
*********************************************************************************************************
*                                         DNScCache_ItemFree()
*
* Description : Free cache item element.
*
* Argument(s) : p_cache     Pointer to cache item element.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_ItemFree (DNSc_CACHE_ITEM  *p_cache)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);
    Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheItemPool, p_cache, &local_err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}



/*
*********************************************************************************************************
*                                        DNScCache_ItemHostGet()
*
* Description : Get host item element (list element).
*
* Argument(s) : none.
*
* Return(s)   : Pointer to host item element.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_HOST_OBJ  *DNScCache_ItemHostGet (void)
{
    DNSc_CACHE_ITEM  *p_item_cur = DNSc_DataPtr->CacheItemListHead;
    DNSc_HOST_OBJ    *p_host     = DEF_NULL;


    if (p_item_cur == DEF_NULL) {
        goto exit;
    }


    while (p_item_cur != DEF_NULL) {
        p_host = p_item_cur->HostPtr;
        switch (p_host->State) {
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 break;

            case DNSc_STATE_FREE:
            case DNSc_STATE_FAILED:
            case DNSc_STATE_RESOLVED:
                 goto exit_found;

            default:
                 p_host = DEF_NULL;
                 goto exit;
        }
        p_item_cur = p_item_cur->NextPtr;
    }


    p_host = DEF_NULL;
    goto exit;


exit_found:
    DNScCache_ItemRemove(p_item_cur);
    DNScCache_HostAddrClr(p_host);

exit:
    return (p_host);
}


/*
*********************************************************************************************************
*                                        DNScCache_ItemRelease()
*
* Description : Release a cache item and everything contained in the item.
*
* Argument(s) : p_cache     Pointer to the cache item.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_ItemRelease (DNSc_CACHE_ITEM  *p_cache)
{
    if (p_cache->HostPtr != DEF_NULL) {
        DNScCache_HostRelease(p_cache->HostPtr);
    }

    DNScCache_ItemRemove(p_cache);
}


/*
*********************************************************************************************************
*                                        DNScCache_ItemRemove()
*
* Description : Remove an item (list element) in the cache.
*
* Argument(s) : p_cache     Pointer to the cache list element to remove.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_ItemRemove (DNSc_CACHE_ITEM  *p_cache)
{
    if (DNSc_DataPtr->CacheItemListHead == p_cache) {
        DNSc_DataPtr->CacheItemListHead = p_cache->NextPtr;
        goto exit_found;

    } else {
        DNSc_CACHE_ITEM  *p_cache_cur  = DNSc_DataPtr->CacheItemListHead->NextPtr;
        DNSc_CACHE_ITEM  *p_cache_prev = DNSc_DataPtr->CacheItemListHead;

        while (p_cache_cur != DEF_NULL) {
            if (p_cache_cur == p_cache) {
                p_cache_prev->NextPtr = p_cache_cur->NextPtr;
                goto exit_found;
            }
        }
    }

    goto exit;


exit_found:
    DNScCache_ItemFree(p_cache);

exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrItemGet()
*
* Description : get an address item element (list)
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Pointer to address element, if no error.
*               DEF_NULL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_ADDR_ITEM  *DNScCache_AddrItemGet (RTOS_ERR  *p_err)
{
    DNSc_ADDR_ITEM  *p_item = DEF_NULL;


    p_item = (DNSc_ADDR_ITEM *)Mem_DynPoolBlkGet(&DNSc_DataPtr->CacheAddrItemPool, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         goto exit;
    }

    p_item->AddrPtr = DEF_NULL;
    p_item->NextPtr = DEF_NULL;


exit:
    return (p_item);
}



/*
*********************************************************************************************************
*                                       DNScCache_AddrItemFree()
*
* Description : Free an address item element.
*
* Argument(s) : p_item  Pointer to the address item element.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_AddrItemFree (DNSc_ADDR_ITEM  *p_item)
{
    RTOS_ERR  local_err;


    RTOS_ERR_SET(local_err, RTOS_ERR_NONE);

    DNScCache_AddrObjFree(p_item->AddrPtr);
    if (p_item->AddrPtr != DEF_NULL) {
        Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheAddrItemPool, p_item, &local_err);
        RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(local_err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }
}

/*
*********************************************************************************************************
*                                        DNScCache_HostRelease()
*
* Description : Release an host and all element contained in the host.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  DNScCache_HostRelease (DNSc_HOST_OBJ  *p_host)
{


    DNScCache_HostAddrClr(p_host);
    DNScCache_HostObjFree(p_host);
}




/*
*********************************************************************************************************
*                                        DNScCache_HostAddrClr()
*
* Description : Remove and free all address elements contained in a host object.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_HostAddrClr (DNSc_HOST_OBJ  *p_host)
{
    DNSc_ADDR_ITEM  *p_addr_coll_cur  = p_host->AddrsFirstPtr;
    DNSc_ADDR_ITEM  *p_addr_coll_next = DEF_NULL;


    while (p_addr_coll_cur != DEF_NULL) {
        p_addr_coll_next = p_addr_coll_cur->NextPtr;

        DNScCache_AddrItemFree(p_addr_coll_cur);

        p_addr_coll_cur = p_addr_coll_next;
    }

    p_host->AddrsFirstPtr = DEF_NULL;
    p_host->AddrsEndPtr   = DEF_NULL;
}


/*
*********************************************************************************************************
*                                          DNScCache_Resolve()
*
* Description : Process resolution of an host (state machine controller).
*
* Argument(s) : p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_STATUS  DNScCache_Resolve (DNSc_HOST_OBJ  *p_host,
                                        RTOS_ERR       *p_err)
{
    DNSc_STATUS       status        = DNSc_STATUS_PENDING;
    NET_IP_ADDR_OBJ  *p_server_addr = DEF_NULL;
    NET_PORT_NBR      server_port   = NET_PORT_NBR_NONE;


    switch (p_host->State) {
        case DNSc_STATE_INIT_REQ:
             if (p_host->ReqCfgPtr != DEF_NULL) {
                 p_server_addr = p_host->ReqCfgPtr->ServerAddrPtr;
                 server_port   = p_host->ReqCfgPtr->ServerPort;
             }

             p_host->SockID = DNScReq_Init(p_server_addr, server_port, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  status = DNSc_STATUS_FAILED;
                  goto exit;
             }

             p_host->ReqCtr = 0u;
             p_host->State  = DNSc_STATE_IF_SEL;
             status         = DNSc_STATUS_PENDING;
             break;


        case DNSc_STATE_IF_SEL:
             p_host->IF_Nbr = DNSc_ReqIF_Sel(p_host->IF_Nbr, p_host->SockID, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  status = DNSc_STATUS_FAILED;
                  break;
             }

#ifdef  NET_IPv4_MODULE_EN
             p_host->State = DNSc_STATE_TX_REQ_IPv4;
#else
   #ifdef  NET_IPv6_MODULE_EN
             p_host->State = DNSc_STATE_TX_REQ_IPv6;
   #else
             RTOS_ASSERT_CRITICAL(DEF_FAIL, RTOS_ERR_NOT_AVAIL, DNSc_STATUS_FAILED);
             goto exit;
   #endif
#endif

            status = DNSc_STATUS_PENDING;
            break;

        case DNSc_STATE_TX_REQ_IPv4:
        case DNSc_STATE_TX_REQ_IPv6:
             DNScCache_Req(p_host, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  status = DNSc_STATUS_FAILED;
                  break;
             }
             status = DNSc_STATUS_PENDING;
             break;


        case DNSc_STATE_RX_RESP_IPv4:
        case DNSc_STATE_RX_RESP_IPv6:
             status = DNScCache_Resp(p_host, p_err);
             if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                  status = DNSc_STATUS_FAILED;
                  break;
             }
             break;


        case DNSc_STATE_RESOLVED:
             status = DNSc_STATUS_RESOLVED;
             RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
             break;


        case DNSc_STATE_FAILED:
             status = DNSc_STATUS_FAILED;
             break;

        case DNSc_STATE_FREE:
        default:
             status = DNSc_STATUS_FAILED;
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_INVALID_STATE, DNSc_STATUS_FAILED);
    }


    switch (status) {
        case DNSc_STATUS_PENDING:
             break;

        case DNSc_STATUS_RESOLVED:
        case DNSc_STATUS_FAILED:
        default:
             DNSc_ReqClose(p_host->SockID);
             break;
    }


exit:
    return (status);
}

/*
*********************************************************************************************************
*                                            DNScCache_Req()
*
* Description : Send an host resolution request.
*
* Argument(s) : p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_Req (DNSc_HOST_OBJ  *p_host,
                             RTOS_ERR       *p_err)
{
    DNSc_REQ_TYPE  req_type;
    CPU_INT16U     query_id;


    switch (p_host->State) {
        case DNSc_STATE_TX_REQ_IPv4:
             req_type = DNSc_REQ_TYPE_IPv4;
             break;

        case DNSc_STATE_TX_REQ_IPv6:
             req_type = DNSc_REQ_TYPE_IPv6;
             break;

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }

    if (p_host->ReqCtr > 0) {
        query_id = p_host->QueryID;
    } else {
        query_id = DNSc_QUERY_ID_NONE;
    }

    p_host->QueryID = DNScReq_TxReq(p_host->NamePtr, p_host->SockID, query_id, req_type, p_err);
    switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
             break;

        case RTOS_ERR_NET_IF_LINK_DOWN:
             p_host->State = DNSc_STATE_IF_SEL;
             RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
             goto exit;

        default:
             goto exit;
    }

    switch (p_host->State) {
        case DNSc_STATE_TX_REQ_IPv4:
             p_host->State = DNSc_STATE_RX_RESP_IPv4;
             break;

        case DNSc_STATE_TX_REQ_IPv6:
             p_host->State = DNSc_STATE_RX_RESP_IPv6;
             break;

        default:
             RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }

    p_host->TS_ms = NetUtil_TS_Get_ms();
    p_host->ReqCtr++;


exit:
    return;
}


/*
*********************************************************************************************************
*                                           DNScCache_Resp()
*
* Description : Receive host resolution request response.
*
* Argument(s) : p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_STATUS  DNScCache_Resp (DNSc_HOST_OBJ  *p_host,
                                     RTOS_ERR       *p_err)
{
    DNSc_STATUS  status;
    NET_TS_MS    ts_cur_ms;
    NET_TS_MS    ts_delta_ms;
    CPU_INT16U   timeout_ms   = DNSc_DataPtr->CfgReqRetryTimeout_ms;
    CPU_INT08U   req_retry    = DNSc_DataPtr->CfgReqRetryNbrMax;
    CPU_BOOLEAN  re_tx        = DEF_NO;
    CPU_BOOLEAN  change_state = DEF_NO;


    if (p_host->ReqCfgPtr != DEF_NULL) {
        timeout_ms = p_host->ReqCfgPtr->ReqTimeout_ms;
        req_retry  = p_host->ReqCfgPtr->ReqRetry;
    }


#ifndef NET_DNSc_CFG_MODE_ASYNC_EN
    status = DNScReq_RxResp(p_host, p_host->SockID, p_host->QueryID, 0, p_err);
#else
    status = DNScReq_RxResp(p_host, p_host->SockID, p_host->QueryID, timeout_ms, p_err);
#endif

    switch (RTOS_ERR_CODE_GET(*p_err)) {
        case RTOS_ERR_NONE:
             change_state = DEF_YES;
             break;

        case RTOS_ERR_WOULD_BLOCK:
        case RTOS_ERR_TIMEOUT:
             if (p_host->ReqCtr >= req_retry) {
                 status        = DNSc_STATUS_FAILED;
                 p_host->State = DNSc_STATE_FAILED;
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NOT_FOUND);
                 goto exit;

             } else {
                 ts_cur_ms   = NetUtil_TS_Get_ms();
                 ts_delta_ms = ts_cur_ms - p_host->TS_ms;
                 if (ts_delta_ms >= timeout_ms) {
                     re_tx        = DEF_YES;
                     change_state = DEF_YES;
                 }
                 RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
             }
             break;

        default:
            goto exit;
    }

    if (change_state == DEF_YES) {
        switch (p_host->State) {
            case DNSc_STATE_RX_RESP_IPv4:
                 if (re_tx == DEF_YES) {
                     p_host->State = DNSc_STATE_TX_REQ_IPv4;

                 } else {
#ifdef  NET_IPv6_MODULE_EN
                     p_host->ReqCtr = 0;
                     p_host->State  = DNSc_STATE_TX_REQ_IPv6;
                     status         = DNSc_STATUS_PENDING;
#else
                     p_host->State = DNSc_STATE_RESOLVED;
                     status        = DNSc_STATUS_RESOLVED;
#endif
                 }
                 break;

            case DNSc_STATE_RX_RESP_IPv6:
                 if (re_tx == DEF_YES) {
                     p_host->State = DNSc_STATE_TX_REQ_IPv6;
                     status        = DNSc_STATUS_PENDING;

                 } else if (status != DNSc_STATUS_RESOLVED) {   /* If the resolution has failed, let try on another     */
                     p_host->State = DNSc_STATE_IF_SEL;         /* interface. It may be possible to reach the DNS       */
                     status        = DNSc_STATUS_PENDING;       /* server using another link.                           */

                 } else {
                     p_host->State = DNSc_STATE_RESOLVED;
                     status        = DNSc_STATUS_RESOLVED;
                 }
                 break;

            default:
                 status = DNSc_STATUS_FAILED;
                 RTOS_CRITICAL_FAIL_EXEC(RTOS_ERR_ASSERT_CRITICAL_FAIL, DNSc_STATUS_FAILED);
        }
    }


exit:
    return (status);
}


/*
*********************************************************************************************************
*                                        NetCache_HostAddrsGet()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_head  $$$$ Add description for 'p_head'
*
*               p_tail  $$$$ Add description for 'p_tail'
*
* Return(s)   : $$$$ Add return value description.
*
* Caller(s)   : DNScCache_HostAddrAlloc(),
*               DNScCache_HostAddrsGet().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  NET_HOST_IP_ADDR *NetCache_HostAddrsGet (NET_HOST_IP_ADDR  **p_head,
                                                 NET_HOST_IP_ADDR  **p_tail)
{
    NET_HOST_IP_ADDR  *p_addrinfo;
    NET_HOST_IP_ADDR  *p_addrinfo_rtn = DEF_NULL;
    RTOS_ERR           err;


    p_addrinfo = (NET_HOST_IP_ADDR  *)Mem_DynPoolBlkGet(&DNSc_DataPtr->CacheHostAddrObjPool, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        goto exit;
    }

    p_addrinfo->NextPtr = DEF_NULL;
    p_addrinfo_rtn      = p_addrinfo;

    if (*p_head == DEF_NULL) {
        *p_head = p_addrinfo_rtn;
    }

    if (*p_tail != DEF_NULL) {
       (*p_tail)->NextPtr = p_addrinfo_rtn;
    }

   *p_tail = p_addrinfo_rtn;

exit:
    return (p_addrinfo_rtn);
}


/*
*********************************************************************************************************
*                                     NetCache_HostAddrsIPObjFree()
*
* Description : Free Host Addresses IP Objects.
*
* Argument(s) : p_addrs     Pointer to the head list of Host addresses.
*
* Return(s)   : none.
*
* Caller(s)   : DNScCache_HostAddrsFree().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void NetCache_HostAddrsFree (NET_HOST_IP_ADDR  *p_addrs)
{
    NET_HOST_IP_ADDR  *p_blk = p_addrs;
    RTOS_ERR           err;


    while (p_blk != DEF_NULL) {
        NET_HOST_IP_ADDR  *p_next = p_blk->NextPtr;


        Mem_DynPoolBlkFree(&DNSc_DataPtr->CacheHostAddrObjPool, p_blk, &err);

        p_blk = p_next;

       (void)&err;
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_DNS_CLIENT_MODULE_EN */
#endif  /* RTOS_MODULE_NET_AVAIL    */

