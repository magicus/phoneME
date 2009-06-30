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
 * Implementation of Java native methods for the <tt>ConnectionRegistry</tt>
 * class.
 */

#include <string.h>

#include <kni.h>
#include <sni.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>

#include <push_server_export.h>
#include <push_server_resource_mgmt.h>
#include <midpError.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>

/**
 * Deletes an entry from the push registry.
 * <p>
 * Java declaration:
 * <pre>
 *     del0([B[B)I
 * </pre>
 *
 * @param connection The connection to remove from the push registry
 * @param storage The storage name of the current MIDlet suite
 *
 * @return <tt>0</tt> if the connection was successfully deleted.
 *         <tt>-1</tt> if the connection was not found. <tt>-2</tt>
 *         if connection was found, but, it belongs to another
 *         MIDlet suite.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_del0) {
    int ret = -1;

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(1, conn)    /* the connection string. */
    GET_PARAMETER_AS_PCSL_STRING(2, storage) /* the storage name string. */
    const jbyte * const conn_bytes = pcsl_string_get_utf8_data(&conn);
    const jbyte * const storage_bytes = pcsl_string_get_utf8_data(&storage);
        if (conn_bytes != NULL && storage_bytes != NULL) {
            /* Perform the delete operation. */
            ret = pushdel((char*)conn_bytes, (char*)storage_bytes);
        } else {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    pcsl_string_release_utf8_data(conn_bytes, &conn);
    pcsl_string_release_utf8_data(storage_bytes, &storage);
    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    KNI_ReturnInt(ret);
}

/**
 * Checks in a connection to the push registry. This effectively takes
 * control of the connection from the MIDlet and gives it to the push
 * registry sub-system.
 * <p>
 * Java declaration:
 * <pre>
 *     checkInByName0([B)I
 * </pre>
 *
 * @param connection The connection to check in to the push registry
 *
 * @return <tt>0</tt> upon successfully checking in the connection,
 *         otherwise <tt>-1</tt> if connection was not found
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_checkInByName0) {
    char *szConn = NULL;
    int connLen;
    int ret = -1;

    KNI_StartHandles(1);
    KNI_DeclareHandle(conn);

    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);

    if (connLen > 0 && (szConn = midpMalloc(connLen)) != NULL) {
        KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
        ret = pushcheckinbyname(szConn);
        midpFree(szConn);
    }
    else {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnInt(ret);
}

/**
 * Checks in a connection to the push registry. This effectively takes
 * control of the connection from the MIDlet and gives it to the push
 * registry sub-system.
 * <p>
 * Java declaration:
 * <pre>
 *     checkInByHandle0(I)
 * </pre>
 *
 * @param handle native handle of the connection
 * to check in to the push registry
 */
KNIEXPORT void
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_checkInByHandle0) {
    jint handle;

    handle = KNI_GetParameterAsInt(1);

    /* We must accept any messages or connections so they can be discarded. */
    pushfindfd((int)handle);
    pushcheckin((int)handle);

    KNI_ReturnVoid();
}

/**
 * Native connection registry method to check in connections that are in
 * launch pending state for a specific MIDlet.
 *
 * @param suiteId Suite ID of the MIDlet
 * @param className Class name of the MIDlet as zero terminated ASCII byte
 *                  array
 */
KNIEXPORT void
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_checkInByMidlet0) {
    SuiteIdType suiteId;

    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_PCSL_STRING(2,className)

        const jbyte * const className_bytes
            = pcsl_string_get_utf8_data(&className);
        if (NULL == className_bytes) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            pushcheckinbymidlet(suiteId, (char*)className_bytes);
            pcsl_string_release_utf8_data(className_bytes, &className);
        }

    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Adds a connection to the push registry.
 * <p>
 * Java declaration:
 * <pre>
 *     add0([B)I
 * </pre>
 *
 * @param connection The connection to add to the push registry
 *
 * @return <tt>0</tt> upon successfully adding the connection, otherwise
 *         <tt>-1</tt> if connection already exists
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_add0) {

    int ret = -1;

    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(1, conn)
        const jbyte * const conn_bytes = pcsl_string_get_utf8_data(&conn);

        if (conn_bytes != NULL) {
            ret = pushadd((char*)conn_bytes);
        }

        if ((conn_bytes == NULL) || (ret == -2)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
        pcsl_string_release_utf8_data(conn_bytes, &conn);
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnInt(ret);
}

/**
 * Adds an entry to the alarm registry.
 * <p>
 * Java declaration:
 * <pre>
 *     addalarm0([BJ)J
 * </pre>
 *
 * @param midlet The entry to add to the alarm registry
 * @param time The time the alarm will be go off
 *
 * @return <tt>0</tt> if this is the first alarm registered with
 *         the given <tt>midlet</tt>, otherwise the time of the
 *         previosly registered alarm.
 */
KNIEXPORT KNI_RETURNTYPE_LONG
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_addAlarm0) {

    char *szConn = NULL;
    int connLen;
    jlong alarm = 0;
    jlong lastalarm = 0;
    int ret = 0;

    alarm = KNI_GetParameterAsLong(2);

    KNI_StartHandles(1);
    KNI_DeclareHandle(conn);

    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);

    if (connLen > 0 && (szConn = midpMalloc(connLen)) != NULL) {
        KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);
        ret = alarmadd(szConn, alarm, &lastalarm);
        midpFree(szConn);
    }

    if ((szConn == NULL) || (ret == -2)) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnLong(lastalarm);
}

/**
 * Gets all registered push connections associated with the given MIDlet
 * suite.
 * <p>
 * Java declaration:
 * <pre>
 *     list0([BZ[BI)I
 * </pre>
 *
 * @param midlet The MIDlet suite to get push registry entries
 * @param available Which connections to return. If <tt>true</tt>,
 *                  only return the connections with available input.
 *                  Otherwise, return all connection.
 * @param connectionlist A comma separated string of connections stored
 *                       as a byte array.
 * @param listsz The maximum length of the byte array that will be
 *               accepted for <tt>connectionlist</tt>
 *
 * @return <tt>0</tt> if successful, otherwise <tt>-1</tt>
 *
 * @throw IllegalArgumentException If the length <tt>midlet</tt> is
 *                                 too long.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_list0) {
    char *midletName = NULL;
    int available;
    int connsize;
    int nameLength;
    char *conn;
    int ret = -1;

    available = KNI_GetParameterAsBoolean(2);
    connsize = KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);

    KNI_DeclareHandle(name);
    KNI_DeclareHandle(connections);
    KNI_GetParameterAsObject(1, name);
    KNI_GetParameterAsObject(3, connections);

    nameLength = KNI_GetArrayLength(name);

    if (nameLength > 0 && (midletName = (char*)midpMalloc(nameLength)) != NULL) {
        KNI_GetRawArrayRegion(name, 0, nameLength, (jbyte*)midletName);

        conn = pushfindsuite(midletName, available);

        if (conn != NULL) {
            KNI_SetRawArrayRegion(connections, 0, strlen(conn), (jbyte*)conn);
            midpFree(conn);
            ret = 0;
        }

        midpFree(midletName);
    }
    else {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }


    KNI_EndHandles();

    KNI_ReturnInt(ret);
}

/**
 * Checks for inbound connections.
 * <p>
 * Java declaration:
 * <pre>
 *     poll0(J)I
 * </pre>
 *
 * @param time The current time in milli seconds.
 *
 * @return A handle to the first pending, inbound connection
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_poll0) {
    jlong ret = (jlong) pushpoll();

    /*if (ret == -1) {
     If there is no pending I/O, check for alarms
        jlong time = KNI_GetParameterAsLong(1);
    }*/

    KNI_ReturnInt(ret);
}


/**
 * Gets the registered MIDlet name for the given inbound connection handle.
 * <p>
 * Java declaration:
 * <pre>
 *     getMIDlet0(J[BI)I
 * </pre>
 *
 * @param handle The handle to inbound connection
 * @param midletName A byte array to store the MIDlet name
 * @param midletNameLength The size of <tt>midlet</tt>
 *
 * @return <tt>0</tt> if successful, otherwise <tt>-1</tt>
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_getMIDlet0) {
    int   midletNameLength;
    char* regentry;
    int   regentryLength;
    int   ret = -1;
    int   handle;

    midletNameLength = (int)KNI_GetParameterAsInt(3);
    handle           = (int)KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);

    KNI_DeclareHandle(midletName);
    KNI_GetParameterAsObject(2, midletName);

    regentry = pushfindfd(handle);
    if (NULL != regentry) {
        regentryLength = strlen(regentry) + 1;      /* Include trailing '\0' */
        if (regentryLength < midletNameLength) {
            memcpy((char*)JavaByteArray(midletName),
                   regentry, regentryLength);
            ret = 0;
        }
        midpFree(regentry);
    }
    KNI_EndHandles();

    KNI_ReturnInt(ret);
}

/**
 * Gets the MIDlet name for the given registered push connection.
 * <p>
 * Java declaration:
 * <pre>
 *     getEntry0([B[BI)I
 * </pre>
 *
 * @param connection The connection to add to the push registry
 * @param midlet A byte array to store the MIDlet name
 * @param midletsize The size of <tt>midlet</tt>
 *
 * @return <tt>0</tt> if successful, otherwise <tt>-1</tt>
 *
 * @throw IOException if the registry entry is too long.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_getEntry0) {
    int midletsize;
    char *regentry;
    int regsize ;
    int ret = -1;
    int connLen;
    char *szConn = NULL;

    midletsize = KNI_GetParameterAsInt(3);

    KNI_StartHandles(2);
    KNI_DeclareHandle(conn);
    KNI_DeclareHandle(regObject);

    KNI_GetParameterAsObject(1, conn);
    connLen = KNI_GetArrayLength(conn);
    ret = -1;
    if (connLen > 0 && (szConn = midpMalloc(connLen)) != NULL) {
        KNI_GetRawArrayRegion(conn, 0, connLen, (jbyte*)szConn);

        KNI_GetParameterAsObject(2, regObject);

        regentry = pushfindconn(szConn);
        if (NULL != regentry) {
            regsize = strlen(regentry) + 1;
            if (regsize < midletsize) {
                KNI_SetRawArrayRegion(regObject, 0, regsize,
                                      (jbyte*)regentry);
                ret = 0;
            }
            else {
                KNI_ThrowNew(midpIOException, "registration too long");
            }
        }
        midpFree(szConn);
    }
    else {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }
    KNI_EndHandles();

    KNI_ReturnInt(ret);
}

/**
 * Deletes all push registry entries for the given MIDlet suite.
 * <p>
 * Java declaration:
 * <pre>
 *     delAllForSuite0(I)V
 * </pre>
 *
 * @param suiteID The MIDlet Suite ID.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_push_ConnectionRegistry_delAllForSuite0) {
    SuiteIdType suiteId;

    suiteId = KNI_GetParameterAsInt(1);
    pushdeletesuite(suiteId);

    KNI_ReturnVoid();
}

