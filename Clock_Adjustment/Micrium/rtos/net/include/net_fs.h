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
*                                      NETWORK FILE SYSTEM PORT
*
* File : net_fs.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _NET_FS_H_
#define  _NET_FS_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/rtos_path.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_FS_PATH_SEP_CHAR                    ASCII_CHAR_SOLIDUS

#define  NET_FS_MAX_FILE_NAME_LEN                NET_FS_CFG_MAX_FILE_NAME_LEN
#define  NET_FS_MAX_PATH_NAME_LEN                NET_FS_CFG_MAX_PATH_NAME_LEN

#define  NET_FS_SEEK_ORIGIN_START                         1u    /* Origin is beginning of file.                         */
#define  NET_FS_SEEK_ORIGIN_CUR                           2u    /* Origin is current file position.                     */
#define  NET_FS_SEEK_ORIGIN_END                           3u    /* Origin is end of file.                               */

#define  NET_FS_ENTRY_ATTRIB_RD                  DEF_BIT_00     /* Entry is readable.                                   */
#define  NET_FS_ENTRY_ATTRIB_WR                  DEF_BIT_01     /* Entry is writeable.                                  */
#define  NET_FS_ENTRY_ATTRIB_HIDDEN              DEF_BIT_02     /* Entry is hidden from user-level processes.           */
#define  NET_FS_ENTRY_ATTRIB_DIR                 DEF_BIT_03     /* Entry is a directory.                                */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        DATE / TIME DATA TYPE
*********************************************************************************************************
*/

typedef  struct  net_fs_date_time {
    CPU_INT16U         Sec;                                     /* Seconds [0..60].                                     */
    CPU_INT16U         Min;                                     /* Minutes [0..59].                                     */
    CPU_INT16U         Hr;                                      /* Hour [0..23].                                        */
    CPU_INT16U         Day;                                     /* Day of month [1..31].                                */
    CPU_INT16U         Month;                                   /* Month of year [1..12].                               */
    CPU_INT16U         Yr;                                      /* Year [0, ..., 2000, 2001, ...].                      */
} NET_FS_DATE_TIME;


/*
*********************************************************************************************************
*                                           ENTRY DATA TYPE
*********************************************************************************************************
*/

typedef  struct  net_fs_entry {
    CPU_INT16U         Attrib;                                  /* Entry attributes.                                    */
    CPU_INT32U         Size;                                    /* File size in octets.                                 */
    NET_FS_DATE_TIME   DateTimeCreate;                          /* Date/time of creation.                               */
    CPU_CHAR          *NamePtr;                                 /* Name.                                                */
    CPU_INT32U         NameSize;
} NET_FS_ENTRY;


/*
*********************************************************************************************************
*                                              FILE MODES
*********************************************************************************************************
*/

typedef  enum  net_fs_file_mode {                               /* File modes. See NetFS_FileOpen() note #1.            */
    NET_FS_FILE_MODE_NONE,
    NET_FS_FILE_MODE_APPEND,
    NET_FS_FILE_MODE_CREATE,
    NET_FS_FILE_MODE_CREATE_NEW,
    NET_FS_FILE_MODE_OPEN,
    NET_FS_FILE_MODE_TRUNCATE
} NET_FS_FILE_MODE;


/*
*********************************************************************************************************
*                                              FILE ACCESS
*********************************************************************************************************
*/

typedef  enum  net_fs_file_access {                            /* File access. See NetFS_FileOpen() note #2.            */
    NET_FS_FILE_ACCESS_RD,
    NET_FS_FILE_ACCESS_RD_WR,
    NET_FS_FILE_ACCESS_WR
} NET_FS_FILE_ACCESS;


/*
*********************************************************************************************************
*                                               FS API
*********************************************************************************************************
*/

                                                                        /* ---------------- NET FS API ---------------- */
                                                                        /* Net FS API fnct ptrs :                       */
typedef  struct  net_fs_api {
                                                                        /* ------- GENERIC  NET FS API MEMBERS -------- */
    CPU_CHAR            (*CfgPathGetSepChar) (void);                    /*   Get path separator character.              */

                                                                        /*   Open file                                  */
    void               *(*Open)              (CPU_CHAR            *p_name,
                                              NET_FS_FILE_MODE     mode,
                                              NET_FS_FILE_ACCESS   access);

                                                                        /*   Close file.                                */
    void                (*Close)             (void                *p_file);

                                                                        /*   Read file.                                 */
    CPU_BOOLEAN         (*Rd)                (void                *p_file,
                                              void                *p_dest,
                                              CPU_SIZE_T           size,
                                              CPU_SIZE_T          *p_size_rd);

                                                                        /*   Write file.                                */
    CPU_BOOLEAN         (*Wr)                (void                *p_file,
                                              void                *p_src,
                                              CPU_SIZE_T           size,
                                              CPU_SIZE_T          *p_size_wr);

                                                                        /*   Set file position.                         */
    CPU_BOOLEAN         (*SetPos)            (void                *p_file,
                                              CPU_INT32S           offset,
                                              CPU_INT08U           origin);

                                                                        /*   Get file size.                             */
    CPU_BOOLEAN         (*GetSize)           (void                *p_file,
                                              CPU_INT32U          *p_size);

                                                                        /*   Open directory.                            */
    void               *(*DirOpen)           (CPU_CHAR            *p_name);

                                                                       /*   Close directory.                           */
    void                (*DirClose)          (void                *p_dir);

                                                                        /*   Read directory.                            */
    CPU_BOOLEAN         (*DirRd)             (void                *p_dir,
                                              NET_FS_ENTRY        *p_entry);

                                                                        /*   Entry create.                              */
    CPU_BOOLEAN         (*EntryCreate)       (CPU_CHAR            *p_name,
                                              CPU_BOOLEAN          dir);

                                                                        /*   Entry delete.                              */
    CPU_BOOLEAN         (*EntryDel)          (CPU_CHAR            *p_name,
                                              CPU_BOOLEAN          file);

                                                                        /*   Entry rename.                              */
    CPU_BOOLEAN         (*EntryRename)       (CPU_CHAR            *p_name_old,
                                              CPU_CHAR            *p_name_new);

                                                                        /*   Entry time set.                            */
    CPU_BOOLEAN         (*EntryTimeSet)      (CPU_CHAR            *p_name,
                                              NET_FS_DATE_TIME    *p_time);

                                                                        /*   Create a date time.                        */
    CPU_BOOLEAN         (*DateTimeCreate)    (void                *p_file,
                                              NET_FS_DATE_TIME    *p_time);

                                                                        /*   Get working folder.                        */
    CPU_BOOLEAN         (*WorkingFolderGet)  (CPU_CHAR            *p_path,
                                              CPU_SIZE_T           path_len_max);

                                                                        /*   Set working folder.                        */
    CPU_BOOLEAN         (*WorkingFolderSet)  (CPU_CHAR            *p_path);
} NET_FS_API;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  RTOS_MODULE_FS_AVAIL
extern  const  NET_FS_API  NetFS_API_Native;
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _NET_FS_H_ */
