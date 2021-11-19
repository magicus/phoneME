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

#ifndef _JAVAUTIL_CONFIGDB_H_
#define _JAVAUTIL_CONFIGDB_H_

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include "javacall_defs.h"

/**
 * Loads a database from an INI file
 * 
 * @param unicodeFileName  input INI file name
 * @param fileNameLen      file name length
 * @return  newly created database object
 */
javacall_handle javacall_configdb_load(javacall_utf16* unicodeFileName, int fileNameLen);

/**
 * Free a database
 * 
 * @param   config_handle database object created in a call to javacall_configdb_load
 */
void javacall_configdb_free(javacall_handle config_handle);

/**
 * Get the value corresponding to the key as a string
 * 
 * @param config_handle   database object created by calling javacall_configdb_load
 * @param key             The key to get the corresponding value of
 * @param def             default parameter to return if the value has not been found
 * @param result          where to store the result string
 * @return                JAVACALL_OK   The property has been found
 *                        JAVACALL_VALUE_NOT_FOUND The value has not been found
 *                        JAVACALL_FAIL   bad arguments are supplied
 */
javacall_result javacall_configdb_getstring(javacall_handle config_handle, char * key, 
                                   char* def, char** result);


/**
 * Find a key in the database
 * 
 * @param config_handle database object created by calling javacall_configdb_load
 * @param key   the key to find
 * @return      JAVACALL_OK if the value corresponding to the key exists and 
 *              is not empty string 
 *              JAVACALL_VALUE_NOT_FOUND otherwise
 */
javacall_result javacall_configdb_find_key(javacall_handle config_handle, char* key);

/**
 * Set new value for key. If key is not in the database, it will be added.
 * 
 * @param config_handle  database object created by calling javacall_configdb_load
 * @param key   the key value corresponding to which to modify. the key is given as INI "section:key"
 * @param val   the new value for key
 * @return      JAVACALL_OK in case of success
 *              JAVACALL_FAIL otherwise
 */
javacall_result javacall_configdb_setstr(javacall_handle config_handle, char* key, char* val);

/**
 * Delete a key from the database
 * 
 * @param config_handle database object created by calling javacall_configdb_load
 * @param key   the key to delete
 */
void javacall_configdb_unset(javacall_handle config_handle, char * key);

/**
 * Get the number of sections in the database
 * 
 * @param config_handle database object created by calling javacall_configdb_load
 * @return number of sections in the database or -1 in case of error
 */
int javacall_configdb_get_num_of_sections(javacall_handle config_handle);

/**
 * Get the name of the n'th section
 * 
 * @param config_handle database object created by calling javacall_configdb_load
 * @param n section number
 * @return the name of the n'th section or NULL in case of error
 *          the returned string was STATICALLY ALLOCATED. DO NOT FREE IT!!
 */
char* javacall_configdb_get_section_name(javacall_handle config_handle, int n);


/**
 * Dump the content of the parameter database to an open file pointer
 * The output format is pairs of [Key]=[Value]
 * 
 * @param config_handle    database object created by calling javacall_configdb_load
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 */
void javacall_configdb_dump(javacall_handle config_handle, javacall_utf16* unicodeFileName, int fileNameLen);

/**
 * Dump the content of the parameter database to an open file pointer
 * The output format is as a standard INI file
 * 
 * @param config_handle    database object created by calling javacall_configdb_load
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 */
void javacall_configdb_dump_ini(javacall_handle config_handle, javacall_utf16* unicodeFileName, int fileNameLen);

#endif /* _JAVAUTIL_CONFIGDB_H_ */
