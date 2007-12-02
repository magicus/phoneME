/*
 * $LastChangedDate: 2006-03-29 20:41:10 +0200 $  
 */

/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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
} property_type;


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
									  char** result);



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
javacall_result javacall_set_property(const char* key, const char* value, 
                                       int replace_if_exist, property_type type);

/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>JAVACALL_OK</tt> for success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_initialize_configurations(void);

/**
 * Finalize the configuration subsystem.
 */
void javacall_finalize_configurations(void);

#endif  /* _JAVACALL_PROPERTIES_H_ */
