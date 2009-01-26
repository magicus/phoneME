/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#ifdef __cplusplus
extern "C" {
#endif

#include <javacall_dom.h>

/**
 * @defgroup JSR290DOM JSR290 DOM API
 *
 * The following API definitions are required by DOM part of the JSR-290.
 *
 * @{
 */

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the character data of the node that implements this interface. The DOM 
 * implementation may not put arbitrary limits on the amount of data 
 * that may be stored in a <code>CharacterData</code> node. However, 
 * implementation limits may mean that the entirety of a node's data may 
 * not fit into a single <code>DOMString</code>. In such cases, the user 
 * may call <code>substringData</code> to retrieve the data in 
 * appropriately sized pieces.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The specified substring.
 * @param ret_value_len Number of code_units of the returned string
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_DOMSTRING_SIZE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_DOMSTRING_SIZE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_get_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_get_data_start(javacall_handle handle,
                                          javacall_int32 invocation_id,
                                          void **context,
                                          /* OUT */ javacall_utf16** ret_value,
                                          /* INOUT */ javacall_uint32* ret_value_len,
                                          /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the character data of the node that implements this interface. The DOM 
 * implementation may not put arbitrary limits on the amount of data 
 * that may be stored in a <code>CharacterData</code> node. However, 
 * implementation limits may mean that the entirety of a node's data may 
 * not fit into a single <code>DOMString</code>. In such cases, the user 
 * may call <code>substringData</code> to retrieve the data in 
 * appropriately sized pieces.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The specified substring.
 * @param ret_value_len Number of code_units of the returned string
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_DOMSTRING_SIZE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_DOMSTRING_SIZE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_get_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_get_data_finish(void *context,
                                           /* OUT */ javacall_utf16** ret_value,
                                           /* INOUT */ javacall_uint32* ret_value_len,
                                           /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets the character data of the node that implements this interface. The DOM 
 * implementation may not put arbitrary limits on the amount of data 
 * that may be stored in a <code>CharacterData</code> node. However, 
 * implementation limits may mean that the entirety of a node's data may 
 * not fit into a single <code>DOMString</code>. In such cases, the user 
 * may call <code>substringData</code> to retrieve the data in 
 * appropriately sized pieces.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param data character data to add to the node, may not be NULL
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_SYNTAX_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_SYNTAX_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_set_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_set_data_start(javacall_handle handle,
                                          javacall_int32 invocation_id,
                                          void **context,
                                          javacall_const_utf16_string data,
                                          /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets the character data of the node that implements this interface. The DOM 
 * implementation may not put arbitrary limits on the amount of data 
 * that may be stored in a <code>CharacterData</code> node. However, 
 * implementation limits may mean that the entirety of a node's data may 
 * not fit into a single <code>DOMString</code>. In such cases, the user 
 * may call <code>substringData</code> to retrieve the data in 
 * appropriately sized pieces.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_SYNTAX_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_SYNTAX_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_set_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_set_data_finish(void *context,
                                           /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the number of 16-bit units that are available through <code>data</code> 
 * and the <code>substringData</code> method below. This may have the 
 * value zero, i.e., <code>CharacterData</code> nodes may be empty.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value the number of 16-bit units available in the CharacterData
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_get_length_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_get_length_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the number of 16-bit units that are available through <code>data</code> 
 * and the <code>substringData</code> method below. This may have the 
 * value zero, i.e., <code>CharacterData</code> nodes may be empty.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value the number of 16-bit units available in the CharacterData
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_get_length_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_get_length_finish(void *context,
                                             /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR extracts a range of data from the node.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param offset Start offset of substring to extract.
 * @param count The number of 16-bit units to extract.
 * @param ret_value The specified substring. If the sum of <code>offset</code> and 
 *   <code>count</code> exceeds the <code>length</code>, then all 16-bit 
 *   units to the end of the data are returned.
 * @param ret_value_len Number of code_units of the returned string
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_DOMSTRING_SIZE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_DOMSTRING_SIZE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_substring_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_substring_data_start(javacall_handle handle,
                                                javacall_int32 invocation_id,
                                                void **context,
                                                javacall_int32 offset,
                                                javacall_int32 count,
                                                /* OUT */ javacall_utf16** ret_value,
                                                /* INOUT */ javacall_uint32* ret_value_len,
                                                /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR extracts a range of data from the node.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The specified substring. If the sum of <code>offset</code> and 
 *   <code>count</code> exceeds the <code>length</code>, then all 16-bit 
 *   units to the end of the data are returned.
 * @param ret_value_len Number of code_units of the returned string
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_DOMSTRING_SIZE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_DOMSTRING_SIZE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_substring_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_substring_data_finish(void *context,
                                                 /* OUT */ javacall_utf16** ret_value,
                                                 /* INOUT */ javacall_uint32* ret_value_len,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR append the string to the end of the character data of the node. Upon 
 * success, <code>data</code> provides access to the concatenation of 
 * <code>data</code> and the <code>DOMString</code> specified.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param arg The <code>DOMString</code> to append.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_append_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_append_data_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             javacall_const_utf16_string arg,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR append the string to the end of the character data of the node. Upon 
 * success, <code>data</code> provides access to the concatenation of 
 * <code>data</code> and the <code>DOMString</code> specified.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_append_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_append_data_finish(void *context,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR insert a string at the specified 16-bit unit offset.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param offset The character offset at which to insert.
 * @param arg The <code>DOMString</code> to insert.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_insert_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_insert_data_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             javacall_int32 offset,
                                             javacall_const_utf16_string arg,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR insert a string at the specified 16-bit unit offset.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_insert_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_insert_data_finish(void *context,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR remove a range of 16-bit units from the node. Upon success, 
 * <code>data</code> and <code>length</code> reflect the change.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param offset The offset from which to start removing.
 * @param count The number of 16-bit units to delete. If the sum of 
 *   <code>offset</code> and <code>count</code> exceeds 
 *   <code>length</code> then all 16-bit units from <code>offset</code> 
 *   to the end of the data are deleted.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_delete_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_delete_data_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             javacall_int32 offset,
                                             javacall_int32 count,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR remove a range of 16-bit units from the node. Upon success, 
 * <code>data</code> and <code>length</code> reflect the change.
 * 
 * @param context The context saved during asynchronous operation.
 *   <code>offset</code> and <code>count</code> exceeds 
 *   <code>length</code> then all 16-bit units from <code>offset</code> 
 *   to the end of the data are deleted.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_delete_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_delete_data_finish(void *context,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR replace the characters starting at the specified 16-bit unit offset 
 * with the specified string.
 * 
 * @param handle Pointer to the object representing this characterdata.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param offset The offset from which to start replacing.
 * @param count The number of 16-bit units to replace. If the sum of 
 *   <code>offset</code> and <code>count</code> exceeds 
 *   <code>length</code>, then all 16-bit units to the end of the data 
 *   are replaced; (i.e., the effect is the same as a <code>remove</code>
 *   method call with the same range, followed by an <code>append</code>
 *   method invocation).
 * @param arg The <code>DOMString</code> with which the range must be 
 *   replaced.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_replace_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_replace_data_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_int32 offset,
                                              javacall_int32 count,
                                              javacall_const_utf16_string arg,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR replace the characters starting at the specified 16-bit unit offset 
 * with the specified string.
 * 
 * @param context The context saved during asynchronous operation.
 *   replaced.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_characterdata_replace_data_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_replace_data_finish(void *context,
                                               /* OUT */ javacall_dom_exceptions* exception_code);

/** 
 * Decrements ref counter of the native object specified number of times
 * 
 * @param handle Pointer to the object representing this node.
 * @param count number of times to decrement.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_characterdata_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_CHARACTERDATA_H_ */
