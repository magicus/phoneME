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
#include "javautil_db.h"

/* Max value size for integers and doubles. */
#define MAX_NUM_SIZE	20  //The largest possible number of characters in a string
                            // representing a number (e.g. "1234")

#define MAX_STR_LEN	    1024

/* Minimal allocated number of entries in a database */
#define MIN_NUMBER_OF_DB_ENTRIES	128

/* Invalid key */
#define DB_INVALID_KEY  ((char*)-1)


/*---------------------------------------------------------------------------
                            Internal functions
 ---------------------------------------------------------------------------*/

/* Doubles the allocated size associated to a pointer */
/* 'size' is the current allocated size. */
static void * mem_double(void * ptr, int size) {
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
 * Calculate the HASH value of the key
 * 
 * @param key the input key value
 * @return the HASH value of the provided key
 */
javacall_int32 string_db_hash(char * key) {
    int         len ;
    unsigned    hash ;
    int         i ;

    len = strlen(key);
    for (hash=0, i=0 ; i<len ; i++) {
        hash += (unsigned)key[i] ;
        hash += (hash<<10);
        hash ^= (hash>>6) ;
    }
    hash += (hash <<3);
    hash ^= (hash >>11);
    hash += (hash <<15);
    return hash ;
}


/**
 * Creates a new database object
 * 
 * @param size the size of the database (use MIN_NUMBER_OF_DB_ENTRIES for default size)
 * @return new database object
 */
string_db * string_db_new(int size) {
    string_db  *d;

    /* If no size was specified, allocate space for MIN_NUMBER_OF_DB_ENTRIES */
    if (size < MIN_NUMBER_OF_DB_ENTRIES)
    {
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
 * @param d the database object created by calling string_db_new
 */
void string_db_del(string_db * d) {
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
 * Get the value of the provided key as string. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
char* string_db_getstr(string_db* d, char* key, char* def) {
    unsigned    hash ;
    int         i ;

    if (NULL == d || d->n == 0) {
        return def;
    }

    hash = string_db_hash(key);

    for (i = 0; i < d->size; i++) {
        if (d->key == NULL) {
            continue;
        }
         
        /* Compare hash */
        if (hash == d->hash[i]) {
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, d->key[i])) {
                return d->val[i] ;
            }
        }
    }
    return def ;
}

/**
 * Get the value of the provided key as char. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
char string_db_getchar(string_db * d, char * key, char def) {
    char * v ;

    if ((v=string_db_getstr(d,key,DB_INVALID_KEY))==DB_INVALID_KEY) {
        return def ;
    } else {
        return v[0] ;
    }
}


/**
 * Get the value of the provided key as int. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
int string_db_getint(string_db * d, char * key, int def) {
    char * v ;

    if ((v=string_db_getstr(d,key,DB_INVALID_KEY))==DB_INVALID_KEY) {
        return def ;
    } else {
        return atoi(v);
    }
}

/**
 * Get the value of the provided key as double. Id key not found, return def
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to search in the database
 * @param def   if key not found in the database, return def
 * @return the value of the key (or def if key not found)
 */
double string_db_getdouble(string_db * d, char * key, double def) {
    char * v ;

    if ((v=string_db_getstr(d,key,DB_INVALID_KEY))==DB_INVALID_KEY) {
        return def ;
    } else {
        return atof(v);
    }
}


/**
 * Set new value for key as string. 
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the value of the key to set
 */
void string_db_set(string_db * d, char * key, char * val) {
    int         i;
    unsigned    hash;

    if (d==NULL || key==NULL) {
        return;
    }

    /* Compute hash for this key */
    hash = string_db_hash(key);
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
            break ;
        }
    }
    /* Copy key */
    d->key[i]  = javautil_string_duplicate(key);
    d->val[i]  = val ? javautil_string_duplicate(val) : NULL;
    d->hash[i] = hash;
    d->n++;
    return;
}

/**
 * Delete a key from the database
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to delete
 */
void string_db_unset(string_db * d, char * key) {
    unsigned    hash;
    int         i;

    if (NULL == d || NULL == key || d->n == 0) {
        /*return upon wrong arguments or if no entries in db*/
        return;
    }
    hash = string_db_hash(key);
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
            }
        }
    }
}


/**
 * Set new value for key as int. 
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the value of the key to set
 */
void string_db_setint(string_db * d, char * key, int val) {
    char    sval[MAX_NUM_SIZE];
    sprintf(sval, "%d", val);
    string_db_set(d, key, sval);
}


/**
 * Set new value for key as double. 
 * 
 * @param d     database object allocated using string_db_new
 * @param key   the key to modify/add to the database
 * @param val   the value of the key to set
 */
void string_db_setdouble(string_db * d, char * key, double val) {
    char    sval[MAX_NUM_SIZE];
    sprintf(sval, "%g", val);
    string_db_set(d, key, sval);
}



/**
 * Dump the content of the database to a file
 * 
 * @param d                database object allocated using string_db_new
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return  JAVACALL_OK in case of success
 *          JAVACALL_FAIL in case of some error
 */
javacall_result string_db_dump(string_db* d, unsigned short* unicodeFileName, int fileNameLen) {
    int             i;
    javacall_handle file_handle;
    char            l[MAX_STR_LEN];
    javacall_result res;

    if (d == NULL || unicodeFileName == NULL || fileNameLen <= 0) {
        return -1;
    }

    if (d->n < 1) {
        javacall_file_delete(unicodeFileName, fileNameLen);
        return - 1;
    }

    res = javacall_file_open(unicodeFileName, 
                             fileNameLen,
                             JAVACALL_FILE_O_WRONLY | JAVACALL_FILE_O_CREAT,
                             &file_handle);
    if (res != JAVACALL_OK) {
        javacall_print("string_db_dump(): ERROR - Can't open the dump file!\n");
        return -1;
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

    return 0;
}

#ifdef __cplusplus
}
#endif
