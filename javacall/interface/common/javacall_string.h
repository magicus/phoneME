/*
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

#ifndef __JAVACALL_STRING_H
#define __JAVACALL_STRING_H

/**
* @file javacall_string.h
* @ingroup StringUtils
* @brief Javacall interfaces string and characters utilities
*/

#include "javacall_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @defgroup StringUtils String Utilities API
* @ingroup JTWI
*
* @{
*/
    
/** 
 * Converts lower case utf16 alpha characters into upper case. Other characters remain unchanged.
 * 
 * @param   chars array of utf16 characters to be converted
 * @param   count number of characters
 * @return	JAVACALL_OK if operation is successfully completed
 */

javacall_result javacall_towupper(javacall_utf16 * chars, unsigned int count);

/** 
 * Converts upper case utf16 alpha characters into lower case. Other characters remain unchanged.
 * 
 * @param   chars array of utf16 characters to be converted
 * @param   count number of characters
 * @return	JAVACALL_OK if operation is successfully completed
 */

javacall_result javacall_towlower(javacall_utf16 * chars, unsigned int count);

/** @} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif
