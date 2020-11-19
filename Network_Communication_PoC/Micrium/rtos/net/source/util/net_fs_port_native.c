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
*                                   NETWORK FILE SYSTEM PORT LAYER
*                                       Micrium OS File System
*
* File : net_fs_port_native.c
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

#if (defined(RTOS_MODULE_NET_AVAIL) && \
     defined(RTOS_MODULE_FS_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/source/logging/logging_priv.h>
#include  <rtos/common/include/rtos_utils.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>

#include  <rtos/net/source/tcpip/net_priv.h>
#include  <rtos/net/include/net_fs.h>

#include  <rtos/fs/include/fs_core.h>
#include  <rtos/fs/include/fs_core_file.h>
#include  <rtos/fs/include/fs_core_dir.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>
#include  <fs_core_cfg.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         CONFIGURATION ERROR
*********************************************************************************************************
*********************************************************************************************************
*/

#if (FS_CORE_CFG_DIR_EN != DEF_ENABLED)
#error  FS_CORE_CFG_DIR_EN not #defined in fs_core_cfg.h [MUST be DEF_ENABLED ]
#endif

#if (FS_CORE_CFG_TASK_WORKING_DIR_EN != DEF_ENABLED)
#error  FS_CORE_CFG_TASK_WORKING_DIR_EN not #defined in fs_core_cfg.h [MUST be DEF_ENABLED ]
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                        (NET, HTTP)
#define  RTOS_MODULE_CUR                     RTOS_CFG_MODULE_NET

#define  NET_FS_FILE_BUF_SIZE                512u

#define  NET_FS_MAX_PATH                      2048u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  net_fs_dir_handle {
    FS_DIR_HANDLE  DirHandle;
} NET_FS_DIR_HANDLE;


typedef  struct  net_fs_file_handle {
    FS_FILE_HANDLE  FileHandle;
    CPU_CHAR       *Filename;
} NET_FS_FILE_HANDLE;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_CHAR      NetFS_CfgPathGetSepChar    (void);

                                                                /* --------------- DIRECTORY FUNCTIONS ---------------- */
static  void         *NetFS_DirOpen              (CPU_CHAR             *p_name);

static  void          NetFS_DirClose             (void                 *p_dir);

static  CPU_BOOLEAN   NetFS_DirRd                (void                 *p_dir,
                                                  NET_FS_ENTRY         *p_entry);

                                                                /* ----------------- ENTRY FUNCTIONS ------------------ */
static  CPU_BOOLEAN   NetFS_EntryCreate          (CPU_CHAR             *p_name,
                                                  CPU_BOOLEAN           dir);

static  CPU_BOOLEAN   NetFS_EntryDel             (CPU_CHAR             *p_name,
                                                  CPU_BOOLEAN           file);

static  CPU_BOOLEAN   NetFS_EntryRename          (CPU_CHAR             *p_name_old,
                                                  CPU_CHAR             *p_name_new);

static  CPU_BOOLEAN   NetFS_EntryTimeSet         (CPU_CHAR             *p_name,
                                                  NET_FS_DATE_TIME     *p_time);

                                                                /* ------------------ FILE FUNCTIONS ------------------ */
static  void         *NetFS_FileOpen             (CPU_CHAR             *p_name,
                                                  NET_FS_FILE_MODE      mode,
                                                  NET_FS_FILE_ACCESS    access);

static  void          NetFS_FileClose            (void                 *p_file);


static  CPU_BOOLEAN   NetFS_FileRd               (void                 *p_file,
                                                  void                 *p_dest,
                                                  CPU_SIZE_T            size,
                                                  CPU_SIZE_T           *p_size_rd);

static  CPU_BOOLEAN   NetFS_FileWr               (void                 *p_file,
                                                  void                 *p_src,
                                                  CPU_SIZE_T            size,
                                                  CPU_SIZE_T           *p_size_wr);

static  CPU_BOOLEAN   NetFS_FilePosSet           (void                 *p_file,
                                                  CPU_INT32S            offset,
                                                  CPU_INT08U            origin);

static  CPU_BOOLEAN   NetFS_FileSizeGet          (void                 *p_file,
                                                  CPU_INT32U           *p_size);

static  CPU_BOOLEAN   NetFS_FileDateTimeCreateGet(void                 *p_file,
                                                  NET_FS_DATE_TIME     *p_time);

static  CPU_BOOLEAN   NetFS_WorkingFolderGet     (CPU_CHAR             *p_path,
                                                  CPU_SIZE_T            path_len_max);

static  CPU_BOOLEAN   NetFS_WorkingFolderSet     (CPU_CHAR             *p_path);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FILE SYSTEM API
*
* Note(s) : (1) File system API structures are used by network applications during calls.  This API structure
*               allows network application to call specific file system functions via function pointer instead
*               of by name.  This enables the network application suite to compile & operate with multiple
*               file system.
*
*           (2) In most cases, the API structure provided below SHOULD suffice for most network application
*               exactly as is with the exception that the API structure's name which MUST be unique &
*               SHOULD clearly identify the file system being implemented.  For example, the Micrium file system
*               V4's API structure should be named NetFS_API_FS_V4[].
*
*               The API structure MUST also be externally declared in the File system port header file
*               ('net_fs_&&&.h') with the exact same name & type.
*********************************************************************************************************
*/
                                                                        /* Net FS static API fnct ptrs :                */
const  NET_FS_API  NetFS_API_Native = {
                                       NetFS_CfgPathGetSepChar,         /*   Path sep char.                             */
                                       NetFS_FileOpen,                  /*   Open                                       */
                                       NetFS_FileClose,                 /*   Close                                      */
                                       NetFS_FileRd,                    /*   Rd                                         */
                                       NetFS_FileWr,                    /*   Wr                                         */
                                       NetFS_FilePosSet,                /*   Set position                               */
                                       NetFS_FileSizeGet,               /*   Get size                                   */
                                       NetFS_DirOpen,                   /*   Open  dir                                  */
                                       NetFS_DirClose,                  /*   Close dir                                  */
                                       NetFS_DirRd,                     /*   Rd    dir                                  */
                                       NetFS_EntryCreate,               /*   Entry create                               */
                                       NetFS_EntryDel,                  /*   Entry del                                  */
                                       NetFS_EntryRename,               /*   Entry rename                               */
                                       NetFS_EntryTimeSet,              /*   Entry set time                             */
                                       NetFS_FileDateTimeCreateGet,     /*   Create date time                           */
                                       NetFS_WorkingFolderGet,          /*   Get working folder                         */
                                       NetFS_WorkingFolderSet,          /*   Set working folder                         */
                                     };

/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  MEM_DYN_POOL   NetFS_DirHandlePool;
static  MEM_DYN_POOL   NetFS_FileHandlePool;

static  CPU_BOOLEAN    NetFS_DirHandlePoolInit  = DEF_NO;
static  CPU_BOOLEAN    NetFS_FileHandlePoolInit = DEF_NO;

#if (FS_CORE_CFG_FILE_BUF_EN == DEF_ENABLED)
static  CPU_DATA  NetFS_FileBuf[NET_FS_FILE_BUF_SIZE/sizeof(CPU_DATA)];

static  FS_FILE_HANDLE  NetFS_BufFileHandle = {0};
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
*                                          NetFS_CfgPathGetSepChar()
*
* Description : Get path separator character
*
* Argument(s) : none.
*
* Return(s)   : separator charater.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_CHAR  NetFS_CfgPathGetSepChar (void)
{
    return ((CPU_CHAR)'/');
}


/*
*********************************************************************************************************
*                                          NetFS_DirOpen()
*
* Description : Open a directory.
*
* Argument(s) : p_name  Name of the directory.
*
* Return(s)   : Pointer to a directory, if NO errors.
*               Pointer to NULL,        otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  *NetFS_DirOpen (CPU_CHAR  *p_name)
{
    NET_FS_DIR_HANDLE  *p_handle   = DEF_NULL;
#if  (FS_CORE_CFG_DIR_EN == DEF_ENABLED)
    FS_DIR_HANDLE       dir_handle = FSDir_NullHandle;
    FS_FLAGS            fs_flags   = 0;
    CPU_BOOLEAN         is_init    = DEF_NO;
    RTOS_ERR            err;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG((p_name != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_NULL);

    CPU_CRITICAL_ENTER();
    is_init = NetFS_DirHandlePoolInit;
    CPU_CRITICAL_EXIT();

    LOG_VRB(("NET-FS : Open Directory: ", (s)p_name));

    if (is_init == DEF_NO) {
        LOG_VRB(("NET-FS : Initialize Directory Handle"));
        Mem_DynPoolCreate("Dir Handle Pool",
                          &NetFS_DirHandlePool,
                           Net_CoreDataPtr->CoreMemSegPtr,
                           sizeof(NET_FS_DIR_HANDLE),
                           sizeof(CPU_ALIGN),
                           1,
                           LIB_MEM_BLK_QTY_UNLIMITED,
                          &err);
        RTOS_ASSERT_CRITICAL(RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE, RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NULL);

        CPU_CRITICAL_ENTER();
        NetFS_DirHandlePoolInit = DEF_YES;
        CPU_CRITICAL_EXIT();
    }

    LOG_VRB(("NET-FS : Get Directory Handle"));
    p_handle = (NET_FS_DIR_HANDLE *)Mem_DynPoolBlkGet(&NetFS_DirHandlePool, &err);
    RTOS_ASSERT_CRITICAL(RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE, RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NULL);


    fs_flags = FS_DIR_ACCESS_MODE_NONE | FS_DIR_ACCESS_MODE_CREATE;

                                                                /* -------------------- OPEN DIR ---------------------- */
    dir_handle = FSDir_Open(FS_WRK_DIR_CUR,
                            p_name,
                            fs_flags,
                           &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSDir_Open Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        goto exit_release;
    }

    p_handle->DirHandle = dir_handle;

    goto exit;

exit_release:
    Mem_DynPoolBlkFree(&NetFS_DirHandlePool, p_handle, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NULL);
    p_handle = DEF_NULL;

exit:
#else
    PP_UNUSED_PARAM(p_name);                                    /* Prevent 'variable unused' compiler warning.          */
    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_NULL);
#endif

    return (p_handle);
}


/*
*********************************************************************************************************
*                                         NetFS_DirClose()
*
* Description : Close a directory.
*
* Argument(s) : p_dir   Pointer to a directory.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetFS_DirClose (void  *p_dir)
{
#if  (FS_CORE_CFG_DIR_EN == DEF_ENABLED)
    NET_FS_DIR_HANDLE  *p_handle = DEF_NULL;
    RTOS_ERR            err;


    LOG_VRB(("NET-FS : Close Directory"));
    RTOS_ASSERT_DBG((p_dir != DEF_NULL), RTOS_ERR_NULL_PTR, ;);

    p_handle = (NET_FS_DIR_HANDLE *)p_dir;

                                                                /* -------------------- CLOSE DIR --------------------- */
    FSDir_Close(p_handle->DirHandle,
               &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSDir_Close Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
    }

    RTOS_ERR_SET(err, RTOS_ERR_NONE);
    LOG_VRB(("NET-FS : Free Directory Handle"));
    Mem_DynPoolBlkFree(&NetFS_DirHandlePool, p_handle, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);

#else
    PP_UNUSED_PARAM(p_dir);                                     /* Prevent 'variable unused' compiler warning.          */
    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, ;);
#endif
}


/*
*********************************************************************************************************
*                                           NetFS_DirRd()
*
* Description : Read a directory entry from a directory.
*
* Argument(s) : p_dir       Pointer to a directory.
*
*               p_entry     Pointer to variable that will receive directory entry information.
*
* Return(s)   : DEF_OK,   if directory entry read.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_DirRd (void          *p_dir,
                                  NET_FS_ENTRY  *p_entry)
{
#if  (FS_CORE_CFG_DIR_EN == DEF_ENABLED)
    NET_FS_DIR_HANDLE  *p_handle;
    CPU_INT16U          attrib;
    FS_ENTRY_INFO       entry_info;
    CLK_DATE_TIME       stime;
    CPU_BOOLEAN         conv_success;
    RTOS_ERR            err;


    RTOS_ASSERT_DBG((p_dir            != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_entry          != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_entry->NamePtr != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

    p_handle = (NET_FS_DIR_HANDLE *)p_dir;

    LOG_VRB(("NET-FS : Read Directory"));

                                                                /* ---------------------- RD DIR ---------------------- */
    FSDir_Rd(p_handle->DirHandle,
            &entry_info,
             p_entry->NamePtr,
             p_entry->NameSize,
            &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSDir_Rd Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        return (DEF_FAIL);
    }

    attrib = DEF_BIT_NONE;
    if (entry_info.Attrib.Rd == DEF_YES) {
        attrib |= NET_FS_ENTRY_ATTRIB_RD;
    }
    if (entry_info.Attrib.Wr == DEF_YES) {
        attrib |= NET_FS_ENTRY_ATTRIB_WR;
    }
    if (entry_info.Attrib.Hidden == DEF_YES) {
        attrib |= NET_FS_ENTRY_ATTRIB_HIDDEN;
    }
    if (entry_info.Attrib.IsDir == DEF_YES) {
        attrib |= NET_FS_ENTRY_ATTRIB_DIR;
    }
    p_entry->Attrib = attrib;
    p_entry->Size   = entry_info.Size;


    conv_success = Clk_TS_UnixToDateTime(entry_info.DateTimeCreate,
                                         0,
                                        &stime);
    if (conv_success == DEF_OK) {
        p_entry->DateTimeCreate.Sec   = stime.Sec;
        p_entry->DateTimeCreate.Min   = stime.Min;
        p_entry->DateTimeCreate.Hr    = stime.Hr;
        p_entry->DateTimeCreate.Day   = stime.Day;
        p_entry->DateTimeCreate.Month = stime.Month;
        p_entry->DateTimeCreate.Yr    = stime.Yr;
    } else {
        p_entry->DateTimeCreate.Sec   = 0u;
        p_entry->DateTimeCreate.Min   = 0u;
        p_entry->DateTimeCreate.Hr    = 0u;
        p_entry->DateTimeCreate.Day   = 1u;
        p_entry->DateTimeCreate.Month = 1u;
        p_entry->DateTimeCreate.Yr    = 0u;
    }

    return (DEF_OK);


#else
    PP_UNUSED_PARAM(p_dir);                                     /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(p_entry);

    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_FAIL);
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                        NetFS_EntryCreate()
*
* Description : Create a file or directory.
*
* Argument(s) : p_name  Name of the entry.
*
*               dir     Indicates whether the new entry shall be a directory :
*
*                           DEF_YES, if the entry shall be a directory.
*                           DEF_NO,  if the entry shall be a file.
*
* Return(s)   : DEF_OK,   if entry created.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_EntryCreate (CPU_CHAR     *p_name,
                                        CPU_BOOLEAN   dir)
{
#if (FS_CORE_CFG_RD_ONLY_EN != DEF_ENABLED)
    CPU_BOOLEAN  ok = DEF_OK;
    FS_FLAGS     entry_type;
    RTOS_ERR     err;



    RTOS_ASSERT_DBG((p_name != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);


                                                                /* ----------------- CREATE FILE/DIR ------------------ */
    if (dir == DEF_YES) {
        entry_type = FS_ENTRY_TYPE_DIR;
        LOG_VRB(("NET-FS : Create Directory : ", (s)p_name));
    } else {
        entry_type = FS_ENTRY_TYPE_FILE;
        LOG_VRB(("NET-FS : Create File : ", (s)p_name));
    }

    FSEntry_Create(FS_WRK_DIR_CUR,
                   p_name,
                   entry_type,
                   DEF_YES,
                  &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSEntry_Create() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        ok = DEF_FAIL;
    }

    return (ok);

#else
    PP_UNUSED_PARAM(p_name);                                    /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(dir);

    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_FAIL);
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                         NetFS_EntryDel()
*
* Description : Delete a file or directory.
*
* Argument(s) : p_name  Name of the entry.
*
*               file    Indicates whether the entry MAY be a file :
*
*                           DEF_YES, if the entry MAY be a file.
*                           DEF_NO,  if the entry is NOT a file.
*
* Return(s)   : DEF_OK,   if entry created.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_EntryDel (CPU_CHAR     *p_name,
                                     CPU_BOOLEAN   file)
{
#if (FS_CORE_CFG_RD_ONLY_EN != DEF_ENABLED)
    CPU_BOOLEAN  ok = DEF_OK;
    FS_FLAGS     entry_type;
    RTOS_ERR     err;


    RTOS_ASSERT_DBG((p_name != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

                                                                /* --------------------- DEL FILE --------------------- */

    if (file == DEF_YES) {
        entry_type = FS_ENTRY_TYPE_ANY;
        LOG_VRB(("NET-FS : Delete File : ", (s)p_name));
    } else {
        entry_type = FS_ENTRY_TYPE_DIR;
        LOG_VRB(("NET-FS : Delete Directory : ", (s)p_name));
    }

    FSEntry_Del(FS_WRK_DIR_CUR,
                p_name,
                entry_type,
               &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSEntry_Del() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        ok = DEF_FAIL;
    }

    return (ok);

#else
    PP_UNUSED_PARAM(p_name);                                    /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(file);

    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_FAIL);
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                        NetFS_EntryRename()
*
* Description : Rename a file or directory.
*
* Argument(s) : p_name_old  Old path of the entry.
*
*               p_name_new  New path of the entry.
*
* Return(s)   : DEF_OK,   if entry renamed.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_EntryRename (CPU_CHAR  *p_name_old,
                                        CPU_CHAR  *p_name_new)
{
#if (FS_CORE_CFG_RD_ONLY_EN != DEF_ENABLED)
    CPU_BOOLEAN  ok = DEF_OK;
    RTOS_ERR     err;


    RTOS_ASSERT_DBG((p_name_old != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_name_new != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

    LOG_VRB(("NET-FS : Rename Entry : Old Name= ", (s)p_name_old, " New Name=", (s)p_name_new));

                                                                /* ------------------- RENAME FILE -------------------- */
    FSEntry_Rename(FS_WRK_DIR_CUR,
                   p_name_old,
                   FS_WRK_DIR_CUR,
                   p_name_new,
                   DEF_YES,
                  &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSEntry_Rename() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        ok = DEF_FAIL;
    }


    return (ok);

#else
    PP_UNUSED_PARAM(p_name_old);                                /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(p_name_new);

    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_FAIL);
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                       NetFS_EntryTimeSet()
*
* Description : Set a file or directory's date/time.
*
* Argument(s) : p_name  Name of the entry.
*
*               p_time  Pointer to date/time.
*
* Return(s)   : DEF_OK,   if date/time set.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_EntryTimeSet (CPU_CHAR          *p_name,
                                         NET_FS_DATE_TIME  *p_time)
{
#if (FS_CORE_CFG_RD_ONLY_EN != DEF_ENABLED)
    CPU_BOOLEAN    ok = DEF_OK;
    CLK_DATE_TIME  stime;
    RTOS_ERR       err;


    RTOS_ASSERT_DBG((p_name != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_time != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

    LOG_VRB(("NET-FS : Set Entry time : ", (s)p_name));


                                                                /* ------------------ SET DATE/TIME ------------------- */
    stime.Sec   = p_time->Sec;
    stime.Min   = p_time->Min;
    stime.Hr    = p_time->Hr;
    stime.Day   = p_time->Day;
    stime.Month = p_time->Month;
    stime.Yr    = p_time->Yr;

    FSEntry_TimeSet(FS_WRK_DIR_CUR,
                    p_name,
                   &stime,
                    FS_DATE_TIME_ALL,
                   &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSEntry_TimeSet() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        ok = DEF_FAIL;
    }

    return (ok);

#else
    PP_UNUSED_PARAM(p_name);                                    /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(p_time);

    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_FAIL);
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                         NetFS_FileOpen()
*
* Description : Open a file.
*
* Argument(s) : p_name  Name of the file.
*
*               mode    Mode of the file :
*
*                           NET_FS_FILE_MODE_APPEND        Open existing file at end-of-file OR create new file.
*                           NET_FS_FILE_MODE_CREATE        Create new file OR overwrite existing file.
*                           NET_FS_FILE_MODE_CREATE_NEW    Create new file OR return error if file exists.
*                           NET_FS_FILE_MODE_OPEN          Open existing file.
*                           NET_FS_FILE_MODE_TRUNCATE      Truncate existing file to zero length.
*
*               access  Access rights of the file :
*
*                           NET_FS_FILE_ACCESS_RD          Open file in read           mode.
*                           NET_FS_FILE_ACCESS_RD_WR       Open file in read AND write mode.
*                           NET_FS_FILE_ACCESS_WR          Open file in          write mode
*
*
* Return(s)   : Pointer to a file, if NO errors.
*               Pointer to NULL,   otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  *NetFS_FileOpen (CPU_CHAR            *p_name,
                               NET_FS_FILE_MODE     mode,
                               NET_FS_FILE_ACCESS   access)
{
    NET_FS_FILE_HANDLE  *p_handle    = DEF_NULL;
    FS_FILE_HANDLE       file_handle = FSFile_NullHandle;
    FS_FLAGS             mode_flags;
    CPU_BOOLEAN          is_init     = DEF_NO;
    RTOS_ERR             err;
#if (FS_CORE_CFG_FILE_BUF_EN == DEF_ENABLED)
    CPU_SR_ALLOC();
#endif

    LOG_VRB(("NET-FS : Open File : ", (s)p_name));
    RTOS_ASSERT_DBG((p_name != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_NULL);


                                                                /* -------------------- OPEN FILE --------------------- */
    mode_flags = FS_FILE_ACCESS_MODE_NONE;

    switch (mode) {
        case NET_FS_FILE_MODE_APPEND:
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_APPEND);
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_CREATE);
             LOG_VRB(("NET-FS : Mode = append"));
             break;

        case NET_FS_FILE_MODE_CREATE:
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_CREATE);
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_TRUNCATE);
             LOG_VRB(("NET-FS : Mode = create"));
             break;

        case NET_FS_FILE_MODE_CREATE_NEW:
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_CREATE);
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_EXCL);
             LOG_VRB(("NET-FS : Mode = create new"));
             break;

        case NET_FS_FILE_MODE_OPEN:
             LOG_VRB(("NET-FS : Mode = open"));
             break;

        case NET_FS_FILE_MODE_TRUNCATE:
             LOG_VRB(("NET-FS : Mode = truncate"));
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_TRUNCATE);
             break;

        default:
             goto exit;
    }


    switch (access) {
        case NET_FS_FILE_ACCESS_RD:
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_RD);
             LOG_VRB(("NET-FS : Access = read"));
             break;

        case NET_FS_FILE_ACCESS_RD_WR:
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_RD);
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_WR);
             LOG_VRB(("NET-FS : Access = read-write"));
             break;

        case NET_FS_FILE_ACCESS_WR:
             DEF_BIT_SET(mode_flags, FS_FILE_ACCESS_MODE_WR);
             LOG_VRB(("NET-FS : Access = write"));
             break;

        default:
             goto exit;
    }

    CPU_CRITICAL_ENTER();
    is_init = NetFS_FileHandlePoolInit;
    CPU_CRITICAL_EXIT();

    if (is_init == DEF_NO) {
        LOG_VRB(("NET-FS : Initialize File handle pool"));
        Mem_DynPoolCreate("File Handle Pool",
                          &NetFS_FileHandlePool,
                           Net_CoreDataPtr->CoreMemSegPtr,
                           sizeof(NET_FS_FILE_HANDLE),
                           sizeof(CPU_ALIGN),
                           1,
                           LIB_MEM_BLK_QTY_UNLIMITED,
                          &err);
        if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
            goto exit;
        }

        CPU_CRITICAL_ENTER();
        NetFS_FileHandlePoolInit = DEF_YES;
        CPU_CRITICAL_EXIT();
    }

    LOG_VRB(("NET-FS : Get a File handle"));
    p_handle = (NET_FS_FILE_HANDLE *)Mem_DynPoolBlkGet(&NetFS_FileHandlePool, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NULL);


    file_handle = FSFile_Open(FS_WRK_DIR_CUR,
                              p_name,
                              mode_flags,
                             &err);
   if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
       LOG_VRB(("NET-FS : FSFile_Open() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
       goto exit_release;
   }

   p_handle->FileHandle = file_handle;
   p_handle->Filename   = p_name;

#if (FS_CORE_CFG_FILE_BUF_EN == DEF_ENABLED)

    CPU_CRITICAL_ENTER();
    if (FS_FILE_HANDLE_IS_NULL(NetFS_BufFileHandle)) {
        NetFS_BufFileHandle = file_handle;
        CPU_CRITICAL_EXIT();

        FSFile_BufAssign(file_handle, &NetFS_FileBuf, FS_FILE_BUF_MODE_RD_WR, NET_FS_FILE_BUF_SIZE, &err);
        if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
            CPU_CRITICAL_ENTER();
            NetFS_BufFileHandle = FSFile_NullHandle;
            CPU_CRITICAL_EXIT();
        }

    } else {
        CPU_CRITICAL_EXIT();
    }
#endif

    goto exit;

exit_release:
    LOG_VRB(("NET-FS : Free File handle"));
    Mem_DynPoolBlkFree(&NetFS_FileHandlePool, p_handle, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, DEF_NULL);
    p_handle = DEF_NULL;

exit:
    return (p_handle);
}


/*
*********************************************************************************************************
*                                         NetFS_FileClose()
*
* Description : Close a file.
*
* Argument(s) : p_file  Pointer to a file.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetFS_FileClose (void  *p_file)
{
    NET_FS_FILE_HANDLE  *p_handle = DEF_NULL;
    RTOS_ERR             err;
#if (FS_CORE_CFG_FILE_BUF_EN == DEF_ENABLED)
    CPU_BOOLEAN          is_same;
    CPU_SR_ALLOC();
#endif


    RTOS_ASSERT_DBG((p_file != DEF_NULL), RTOS_ERR_NULL_PTR, ;);

    p_handle = (NET_FS_FILE_HANDLE *)p_file;
    LOG_VRB(("NET-FS : Close File : ", (s)p_handle->Filename));

#if (FS_CORE_CFG_FILE_BUF_EN == DEF_ENABLED)
    CPU_CRITICAL_ENTER();
    is_same = Mem_Cmp(&NetFS_BufFileHandle, &p_handle->FileHandle, sizeof(FS_FILE_HANDLE));
    if (is_same == DEF_YES) {
        NetFS_BufFileHandle = FSFile_NullHandle;
    }
    CPU_CRITICAL_EXIT();
#endif


    RTOS_ERR_SET(err, RTOS_ERR_NONE);
                                                                /* -------------------- CLOSE FILE -------------------- */
    FSFile_Close(p_handle->FileHandle, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSFile_Close() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
    }

    LOG_VRB(("NET-FS : Free File handle: "));
    Mem_DynPoolBlkFree(&NetFS_FileHandlePool, p_handle, &err);
    RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
}


/*
*********************************************************************************************************
*                                          NetFS_FileRd()
*
* Description : Read from a file.
*
* Argument(s) : p_file      Pointer to a file.
*
*               p_dest      Pointer to destination buffer.
*
*               size        Number of octets to read.
*
*               p_size_rd   Pointer to variable that will receive the number of octets read.
*
* Return(s)   : DEF_OK,   if no error occurred during read (see Note #2).
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*
*               (2) If the read request could not be fulfilled because the EOF was reached, the return
*                   value should be 'DEF_OK'.  The application should compare the value in 'psize_rd' to
*                   the value passed to 'size' to detect an EOF reached condition.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_FileRd (void        *p_file,
                                   void        *p_dest,
                                   CPU_SIZE_T   size,
                                   CPU_SIZE_T  *p_size_rd)
{
    NET_FS_FILE_HANDLE  *p_handle = DEF_NULL;
    CPU_BOOLEAN          ok       = DEF_FAIL;
    CPU_SIZE_T           size_rd  = 0;
    RTOS_ERR             err;


    RTOS_ASSERT_DBG((p_file    != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_size_rd != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_dest    != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

    p_handle = (NET_FS_FILE_HANDLE *)p_file;
    LOG_VRB(("NET-FS : Read File : ", (s)p_handle->Filename));

   *p_size_rd = 0u;                                             /* Init to dflt size for err (see Note #1).             */

                                                                /* --------------------- RD FILE ---------------------- */
    size_rd  = FSFile_Rd(p_handle->FileHandle,
                         p_dest,
                         size,
                        &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSFile_Rd() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
         ok = DEF_FAIL;
    } else {
        LOG_VRB(("NET-FS : Bytes read : ", (u)size_rd));
        ok = DEF_OK;
    }

   *p_size_rd = size_rd;

    return (ok);
}


/*
*********************************************************************************************************
*                                          NetFS_FileWr()
*
* Description : Write to a file.
*
* Argument(s) : p_file      Pointer to a file.
*
*               p_src       Pointer to source buffer.
*
*               size        Number of octets to write.
*
*               p_size_wr   Pointer to variable that will receive the number of octets written.
*
* Return(s)   : DEF_OK,   if no error occurred during write.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_FileWr (void        *p_file,
                                   void        *p_src,
                                   CPU_SIZE_T   size,
                                   CPU_SIZE_T  *p_size_wr)
{
#if (FS_CORE_CFG_RD_ONLY_EN != DEF_ENABLED)
    NET_FS_FILE_HANDLE  *p_handle = DEF_NULL;
    CPU_BOOLEAN          ok = DEF_OK;
    CPU_SIZE_T           size_wr;
    RTOS_ERR             err;
#endif


    RTOS_ASSERT_DBG((p_file    != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_src     != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_size_wr != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

   *p_size_wr = 0u;                                             /* Init to dflt size (see Note #1).                     */



                                                                /* --------------------- WR FILE ---------------------- */
#if (FS_CORE_CFG_RD_ONLY_EN != DEF_ENABLED)

    p_handle = (NET_FS_FILE_HANDLE *)p_file;
    LOG_VRB(("NET-FS : File Name  : ", (s)p_handle->Filename));
    LOG_VRB(("NET-FS : Write Size : ", (u)size));
    size_wr   =  FSFile_Wr(p_handle->FileHandle,
                           p_src,
                           size,
                          &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSFile_Wr() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        ok = DEF_FAIL;
    }

   *p_size_wr =  size_wr;
    LOG_VRB(("NET-FS : Bytes wrote : ", (u)size_wr));


    return (ok);

#else
    PP_UNUSED_PARAM(p_file);                                    /* Prevent 'variable unused' compiler warning.          */
    PP_UNUSED_PARAM(p_src);
    PP_UNUSED_PARAM(size);

    RTOS_DBG_FAIL_EXEC(RTOS_ERR_NOT_AVAIL, DEF_FAIL);
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                        NetFS_FilePosSet()
*
* Description : Set file position indicator.
*
* Argument(s) : p_file  Pointer to a file.
*
*               offset  Offset from the file position specified by 'origin'.
*
*               origin  Reference position for offset :
*
*                           NET_FS_SEEK_ORIGIN_START    Offset is from the beginning of the file.
*                           NET_FS_SEEK_ORIGIN_CUR      Offset is from current file position.
*                           NET_FS_SEEK_ORIGIN_END      Offset is from the end       of the file.
*
* Return(s)   : DEF_OK,   if file position set.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_FilePosSet (void        *p_file,
                                       CPU_INT32S   offset,
                                       CPU_INT08U   origin)
{
    NET_FS_FILE_HANDLE  *p_handle = DEF_NULL;
    CPU_BOOLEAN          ok       = DEF_OK;
    FS_FLAGS             fs_flags;
    RTOS_ERR             err;


    RTOS_ASSERT_DBG((p_file != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    p_handle = (NET_FS_FILE_HANDLE *)p_file;
    LOG_VRB(("NET-FS : Set File Position: ", (s)p_handle->Filename));
                                                                /* ------------------ SET FILE POS -------------------- */
    switch (origin) {
        case NET_FS_SEEK_ORIGIN_START:
             LOG_VRB(("NET-FS : origin = Start"));
             fs_flags = FS_FILE_ORIGIN_START;
             break;

        case NET_FS_SEEK_ORIGIN_CUR:
             LOG_VRB(("NET-FS : origin = Current"));
             fs_flags = FS_FILE_ORIGIN_CUR;
             break;

        case NET_FS_SEEK_ORIGIN_END:
             LOG_VRB(("NET-FS : origin = End"));
             fs_flags = FS_FILE_ORIGIN_END;
             break;

        default:
             return (DEF_FAIL);
    }



    FSFile_PosSet(p_handle->FileHandle,
                  offset,
                  fs_flags,
                 &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSFile_PosSet() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        ok = DEF_FAIL;
    }

    return (ok);
}


/*
*********************************************************************************************************
*                                        NetFS_FileSizeGet()
*
* Description : Get file size.
*
* Argument(s) : p_file  Pointer to a file.
*
*               p_size  Pointer to variable that will receive the file size.
*
* Return(s)   : DEF_OK,   if file size gotten.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_FileSizeGet (void        *p_file,
                                        CPU_INT32U  *p_size)
{
    NET_FS_FILE_HANDLE  *p_handle = DEF_NULL;
    FS_ENTRY_INFO        info;
    RTOS_ERR             err;


    RTOS_ASSERT_DBG((p_file != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_size != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);


   *p_size = 0u;                                                /* Init to dflt size for err (see Note #1).             */

    p_handle = (NET_FS_FILE_HANDLE *)p_file;
    LOG_VRB(("NET-FS : Get File Size: ", (s)p_handle->Filename));

                                                                /* ------------------ GET FILE SIZE ------------------- */
    FSFile_Query(p_handle->FileHandle,
                &info,
                &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSFile_Query() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        return (DEF_FAIL);
    }

    LOG_VRB(("NET-FS : File Size : ", (u)info.Size));
   *p_size = (CPU_INT32U)info.Size;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   NetFS_FileDateTimeCreateGet()
*
* Description : Get file creation date/time.
*
* Argument(s) : p_file  Pointer to a file.
*
*               p_time  Pointer to variable that will receive the date/time :
*
*                           Default/epoch date/time,        if any error(s);
*                           Current       date/time,        otherwise.
*
* Return(s)   : DEF_OK,   if file creation date/time gotten.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_FileDateTimeCreateGet (void              *p_file,
                                                  NET_FS_DATE_TIME  *p_time)
{
    NET_FS_FILE_HANDLE  *p_handle    = DEF_NULL;
    CLK_DATE_TIME        time;
    CLK_TS_SEC           time_ts_sec;
    CPU_BOOLEAN          rtn_code;
    FS_ENTRY_INFO        info;
    RTOS_ERR             err;


    RTOS_ASSERT_DBG((p_file != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);
    RTOS_ASSERT_DBG((p_time != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

                                                    /* Init to dflt date/time for err (see Note #1).        */
    p_time->Yr    = 0u;
    p_time->Month = 0u;
    p_time->Day   = 0u;
    p_time->Hr    = 0u;
    p_time->Min   = 0u;
    p_time->Sec   = 0u;

    p_handle = (NET_FS_FILE_HANDLE *)p_file;
    LOG_VRB(("NET-FS : Get File time created: ", (s)p_handle->Filename));

                                                                /* ---------- GET FILE CREATION DATE/TIME ------------- */
    FSFile_Query(p_handle->FileHandle,
                &info,
                &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSFile_Query() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        return (DEF_FAIL);
    }


    time_ts_sec = info.DateTimeCreate;
    rtn_code    = Clk_TS_ToDateTime(time_ts_sec, 0, &time);     /* Convert clk timestamp to date/time structure.        */
    if (rtn_code != DEF_OK) {
        LOG_VRB(("NET-FS : Clk_TS_ToDateTime() Error: converting time failed"));
        return (DEF_FAIL);
    }
                                                                /* Set each creation date/time field to be rtn'd.       */
    p_time->Yr    = time.Yr;
    p_time->Month = time.Month;
    p_time->Day   = time.Day;
    p_time->Hr    = time.Hr;
    p_time->Min   = time.Min;
    p_time->Sec   = time.Sec;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      NetFS_WorkingFolderGet()
*
* Description : Get current working folder.
*
* Argument(s) : p_path          Pointer to string that will receive the working folder
*
* Return(s)   : DEF_OK,   if p_path successfully copied.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_WorkingFolderGet (CPU_CHAR    *p_path,
                                             CPU_SIZE_T   path_len_max)
{
    RTOS_ERR  err;


    RTOS_ASSERT_DBG((p_path != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

    LOG_VRB(("NET-FS : Get current workspace: "));
    FSWrkDir_PathGet(FS_WRK_DIR_CUR,
                     p_path,
                     path_len_max,
                    &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSWrkDir_PathGet() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        return (DEF_FAIL);
    }

    LOG_VRB(("NET-FS : Current workspace = ", (s)p_path));

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      NetFS_WorkingFolderSet()
*
* Description : Set current working folder.
*
* Argument(s) : p_path  String that specifies EITHER...
*                           (a) the absolute working directory path to set;
*                           (b) a relative path that will be applied to the current working directory.
*
* Return(s)   : DEF_OK,   if file creation date/time gotten.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*********************************************************************************************************
*/

static  CPU_BOOLEAN  NetFS_WorkingFolderSet (CPU_CHAR  *p_path)
{
    RTOS_ERR  err;


    RTOS_ASSERT_DBG((p_path != DEF_NULL), RTOS_ERR_NULL_PTR, DEF_FAIL);

    LOG_VRB(("NET-FS : Set workspace : ", (s)p_path));

    FSWrkDir_TaskBind(FS_WRK_DIR_NULL, p_path, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        LOG_VRB(("NET-FS : FSWrkDir_TaskBind() Error: ", (s)RTOS_ERR_STR_GET(RTOS_ERR_CODE_GET(err))));
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL && RTOS_MODULE_FS_AVAIL */

