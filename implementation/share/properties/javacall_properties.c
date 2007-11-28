/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_properties.h"
#include "javautil_config_db.h"

static unsigned short property_file_name[] = {'j','w','c','_','p','r','o','p','e','r','t','i','e','s','.','i','n','i',0};

static javacall_handle handle = NULL;
static int property_was_updated = 0;

static const char application_prefix[] = "application:";
static const char internal_prefix[] = "internal:";

/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>JAVACALL_OK</tt> for success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_initialize_configurations(void) {
    int file_name_len = sizeof(property_file_name)/sizeof(unsigned short);
    property_was_updated = 0;

	handle = configdb_load(property_file_name, file_name_len);
    if(handle == NULL) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Finalize the configuration subsystem.
 */
void javacall_finalize_configurations(void) {
    int file_name_len = sizeof(property_file_name)/sizeof(unsigned short);
    if(property_was_updated != 0) {
#ifdef USE_FILE_SYSTEM
		configdb_dump_ini(handle, property_file_name, file_name_len);
#endif //USE_FILE_SYSTEM
    }
    configdb_free(handle);
    handle = NULL; 
}

/**
 * Gets the value of the specified property key in the specified
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
									  property_type type,
									  char** result){
	char* value = NULL;
    char* joined_key = NULL;

	if (JAVACALL_APPLICATION_PROPERTY == type) {
		joined_key = javautil_string_concatenate(application_prefix, key);
	}
	else if (JAVACALL_INTERNAL_PROPERTY == type) {
		joined_key = javautil_string_concatenate(internal_prefix, key);
	}

	if (joined_key == NULL) {
		*result = NULL;
		return JAVACALL_FAIL;		
    }

	if (CONFIGDB_OK == configdb_getstring(handle, joined_key, NULL, result)) {		
		return JAVACALL_OK;	
	}
	else {
		*result = NULL;
		return JAVACALL_FAIL;
	}
}


/**
 * Sets a property key to the specified value in the application
 * property set.
 *
 * @param key The key to set
 * @param value The value to set <tt>key</tt> to
 * @param replace_if_exist The value to decide if it's needed to replace
 * existing key value if already defined <tt>replace_if_exist</tt>.
 * @param type The property type 
 * 
 * @return If found: <tt>JAVACALL_OK</tt>, otherwise
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_set_property(const char* key, 
                                    const char* value, 
                                    int replace_if_exist,
									property_type type) {
    configdb_result res;
    char* joined_key = NULL;

    if (JAVACALL_APPLICATION_PROPERTY == type) {
		joined_key = javautil_string_concatenate(application_prefix, key);
	}
	else if (JAVACALL_INTERNAL_PROPERTY == type) {
		joined_key = javautil_string_concatenate(internal_prefix, key);
	}

    /*
      The default behaviour will be to replace the existing 
      value of the key. 
      If request is not to replace the value, than function should 
      check if the key exist and if it is, not to set the value.
    */
    if(replace_if_exist == 0) { /* don't replace existing value */
        if(CONFIGDB_FOUND == configdb_find_key(handle,joined_key)) {
			/* key exist, don't set */
            javacall_free(joined_key);
        } else {/* key doesn't exist, set it */
            res = configdb_setstr(handle, joined_key, (char *)value);
            property_was_updated=1;
            javacall_free(joined_key);
        }
    } else { /* replace existing value */
        res = configdb_setstr(handle, joined_key, (char *)value);
        property_was_updated=1;
    }
	return JAVACALL_OK;
}



#ifdef __cplusplus
}
#endif

