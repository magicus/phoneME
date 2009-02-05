/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * Implementation of Java native methods for the <tt>PushRegistryImpl</tt>
 * class.
 */

#include <string.h>

#include <kni.h>
#include <sni.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>

#include <nativepush_port_export.h>


/**
 * Unregister push from platform's pushDB.
 * 
 * <p>
 * Java declaration:
 * <pre>
 *     del0([BI)I
 * </pre>
 * 
 * @param connections  Connection to unregister
 * @param suiteID      suite ID the conenction belongs to
 * @return  MIDP_ERROR_NONE if there was no error,
 *          JAVACALL_PUSH_ILLEGAL_ARGUMENT if there is no
 *          registered connection,
 *          MIDP_ERROR_AMS_SUITE_NOT_FOUND if suiteID was not
 *          found, MIDP_ERROR_PERMISSION_DENIED if registered
 *           connection belongs to different suite
 * 
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_PushRegistryImpl_del0) {
    MIDP_ERROR ret;
    int suite_id = KNI_GetParameterAsInt(2);

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, conn)    /* the connection string. */
    /* Perform the delete operation. */
    ret = midpport_push_unregister_connection(suite_id, &conn);
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    
    KNI_ReturnInt(ret);
}

/**
 * Register connection within native DB
 * 
 * @param suiteID   suite ID the connection belongs to
 * @param connectin connection string
 * @param midlet    class name of the <code>MIDlet</code> to be launched,
 *                  when new external data is available
 * @param filter    connection URL string indicating which
 *                  senders are allowed to cause the MIDlet to be
 *                  launched
 * 
 * @return int      MIDP_ERROR_NONE if there is no error,
 *         MIDP_ERROR_ILLEGAL_ARGUMENT connection or filter
 *         string is invalid, MIDP_ERROR_UNSUPPORTED if the
 *         runtime system does not support push delivery for the
 *         requested connection protocol,
 *         MIDP_ERROR_AMS_MIDLET_NOT_FOUND  if class name was
 *         not found, MIDP_ERROR_AMS_SUITE_NOT_FOUND if suite ID
 *         was not found, MIDP_ERROR_PUSH_CONNECTION_IN_USE  if
 *         the connection is registered already
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_PushRegistryImpl_add0) {
    int suite_id = KNI_GetParameterAsInt(1);
    MIDP_ERROR ret;
    MIDP_PUSH_ENTRY entry;

    KNI_StartHandles(3);

    GET_PARAMETER_AS_PCSL_STRING(2, connection)
    GET_PARAMETER_AS_PCSL_STRING(3, midlet)
    GET_PARAMETER_AS_PCSL_STRING(4, filter)

    /* it is safe to use pcsl_string_get_utf16_data - no memory allocation preformed */
    entry.connection = (jchar*)pcsl_string_get_utf16_data(&connection);
    entry.connectionLen = pcsl_string_length(&connection);
    entry.midlet = (jchar*)pcsl_string_get_utf16_data(&midlet);
    entry.midletLen = pcsl_string_length(&midlet);
    entry.filter = (jchar*)pcsl_string_get_utf16_data(&filter);
    entry.filterLen = pcsl_string_length(&filter);

    ret = midpport_push_register_connection(suite_id, &entry);

    pcsl_string_release_utf16_data(entry.connection, &connection);
    pcsl_string_release_utf16_data(entry.midlet, &midlet);
    pcsl_string_release_utf16_data(entry.filter, &filter);

    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnInt(ret);
}

/**
 * Adds an entry to the alarm registry.
 * <p>
 * Java declaration:
 * <pre>
 *     addalarm0(I[BJ)J
 * </pre>
 *
 * @param suiteId <code>MIDlet</code> suite to register alarm for
 * @param midlet  class name of the <code>MIDlet</code> within the
 *                current running <code>MIDlet</code> suite
 *                to be launched,
 *                when the alarm time has been reached
 * @param time time at which the <code>MIDlet</code> is to be executed
 *        in the format returned by <code>Date.getTime()</code>
 * @return <tt>0</tt> if this is the first alarm registered with
 *         the given <tt>midlet</tt>, otherwise the time of the
 *         previosly registered alarm.
 */
KNIEXPORT KNI_RETURNTYPE_LONG
KNIDECL(com_sun_midp_io_j2me_push_PushRegistryImpl_addAlarm0) {
    jint suite_id = KNI_GetParameterAsInt(1);
    jlong time = KNI_GetParameterAsLong(3);
    jlong prev_time;
    MIDP_ERROR ret;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(2, midlet)

    ret = midpport_push_register_alarm(suite_id, &midlet, time, &prev_time);

    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnLong(prev_time);
}

/**
 * Gets all registered push connections associated with the given MIDlet
 * suite.
 * 
 * @param id        suite ID
 * @param available if <code>true</code>, only return the list
 *      of connections with input available
 * 
 * @return array of connection strings
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_io_j2me_push_PushRegistryImpl_list0) {
    jint suite_id = KNI_GetParameterAsInt(1);
    jboolean available = KNI_GetParameterAsInt(2);
    MIDP_PUSH_ENTRY* entries;
    jint len;

    MIDP_ERROR ret = midpport_push_list_entries(suite_id, available,
                                                &entries, &len);
    KNI_StartHandles(2);
    KNI_DeclareHandle(strArr);
    KNI_DeclareHandle(tmp);

    if (MIDP_ERROR_NONE == ret && 0 != len && NULL != entries) {
        SNI_NewArray(SNI_STRING_ARRAY, len, strArr);
        if (KNI_IsNullHandle(strArr)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            int i;
            for (i = 0; i < len; i++) {
                KNI_NewString(entries[i].connection, entries[i].connectionLen, 
                              tmp);
                if (KNI_IsNullHandle(tmp)) {
                    KNI_ThrowNew(midpOutOfMemoryError, NULL);
                    /* JVM will take care about created String[] */
                    break;
                } else {
                    KNI_SetObjectArrayElement(strArr, i, tmp);
                }
            }
        }
        /* release allocated MIDP_PUSH_ENTRY array */
        while (len--) {
            midpFree(entries[len].connection);
            midpFree(entries[len].midlet);
            midpFree(entries[len].filter);
        }
        midpFree(entries);
    }

    KNI_EndHandlesAndReturnObject(strArr);
}

/**
 * Looks for push entry with given suite ID and conenction
 * string
 * 
 * @param suite_id     suite to fetch  
 * @param connection   generic connection string
 * @param pentry       pointer to store entry data
 * 
 * @return jboolean KNI_TRUE if connection was found, KNI_FALSE
 *         otherwise
 */
static jboolean _get_entry(jint suite_id, pcsl_string* connection, 
                           MIDP_PUSH_ENTRY* pentry) {
    MIDP_ERROR ret;
    MIDP_PUSH_ENTRY* entries;
    jint len;
    jchar* buf;
    jint buf_len;
    jboolean found = KNI_FALSE;

    buf = (jchar*)pcsl_string_get_utf16_data(connection);
    buf_len = pcsl_string_length(connection);

    ret = midpport_push_list_entries(suite_id, KNI_FALSE,
                                     &entries, &len);
    if (MIDP_ERROR_NONE == ret && 0 != len) {
        while (len--) {
            if (KNI_FALSE == found && buf_len == entries[len].connectionLen) {
                jint i = buf_len;
                while (i--) {
                    if (buf[i] != entries[len].connection[i]) {
                        break;
                    }
                }
                if (!i) {
                    found = KNI_TRUE;
                    /* copy data */
                    *pentry = entries[len];
                    /* don't release this entry data */
                    continue;
                }
            }
            /* delete processed record */
            midpFree(entries[len].connection);
            midpFree(entries[len].midlet);
            midpFree(entries[len].filter);
        }
        midpFree(entries);
    }

    return found;
}

/**
 * Retrieve the registered <code>MIDlet</code> for a requested connection.
 *
 * @param suiteID suite to fetch class name for
 * @param connection generic connection <em>protocol</em>, <em>host</em>
 *              and <em>port number</em>
 *              (optional parameters may be included
 *              separated with semi-colons (;))
 * @return  class name of the <code>MIDlet</code> to be launched,
 *              when new external data is available, or
 *              <code>null</code> if the connection was not
 *              registered
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_io_j2me_push_PushRegistryImpl_getMidlet0) {
    jint suite_id = KNI_GetParameterAsInt(1);
    MIDP_PUSH_ENTRY entry;

    KNI_StartHandles(2);
    KNI_DeclareHandle(result);
    GET_PARAMETER_AS_PCSL_STRING(2, connection)
    if (_get_entry(suite_id, &connection, &entry)) {
        /* found, create jstring */
        KNI_NewString(entry.midlet, 
                      entry.midletLen,
                      result);
        if (KNI_IsNullHandle(result)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
        /* release the rest of the data */
        midpFree(entry.connection);
        midpFree(entry.midlet);
        midpFree(entry.filter);
    }
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandlesAndReturnObject(result);
}

/**
 * Retrieve the registered filter for a requested connection.
 *
 * @param suiteID suite to fetch filter for
 * @param connection generic connection <em>protocol</em>, <em>host</em>
 *              and <em>port number</em>
 *              (optional parameters may be included
 *              separated with semi-colons (;))
 * @return a filter string indicating which senders
 *              are allowed to cause the MIDlet to be launched or
 *              <code>null</code> if the connection was not
 *              registered
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_io_j2me_push_PushRegistryImpl_getFilter0) {
    jint suite_id = KNI_GetParameterAsInt(1);
    MIDP_PUSH_ENTRY entry;

    KNI_StartHandles(2);
    KNI_DeclareHandle(result);
    GET_PARAMETER_AS_PCSL_STRING(2, connection)
    if (_get_entry(suite_id, &connection, &entry)) {
        /* found, create jstring */
        KNI_NewString(entry.filter, 
                      entry.filterLen,
                      result);
        if (KNI_IsNullHandle(result)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
        /* release the rest of the data */
        midpFree(entry.connection);
        midpFree(entry.midlet);
        midpFree(entry.filter);
    }
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandlesAndReturnObject(result);
}
