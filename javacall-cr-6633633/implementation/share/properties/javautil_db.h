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
    int             n ;     /** Number of entries in string_db */
    int             size ;  /** Storage size */
    char        **  val ;   /** List of string values */
    char        **  key ;   /** List of string keys */
    unsigned     *  hash ;  /** List of hash values for keys */
} string_db ;


/*---------------------------------------------------------------------------
                            Function prototypes
 ---------------------------------------------------------------------------*/

/**
 * Calculate the HASH value of the key
 * 
 * @param key the input key value
 * @return the HASH value of the provided key. the return value should 
 *          be at least 32bit size
 */
javacall_int32 string_db_hash(char * key);

/**
 * Creates a new database object
 * 
 * @param size the size of the database (use 0 for default size)
 * @return new database object
 */
string_db * string_db_new(int size);

/**
 * Deletes a database object
 * 
 * @param d the database object created by calling string_db_new
 */
void string_db_del(string_db * d);

/**
 * Get the value of the provided key as string. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
char * string_db_getstr(string_db * d, char * key, char * def);


/**
 * Get the value of the provided key as char. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
char string_db_getchar(string_db * d, char * key, char def) ;

/**
 * Get the value of the provided key as int. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
int string_db_getint(string_db * d, char * key, int def);

/**
 * Get the value of the provided key as double. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
double string_db_getdouble(string_db * d, char * key, double def);

/**
 * Set new value for key as string. 
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the value of the key to set
 * @return void
 */
void string_db_set(string_db * d, char * key, char * val);

/**
 * Delete a key from the database
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to delete
 * @return void
 */
void string_db_unset(string_db * d, char * key);


/**
 * Set new value for key as int. 
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the value of the key to set
 * @return void
 */
void string_db_setint(string_db * d, char * key, int val);

/**
 * Set new value for key as double. 
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the value of the key to set
 * @return void
 */
void string_db_setdouble(string_db * d, char * key, double val);

/**
 * Dump the content of the database to a file
 * 
 * @param d                database object allocated using string_db_new
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return  0 in case of success
 *          -1 in case of some error
 */
int string_db_dump(string_db * d, unsigned short* unicodeFileName, int fileNameLen);

#endif
