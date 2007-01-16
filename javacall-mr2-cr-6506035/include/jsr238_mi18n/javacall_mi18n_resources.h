/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef __javacall_mi18n_resources_h
#define __javacall_mi18n_resources_h

/**
 * @defgroup JSR238 JSR238 Mobile Internationalization API (MI18N)
 * @ingroup stack
 * 
 * Porting interface for native implementation Mobile Internationalization API.
 * 
 * @{
 */
/** @} */

/**
 * @file javacall_mi18n_resources.h
 * @ingroup JSR238
 * @brief JSR238 Mobile Internationalization API (MI18N)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

/**
 * @defgroup jsrMandatoryResources Low-level device resources porting API
 * @ingroup JSR238
 * 
 * Porting interface for native implementation of device resource access functionality.
 * 
 * @{
 */

/**
 * Gets the number of supported locales for device resources.
 *
 * @return the number of supported locales or 0 if something is wrong
 */
int javacall_mi18n_get_resource_locales_count();

/**
 * Gets a locale name for device resources for the index.
 *
 * @param loc    buffer for the locale.
 * @param len    buffer length
 * @param index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_resource_locale(char* loc, int len, int index);

/**
 * Gets a resource for pointed reource identifier and locale.
 *
 * @param resource      buffer for the resource.
 * @param res_len       length of the resource buffer.
 * @param resource_id   resource identifier.
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_resource(char* resource, int res_len, 
                                         int resource_id, int locale_index);

/**
 * Gets a resource type for pointed resource identifier and locale.
 *
 * @param resType  resource type.
 * @param resource_id   resource identifier.
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_resource_type(int* resType /* OUT */, 
                                         int resource_id, int locale_index);

/**
 * Checks if resource with given identifier exists.
 *
 * @param resource_id   resource identifier.
 * @param locale_index  index of the locale.
 * @return JAVACALL_TRUE if resource ID is valid and 
 *         JAVACALL_FALSE if something is wrong.
 */
javacall_bool javacall_mi18n_is_valid_resource_id(int resource_id, 
                                            int locale_index);

/**
 * Gets a resource length for pointed reource identifier and locale.
 *
 * @param length  size of the resource (in bytes).
 * @param resource_id   resource identifier.
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_resource_length(int* length /* OUT */, 
                                         int resource_id, int locale_index);

/** @} */

#ifdef __cplusplus
}
#endif

#endif //__javacall_mi18n_resources_h
