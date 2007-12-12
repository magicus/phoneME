/*
 * $LastChangedDate: 2006-03-29 20:41:10 +0200 $  
 */

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "javacall_defs.h"
#include "javacall_memory.h"
#include "javacall_logging.h"
#include "javacall_file.h"

#include "javautil_db.h"
#include "javautil_string.h"
#include "javautil_config_db.h"

/* Max value size for integers and doubles. */
#define MAX_INT_SIZE	1024

/* Invalid key */
#define INI_INVALID_KEY    ((char*)-1)

#ifndef min
  #define min(x,y)        (x > y ? y : x)
#endif

/*---------------------------------------------------------------------------
                        Internal Functions
 ---------------------------------------------------------------------------*/

static char *configdb_fgets(char *lin, int len, javacall_handle file_handle) {
    long read_len = 0;
    long actual_read = 1;

    while((read_len < len-1) && (actual_read>0)) {
        actual_read = javacall_file_read(file_handle, (unsigned char *)&lin[read_len], 1); 
        if(0 < actual_read) {
            if(read_len==0 && ((lin[0]=='\n') || (lin[0]=='\r')))
                continue;

            if(lin[read_len] == '\n' || lin[read_len] == '\r')
                break;
            if(actual_read>0)
                read_len += actual_read;
        }
    }
    lin[min(read_len,len-1)] = 0;
    if(read_len>0)
        return lin;
    return NULL;
}


/* convert an INI section/key to a database key and add it to the database */
static void configdb_add_entry(
                              string_db * d,
                              char * sec,
                              char * key,
                              char * val) {
    char longkey[2*MAX_INT_SIZE+1];

    if(NULL==d)
        return;
    /* Make a key as section:keyword */
    if(key!=NULL) {
        sprintf(longkey, "%s:%s", sec, key);
    } else {
        strcpy(longkey, sec);
    }

    /* Add (key,val) to string_db */
    string_db_set(d, javautil_str_tolwc(longkey), val);
    return ;
}

/*---------------------------------------------------------------------------
                        Public Functions
 ---------------------------------------------------------------------------*/

/**
 * Get the number of sections in the database
 * 
 * @param d database object created by calling configdb_load
 * @return number of sections in the database or -1 in case of error
 */
int configdb_get_num_of_sections(javacall_handle config_handle) {
    int i ;
    int nsec ;
    string_db *d = (string_db *)config_handle;

    if(d==NULL) return -1 ;
    nsec=0 ;
    for(i=0 ; i<d->size ; i++) {
        if(d->key[i]==NULL)
            continue ;
        if(strchr(d->key[i], ':')==NULL) {
            nsec ++ ;
        }
    }
    return nsec ;
}


/**
 * Get the name of the n'th section
 * 
 * @param d database object created by calling configdb_load
 * @return the name of the n'th section or NULL in case of error
 *          the returned function was STATICALLY ALLOCATED. DO NOT FREE IT!!
 */
char * configdb_get_section_name(javacall_handle config_handle, int n) {
    int i ;
    int foundsec ;
    string_db *d = (string_db *)config_handle;


    if(d==NULL || n<0) return NULL ;
    foundsec=0 ;
    for(i=0 ; i<d->size ; i++) {
        if(d->key[i]==NULL)
            continue ;
        if(strchr(d->key[i], ':')==NULL) {
            foundsec++ ;
            if(foundsec>n)
                break ;
        }
    }
    if(foundsec<=n) {
        return NULL ;
    }
    return d->key[i] ;
}


/**
 * Dump the content of the parameter database to an open file pointer
 * The output format is pairs of [Key]=[Value]
 * 
 * @param d                database object created by calling configdb_load
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return void
 */
#define MAX_STR_LEN	1024

void configdb_dump(javacall_handle config_handle, 
                   unsigned short* unicodeFileName, 
                   int fileNameLen) {

    int     i ;
    string_db *d = (string_db *)config_handle;
    javacall_handle file_handle;
    char            l[MAX_STR_LEN];
    javacall_result res;

    if(d==NULL || unicodeFileName==NULL || fileNameLen<=0) {
        return; 
    }

    if(fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH) {
        javacall_print("configdb_dump(): ERROR - File name length exceeds max file length\n");
        return;
    }


    res = javacall_file_open(unicodeFileName,
                             fileNameLen,
                             JAVACALL_FILE_O_RDWR | JAVACALL_FILE_O_CREAT,
                             &file_handle);
    if(res != JAVACALL_OK) {
        javacall_print("configdb_dump(): ERROR - Can't open the dump file!\n");
        return;
    }

    for(i=0 ; i<d->size ; i++) {
        if(d->key[i]==NULL)
            continue ;
        if(d->val[i]!=NULL) {
            sprintf(l, "[%s]=[%s]\n", d->key[i], d->val[i]);
        } else {
            sprintf(l, "[%s]=UNDEF\n", d->key[i]);
        }
        javacall_file_write(file_handle, (unsigned char*)l, strlen(l)); 
    }
    javacall_file_close(file_handle);
    return ;
}

/**
 * Dump the content of the parameter database to an open file pointer
 * The output format is as a standard INI file
 * 
 * @param d                database object created by calling configdb_load
 * @param unicodeFileName  output file name
 * @param fileNameLen      file name length
 * @return void
 */
void configdb_dump_ini(javacall_handle config_handle, 
                       unsigned short* unicodeFileName, 
                       int fileNameLen) {
    int     i, j ;
    char    keym[MAX_INT_SIZE+1];
    int     nsec ;
    char *  secname ;
    int     seclen ;
    string_db *d = (string_db *)config_handle;
    javacall_handle file_handle;
    char            l[MAX_STR_LEN];
    javacall_result res;

    if(d==NULL || unicodeFileName==NULL || fileNameLen<=0) {
        return; 
    }

    if(fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH) {
        javacall_print("configdb_dump_ini: ERROR - File name length exceeds max file length\n");
        return;
    }


    res = javacall_file_open(unicodeFileName,
                             fileNameLen,
                             JAVACALL_FILE_O_RDWR | JAVACALL_FILE_O_CREAT,
                             &file_handle);
    if(res != JAVACALL_OK) {
        javacall_print("configdb_dump_ini(): ERROR - Can't open the dump file!\n");
        return;
    }

    nsec = configdb_get_num_of_sections(d);
    if(nsec<1) {
        /* No section in file: dump all keys as they are */
        for(i=0 ; i<d->size ; i++) {
            if(d->key[i]==NULL)
                continue ;
            sprintf(l, "%s = %s\n", d->key[i], d->val[i]);
            javacall_file_write(file_handle, (unsigned char*)l, strlen(l)); 
        }
        javacall_file_close(file_handle);
        return ;
    }
    for(i=0 ; i<nsec ; i++) {
        secname = configdb_get_section_name(d, i) ;
        seclen  = (int)strlen(secname);
        sprintf(l, "\n[%s]\n", secname);
        javacall_file_write(file_handle, (unsigned char*)l, strlen(l)); 
        sprintf(keym, "%s:", secname);
        for(j=0 ; j<d->size ; j++) {
            if(d->key[j]==NULL)
                continue ;
            if(!strncmp(d->key[j], keym, seclen+1)) {
                sprintf(l,
                        "%-30s = %s\n",
                        d->key[j]+seclen+1,
                        d->val[j] ? d->val[j] : "");
                javacall_file_write(file_handle, (unsigned char*)l, strlen(l)); 
            }
        }
    }
    sprintf(l, "\n");
    javacall_file_write(file_handle, (unsigned char*)l, strlen(l)); 

    javacall_file_close(file_handle);
    return ;
}


/**
 * Get a the key value as a string
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to get its value. the key is given as INI "section:key"
 * @return STATICALLY ALLOCATED string with the value of key. if key not found, return def.
 */
configdb_result configdb_getstring(javacall_handle config_handle, char * key, 
									 char * def, char** result) {
    char * lc_key ;
    string_db *d = (string_db *)config_handle;

    if(d==NULL || key==NULL)
	{
		*result = def;
		return CONFIGDB_FAIL;
	}        

    lc_key = javautil_str_duplicate(javautil_str_tolwc(key));
    *result = string_db_getstr(d, lc_key, def);
    javacall_free(lc_key);
    return CONFIGDB_OK;
}


/**
 * Get a the key value as a int
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to get its value. the key is given as INI "section:key"
 * @return      the value of key. if key not found, return def.
 */
//int configdb_getint(javacall_handle config_handle, char * key, int notfound) {
configdb_result configdb_getint(javacall_handle config_handle, char * key, 
								int notfound, int* result) {
    char    *   str ;
    string_db *d = (string_db *)config_handle;

	if (CONFIGDB_OK == configdb_getstring(d, key, INI_INVALID_KEY, &str)) {
		*result = atoi(str);
		return CONFIGDB_OK;
	}
	else { 
		*result = 0;
		return CONFIGDB_FAIL;
	}
}


/**
 * Get a the key value as a double
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to get its value. the key is given as INI "section:key"
 * @return      the value of key. if key not found, return def.
 */
configdb_result configdb_getdouble(javacall_handle config_handle, char * key, 
								   double notfound, double* result) {

    char*   str ;
    string_db *d = (string_db *)config_handle;

	if (CONFIGDB_OK == configdb_getstring(d, key, INI_INVALID_KEY, &str)) {
		*result = atof(str);
		return CONFIGDB_OK;
	}
	else { 
		*result = 0;
		return CONFIGDB_FAIL;
	}
}


/**
 * Find a key in the database
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to find
 * @return      1 if the key exists or 0 if it does not exists
 */
configdb_result configdb_find_key(javacall_handle config_handle, char * key) {
	char* str;
    string_db *d = (string_db *)config_handle;

	if (CONFIGDB_OK == configdb_getstring(d, key, INI_INVALID_KEY, &str)) {
		return CONFIGDB_FOUND;
	}
	else {
		return CONFIGDB_NOT_FOUND;
	}
}



/**
 * Set new value for key. If key is not in the database, it will be added.
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to modify. the key is given as INI "section:key"
 * @param val   the new value for key
 * @return      -1 in case of error
 */
configdb_result configdb_setstr(javacall_handle config_handle, char * key, char * val) {
    string_db *d = (string_db *)config_handle;

    string_db_set(d, javautil_str_tolwc(key), val);
    return 0 ;
}

/**
 * Delete a key from the database
 * 
 * @param d     database object created by calling configdb_load
 * @param key   the key to delete
 * @return      void
 */
void configdb_unset(javacall_handle config_handle, char * key) {
    string_db *d = (string_db *)config_handle;
    string_db_unset(d, javautil_str_tolwc(key));
}


/**
 * Creates a new database from an INI file
 * 
 * @param unicodeFileName  input INI file name
 * @param fileNameLen      file name length
 * @return  newly created database object
 */

//TODO@gd212247:  debug
//#define USE_PROPERTIES_FROM_FS



#ifndef USE_PROPERTIES_FROM_FS
#include "javacall_static_properties.h"

javacall_handle configdb_load_no_fs () {
	string_db* db;
	int i, j;
	for (i = 0; javacall_static_properties_sections[i] != NULL; i++) {
		//add section
		db = string_db_new(0);
		if(NULL==db)
		{
			return NULL;
		}
		//add section to db
		configdb_add_entry(db, javacall_static_properties_sections[i], NULL, NULL);
		//add keys and values
		for (j = 0; javacall_static_properties_keys[i][j] != NULL; j++) {
			configdb_add_entry(db, javacall_static_properties_sections[i], javacall_static_properties_keys[i][j], 
							   javacall_static_properties_values[i][j]);
		}
	}
    return(javacall_handle)db;
}
#endif	//USE_PROPERTIES_FROM_FS

javacall_handle configdb_load_from_fs(unsigned short* unicodeFileName, int fileNameLen) {
	string_db  *   d ;
    char        lin[MAX_INT_SIZE+1];
    char        sec[MAX_INT_SIZE+1];
    char        key[MAX_INT_SIZE+1];
    char        val[MAX_INT_SIZE+1];
    char    *   where ;
    int         lineno ;
    javacall_handle    file_handle;
    javacall_result    res;

    if(fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH) {
        javacall_print("configdb_load: ERROR - File name length exceeds max file length\n");
        return NULL;
    }


    res = javacall_file_open(unicodeFileName,
                             fileNameLen,
                             JAVACALL_FILE_O_RDWR | JAVACALL_FILE_O_CREAT,
                             &file_handle);
    if(res != JAVACALL_OK) {
        javacall_print("configdb_load(): ERROR - Can't open the dump file!\n");
        return NULL;
    }

    sec[0]=0;

    /*
     * Initialize a new string_db entry
     */
    d = string_db_new(0);
    if(NULL==d)
        return NULL;
    lineno = 0 ;
    while(configdb_fgets(lin, MAX_INT_SIZE, file_handle)!=NULL) {
        lineno++ ;
        where = javautil_str_skip_leading_blanks(lin); /* Skip leading spaces */
        if(*where==';' || *where=='#' || *where==0)
            continue ; /* Comment lines */
        else {
            if(sscanf(where, "[%[^]]", sec)==1) {
                /* Valid section name */
                strcpy(sec, javautil_str_tolwc(sec));
                configdb_add_entry(d, sec, NULL, NULL);
            } else if(sscanf (where, "%[^=] = \"%[^\"]\"", key, val) == 2
                      ||  sscanf (where, "%[^=] = '%[^\']'",   key, val) == 2
                      ||  sscanf (where, "%[^=] = %[^;#]",     key, val) == 2) {
                strcpy(key, javautil_str_tolwc(javautil_str_skip_trailing_blanks(key)));
                /*
                 * sscanf cannot handle "" or '' as empty value,
                 * this is done here
                 */
                if(!strcmp(val, "\"\"") || !strcmp(val, "''")) {
                    val[0] = (char)0;
                } else {
                    strcpy(val, javautil_str_skip_trailing_blanks(val));
                }
                configdb_add_entry(d, sec, key, val);
            }
        }
    }
    javacall_file_close(file_handle);
    return(javacall_handle)d ;
}

javacall_handle configdb_load(unsigned short* unicodeFileName, int fileNameLen) {

#ifdef USE_PROPERTIES_FROM_FS
	return configdb_load_from_fs(unicodeFileName, fileNameLen);
#else
	return configdb_load_no_fs();
#endif	//USE_PROPERTIES_FROM_FS
}


/**
 * Free a database
 * 
 * @param   d database object created in a call to configdb_load
 * @return  void
 */
void configdb_free(javacall_handle config_handle) {
    string_db *d = (string_db *)config_handle;
    string_db_del(d);
}

