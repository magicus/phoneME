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
    (void)isolateId;
    (void)exceptionClassName;
    (void)exceptionClassNameLength;
    (void)exceptionMessage;
    (void)flags;

    return 0;
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

    (void)isolateId;
    (void)limit;
    (void)reserve;
    (void)used;
    (void)allocSize;
    (void)flags;
    
    return 0;
}			    
