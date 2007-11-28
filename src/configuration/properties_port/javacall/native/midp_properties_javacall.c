/*
 *   
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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <kni.h>

#include <midpMalloc.h>
#include <midp_properties_port.h>
#include <midpStorage.h>
#include <gcf_export.h>
#include <midp_logging.h>
#include <javacall_properties.h>

/**
 * @file
 * MIDP native configuration property implementation.
 * <b>OVERVIEW:</b>
 * <p>
 * This file defines the functions that access runtime configuration
 * properties that are predefined in configuration files or
 * overridden on the command line or environmental variables.
 * <p>
 * <b>ASSUMPTIONS:</b>
 * <p>
 * The complete property mechanism will not be needed when
 * MIDP is ported to a physical device where there is no
 * need for command line arguments or environment variable
 * overrides. This module is intended to simplify the porting
 * effort in identifying runtime switches that can be
 * optimized when shrinking the complete system by hard
 * wiring specific components in the system.
 * <p>
 * <b>DATASTRUCTURE:</b>
 * <p>
 * Two configuration files are supported. First, the file
 * lib/system.config contains the properties visible to the
 * application code via the System.getProperty() interface.
 * Second, the lib/internal.config contains implementation
 * specific properties that are not intended to be exposed
 * to the MIDlet application.
 * <p>
 * A configuration file contains "key: value\n" lines. Carriage
 * returns are ignored, and lines beginning with the pound sign
 * are skipped as comment lines. White space after the colon are
 * trimmed before the key and value are recorded.
 * <p>
 * The system will continue to run even if the configuration files
 * can not be read or contain parsing errors in the data.
 */


/** Configuration property name, as defined by the CLDC specification */
#define DEFAULT_CONFIGURATION "microedition.configuration"
/** Default configuration, as required by the MIDP specification */
#define DEFAULT_CLDC "CLDC-1.0"

/** Character encoding property name, as defined by the MIDP specification */
#define ENCODING_PROP_NAME "microedition.encoding"
/** Default character encoding, as required by the MIDP specification */
#define DEFAULT_CHARACTER_ENCODING "ISO-8859-1"

/** Profile property name, as defined by the MIDP specification */
#define PROFILES_PROP_NAME "microedition.profiles"
/** Default profile, as required by the MIDP specification */
#define DEFAULT_PROFILE "MIDP-2.1"


/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>0</tt> for success, otherwise a non-zero value
 */
int
initializeConfig(void) {
     return 0;
}

/**
 * Finalize the configuration subsystem by releasing all the
 * allocating memory buffers. This method should only be called by
 * midpFinalize or internally by initializeConfig.
 */
void
finalizeConfig(void) {
}

/**
 * Sets a property key to the specified value in the internal
 * property set.
 *
 * @param key The key to set
 * @param value The value to set <tt>key</tt> to
 */
void
setInternalProperty(const char* key , const char* value) {
	javacall_set_property(key, value, KNI_TRUE, JAVACALL_INTERNAL_PROPERTY);
}

/**
 * Gets the value of the specified property key in the internal
 * property set. If the key is not found in the internal property
 * set, the application  property set is searched.
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>NULL</tt>
 */
const char*
getInternalProperty(const char* key) {
	char *str;

	if (JAVACALL_OK == javacall_get_property(key, JAVACALL_INTERNAL_PROPERTY, &str)) {
		return str;
	}
	else if (JAVACALL_OK == javacall_get_property(key, JAVACALL_APPLICATION_PROPERTY, &str)) {
		return str;
	}

	return NULL;
}

/**
 * Gets the integer value of the specified property key in the internal
 * property set.  
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>0</tt>
 */
int getInternalPropertyInt(const char* key) {
    const char *tmp;	

    tmp = getInternalProperty(key);	

	return (NULL == tmp) ? 0 : atoi(tmp);
}



/**
 * Sets a property key to the specified value in the application
 * property set.
 *
 * @param key The key to set
 * @param value The value to set <tt>key</tt> to
 */
void
setSystemProperty(const char* key , const char* value) {
	javacall_set_property(key, value, KNI_TRUE, JAVACALL_APPLICATION_PROPERTY);	
}

/**
 * Gets the value of the specified property key in the application
 * property set.
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>NULL</tt>
 */
const char*
getSystemProperty(const char* key) {
    char *str;

	if (JAVACALL_OK == javacall_get_property(key, JAVACALL_APPLICATION_PROPERTY, &str)) {
		return str;
	}

	return NULL;
}

