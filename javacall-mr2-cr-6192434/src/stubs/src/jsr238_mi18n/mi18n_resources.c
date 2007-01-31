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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

/*
Implementation of low-level porting API for JSR 238 (MI18N).
*/

/**
 * Gets the number of supported locales for string collation.
 * @param count_out pointer to integer that recieves
 *					the number of supported locales
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource_locales_count(/*OUT*/int* count_out){
	(void)count_out;
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets a locale name for device resources for the index.
 *
 * @param locale_index  index of the locale.
 * @param locale_name_out  buffer for the locale.
 * @param plen	pointer to integer initially containing the buffer length
 *				and receiving length of result string in wchars including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
*/
javacall_result javacall_mi18n_get_resource_locale_name(int locale_index, javacall_utf16* locale_name_out,
							/*IN|OUT*/int* plen)
{
	(void)locale_index;
	(void)locale_name_out;
	(void)plen;
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets locale index used for accessing device resources by the given miroedition.locale name.
 *
 * @param loc    utf16 string containing requested locale name or null for neutral locale
 * @param index_out	pointer to integer receiving index of requested locale,
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If neutral (empty string) locale is supported it must have index 0
 */
javacall_result javacall_mi18n_get_resource_locale_index(const javacall_utf16* locale, /*OUT*/int* index_out)
{
	(void)locale;
	(void)index_out;
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets a resource data for pointed reource identifier and locale.
 * String resources must be UTF8 encoded
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param resource      buffer for the resource.
 * @param offset	offset of first byte of resource data 
 * @param length	pointer to integer that set to desired number of bytes to copy from resource data
 *					receiving length of copied resource data in bytes, 
 *					if length is less than remaining resource size only length bytes are copied		
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource(int locale_index, int resource_id,
						/* OUT */char* resource, int offset, /*IN|OUT*/ int* length){
	(void)locale_index;
	(void)resource_id;
	(void)resource;
	(void)offset;
	(void)length;
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets a resource type for pointed resource identifier and locale.
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param resType  (out) resource type.
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource_type(int locale_index,int resource_id,
												 /* OUT */int* resType ) {
	(void)locale_index;
	(void)resource_id;
	(void)resType;
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Checks if resource with given identifier exists.
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param result	poiner to boolean that is set to
					JAVACALL_TRUE if resource ID is valid, 
					JAVACALL_FALSE if resource ID is not valid
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_is_valid_resource_id(int locale_index,int resource_id,javacall_bool* result){
	(void)locale_index;
	(void)resource_id;
	(void)result;
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Gets a resource length for pointed reource identifier and locale.
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param length  (out) size of the resource (in bytes).
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource_length(int locale_index, int resource_id,
						   /* OUT */int* length) {
	(void)locale_index;
	(void)resource_id;
	(void)length;
    return JAVACALL_NOT_IMPLEMENTED;
}



#ifdef __cplusplus
}
#endif
