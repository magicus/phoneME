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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "javacall_memory.h"
#include "javacall_logging.h"
#include "javautil_string.h"
#include "javacall_file.h"
#include "javacall_db.h"

/* Max value size for integers and doubles. */
#define MAX_NUM_SIZE	20  //The largest possible number of characters in a string
                            // representing a number (e.g. "1234")

#define MAX_STR_LEN	    1024

/* Minimal allocated number of entries in a database */
#define MIN_NUMBER_OF_DB_ENTRIES	128


/*---------------------------------------------------------------------------
                            Internal functions
 ---------------------------------------------------------------------------*/

/* Doubles the allocated size associated to a pointer */
/* 'size' is the current allocated size. */
static void* mem_double(void * ptr, int size) {
    int    newsize = size*2;
    void*  newptr ;
    if (NULL==ptr) {
        return NULL;
    }
    newptr = javacall_malloc(newsize);
    if (newptr == NULL) {
        return NULL;
    }
    memset(newptr, 0, newsize);
    memcpy(newptr, ptr, size);
    javacall_free(ptr);
    return newptr ;
}


/*---------------------------------------------------------------------------
                            Public Functions
 ---------------------------------------------------------------------------*/

/**
 * Calculate the hash value corresponding to the key
 * 
 * @param key the input key
 * @return the hash value of the provided key
 */
javacall_int32 javacall_string_db_hash(char* key) {
    int length;
    javacall_int32 result;
    int i;

    length = strlen(key);

    for (result = 0, i = 0; i < length; i++) {
        result += (javacall_int32)key[i];
        result += (result << 10);
        result ^= (result >> 6);
    }

    return result;
}


/**
 * Creates a new database object
 * 
 * @param 	size The size of the database. If the size is smaller than MIN_NUMBER_OF_DB_ENTRIES,
 * MIN_NUMBER_OF_DB_ENTRIES will be used instead.
 * @return 	new database object
 */
string_db * javacall_string_db_new(int size) {
    string_db  *d;

    if (size < MIN_NUMBER_OF_DB_ENTRIES) {
        size = MIN_NUMBER_OF_DB_ENTRIES;
    }
         
    d = javacall_malloc(sizeof(string_db));
    if (NULL==d) {
        return NULL;
    }
    memset(d, 0, sizeof(string_db));
    d->n = 0;
    d->size = size ;

    d->val  = javacall_malloc(size*sizeof(char*));
    if (NULL==d->val) {
        javacall_free(d);
        return NULL;
    }
    memset(d->val, 0,size*sizeof(char*));

    d->key  = javacall_malloc(size*sizeof(char*));
    if (NULL==d->key) {
        javacall_free(d->val);
        javacall_free(d);
        return NULL;
    }
    memset(d->key, 0, size*sizeof(char*));

    d->hash = javacall_malloc(size*sizeof(unsigned));
    if (NULL==d->hash) {
        javacall_free(d->val);
        javacall_free(d->key);
        javacall_free(d);
        return NULL;
    }
    memset(d->hash, 0, size*sizeof(unsigned));

    return d ;
}


/**
 * Deletes a database object
 * 
 * @param d the database object created by calling javacall_string_db_new
 */
void javacall_string_db_del(string_db * d) {
    int     i ;

    if (d==NULL) return ;
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]!=NULL)
            javacall_free(d->key[i]);
        if (d->val[i]!=NULL)
            javacall_free(d->val[i]);
    }
    javacall_free(d->val);
    javacall_free(d->key);
    javacall_free(d->hash);
    javacall_free(d);
}



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
javacall_result javacall_string_db_getstr(string_db* d, char* key, char* def, char** result) {
    unsigned    hash;
    int         i;

    /* Set the default value */
    *result = def;

    if (NULL == d || d->n == 0) {        
        return JAVACALL_INVALID_ARGUMENT;
    }

    hash = javacall_string_db_hash(key);

    for (i = 0; i < d->size; i++) {
        if (d->key == NULL) {
            continue;
        }
        /* Compare hash */
        if (hash == d->hash[i]) {
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, d->key[i])) {
                *result = d->val[i];
                return JAVACALL_OK;
            }
        }
    }
    /* not found */
    return JAVACALL_VALUE_NOT_FOUND;
}

/**
 * Set new value for key as string. 
 * 
 * @param d     database object allocated using javacall_string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the property value to set
 */
void javacall_string_db_set(string_db * d, char * key, char * val) {
    int         i;
    unsigned    hash;

    if (d==NULL || key==NULL) {
        return;
    }

    /* Compute hash for this key */
    hash = javacall_string_db_hash(key);
    /* Find if value is already in database */
    if (d->n > 0) {
        for (i = 0; i < d->size; i++) {
            if (d->key[i]==NULL) {
                continue;
            }
            if (hash == d->hash[i]) { /* Same hash value */
                if (!strcmp(key, d->key[i])) {   /* Same key */
                    /* Found a value: modify and return */
                    if (d->val[i] != NULL){
                        javacall_free(d->val[i]);
                    }
                    d->val[i] = val ? javautil_string_duplicate(val) : NULL ;
                    /* Value has been modified: return */
                    return;
                }
            }
        }
    }
    /* Add a new value */
    /* See if string_db needs to grow */
    if (d->n == d->size) {

        /* Reached maximum size: reallocate blackboard */
        d->val  = mem_double(d->val,  d->size * sizeof(char*)) ;
        d->key  = mem_double(d->key,  d->size * sizeof(char*)) ;
        d->hash = mem_double(d->hash, d->size * sizeof(unsigned)) ;

        /* Double size */
        d->size *= 2 ;
    }

    /* Insert key in the first empty slot */
    for (i = 0; i < d->size; i++) {
        if (d->key[i] == NULL) {
            /* Add key here */
            d->key[i]  = javautil_string_duplicate(key);
            d->val[i]  = val ? javautil_string_duplicate(val) : NULL;
            d->hash[i] = hash;
            d->n++;
            return;
        }
    }
}

/**
 * Delete a key from the database
 * 
 * @param d     database object allocated using javacall_string_db_new
 * @param key   the key to delete
 */
void javacall_string_db_unset(string_db * d, char * key) {
    unsigned    hash;
    int         i;

    if (NULL == d || NULL == key || d->n == 0) {
        /*return upon wrong arguments or if no entries in db*/
        return;
    }
    hash = javacall_string_db_hash(key);
    for (i = 0; i < d->size; i++) {
        if (d->key[i] == NULL){
            continue;
        }
        /* Compare hash */
        if (hash == d->hash[i]){
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, d->key[i])) {
                /* Found key */
                    javacall_free(d->key[i]);
                    d->key[i] = NULL ;
                    if (d->val[i]!=NULL) {
                        javacall_free(d->val[i]);
                        d->val[i] = NULL ;
                    }
                    d->hash[i] = 0;
                    d->n--;
                    return;
            }
        }
    }
}


/**
 * Dump the content of the database to a file
 * 
 * @param d                database object allocated using javacall_string_db_new
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return  JAVACALL_OK in case of success
 *          JAVACALL_FAIL in case of some error
 */
javacall_result javacall_string_db_dump(string_db* d, const javacall_utf16* unicodeFileName, int fileNameLen) {
    int             i;
    javacall_handle file_handle;
    char            l[MAX_STR_LEN];
    javacall_result res;

    if (d == NULL || unicodeFileName == NULL || fileNameLen <= 0) {
        return JAVACALL_FAIL;
    }

    res = javacall_file_open(unicodeFileName, 
                             fileNameLen,
                             JAVACALL_FILE_O_WRONLY | JAVACALL_FILE_O_CREAT,
                             &file_handle);
    if (res != JAVACALL_OK) {
        javacall_print("javacall_string_db_dump(): ERROR - Can't open the dump file!\n");
        return JAVACALL_FAIL;
    }

    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]) {
            sprintf(l, "%20s\t[%s]\n",
                    d->key[i],
                    d->val[i] ? d->val[i] : "UNDEF");
            javacall_file_write(file_handle, (unsigned char*)l, strlen(l)); 
        }
    }
    javacall_file_close(file_handle);

    return JAVACALL_OK;
}

#ifdef __cplusplus
}
#endif
