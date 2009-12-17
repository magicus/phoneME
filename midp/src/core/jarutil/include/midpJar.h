/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

#ifndef _MIDP_JAR_H_
#define _MIDP_JAR_H_

/**
 * @file
 * @ingroup jar
 * @brief Java Archive (JAR) File Extraction Porting Interface.
 */

#include <midpString.h>
#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Jar reader error codes
 * @{
 */
/** Out of memory error: cannot allocate a memory area of requested size */
#define MIDP_JAR_OUT_OF_MEM_ERROR   -2
/** I/O error, for example, cannot open, the size return is negative */
#define MIDP_JAR_IO_ERROR      -3
/** The JAR file is corrupt (for example, mandatory JAR entries are missing) */
#define MIDP_JAR_CORRUPT_ERROR -4
/** @} */

/**
 * Open a JAR file for use.
 *
 * @param pError where to put the error, 0 for success
 * @param name absolute filename of the JAR file
 *
 * @return handle to a JAR file or NULL if an error with pError containing a status
 */
void* midpOpenJar(int* pError, const pcsl_string * name);

/**
 * Close a JAR file previously opened by midpOpenJar.
 *
 * @param handle handle structure previously returned by midpOpenJar,
 *               can be NULL
 */
void midpCloseJar(void* handle);

/**
 * Get the size of JAR file previously opened by midpOpenJar.
 *
 * @param handle handle structure previously returned by midpOpenJar
 *
 * @return size of JAR file or -1 if and ppEntry null if not found, less
 */
long midpGetJarSize(void* handle);

/**
 * Get a the named entry from a JAR file previously opened by
 * midpOpenJar.
 *
 * @param handle handle structure previously returned by midpOpenJar
 * @param name name of the entry to get
 * @param ppEntry where to put the entry, free data with midpFree
 *
 * @return size of data if found, 0 and ppEntry null if not found,
 *         less than zero and ppEntry null if an error
 */
long midpGetJarEntry(void* handle, const pcsl_string * name,
                     unsigned char** ppEntry);

/**
 * Check to see if the named entry exists in a JAR file previously
 * opened by midpOpenJar.
 *
 * @param handle handle structure previously returned by midpOpenJar
 * @param name name of the entry to find
 *
 * @return positive if found, 0 if not found, negative if an error
 */
int midpJarEntryExists(void* handle, const pcsl_string * name);

/**
 * The filter function checks if the name matches and returns true if yes.
 */
typedef jboolean filterFuncT(const pcsl_string * name);

/**
 * The action function performs an action on the named entry and returns true 
 * if the action was successful.
 */
typedef jboolean actionFuncT(const pcsl_string * name);

/**
 * Iterates over all entries in a JAR file previously opened 
 * by midpOpenJar. For each entry the filter function is called. If the
 * filter returns true, then the action function is called. If the
 * action function returns false then the function exits.
 *
 * @param handle   handle structure previously returned by midpOpenJar
 * @param filter   filter function
 * @param action   action function
 *
 * @return         1 if completed normally, 0 if action failed, negative 
 *                 if an error
 */
int midpIterateJarEntries(void *handle, filterFuncT *filter, 
                          actionFuncT *action);

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _MIDP_JAR_H_ */
