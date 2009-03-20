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

#include <string.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <midp_logging.h>
#include <commandLineUtil_md.h>

/**
 * Maximal number of exceptions that can be processed at once.
 * If the list if full, MAX_EXCEPTIONS/2 exceptions with lower
 * indexes will be thrown away.
 */
#define MAX_EXCEPTIONS 10

/**
 * A list of IDs of the exceptions that are being processed.
 */
static jlong exceptionIds[MAX_EXCEPTIONS];

/**
 * The number of exception that are currently being processed.
 * I.e., it is the number of filled entries in exceptionIds[]. 
 */
static int currentNumberOfExceptions = 0;

/**
 * Adds the given ID into the list of IDs of the exceptions being processed.
 *
 * @param exceptionId ID to add into the list
 */
static void add_exception_id(jlong exceptionId) {
    if (currentNumberOfExceptions >= MAX_EXCEPTIONS) {
        int i, lowerBound = MAX_EXCEPTIONS >> 1;
        for (i = lowerBound; i < currentNumberOfExceptions; i++) {
            exceptionIds[i - lowerBound] = exceptionIds[i];
        }
        currentNumberOfExceptions = lowerBound;
    }

    exceptionIds[currentNumberOfExceptions++] = exceptionId;
}

/**
 * Removes the given ID from the list of IDs of the exceptions being processed.
 *
 * @param index index of the ID to remove from the list
 */
static void remove_exception_id_by_index(int index) {
    if (index >= 0 && index < currentNumberOfExceptions) {
        int i;
        for (i = index + 1; i < currentNumberOfExceptions; i++) {
            exceptionIds[i - 1] = exceptionIds[i];
        }
        currentNumberOfExceptions--;
    }
}

/**
 * Looks for the given exceptionId in the list.
 *
 * @param exceptionId exception ID to find
 *
 * @return non-negative index of the entry having the given ID if found,
 *         -1 otherwise
 */
static int get_exception_id_index(jlong exceptionId) {
    int i;
    for (i = 0; i < currentNumberOfExceptions; i++) {
        if (exceptionIds[i] == exceptionId) {
            return i;
        }
    }

    return -1;
}

/**
 * This function is called by the VM before a Java thread is terminated
 * because of an uncaught exception.
 *
 * @param isolateId ID of an isolate in which this exception was thrown
 *                   (always 1 in SVM mode).
 * @param exceptionClassName name of the class containing the method.
 *              This string is a fully qualified class name
 *              encoded in internal form (see JVMS 4.2).
 *              This string is NOT 0-terminated.
 * @param exceptionClassNameLength number of UTF8 characters in
 *                                 exceptionClassNameLength.
 * @param message exception message as a 0-terminated ASCII string
 * @param flags a bitmask of flags
 *
 * @return zero to apply the default behavior, non-zero to suspend the isolate
 */
int midp_ams_uncaught_exception(int isolateId,
                                const char* exceptionClassName,
                                int exceptionClassNameLength,
                                const char* exceptionMessage,
                                int flags) {
#if ENABLE_NATIVE_APP_MANAGER /* JAMS currently doesn't handle this message */
    pcsl_string_status res;
    pcsl_string strExceptionClassName, strExceptionMessage;
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = MIDP_HANDLE_UNCAUGHT_EXCEPTION;
    /*
     * Use higher 16 bits for isolateId to be consistent
     * with midp_ams_out_of_memory().
     */
    evt.intParam1 = (isolateId << 16) | (flags & 0xffff);

    /* convert exceptionClassName to pcslString */
    res = pcsl_string_convert_from_utf8((const jbyte*)exceptionClassName,
                                        exceptionClassNameLength,
				                        &strExceptionClassName);
    if (res != PCSL_STRING_OK) {
        strExceptionClassName = PCSL_STRING_NULL;
    }

    evt.stringParam1 = strExceptionClassName;

    /* convert exceptionClassName to pcslString */
    if (exceptionMessage != NULL) {
        res = pcsl_string_convert_from_utf8((const jbyte*)exceptionClassName,
                                            strlen(exceptionMessage),
                                            &strExceptionMessage);
        if (res != PCSL_STRING_OK) {
            strExceptionMessage = PCSL_STRING_NULL;
        }
    } else {
        strExceptionMessage = PCSL_STRING_NULL;
    }

    evt.stringParam2 = strExceptionMessage;

    midpStoreEventAndSignalAms(evt);

    return 1;
#else
    (void)isolateId;
    (void)exceptionClassName;
    (void)exceptionClassNameLength;
    (void)exceptionMessage;
    (void)flags;

    return 0;
#endif /* ENABLE_NATIVE_APP_MANAGER */
}

/**
 * This function is called by the VM when it fails to fulfill
 * a memory allocation request.
 *
 * @param isolateId ID of an isolate in which the allocation was requested
 *    (always 1 in SVM mode).
 * @param limit in SVM mode - heap capacity, in MVM mode - memory limit for
 *    the isolate, i.e. the max amount of heap memory that can possibly
 *    be allocated
 * @param reserve in SVM mode - heap capacity, in MVM mode - memory reservation
 *    for the isolate, i.e. the max amount of heap memory guaranteed
 *    to be available
 * @param used how much memory is already allocated on behalf of this isolate
 *    in MVM mode, or for the whole VM in SVM mode.
 * @param allocSize the requested amount of memory that the VM failed to allocate
 * @param flags a bitmask of flags
 *
 * @return zero to apply the default behavior, non-zero to suspend the isolate
 */
int midp_ams_out_of_memory(int isolateId, int limit, int reserve, int used,
                           int allocSize, int flags) {

    /*
     * This implementation doesn't call javacall_ams_out_of_memory() or
     * nams_listeners_notify() directly due to the following reasons:
     * - to allow handling OOM in JAMS in the future;
     * - NAMS Peer can provide more appropriate information for the platform
     *   about the MIDlet by its isolateId.
     */
#if ENABLE_NATIVE_APP_MANAGER /* JAMS currently doesn't handle this message */
    /*
     * IMPL_NOTE: a pair of isolateId and allocSize is used to uniquely identify
     *            this exception. If the same exception is thrown again after
     *            re-trying the allocation attempt, it will be ignored.
     */
    MidpEvent evt;
    jlong exceptionId = ((jlong)isolateId << 32) | ((jlong)allocSize);
    int index = get_exception_id_index(exceptionId);

    if (index >= 0) {
        /* apply the default behavior (i.e., ignore the exception) */
        remove_exception_id_by_index(index);
        return 0;
    }

    /* add the ID to the list of IDs of the exceptions being processed */  
    add_exception_id(exceptionId);

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = MIDP_HANDLE_OUT_OF_MEMORY;
    /* pack two arguments into one jint because MidpEvent has just 5 intParams */
    evt.intParam1 = (isolateId << 16) | (flags & 0xffff);
    evt.intParam2 = limit;
    evt.intParam3 = reserve;
    evt.intParam4 = used;
    evt.intParam5 = allocSize;

    midpStoreEventAndSignalAms(evt);

    return 1;
#else
    (void)isolateId;
    (void)limit;
    (void)reserve;
    (void)used;
    (void)allocSize;
    (void)flags;
    
    return 0;
#endif /* ENABLE_NATIVE_APP_MANAGER */
}			    
