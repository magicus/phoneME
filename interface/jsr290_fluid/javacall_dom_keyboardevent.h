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

#ifndef __JAVACALL_DOM_KEYBOARDEVENT_H_
#define __JAVACALL_DOM_KEYBOARDEVENT_H_

/**
 * @file javacall_dom_keyboardevent.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for KeyboardEvent
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
 * OR returns  <code>keyIdentifier</code> holds the identifier of the key. The key 
 * identifiers are defined in Appendix A.2 <a 
 * href="http://www.w3.org/TR/DOM-Level-3-Events/keyset.html#KeySet-Set">
 * "Key identifiers set"</a>. Implementations that are 
 * unable to identify a key must use the key identifier 
 * <code>"Unidentified"</code>. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_key_identifier_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_key_identifier_start(javacall_handle handle,
                                                    javacall_int32 invocation_id,
                                                    void **context,
                                                    /* OUT */ javacall_utf16** ret_value,
                                                    /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>keyIdentifier</code> holds the identifier of the key. The key 
 * identifiers are defined in Appendix A.2 <a 
 * href="http://www.w3.org/TR/DOM-Level-3-Events/keyset.html#KeySet-Set">
 * "Key identifiers set"</a>. Implementations that are 
 * unable to identify a key must use the key identifier 
 * <code>"Unidentified"</code>. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_key_identifier_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_key_identifier_finish(void *context,
                                                     /* OUT */ javacall_utf16** ret_value,
                                                     /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The <code>keyLocation</code> attribute contains an indication of the 
 * location of they key on the device, as described in <a 
 * href="http://www.w3.org/TR/DOM-Level-3-Events/events.html#ID-KeyboardEvent-KeyLocationCode">
 * Keyboard event types</a>. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_key_location_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_key_location_start(javacall_handle handle,
                                                  javacall_int32 invocation_id,
                                                  void **context,
                                                  /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The <code>keyLocation</code> attribute contains an indication of the 
 * location of they key on the device, as described in <a 
 * href="http://www.w3.org/TR/DOM-Level-3-Events/events.html#ID-KeyboardEvent-KeyLocationCode">
 * Keyboard event types</a>. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_key_location_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_key_location_finish(void *context,
                                                   /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the control (Ctrl) key modifier is activated. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_ctrl_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_ctrl_key_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the control (Ctrl) key modifier is activated. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_ctrl_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_ctrl_key_finish(void *context,
                                               /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the shift (Shift) key modifier is activated. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_shift_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_shift_key_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the shift (Shift) key modifier is activated. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_shift_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_shift_key_finish(void *context,
                                                /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the alternative (Alt) key modifier is activated. 
 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_alt_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_alt_key_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the alternative (Alt) key modifier is activated. 
 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_alt_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_alt_key_finish(void *context,
                                              /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the meta (Meta) key modifier is activated. 
 * <p ><b>Note:</b>  The Command key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_meta_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_meta_key_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the meta (Meta) key modifier is activated. 
 * <p ><b>Note:</b>  The Command key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_get_meta_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_meta_key_finish(void *context,
                                               /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initKeyboardEvent</code> method is used to initialize the 
 * value of a <code>KeyboardEvent</code> object and has the same 
 * behavior as <code>UIEvent.initUIEvent()</code>. The value of 
 * <code>UIEvent.detail</code> remains undefined. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param type_arg  Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
 * @param view_arg Pointer to the object of
 *    Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param key_identifier_arg  Specifies 
 *   <code>KeyboardEvent.keyIdentifier</code>. 
 * @param key_location_arg  Specifies <code>KeyboardEvent.keyLocation</code>
 *   . 
 * @param modifiersList  A 
 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
 *   </a> separated list of modifier key identifiers to be activated on 
 *   this object. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_init_keyboard_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_init_keyboard_event_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     javacall_const_utf16_string type_arg,
                                                     javacall_bool can_bubble_arg,
                                                     javacall_bool cancelable_arg,
                                                     javacall_handle view_arg,
                                                     javacall_const_utf16_string key_identifier_arg,
                                                     javacall_int32 key_location_arg,
                                                     javacall_bool ctrl_key,
                                                     javacall_bool alt_key,
                                                     javacall_bool shift_key,
                                                     javacall_bool meta_key);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initKeyboardEvent</code> method is used to initialize the 
 * value of a <code>KeyboardEvent</code> object and has the same 
 * behavior as <code>UIEvent.initUIEvent()</code>. The value of 
 * <code>UIEvent.detail</code> remains undefined. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param modifiersList  A 
 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
 *   </a> separated list of modifier key identifiers to be activated on 
 *   this object. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_init_keyboard_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_init_keyboard_event_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initKeyboardEventNS</code> method is used to initialize the 
 * value of a <code>KeyboardEvent</code> object and has the same 
 * behavior as <code>UIEvent.initUIEventNS()</code>. The value of 
 * <code>UIEvent.detail</code> remains undefined. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>UIEvent.initUIEventNS()</code>
 *   method for a description of this parameter. 
 * @param view_arg Pointer to the object of
 *    Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param key_identifier_arg  Refer to the 
 *   <code>KeyboardEvent.initKeyboardEvent()</code> method for a 
 *   description of this parameter. 
 * @param key_location_arg  Refer to the 
 *   <code>KeyboardEvent.initKeyboardEvent()</code> method for a 
 *   description of this parameter. 
 * @param modifiersList  A 
 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
 *   </a> separated list of modifier key identifiers to be activated on 
 *   this object. As an example, <code>"Control Alt"</code> will activated 
 *   the control and alt modifiers. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_init_keyboard_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_init_keyboard_event_ns_start(javacall_handle handle,
                                                        javacall_int32 invocation_id,
                                                        void **context,
                                                        javacall_const_utf16_string namespace_uri_arg,
                                                        javacall_const_utf16_string type_arg,
                                                        javacall_bool can_bubble_arg,
                                                        javacall_bool cancelable_arg,
                                                        javacall_handle view_arg,
                                                        javacall_const_utf16_string key_identifier_arg,
                                                        javacall_int32 key_location_arg,
                                                        javacall_bool ctrl_key,
                                                        javacall_bool alt_key,
                                                        javacall_bool shift_key,
                                                        javacall_bool meta_key);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initKeyboardEventNS</code> method is used to initialize the 
 * value of a <code>KeyboardEvent</code> object and has the same 
 * behavior as <code>UIEvent.initUIEventNS()</code>. The value of 
 * <code>UIEvent.detail</code> remains undefined. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param modifiersList  A 
 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
 *   </a> separated list of modifier key identifiers to be activated on 
 *   this object. As an example, <code>"Control Alt"</code> will activated 
 *   the control and alt modifiers. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_keyboardevent_init_keyboard_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_init_keyboard_event_ns_finish(void *context);

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
javacall_dom_keyboardevent_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_KEYBOARDEVENT_H_ */
