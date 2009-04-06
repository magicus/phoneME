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

#ifndef KNI_GLOBALS_H
#define KNI_GLOBALS_H

/**
 * @file
 * @ingroup core_kni
 *
 * @brief KNI global declarations
 */

/**
 * The size of gKNIBuffer
 * @see gKNIBuffer
 */
#define KNI_BUFFER_SIZE 512

/**
 * Internal utility buffer for use in KNI functions.
 * Integrity of information stored there across MIDP
 * function calls is NOT guaranteed.
 * Use it if you need a temporary buffer, but do not want to
 * allocate memory dynamically, nor to create a dedicated static buffer.
 *
 * This buffer is not used to pass data from one function to another.
 */
extern char gKNIBuffer[KNI_BUFFER_SIZE];

/**
 * Returns a pointer to the gKNIBuffer. It is an alternative and preferred
 * way how to get a pointer to the buffer.
 *
 * @return a pointer to the gKNIBuffer
 */
char * get_KNIBuffer();

#endif /* KNI_GLOBALS_H */

