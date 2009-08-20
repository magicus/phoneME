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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_properties.h"
#include "javacall_config_db.h"

typedef enum {
    PROPERTIES_INIT_NOT_STARTED,
    PROPERTIES_INIT_IN_PROGRESS,
    PROPERTIES_INIT_COMPLETED
} properties_init_state;

static javacall_utf16 default_property_file_name[] = 
    {'j','w','c','_','p','r','o','p','e','r','t','i','e','s','.','i','n','i'};

static javacall_utf16* property_file_name = NULL;
static int property_file_name_len = 0;

static javacall_handle handle = NULL;
static int property_was_updated = 0;
static properties_init_state init_state = PROPERTIES_INIT_NOT_STARTED;

static const char application_prefix[] = "application:";
static const char internal_prefix[] = "internal:";

static javacall_result set_properties_file_name(
        const javacall_utf16* unicodeFileName, int fileNameLen);
static javacall_utf16* get_properties_file_name(int* fileNameLen);

/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>JAVACALL_OK</tt> for success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_initialize_configurations(void) {
    javacall_utf16* file_name;
    int file_name_len;

    if (PROPERTIES_INIT_COMPLETED == init_state) {
        return JAVACALL_OK;
    }
    else if (PROPERTIES_INIT_IN_PROGRESS == init_state) {
        /* Properties initialization has not been finished yet */
        return JAVACALL_FAIL;
    }

    /* initialize state */
    init_state = PROPERTIES_INIT_IN_PROGRESS;
    property_was_updated = 0;

    file_name = get_properties_file_name(&file_name_len);

    handle = javacall_configdb_load(file_name, file_name_len);
    if (handle == NULL) {
        init_state = PROPERTIES_INIT_NOT_STARTED;
        return JAVACALL_FAIL;
    }
    init_state = PROPERTIES_INIT_COMPLETED;
    return JAVACALL_OK;
}

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
        const javacall_utf16* unicodeFileName, int fileNameLen) {
    javacall_result result = 
            set_properties_file_name(unicodeFileName, fileNameLen);
            
    if (result != JAVACALL_OK) {
        return result;
    }
    
    return javacall_initialize_configurations();
}

/**
 * Finalize the configuration subsystem.
 */
void javacall_finalize_configurations(void) {
    javacall_utf16* file_name;
    int file_name_len;

    if (PROPERTIES_INIT_COMPLETED != init_state) {
        return;
    }

    if (property_was_updated != 0) {
#ifdef USE_PROPERTIES_FROM_FS
        file_name = get_properties_file_name(&file_name_len);
        javacall_configdb_dump_ini(handle, file_name, file_name_len);
#endif //USE_PROPERTIES_FROM_FS
    }
    javacall_configdb_free(handle);
    handle = NULL;
    if (property_file_name != NULL) {
        javacall_free(property_file_name);
        
        property_file_name = NULL;
        property_file_name_len = 0;
    }
    init_state = PROPERTIES_INIT_NOT_STARTED;
}

/**
 * Gets the value of the specified property in the specified
 * property set.  If the property has not been found, assigns
 * NULL to result.
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
                                      char** result){
    char* joined_key = NULL;

    /* protection against access to uninitialized properties */
    if (JAVACALL_FAIL == javacall_initialize_configurations()) {
        *result = NULL;
        return JAVACALL_FAIL;
    }

    if (JAVACALL_APPLICATION_PROPERTY == type) {
        joined_key = javautil_string_strcat(application_prefix, key);
    } else if (JAVACALL_INTERNAL_PROPERTY == type) {
        joined_key = javautil_string_strcat(internal_prefix, key);
    }

    if (joined_key == NULL) {
        *result = NULL;
        return JAVACALL_FAIL;
    }

    if (JAVACALL_OK == javacall_configdb_getstring(handle, joined_key, NULL, result)) {
        javacall_free(joined_key);
        return JAVACALL_OK;
    } else {
        javacall_free(joined_key);
        *result = NULL;
        return JAVACALL_FAIL;
    }
}


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
javacall_result javacall_set_property(const char* key,
                                      const char* value,
                                      int replace_if_exist,
                                      javacall_property_type type) {
    char* joined_key = NULL;

    /* protection against access to uninitialized properties */
    if (JAVACALL_FAIL == javacall_initialize_configurations()) {
        return JAVACALL_FAIL;
    }

    if (JAVACALL_APPLICATION_PROPERTY == type) {
        joined_key = javautil_string_strcat(application_prefix, key);
    } else if (JAVACALL_INTERNAL_PROPERTY == type) {
        joined_key = javautil_string_strcat(internal_prefix, key);
    }

    if (joined_key == NULL) {
        return JAVACALL_FAIL;
    }

    if (replace_if_exist == 0) { /* don't replace existing value */
        if (JAVACALL_OK == javacall_configdb_find_key(handle,joined_key)) {
            /* key exist, don't set */
            javacall_free(joined_key);
        } else {/* key doesn't exist, set it */
            javacall_configdb_setstr(handle, joined_key, (char *)value);
            property_was_updated=1;
            javacall_free(joined_key);
        }
    } else { /* replace existing value */
        javacall_configdb_setstr(handle, joined_key, (char *)value);
        javacall_free(joined_key);
        property_was_updated=1;
    }
    return JAVACALL_OK;
}

/**
 * Sets the name of the file used to load from or save the properties to. If no 
 * configuration file name is set, is set to an empty string or <tt>NULL</tt>, 
 * the default configuration file is used.
 * 
 * @param unicodeFileName the file name as an unicode string
 * @param fileNameLen the length of the file name in UTF16 characters
 * @return <tt>JAVACALL_OK</tt> if successful, <tt>JAVACALL_FAIL</tt> otherwise 
 */ 
static javacall_result set_properties_file_name(
        const javacall_utf16* unicodeFileName, int fileNameLen) {
    int fileNameSize;
    javacall_utf16* fileNameCopy;
    
    if ((unicodeFileName == NULL) || (fileNameLen == 0)) {
        /* the default name is to be used */

        if (property_file_name != NULL) {
            javacall_free(property_file_name);
            
            property_file_name = NULL;
            property_file_name_len = 0;
        }
        
        return JAVACALL_OK;
    }
    
    fileNameSize = fileNameLen * sizeof(javacall_utf16);
    fileNameCopy = (javacall_utf16*)javacall_malloc(fileNameSize);
    
    if (fileNameCopy == NULL) {
        return JAVACALL_FAIL;
    }

    memcpy(fileNameCopy, unicodeFileName, fileNameSize);

    if (property_file_name != NULL) {
        javacall_free(property_file_name);
    }

    property_file_name = fileNameCopy;
    property_file_name_len = fileNameLen;    
    
    return JAVACALL_OK;
}

/**
 * Returns the name of the properties file set via 
 * <tt>javacall_set_properties_file_name</tt> or the default properties file
 * name if the name hasn't been set or has been set to <tt>NULL</tt> or an
 * empty string.
 * 
 * @param fileNameLen pointer to the length of the returned string in utf16
 *      characters
 * @return the properties file name    
 */  
static javacall_utf16* get_properties_file_name(int* fileNameLen) {
    if (property_file_name == NULL) {
        /* return the default name */
        *fileNameLen = sizeof(default_property_file_name) 
                           / sizeof(default_property_file_name[0]);
        return default_property_file_name;
    }
    
    *fileNameLen = property_file_name_len;
    return property_file_name;    
} 

#ifdef __cplusplus
}
#endif

