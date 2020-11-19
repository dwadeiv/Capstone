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
*                                          NETWORK DICTIONARY
*
* File : net_dict.c
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
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "net_dict_priv.h"
#include  <rtos/common/include/lib_str.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           NetDict_KeyGet()
*
* Description : Get the dictionary key associated with a string entry by comparing given string with
*               dictionary string entries.
*
* Argument(s) : p_dict_tbl    Pointer to the dictionary table.
*
*               dict_size     Size of the given dictionary table in octet.
*
*               p_str_cmp     Pointer to the string use for comparison.
*
*               str_len       String's length.
*
* Return(s)   : Key,                if string found within dictionary.
*               Invalid key number, otherwise.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  NetDict_KeyGet (const  NET_DICT     *p_dict_tbl,
                                   CPU_INT32U    dict_size,
                            const  CPU_CHAR     *p_str_cmp,
                                   CPU_BOOLEAN   case_sensitive,
                                   CPU_INT32U    str_len)
{
    CPU_INT32U   nbr_entry;
    CPU_INT32U   ix;
    CPU_INT32U   len;
    CPU_INT16S   cmp;
    NET_DICT    *p_srch;


    nbr_entry =  dict_size / sizeof(NET_DICT);
    p_srch    = (NET_DICT *)p_dict_tbl;
    for (ix = 0; ix < nbr_entry; ix++) {
        len = DEF_MIN(str_len, p_srch->StrLen);
        if (case_sensitive == DEF_YES) {
            cmp = Str_Cmp_N(p_str_cmp, p_srch->StrPtr, len);
            if (cmp == 0) {
                return (p_srch->Key);
            }
        } else {
            cmp = Str_CmpIgnoreCase_N(p_str_cmp, p_srch->StrPtr, len);
            if (cmp == 0) {
                return (p_srch->Key);
            }
        }

        p_srch++;
    }

    return (DICT_KEY_INVALID);
}


/*
*********************************************************************************************************
*                                          NetDict_EntryGet()
*
* Description : Get an dictionary entry object from a dictionary key entry.
*
* Argument(s) : p_dict_tbl    Pointer to the dictionary table.
*
*               dict_size     Size of the given dictionary table in octet.
*
*               key           Dictionary key entry.
*
* Return(s)   : Pointer to the dictionary entry object.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

NET_DICT  *NetDict_EntryGet (const  NET_DICT    *p_dict_tbl,
                                    CPU_INT32U   dict_size,
                                    CPU_INT32U   key)
{
    NET_DICT    *p_entry;
    CPU_INT32U   nbr_entry;
    CPU_INT32U   ix;


    nbr_entry =  dict_size / sizeof(NET_DICT);
    p_entry   = (NET_DICT *)p_dict_tbl;
    for (ix = 0; ix < nbr_entry; ix++) {                        /* Srch until last entry is reached.                    */
        if (p_entry->Key == key) {                              /* If keys match ...                                    */
            return (p_entry);                                   /* ... the first entry is found.                        */
        }

        p_entry++;                                              /* Move to next entry.                                  */
    }


    return (DEF_NULL);                                          /* No entry found.                                      */
}


/*
*********************************************************************************************************
*                                           NetDict_ValCopy()
*
* Description : Copy dictionary entry string to destination string buffer, up to a maximum number of characters.
*
* Argument(s) : p_dict        Pointer to the dictionary table.
*
*               dict_size     Size of the given dictionary table in octet.
*
*               key           Dictionary key entry.
*
*               p_buf         Pointer to destination string buffer where string will be copied.
*
*               buf_len       Maximum number of characters to copy.
*
* Return(s)   : Pointer to the end of the value copied, if value successfully copied.
*               Pointer to NULL,                        otherwise.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_CHAR  *NetDict_ValCopy (const  NET_DICT    *p_dict_tbl,
                                   CPU_INT32U   dict_size,
                                   CPU_INT32U   key,
                                   CPU_CHAR    *p_buf,
                                   CPU_SIZE_T   buf_len)
{
    const  NET_DICT  *p_entry;
           CPU_CHAR  *p_str_rtn;


    p_entry = &p_dict_tbl[key];
    if (p_entry->Key != key) {                                  /* If entry key doesn't match dictionary key ...        */
        p_entry = NetDict_EntryGet(p_dict_tbl,                     /* ... get first entry that match the    key.           */
                                dict_size,
                                key);
        if (p_entry == DEF_NULL) {
            return ((CPU_CHAR *)0);
        }
    }


    if ((p_entry->StrLen == 0u)       ||                        /* Validate entry value.                                */
        (p_entry->StrPtr == DEF_NULL)) {
         return ((CPU_CHAR *)0);
    }

    if (p_entry->StrLen > buf_len) {                            /* Validate value len and buf len.                      */
        return ((CPU_CHAR *)0);
    }


   (void)Str_Copy_N(p_buf, p_entry->StrPtr, p_entry->StrLen);   /* Copy string to the buffer.                           */

    p_str_rtn = p_buf + p_entry->StrLen;                        /* Set ptr to return.                                   */

    return (p_str_rtn);
}


/*
*********************************************************************************************************
*                                         NetDict_StrKeySrch()
*
* Description : (1) Search string for first occurrence of a specific dictionary key, up to a maximum number
*                   of characters:
*
*                   (a) Validate pointers
*                   (b) Set dictionary entry
*                   (c) Search for string
*
*
* Argument(s) : p_dict_tbl    Pointer to the dictionary table.
*
*               dict_size     Size of the given dictionary table in octet.
*
*               key           Dictionary key entry.
*
*               p_str         Pointer to string to found in entry.
*
*               str_len       Maximum number of characters to search.
*
* Return(s)   : Pointer to first occurrence of search string key, if any.
*               Pointer to NULL,                                  otherwise.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_CHAR  *NetDict_StrKeySrch (const  NET_DICT    *p_dict_tbl,
                                      CPU_INT32U   dict_size,
                                      CPU_INT32U   key,
                               const  CPU_CHAR    *p_str,
                                      CPU_SIZE_T   str_len)
{
    const  NET_DICT   *p_entry;
           CPU_CHAR   *p_found;

                                                                /* ------------------ VALIDATE PTRS ------------------- */
    if ((p_str      == DEF_NULL) ||
        (p_dict_tbl == DEF_NULL)) {
         return ((CPU_CHAR *)0);
    }


                                                                /* --------------- SET DICTIONARY ENTRY --------------- */
    p_entry = &p_dict_tbl[key];
    if (p_entry->Key != key) {                                  /* If entry key doesn't match dictionary key ...        */
        p_entry  = NetDict_EntryGet(p_dict_tbl,                    /* ... get first entry that match the    key.           */
                                 dict_size,
                                 key);
        if (p_entry == DEF_NULL) {
            return (DEF_NULL);
        }
    }

                                                                /* ----------------- SRCH FOR STRING ------------------ */
    p_found = Str_Str_N((const  CPU_CHAR *)p_str,
                        (const  CPU_CHAR *)p_entry->StrPtr,
                                           str_len);


    return (p_found);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_NET_AVAIL */
