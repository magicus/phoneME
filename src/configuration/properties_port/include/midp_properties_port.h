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
 *
 */

#ifndef __MIDP_PROPERTIES_PORT_H
#define __MIDP_PROPERTIES_PORT_H

/* External function declarations */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets the value of the specified property key in the application
 * property set.
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key<tt> if found, otherwise
 *         <tt>NULL<tt>
 */
const char* getSystemProperty(const char* key);

/**
 * Gets the value of the specified property key in the internal
 * property set. If the key is not found in the internal property
 * set, the application property set is then searched.
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key<tt> if found, otherwise
 *         <tt>NULL<tt>
 */
const char* getInternalProperty(const char* key);

/**
 * Gets the integer value of the specified property key in the internal
 * property set. If the key is not found in the internal property
 * set, the application and afterwards the dynamic property sets
 * are searched.  
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key</tt> if found, otherwise
 *         <tt>0</tt>
 */
int getInternalPropertyInt(const char* key);

/**
 * Sets a property key to the specified value in the application
 * property set.
 *
 * @param key The key to set
 * @param value The value to set <tt>key<tt> to
 */
void  setSystemProperty(const char* key, const char* value);

/**
 * Sets a property key to the specified value in the internal
 * property set.
 *
 * @param key The key to set
 * @param value The value to set <tt>key<tt> to
 */
void  setInternalProperty (const char* key, const char* value);

/**
 * Finalize the configuration subsystem by releasing all the
 * allocating memory buffers. This method should only be called by
 * midpFinalize or internally by initializeConfig.
 */
void  finalizeConfig();

/**
 * Initializes the configuration sub-system.
 *
 * @return <tt>0<tt> for success, otherwise a non-zero value
 */

int   initializeConfig();
#ifdef __cplusplus
}
#endif

#endif /* __MIDP_PROPERTIES_PORT_H */
