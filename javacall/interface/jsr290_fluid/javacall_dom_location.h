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

#ifndef __JAVACALL_DOM_LOCATION_H_
#define __JAVACALL_DOM_LOCATION_H_

/**
 * @file javacall_dom_location.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Location
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
 * OR returns the value of the <code>href</code> attribute. 
 *
 * <p>It <span class="rfc2119">must</span> be the absolute URI reference 
 * [<a href="http://www.ietf.org/rfc/rfc2396.txt">RFC2396</a>] that is the
 * Location's current location.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>href</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_href_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_href_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     /* OUT */ javacall_utf16** ret_value,
                                     /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>href</code> attribute. 
 *
 * <p>It <span class="rfc2119">must</span> be the absolute URI reference 
 * [<a href="http://www.ietf.org/rfc/rfc2396.txt">RFC2396</a>] that is the
 * Location's current location.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>href</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_href_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_href_finish(void *context,
                                      /* OUT */ javacall_utf16** ret_value,
                                      /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>hash</code> attribute. 
 *
 * <p>If the absolute URI reference 
 * [<a href="http://www.ietf.org/rfc/rfc2396.txt">RFC2396</a>]
 * for the Location's current location has a fragment identifier, then the
 * value of the <code>hash</code> attribute the value of this attribute
 * <span class="rfc2119">must</span> be the string concatenation of the
 * hash mark (<code>#</code>) and the fragment identifier. Otherwise, the
 * value of the hash attribute <span class="rfc2119">must</span> be the
 * empty string.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>hash</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_hash_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_hash_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     /* OUT */ javacall_utf16** ret_value,
                                     /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>hash</code> attribute. 
 *
 * <p>If the absolute URI reference 
 * [<a href="http://www.ietf.org/rfc/rfc2396.txt">RFC2396</a>]
 * for the Location's current location has a fragment identifier, then the
 * value of the <code>hash</code> attribute the value of this attribute
 * <span class="rfc2119">must</span> be the string concatenation of the
 * hash mark (<code>#</code>) and the fragment identifier. Otherwise, the
 * value of the hash attribute <span class="rfc2119">must</span> be the
 * empty string.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>hash</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_hash_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_hash_finish(void *context,
                                      /* OUT */ javacall_utf16** ret_value,
                                      /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>host</code> attribute.
 *
 * <p>This attribute represents the network host of the Location's URI. If
 * the <code>port</code> attribute is not <code>NULL</code> then the host
 * attribute's value is the concatenation of the <code>hostname</code>
 * attribute, a colon (<code>:</code>) and the <code>port</code> 
 * attribute.</p>
 *
 * <p>If the <code>port</code> attribute is <code>NULL</code> then the
 * <code>host</code> attribute's value is identical to the 
 * <code>hostname</code> attribute.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>host</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_host_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_host_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     /* OUT */ javacall_utf16** ret_value,
                                     /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>host</code> attribute.
 *
 * <p>This attribute represents the network host of the Location's URI. If
 * the <code>port</code> attribute is not <code>NULL</code> then the host
 * attribute's value is the concatenation of the <code>hostname</code>
 * attribute, a colon (<code>:</code>) and the <code>port</code> 
 * attribute.</p>
 *
 * <p>If the <code>port</code> attribute is <code>NULL</code> then the
 * <code>host</code> attribute's value is identical to the 
 * <code>hostname</code> attribute.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>host</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_host_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_host_finish(void *context,
                                      /* OUT */ javacall_utf16** ret_value,
                                      /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>hostname</code> attribute. 
 *
 * <p>This attribute represents the name or IP address of the network
 * location without any port number.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>hostname</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_hostname_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_hostname_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         /* OUT */ javacall_utf16** ret_value,
                                         /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>hostname</code> attribute. 
 *
 * <p>This attribute represents the name or IP address of the network
 * location without any port number.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>hostname</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_hostname_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_hostname_finish(void *context,
                                          /* OUT */ javacall_utf16** ret_value,
                                          /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>pathname</code> attribute.
 * 
 * <p>This attribute represents the path component of the Location's URI
 * which consists of everything after the host and port up to and excluding
 * the first question mark (<code>?</code>) or hash mark (<code>#</code>).
 * </p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>pathname</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_pathname_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_pathname_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         /* OUT */ javacall_utf16** ret_value,
                                         /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>pathname</code> attribute.
 * 
 * <p>This attribute represents the path component of the Location's URI
 * which consists of everything after the host and port up to and excluding
 * the first question mark (<code>?</code>) or hash mark (<code>#</code>).
 * </p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>pathname</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_pathname_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_pathname_finish(void *context,
                                          /* OUT */ javacall_utf16** ret_value,
                                          /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>port</code> attribute.
 *
 * <p>This attribute represents the port number of the network 
 * location.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>port</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_port_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_port_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     /* OUT */ javacall_utf16** ret_value,
                                     /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>port</code> attribute.
 *
 * <p>This attribute represents the port number of the network 
 * location.</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>port</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_port_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_port_finish(void *context,
                                      /* OUT */ javacall_utf16** ret_value,
                                      /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>protocol</code> attribute.
 *
 * <p>This attribute represents the scheme of the URI including the
 * trailing colon (<code>:</code>)</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>protocol</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_protocol_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_protocol_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         /* OUT */ javacall_utf16** ret_value,
                                         /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>protocol</code> attribute.
 *
 * <p>This attribute represents the scheme of the URI including the
 * trailing colon (<code>:</code>)</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>protocol</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_protocol_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_protocol_finish(void *context,
                                          /* OUT */ javacall_utf16** ret_value,
                                          /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>search</code> attribute.
 *
 * <p>This attribute represents the query portion of a URI. It consists 
 * of everything after the pathname up to and excluding the first hash
 * mark (<code>#</code>).</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this location.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>search</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_search_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_search_start(javacall_handle handle,
                                       javacall_int32 invocation_id,
                                       void **context,
                                       /* OUT */ javacall_utf16** ret_value,
                                       /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>search</code> attribute.
 *
 * <p>This attribute represents the query portion of a URI. It consists 
 * of everything after the pathname up to and excluding the first hash
 * mark (<code>#</code>).</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>search</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_location_get_search_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_location_get_search_finish(void *context,
                                        /* OUT */ javacall_utf16** ret_value,
                                        /* INOUT */ javacall_uint32* ret_value_len);

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
javacall_dom_location_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_LOCATION_H_ */
