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

#include <kni.h>

#include <midpMalloc.h>
#include <midp_properties_port.h>
#include <midpStorage.h>
#include <gcf_export.h>
#include <midp_logging.h>
#include <midp_constants_data.h>

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

/** The name of the internal property file */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(IMPL_PROPERTY_FILE)
    {'i', 'n', 't', 'e', 'r', 'n', 'a', 'l', '.',
     'c', 'o', 'n', 'f', 'i', 'g', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(IMPL_PROPERTY_FILE);

/** The name of the application property file */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(APPL_PROPERTY_FILE)
    {'s', 'y', 's', 't', 'e', 'm', '.',
     'c', 'o', 'n', 'f', 'i', 'g', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(APPL_PROPERTY_FILE);

/** Storage structure for a property set */
typedef struct _configproperty {
    struct _configproperty *next ;
    const char *key;
    const char *value;
} Property ;

/*
 * Use a simple linked list of name value pairs for application level and
 * implementation targeted properties. The space is partitioned to provide
 * some separation of values that can be protected.
 */
/** Application property set */
static Property * applicationProperties = NULL ;
/** Internal property set */
static Property * implementationProperties = NULL ;

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
 * Trims leading and trailing white space from an array of
 * C chars (8-bit) i.e C string.
 * This function shifts the characters in the string for leading spaces.
 *
 * @param str The array of C characters, c string
 *
 */
void trim_WhiteSpace(char *str) {
    char* s;
    int i;

    if (str == NULL) {
        return;
    }

    /* Remove trailing whitespace */
    for (i = strlen(str) - 1; isspace(str[i]); i--);
    str[++i] = '\0';

    /* Remove leading whitespace */
    for (s = str; isspace(*s); s++);

    memmove(str, s, strlen(s) + 1);
}

/**
 * Reads in a property file and makes the key/value pairs available
 * to MIDP as a property set.
 *
 * @param fd An open file descriptor of the property file to read.
 * @param props A property set to hold the key/value pairs read from
 *              the property file.
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 */
static int
parseConfig(int fd, Property** props) {
    char *buffer;
    int bufferSize;
    int i;
    int j;
    int key_index;
    int value_index;
    int len;
    char *errStr = NULL;
    int startPos;
    int endPos;

    bufferSize = storageSizeOf(&errStr, fd);
    buffer = (char *)midpMalloc(bufferSize);
    if (buffer == NULL) {
        REPORT_WARN(LC_CORE, 
            "midpMalloc failed to allocate buffer for config file."); 
        return -1;
    }

    /* Read the config file in one pass */
    len = storageRead(&errStr, fd, buffer, bufferSize);
    if ((errStr != NULL) || (len != bufferSize)) {
        REPORT_WARN1(LC_CORE, 
             "Warning: can not read config file: %s", errStr);
    
        storageFreeError(errStr);
        midpFree(buffer);
        return 0;
    }

    startPos = 0;
    for (i = startPos; i < bufferSize; i++) {
        if (buffer[i] == '\n') {
            
            buffer[i] = 0;

            /* Skip comment lines which begin  with '#'*/
            if (buffer[startPos] != '#') {
                
                /* Parse the line */
                key_index = startPos;
                for (j = key_index; buffer[j]; j++){

                    Property *prop;

                    if (buffer[j] == ':') {
                        buffer[j] = 0;
                        value_index = ++j;

                        prop = (Property *) midpMalloc(sizeof(Property));
                        if (NULL != prop) {
                            char *key, *value;
                            key = midpStrdup(buffer + key_index);
                            value = midpStrdup(buffer + value_index);
                            
                            /* trim leading and trailing white spaces */
                            trim_WhiteSpace(key);
                            trim_WhiteSpace(value);

                            prop->next = *props;
                            prop->key = key;
                            prop->value = value;
        
                            if ((prop->key == NULL) || (prop->value == NULL)) {
                                midpFree((void*)prop->key);
                                midpFree((void*)prop->value);
                                midpFree(prop);

                                /*
                                 * since we are freeing memory, we're not
                                 * exactly out of memory at this point
                                 */
                                break;
                            }

                            *props = prop;
                        }

                        break;
                    }
                }
            }
            endPos = i;
            startPos = endPos + 1;
        }
    }
    midpFree(buffer);
    return 0;
}

/**
 * Initializes a property set with the contents of a property file.
 *
 * @param props The property set to initialize
 * @param name The name of the property file to load. It is relative
 *             to the <tt>configRoot</tt> path.
 * @param configRoot The fully qualified pathname to the root
 *                   configuration directory.
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 */
static int
initProps(Property** props, const pcsl_string * name,
    const pcsl_string * configRoot) {

    pcsl_string pathname;
    int fd = -1 ;
    char * errStr;

    /* Property file can be relative or at midp_home variable */
    pcsl_string_cat(configRoot, name, &pathname);

    fd = storage_open(&errStr, &pathname, OPEN_READ);
    pcsl_string_free(&pathname);
    if (errStr != NULL) {
        REPORT_WARN2(LC_CORE,
             "Warning: could not open config file(%s): %s\n",
             pathname, errStr);
             
        storageFreeError(errStr);

        return 0;
    }

    /* Read through the file one line at a time */
    if (parseConfig(fd, props) != 0) {
        return -1;
    }

    /* Close the storage handle */
    storageClose(&errStr, fd);
    return 0;
}

/**
 * Finalizes a property set.
 *
 * @param props The property set to finalize
 */
static void
finalizeProps(Property* props) {
    Property *prop, *tmp;

    if (props != NULL) {
    for (prop = props ; prop ; prop = tmp ){
        tmp = prop->next;
        midpFree((void*)prop->key);
        midpFree((void*)prop->value);
        midpFree(prop);
    }
    }
}

/**
 * Sets a property key to the specified value.
 *
 * @param props The property set in which to add/modify <tt>key</tt>
 * @param key The key to set
 * @param value The value to set <tt>key</tt> to
 */
static void
setProp(Property** props, const char* key , const char* value) {
    /* Try to find the property in the current pool. */
    Property *p;
    for (p= *props; p; p = p->next) {
        if (strcmp(key, p->key) == 0) {
            midpFree((void*)p->value);
            p->value = midpStrdup(value);
            /*
             * if midpStrdup fails we will just return without setting
             * the value to anything other than NULL
             */
            return;
        }
    }

    /* If the value is not defined, add it now */
    p = (Property*)midpMalloc(sizeof(Property));
    if (NULL != p){
        p->next = *props ;
        p->key = midpStrdup(key);
        p->value = midpStrdup(value);

        if ((p->key == NULL) || (p->value == NULL)) {
            /* do nothing if there is no memory */
            midpFree((void*)p->key);
            midpFree((void*)p->value);
            midpFree(p);
            return;
        }
        *props = p ;
    }
}

/**
 * Finds a property key and returns its value.
 *
 * @param prop The property set to search
 * @param key The key to search for
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>NULL</tt>
 */
static const char*
findProp(Property* prop, const char* key) {
    Property* p;

    p = prop;
    while (p) {
        if (strcmp(key, p->key) == 0) {
            return (p->value);
        }
        p = p->next;
    }
    return (NULL);
}

/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>0</tt> for success, otherwise a non-zero value
 */
int
initializeConfig(void) {
    if (implementationProperties != NULL) {
        /* Already initialized. */
        return 0;
    }

    if (initProps(&implementationProperties, &IMPL_PROPERTY_FILE,
                  storage_get_config_root(INTERNAL_STORAGE_ID)) != 0) {
        return -1;
    }

    if (initProps(&applicationProperties, &APPL_PROPERTY_FILE,
                  storage_get_config_root(INTERNAL_STORAGE_ID)) != 0) {
        finalizeConfig();
        return -1;
    }

    /*
     * Make sure the configuration was specified, because
     * some older code requires it in the CLDC classes.
     */
    if (getSystemProperty(DEFAULT_CONFIGURATION) == NULL) {
        setSystemProperty(DEFAULT_CONFIGURATION, DEFAULT_CLDC);
    }

    if (getSystemProperty(PROFILES_PROP_NAME) == NULL) {
        setSystemProperty(PROFILES_PROP_NAME, DEFAULT_PROFILE);
    }

    if (getSystemProperty(ENCODING_PROP_NAME) == NULL) {
        setSystemProperty(ENCODING_PROP_NAME, DEFAULT_CHARACTER_ENCODING);
    }

    return 0;
}

/**
 * Finalize the configuration subsystem by releasing all the
 * allocating memory buffers. This method should only be called by
 * midpFinalize or internally by initializeConfig.
 */
void
finalizeConfig(void) {
    finalizeProps(implementationProperties);
    implementationProperties = NULL;
    finalizeProps(applicationProperties);
    applicationProperties = NULL;
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
    setProp(&implementationProperties, key, value);
}

/**
 * Gets the value of the specified property key in the internal
 * property set. If the key is not found in the internal property
 * set, the application property set is then searched.
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>NULL</tt>
 */
const char*
getInternalProperty(const char* key) {
    const char *result;
    
    result = findProp(implementationProperties, key);
    if (NULL == result) {
        result = findProp(applicationProperties, key);
    }

    return result;
}

/**
 * Gets the value of the specified property key in the internal
 * property set. If the key is not found in the internal property
 * set, the application property set is then searched. If neither
 * search finds the specified key, a default value is returned.
 *
 * @param key The key to search for
 * @param def The default value to return if <tt>key</tt> is not found.
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>def</tt>
 */
const char*
getInternalPropertyDefault(const char* key, const char* def) {
    const char *result;

    result = findProp(implementationProperties, key);
    if (NULL == result) {
        result = findProp(applicationProperties, key);
    }

    return (NULL == result) ? def : result;
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
    setProp(&applicationProperties, key, value);
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
    const char *result;

    result = findProp(applicationProperties, key);
    if ((NULL == result) && (strcmp(key, "microedition.hostname") == 0)) {
        /* Get the local hostname from the native networking subsystem */
        result = getLocalHostName();
    }

    return result;
}
