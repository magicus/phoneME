/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/**
 * @file javacall_storage.h
 * @ingroup CHAPI
 * @brief Storage abstraction API
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *  These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

#ifndef __JAVAUTIL_STORAGE_H___
#define __JAVAUTIL_STORAGE_H___

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

// opens on read, storage must exist
#define JUS_O_RDONLY 0

// opens on read/wrie, storage must exist (unless JUS_O_CREATE is set)
#define JUS_O_RDWR 1

// opens on append (all writes allowed only at the end of file) storage must exist (unless JUS_O_CREATE is set)
#define JUS_O_APPEND 2

// additional flag, used along with JUS_O_RDWR and JUS_O_APPEND, file created if it does not exist
#define JUS_O_CREATE 4


typedef void* javautil_storage;

#define JAVAUTIL_INVALID_STORAGE_HANDLE ((void*)-1L)


#define JUS_SEEK_CUR    1
#define JUS_SEEK_END    2
#define JUS_SEEK_SET    0

#define MAX_STORAGE_NAME 256


/* opens or creates new storage, see JUS_* flags for info */
javacall_result javautil_storage_open(const char* name, int flag, /* OUT */ javautil_storage* storage);

/* returns storage name */
javacall_result javautil_storage_gettmpname(char* name, int* len);

/* closes opened storage*/
javacall_result javautil_storage_close(javautil_storage storage);

/* removes closed storage by name */
javacall_result javautil_storage_remove(const char* name);

/* renames closed storage */
javacall_result javautil_storage_rename(const char *oldname, const char *newname);

/* returns current storage size */
javacall_result javautil_storage_getsize(javautil_storage storage,/* OUT */ long* size);

/* set current storage read/write position */
javacall_result javautil_storage_setpos(javautil_storage storage, long pos, int flag);

/*  get current storage read/write position */
javacall_result javautil_storage_getpos(javautil_storage storage, /* OUT */long* pos);

/* returns number of bytes read or -1 in case of error */
int javautil_storage_read(javautil_storage storage, void* buffer, unsigned int size);

/* returns number of bytes written or -1 in case of error */
int javautil_storage_write(javautil_storage storage, void* buffer, unsigned int size);

/* flushes write buffer to phisical storage */
javacall_result  javautil_storage_flush(javautil_storage storage);

/* locks storage for shared access */
javacall_result  javautil_storage_lock(javautil_storage storage);

/* unlocks locked storage */
javacall_result  javautil_storage_unlock(javautil_storage storage);

/* get modification time */
javacall_result  javautil_storage_get_modified_time(const char* fname, unsigned long* modified_time);


#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif //__JAVAUTIL_STORAGE_H___
