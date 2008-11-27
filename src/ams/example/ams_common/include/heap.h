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

#ifndef _HEAP_H_
#define _HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reads JAVA_HEAP_SIZE property and returns it as required heap size.
 * If JAVA_HEAP_SIZE has not been found, then reads MAX_ISOLATES property,
 * calculates and returns size of the required heap. If the MAX_ISOLATES
 * has not been found, default heap size is returned.
 *
 * @return <tt>heap size</tt>
 */
int getHeapRequirement();

/** 
 * Reads properties with Java heap parameters
 * and passes them to VM.
 */
void setHeapParameters();

#if ENABLE_MULTIPLE_ISOLATES

/**
 * Reads AMS_MEMORY_RESERVED_MVM property and returns the amount of Java
 * heap memory reserved for AMS isolate. Whether the property is not found,
 * the same name hardcoded constant is used instead.
 *
 * @return total heap size in bytes available for AMS isolate,
 *    or -1 if unlimitted
 */
int getAmsHeapReserved();

/**
 * Reads AMS_MEMORY_LIMIT_MVM property and returns the maximal Java heap size
 * avilable for AMS isolate. Whether the property is not found, the same name
 * hardcoded constant is used instead.
 *
 * @return total heap size available for AMS isolate
 */
int getAmsHeapLimit();

#endif /* ENABLE_MULTIPLE_ISOLATES */

#ifdef __cplusplus
}
#endif

#endif /* _HEAP_H_ */
