/*
 *
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

/**
 * @file
 *
 * Stub implementation of Push registry.
 *
 */

#include <stdio.h>
#include <push_server_export.h>
#include <push_server_resource_mgmt.h>
#include <midp_logging.h>


/**
 * Returns the number of buffered bytes for the given socket.
 *
 * @param fd handle of the socket
 * @return number of cached bytes of -1 if none
 */
int pushcacheddatasize(int fd) {
    (void)fd;
    REPORT_WARN(LC_PUSH, "pushcacheddatasize: Stubbed out.");
    return -1;
}
/**
 * Opens the pushregistry files and populate the push memory structures.
 *
 * @param <none>
 * @return <tt>0</tt> for success, non-zero if there is a resource problem
 *
 */
int pushopen() {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushopen: Stubbed out.");
    return 0;
}

/**
 * Destroys the push and alarm memory resources. Maintain the push
 * registrations in the push registry file.
 */
void pushclose() {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushclose: Stubbed out.");
}

/**
 * Adds one entry to the push registry.
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
                     const pcsl_string * filter) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "midpAddPushEntry: Stubbed out.");
    (void)suiteId;
    (void)connection;
    (void)midlet;
    (void)filter;
    return 0;
}

/**
 * Adds one entry to the push registry.
 * If the entry already exists return an error.
 * On succesful registration, write a new copy of the file to disk.
 *
 * @param str A push entry string.
 * @return <tt>0</tt> if successful, <tt>-1</tt> if the entry already
 *         exists, <tt>-2</tt> if out of memory
 */
int pushadd(char *str) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushadd: Stubbed out.");
    (void)str;
    return 0;
}

/**
 * Removes one entry from the push registry.
 * If the entry is not registered return an error.
 * On successful deletion, write a new copy of the file to disk.
 *
 * @param str The push entry string to be deleted
 * @param store The MIDletSuite storagename
 * @return <tt>0</tt> if successful, <tt>-2</tt> if the entry belongs
 *         to another suite.
 */
int pushdel(char *str, char *store) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushdel: Stubbed out.");
    (void)str;
    (void)store;
    return 0;
}

/**
 *  Fetch the datagram data into a buffer.
 *
 * @param fd The handle of the datagram port
 * @param ip The ip address of the incoming datagram
 * @param sndport The port from which the data was sent
 * @param buf A pointer to a buffer into which the data should be copied
 * @param len The size of buf
 * @return the length of the datagram data if successful, or <tt>-1</tt>
 *         unsuccessful.
 */
int pushgetcachedpacket(int fd, int *ip, int *sndport, char *buf, int len) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushgetcachedpacket : Stubbed out.");
    (void)fd;
    (void)ip;
    (void)sndport;
    (void)buf;
    (void)len;
    return -1;
}

/**
 * Checks out the handle for the requested server socket.
 * @param fd The handle to check out
 * @return the handle to the checked-out server socket
 */
int pushcheckoutaccept(int fd) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckoutaccept: Stubbed out.");
    (void)fd;
    return -1;
}

/**
 * Checks out the handle for the requested connection.
 * The CHECKED_OUT token
 * is left in the registry to indicate that an application is
 * actively using the connection.
 *
 * @param protocol The protocol of the connection
 * @param port The port number of the connection
 * @param store The storage name of the requesting Suite
 *
 * @return <tt>-1</tt> if the connection is not found, other wise returns
 * the previously opened file descriptor.
 */
int pushcheckout(char* protocol, int port, char* store) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckout: Stubbed out.");
    (void)protocol;
    (void)port;
    (void)store;
    return -1;
}

/**
 * Checks in the handle for the requested connection.
 * @param fd The handle to be checked in
 * @return <tt>0</tt> if successful, or <tt>-1</tt> on failure to check in the
 * file descriptor to the cached push registry.
 */
int pushcheckin(int fd) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckin: Stubbed out.");
    (void)fd;
    return -1;
}

/**
 * Checks in all the push connections. Used to cleanup by runMIDletWithArgs.
 */
void pushcheckinall() {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckinall: Stubbed out.");
}

/**
 * Checks in the push connection(s) left over from the previous suite
 * that were not opened. Used between VM starts by runMIDletWithArgs.
 * The method does not check in connections that may still be used
 * by the next MIDlet, but leaves them ready to check out.
 *
 * @param nextToRun suiteID of the next suite to run, any connection
 *     that is not checked in and does not belong to the next suite to
 *     to run will be checked in.
 */
void pushcheckinLeftOvers(SuiteIdType nextToRun) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckinLeftOvers: Stubbed out.");
    (void)nextToRun;
}

/**
 * Checks in connections that are in
 * launch pending state for a specific MIDlet.
 *
 * @param suiteId Suite ID of the MIDlet
 * @param pszClassName Class name of the MIDlet
 */
void pushcheckinbymidlet(SuiteIdType suiteId, char* pszClassName) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckinbymidlet: Stubbed out.");
    (void)suiteId;
    (void)pszClassName;
}

/**
 * Checks in the handle for the requested connection,
 * given the connection name.
 * @param str The connection name
 * @return <tt>0</tt> on success, or <tt>-1</tt> on failure to check in the
 * file descriptor to the cached push registry.
 */
int pushcheckinbyname(char* str) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushcheckinbyname: Stubbed out.");
    (void)str;
    return -1;
}

/**
 * Lookup the push entry, given a handle.
 * @param fd The handle to a push connection
 * @return The full text push entry from the push list
 */
char* pushfindfd(int fd) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushfindfd: Stubbed out.");
    (void)fd;
    return NULL;
}

/**
 * Lookup the push entry, given the connection URL.
 *
 * @param str the connection URL
 * @return the full-text push entry from the registry
 */
char* pushfindconn(char* str) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushfindconn: Stubbed out.");
    (void)str;
    return NULL;
}

/**
 * Given the connection string and port number, look up the
 * push entry and return its filter.
 *
 * @param conn the connection string
 * @param port the port number to match
 * @return the filter from the registry
 */
char* pushgetfilter(char* conn, int port) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushgetfilter: Stubbed out.");
    (void)conn;
    (void)port;
    return NULL;
}

/**
 * Given the connection string and application ID, look up the
 * push entry and return its filter.
 *
 * @param conn the connection string
 * @param appID The MMS application ID to match
 * @return the filter from the registry
 */
char* pushgetfiltermms(char *conn, char *appID) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushgetfiltermms: Stubbed out.");
    (void)conn;
    (void)appID;
    return NULL;
}

/**
 * To be called when a WMA SMS or CBS message has arrived and has been
 * cached in an inbox. For the given connection string and
 * port number, a push entry is looked up. If one is found,
 * isWMAMessCached flag is set to KNI_TRUE.
 *
 * @param conn the connection string
 * @param port the port number to match
 */
void pushsetcachedflag(char* conn, int port) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushsetcachedflag: Stubbed out.");
    (void)conn;
    (void)port;
}

/**
 * To be called when a WMA MMS message has arrived and has been
 * cached in an inbox. For the given connection string and
 * port number, a push entry is looked up. If one is found,
 * isWMAMessCached flag is set to KNI_TRUE.
 *
 * @param conn the connection string
 * @param appId The MMS application ID to match
 */
void pushsetcachedflagmms(char* conn, char* appID) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushsetcachedflagmms: Stubbed out.");
    (void)conn;
    (void)appID;
}

/**
 * Finds push entries that belong to the given suite. If the available
 * flag is set, return only those entries with live connections (socket)
 * or cached incoming data.
 *
 * @param store The storagename of the suite
 * @param available If set, return only "live" push entries
 *
 * @return A comma delimited list of full-text push entries
 */
char* pushfindsuite(char* store, int available) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushfindsuite: Stubbed out.");
    (void)store;
    (void)available;
    return NULL;
}

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
int findPushBlockedHandle(int handle) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "findPushBlockedHandle: Stubbed out.");
    (void)handle;
    return 0;
}

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
int findPushTimerBlockedHandle(int handle) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "findPushTimerBlockedHandle: Stubbed out.");
    (void)handle;
    return 0;
}

/**
 * Test for incoming events, and block current java thread if none have
 * occurred. The method also initializes any new timers that are needed,
 * and blocks the current Java thread until a push event will wake it up.
 *
 * @return a <tt>handle</tt> to a push entry if an event is pending, or
 *         <tt>-1</tt> if the currently running Java thread is to block.
 */
int pushpoll() {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushpoll: Stubbed out.");
    return -1;
}

/**
 * Deletes all of connections for a suite, using the suite ID.
 *
 * @param id The suite ID to be removed from the push registry
 */
void pushdeletesuite(SuiteIdType id) {
    /* need revisit */
    REPORT_WARN(LC_PUSH, "pushdeletesuite: Stubbed out.");
    (void)id;
}

/**
 * Adds one entry to the alarm registry.
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
int alarmadd(char* str, jlong alarm, jlong* lastalarm){
    /* need revisit */
    REPORT_WARN(LC_PUSH, "alarmadd: Stubbed out.");
    (void)str;
    (void)alarm;
    (void)lastalarm;
    return -2;
}
