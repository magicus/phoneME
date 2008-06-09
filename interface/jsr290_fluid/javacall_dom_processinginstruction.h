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

#ifndef __JAVACALL_DOM_PROCESSINGINSTRUCTION_H_
#define __JAVACALL_DOM_PROCESSINGINSTRUCTION_H_

/**
 * @file javacall_dom_processinginstruction.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for ProcessingInstruction
 */

#include <javacall_dom.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the target of this processing instruction. XML defines this as being 
 * the first token following the markup that begins the processing 
 * instruction.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this processinginstruction.
 * @param retValue The target of this processing instruction.
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_processinginstruction_get_target(javacall_handle handle,
                                              /* OUT */ javacall_utf16_string retValue,
                                              /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns the content of this processing instruction. This is from the first non 
 * white space character after the target to the character immediately 
 * preceding the <code>?&gt;</code>.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this processinginstruction.
 * @param retValue The content of this processing instruction
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_processinginstruction_get_data(javacall_handle handle,
                                            /* OUT */ javacall_utf16_string retValue,
                                            /* INOUT */ javacall_uint32* retValueLen);

/**
 * Sets the content of this processing instruction. This is from the first non 
 * white space character after the target to the character immediately 
 * preceding the <code>?&gt;</code>.
 * 
 * @param handle Pointer to the object representing this processinginstruction.
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
javacall_dom_processinginstruction_set_data(javacall_handle handle,
                                            javacall_const_utf16_string data,
                                            /* OUT */ javacall_dom_exceptions* exceptionCode);

/** 
 * Deletes object representing this processinginstruction
 * 
 * @param handle Pointer to the object representing this processinginstruction.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_processinginstruction_finalize(javacall_handle handle);


#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_PROCESSINGINSTRUCTION_H_ */
