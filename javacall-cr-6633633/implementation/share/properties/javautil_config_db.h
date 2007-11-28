/*
 * $LastChangedDate: 2006-03-29 20:41:10 +0200 $  
 */
 
/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#ifndef _JAVAUTIL_CONFIGDB_H_
#define _JAVAUTIL_CONFIGDB_H_

/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/
#include "javacall_defs.h"


typedef enum {
	CONFIGDB_OK,
	CONFIGDB_FAIL,
	CONFIGDB_FOUND,
	CONFIGDB_NOT_FOUND,	
} configdb_result;


/**
 * Creates a new database from an INI file
 * 
 * @param unicodeFileName  input INI file name
 * @param fileNameLen      file name length
 * @return  newly created database object
 */
javacall_handle configdb_load(unsigned short* unicodeFileName, int fileNameLen);

/**
 * Free a database
 * 
 * @param   d database object created in a call to configdb_load
 * @return  void
 */
void configdb_free(javacall_handle config_handle);

/**
 * Get a the key value as a string
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to get its value. the key is given as INI "section:key"
 * @return STATICALLY ALLOCATED string with the value of key. if key not found, return notfound.
 */
configdb_result configdb_getstring(javacall_handle config_handle, char * key, 
									 char * def, char** result);

/**
 * Get a the key value as a int
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to get its value. the key is given as INI "section:key"
 * @return      the value of key. if key not found, return notfound.
 */
configdb_result configdb_getint(javacall_handle config_handle, char * key, 
								int notfound, int* result);

/**
 * Get a the key value as a double
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to get its value. the key is given as INI "section:key"
 * @return      the value of key. if key not found, return notfound.
 */
configdb_result configdb_getdouble(javacall_handle config_handle, char * key, 
								   double notfound, double* result);



/**
 * Find a key in the database
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to find
 * @return      1 if the key exists or 0 if it does not exists
 */
configdb_result configdb_find_key(javacall_handle config_handle, char * key);

/**
 * Set new value for key. If key is not in the database, it will be added.
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to modify. the key is given as INI "section:key"
 * @param val   the new value for key
 * @return      -1 in case of error
 */
int configdb_setstr(javacall_handle config_handle, char * key, char * val);

/**
 * Delete a key from the database
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to delete
 * @return      void
 */
void configdb_unset(javacall_handle config_handle, char * key);

/**
 * Get the number of sections in the database
 * 
 * @param d database object created by calling configdb_load
 * @return number of sections in the database or -1 in case of error
 */
int configdb_get_num_of_sections(javacall_handle config_handle);

/**
 * Get the name of the n'th section
 * 
 * @param d database object created by calling configdb_load
 * @return the name of the n'th section or NULL in case of error
 *          the returned function was STATICALLY ALLOCATED. DO NOT FREE IT!!
 */
char * configdb_get_section_name(javacall_handle config_handle, int n);


/**
 * Dump the content of the parameter database to an open file pointer
 * The output format is pairs of [Key]=[Value]
 * 
 * @param d                database object created by calling configdb_load
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return void
 */
void configdb_dump(javacall_handle config_handle, unsigned short* unicodeFileName, int fileNameLen);

/**
 * Dump the content of the parameter database to an open file pointer
 * The output format is as a standard INI file
 * 
 * @param d                database object created by calling configdb_load
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return void
 */
void configdb_dump_ini(javacall_handle config_handle, unsigned short* unicodeFileName, int fileNameLen);

#endif /* _JAVAUTIL_CONFIGDB_H_ */
