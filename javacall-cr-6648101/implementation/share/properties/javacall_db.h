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

#ifndef _DB_H_
#define _DB_H_

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
                            Types
 ---------------------------------------------------------------------------*/

typedef struct _string_db_ {
    int			n ;     /** Number of entries in string_db */
    int 		size ;  /** Storage size */
    char**  	val ;   /** List of string values */
    char**  	key ;   /** List of string keys */
    unsigned*  	hash ;  /** List of hash values for keys */
} string_db;


/*---------------------------------------------------------------------------
                            Function prototypes
 ---------------------------------------------------------------------------*/

/**
 * Calculate the hash value corresponding to the key
 * 
 * @param key the input key
 * @return the hash value of the provided key
 */
javacall_int32 javacall_string_db_hash(char* key);

/**
 * Creates a new database object
 * 
 * @param size the size of the database (use 0 for default size)
 * @return new database object
 */
string_db* javacall_string_db_new(int size);

/**
 * Deletes a database object
 * 
 * @param d the database object created by calling javacall_string_db_new
 */
void javacall_string_db_del(string_db* d);

/**
 * Search the value corresponding to the provided key. If the value has not 
 * been found, set default provided value.
 * 
 * @param d      database object allocated using javacall_string_db_new
 * @param key    the key to search in the database
 * @param def    if key not found in the database, set the value
 * @param result where to store the result (shallow copy)
 * @return 		JAVACALL_OK value found
 *              JAVACALL_INVALID_ARGUMENT bad arguments are supplied
 *              JAVACALL_VALUE_NOT_FOUND value has not been found
 */
javacall_result javacall_string_db_getstr(string_db* d, char* key, char* def, char** result);


/**
 * Set new value for key as string. 
 * 
 * @param d     database object allocated using javacall_string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the property value to set
 */
void javacall_string_db_set(string_db* d, char* key, char* val);

/**
 * Delete a key from the database
 * 
 * @param d     database object allocated using javacall_string_db_new
 * @param key   the key to delete
 */
void javacall_string_db_unset(string_db* d, char* key);


/**
 * Dump the content of the database to a file
 * 
 * @param d                database object allocated using javacall_string_db_new
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return  JAVACALL_OK in case of success
 *          JAVACALL_FAIL in case of some error
 */
javacall_result javacall_string_db_dump(string_db* d, const javacall_utf16* unicodeFileName, int fileNameLen);

#endif
