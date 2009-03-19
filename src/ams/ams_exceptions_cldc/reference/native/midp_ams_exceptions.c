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
 * This function is called by the VM when it fails to fulfil
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
    MidpEvent evt;

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
