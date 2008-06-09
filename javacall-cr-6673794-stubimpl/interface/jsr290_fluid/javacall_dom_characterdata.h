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

#ifndef __JAVACALL_DOM_CHARACTERDATA_H_
#define __JAVACALL_DOM_CHARACTERDATA_H_

/**
 * @file javacall_dom_characterdata.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for CharacterData
 */

#include <javacall_dom.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the character data of the node that implements this interface. The DOM 
 * implementation may not put arbitrary limits on the amount of data 
 * that may be stored in a <code>CharacterData</code> node. However, 
 * implementation limits may mean that the entirety of a node's data may 
 * not fit into a single <code>DOMString</code>. In such cases, the user 
 * may call <code>substringData</code> to retrieve the data in 
 * appropriately sized pieces.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this characterdata.
 * @param retValue The specified substring.
 * @param retValueLen Length of the returned string
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
 *                                filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_get_data(javacall_handle handle,
                                    /* OUT */ javacall_utf16_string retValue,
                                    /* INOUT */ javacall_uint32* retValueLen,
                                    /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Sets the character data of the node that implements this interface. The DOM 
 * implementation may not put arbitrary limits on the amount of data 
 * that may be stored in a <code>CharacterData</code> node. However, 
 * implementation limits may mean that the entirety of a node's data may 
 * not fit into a single <code>DOMString</code>. In such cases, the user 
 * may call <code>substringData</code> to retrieve the data in 
 * appropriately sized pieces.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param data character data to add to the node, may not be NULL
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_set_data(javacall_handle handle,
                                    javacall_const_utf16_string data,
                                    /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns the number of 16-bit units that are available through <code>data</code> 
 * and the <code>substringData</code> method below. This may have the 
 * value zero, i.e., <code>CharacterData</code> nodes may be empty.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param retValue the number of 16-bit units available in the CharacterData
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_get_length(javacall_handle handle,
                                      /* OUT */ javacall_int32* retValue);

/**
 * Extracts a range of data from the node.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this characterdata.
 * @param offset Start offset of substring to extract.
 * @param count The number of 16-bit units to extract.
 * @param retValue The specified substring. If the sum of <code>offset</code> and 
 *   <code>count</code> exceeds the <code>length</code>, then all 16-bit 
 *   units to the end of the data are returned.
 * @param retValueLen Length of the returned string
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
 *                                filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_substring_data(javacall_handle handle,
                                          javacall_int32 offset,
                                          javacall_int32 count,
                                          /* OUT */ javacall_utf16_string retValue,
                                          /* INOUT */ javacall_uint32* retValueLen,
                                          /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Append the string to the end of the character data of the node. Upon 
 * success, <code>data</code> provides access to the concatenation of 
 * <code>data</code> and the <code>DOMString</code> specified.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param arg The <code>DOMString</code> to append.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_append_data(javacall_handle handle,
                                       javacall_const_utf16_string arg,
                                       /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Insert a string at the specified 16-bit unit offset.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param offset The character offset at which to insert.
 * @param arg The <code>DOMString</code> to insert.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_insert_data(javacall_handle handle,
                                       javacall_int32 offset,
                                       javacall_const_utf16_string arg,
                                       /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Remove a range of 16-bit units from the node. Upon success, 
 * <code>data</code> and <code>length</code> reflect the change.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param offset The offset from which to start removing.
 * @param count The number of 16-bit units to delete. If the sum of 
 *   <code>offset</code> and <code>count</code> exceeds 
 *   <code>length</code> then all 16-bit units from <code>offset</code> 
 *   to the end of the data are deleted.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_delete_data(javacall_handle handle,
                                       javacall_int32 offset,
                                       javacall_int32 count,
                                       /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Replace the characters starting at the specified 16-bit unit offset 
 * with the specified string.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param offset The offset from which to start replacing.
 * @param count The number of 16-bit units to replace. If the sum of 
 *   <code>offset</code> and <code>count</code> exceeds 
 *   <code>length</code>, then all 16-bit units to the end of the data 
 *   are replaced; (i.e., the effect is the same as a <code>remove</code>
 *    method call with the same range, followed by an <code>append</code>
 *    method invocation).
 * @param arg The <code>DOMString</code> with which the range must be 
 *   replaced.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_replace_data(javacall_handle handle,
                                        javacall_int32 offset,
                                        javacall_int32 count,
                                        javacall_const_utf16_string arg,
                                        /* OUT */ javacall_dom_exceptions* exceptionCode);

/** 
 * Deletes object representing this characterdata
 * 
 * @param handle Pointer to the object representing this characterdata.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_finalize(javacall_handle handle);


#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_CHARACTERDATA_H_ */
