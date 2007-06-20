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

#ifndef _MIDP_RUNTIME_INFO_H_
#define _MIDP_RUNTIME_INFO_H_

/**
 * @defgroup ams_base Application Management System External Interface
 * @ingroup ams
 */
/**
 * @file
 * @ingroup ams_base
 *
 * @brief Definition of MidletRuntimeInfo structure.
 */

#include <java_types.h>

/**
 * Structure where run time information about the midlet will be placed.
 */
typedef struct _midletRuntimeInfo {
    /**
     * The minimum amount of memory guaranteed to be available to the isolate
     * at any time. Used to pass a parameter to midlet_create_start(),
     * < 0 if not used.
     */
    jint memoryReserved;
    /**
     * The total amount of memory that the isolate can reserve.
     * Used to pass a parameter to midlet_create_start(), < 0 if not used.
     */
    jint memoryTotal;
    /**
     * The approximate amount of object heap memory currently
     * used by the isolate.
     */
    jint usedMemory;
    /**
     * Priority of the isolate (< 0 if not set).
     */
    jint priority;
    /**
     * Name of the VM profile that should be used for the new isolate.
     * Used (1) to pass a parameter to midlet_create_start();
     * (2) to get a profile's name of the given isolate in run time.
     */
    jchar *profileName;
    /**
     * UTF16 length of the profile's name.
     */
    jint profileNameLen;
} MidletRuntimeInfo;

/* @} */

#endif /* _MIDP_RUNTIME_INFO_H_ */
