/*
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
 * Interface for property set and get handling.
 * 
 */

#ifndef _JAVACALL_PROPERTIES_H_
#define _JAVACALL_PROPERTIES_H_

#include "javacall_defs.h"



typedef enum {
    JAVACALL_APPLICATION_PROPERTY,
    JAVACALL_INTERNAL_PROPERTY,
    JAVACALL_NUM_OF_PROPERTIES
} javacall_property_type;


/**
 * Gets the value of the specified property in the specified
 * property set.
 *
 * @param key The key to search for
 * @param type The property type 
 * @param result Where to put the result
 *
 * @return If found: <tt>JAVACALL_OK</tt>, otherwise
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_get_property(const char* key, 
                                      javacall_property_type type,
                                      char** result);




/**
 * Sets a property value matching the key in the specified
 * property set.
 *
 * @param key The key to set
 * @param value The value to set <tt>key</tt> to
 * @param replace_if_exist The value to decide if it's needed to replace
 * the existing value corresponding to the key if already defined
 * @param type The property type 
 * 
 * @return Upon success <tt>JAVACALL_OK</tt>, otherwise
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_set_property(const char* key, const char* value, 
                                      int replace_if_exist, javacall_property_type type);

/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>JAVACALL_OK</tt> for success, <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_initialize_configurations(void);

/**
 * Initializes the configuration sub-system, reading the initial set of 
 * properties from the file with the specified name. If the file name is 
 * an empty string or <tt>NULL</tt>, the default configuration file is used.
 *
 * @param unicodeFileName the file name as an unicode string
 * @param fileNameLen the length of the file name in UTF16 characters
 * @return <tt>JAVACALL_OK</tt> for success, <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_initialize_configurations_from_file(
        const javacall_utf16* unicodeFileName, int fileNameLen);

/**
 * Finalize the configuration subsystem.
 */
void javacall_finalize_configurations(void);

#endif  /* _JAVACALL_PROPERTIES_H_ */
