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

#include <javacall_dom.h>

/**
 * Returns the public identifier associated with the entity, if specified. If the 
 * public identifier was not specified, this is <code>NULL</code>.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this entity.
 * @param retValue The public identifier associated with the entity, or <code>NULL</code>
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_entity_get_public_id(javacall_handle handle,
                                  /* OUT */ javacall_utf16_string retValue,
                                  /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns the system identifier associated with the entity, if specified. If the 
 * system identifier was not specified, this is <code>NULL</code>.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this entity.
 * @param retValue The system identifier associated with the entity or <code>NULL</code>
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_entity_get_system_id(javacall_handle handle,
                                  /* OUT */ javacall_utf16_string retValue,
                                  /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns for unparsed entities, the name of the notation for the entity. For 
 * parsed entities, this is <code>NULL</code>. 
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this entity.
 * @param retValue the name of the notation for the entity or <code>NULL</code>
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_entity_get_notation_name(javacall_handle handle,
                                      /* OUT */ javacall_utf16_string retValue,
                                      /* INOUT */ javacall_uint32* retValueLen);

/** 
 * Deletes object representing this entity
 * 
 * @param handle Pointer to the object representing this entity.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_entity_finalize(javacall_handle handle);

