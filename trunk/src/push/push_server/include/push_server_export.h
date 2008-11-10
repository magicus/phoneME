/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _PUSH_SERVER_EXPORT_H_
#define _PUSH_SERVER_EXPORT_H_


/**
 * @defgroup push_server Push Registry low-level implementation
 * @ingroup push
 */

/**
 * @file
 * @ingroup push_server
 *
 * @brief Push server functionality provided for Push Registry and AMS.
 *
 * ##include &lt;&gt;
 *
 * @{
 */

#include <kni.h>
#include <pcsl_string.h>
#include <suitestore_common.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Add one entry to the push registry.
 * If the entry already exists return an error.
 * On succesful registration, write a new copy of the file to disk.
 *
 * @param str A push entry string.
 * @return <tt>0</tt> if successful, <tt>-1</tt> if the entry already
 *         exists, <tt>-2</tt> if out of memory
 */
int pushadd(char* str);

/**
 * Remove one entry from the push registry.
 * If the entry is not registered return an error.
 * On successful deletion, write a new copy of the file to disk.
 *
 * @param str The push entry string to be deleted
 * @param store The MIDletSuite storagename
 * @return <tt>0</tt> if successful, <tt>-2</tt> if the entry belongs
 *         to another suite.
 */
int pushdel(char* str, char* store);

/**
 * Test for incoming events, and block current Java platform thread if
 * none have occurred. The method also initializes any new timers that
 * are needed, and sets up the reentryData struct so that the current
 * Java platform thread can be woken correctly.
 *
 * @return a <tt>handle</tt> to a push entry if an event is pending, or
 *         <tt>-1</tt> if the currently running Java platform thread
 *         is to block.
 */
int pushpoll();

/**
 * Find push entries that belong to the given suite. If the available
 * flag is set, return only those entries with live connections (socket)
 * or cached incoming data.
 *
 * @param store The storagename of the suite
 * @param available If set, return only "live" push entries
 *
 * @return A comma delimited list of full-text push entries
 */
char* pushfindsuite(char* store, int available);

/**
 * Lookup the push entry, given a handle.
 * @param fd The handle to a push connection
 * @return The full text push entry from the push list
 */
char* pushfindfd(int fd);

/**
 * Lookup the push entry, given the connection URL.
 *
 * @param str the connection URL
 * @return the full-text push entry from the registry
 */
char* pushfindconn(char* str);

#if !defined(ASSERT)
#    if 0
#        define ASSERT(x) assert((x))
#    else
#        define ASSERT(x) (void)0
#    endif
#endif

/**
 * Check in the handle for the requested connection,
 * given the connection name.
 * @param str The connection name
 * @return <tt>0</tt> on success, or <tt>-1</tt> on failure to check in the
 * file descriptor to the cached push registry.
 */
int pushcheckinbyname(char* str);

/**
 * Check in all the push connections. Used to cleanup by runMIDletWithArgs.
 *
 */
void pushcheckinall();

/**
 * Check in the push connection(s) left over from the previous suite
 * that were not opened. Used between VM starts by runMIDletWithArgs.
 * The method does not check in connections that may still be used
 * by the next MIDlet, but leaves them alive.
 *
 * @param nextToRun suiteId of the next suite to run
 */
void pushcheckinLeftOvers(SuiteIdType nextToRun);

/**
 * Check in connections that are in
 * launch-pending state for a specific MIDlet.
 *
 * @param suiteId Suite ID of the MIDlet
 * @param pszClassName Class name of the MIDlet
 *                  array
 */
void pushcheckinbymidlet(SuiteIdType suiteId, char* pszClassName);

/**
 * Open the push registry files and populate the push memory structures.
 *
 * @return <tt>0</tt> for success, non-zero if there is a resource problem
 *
 */
int pushopen();

/**
 * Destroy the push and alarm memory resources. Maintain the push
 * registrations in the push registry file.
 *
 */
void pushclose();

/**
 * Delete all of connections for a suite, using the suite ID.
 *
 * @param id The suite ID to be removed from the push registry
 */
void pushdeletesuite(SuiteIdType id);

/**
 * Add one entry to the alarm registry.
 * If the entry already exists, return previous alarm time.
 * On succesful registration, write a new copy of the file to disk.
 *
 * @param str The alarm entry to add
 * @param alarm The absolute time at which this alarm is fired
 * @param lastalarm The place to return the previous alarm time, if it exists
 *
 * @return <tt>0</tt> if successful, <tt>-2</tt> if there was an error
 *         allocating this alarm.
 */
int alarmadd(char* str, jlong alarm, jlong* lastalarm);

/**
 * Find blocking thread for a given socket push handle. Walks through the
 * registry of push entries for a handle that matches the argument. If one is
 * found, its push entry state is set to RECEIVED_EVENT and the handle is
 * returned.
 *
 * @param handle The handle to test for in the push registry
 * @return <tt>0</tt> if no entry is found. Otherwise, <tt>handle</tt> is
 *          returned
 */
int findPushBlockedHandle(int handle);

/**
 * Find blocking thread for a given timer push handle. Walks through the
 * registry of alarm entries for a handle that matches the argument. If
 * one is found, its entry state is set to RECEIVED_EVENT and the handle is
 * returned.
 *
 * @param handle The handle to test for in the alarm registry
 * @return <tt>0</tt> if no entry is found. Otherwise, <tt>handle</tt> is
 *          returned
 */
int findPushTimerBlockedHandle(int handle);

/**
 * Add one entry to the push registry.
 * If the entry already exists return IO_ERROR_LEN (midpString.h).
 *
 * @param suiteId ID of the suite
 * @param connection generic connection name (no spaces)
 * @param midlet class name of the MIDlet (no spaces)
 * @param filter filter string (no spaces)
 *
 * @return 0 for success, OUT_OF_MEM_LEN for out of memory,
 * IO_ERROR_LEN if already registered
 */
int midpAddPushEntry(SuiteIdType suiteId,
                     const pcsl_string * connection,
                     const pcsl_string * midlet,
                     const pcsl_string * filter);

/**
 * Wildcard comparing the pattern and the string.
 * @param pattern The pattern that can contain '*' and '?'
 * @param str The string for comparing
 * @return <tt>1</tt> if the comparison is successful, <tt>0</tt> if it fails
 */
int wildComp(const char *pattern, const char *str);

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _PUSH_SERVER_EXPORT_H_ */
