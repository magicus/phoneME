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
 * Returns  <code>keyIdentifier</code> holds the identifier of the key. The key 
 * identifiers are defined in Appendix A.2 <a 
 * href="http://www.w3.org/TR/DOM-Level-3-Events/keyset.html#KeySet-Set">
 * "Key identifiers set"</a>. Implementations that are 
 * unable to identify a key must use the key identifier 
 * <code>"Unidentified"</code>. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in ret_value_len,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_key_identifier(javacall_handle handle,
                                              /* OUT */ javacall_utf16_string ret_value,
                                              /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Returns  The <code>keyLocation</code> attribute contains an indication of the 
 * location of they key on the device, as described in <a 
 * href="http://www.w3.org/TR/DOM-Level-3-Events/events.html#ID-KeyboardEvent-KeyLocationCode">
 * Keyboard event types</a>. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_key_location(javacall_handle handle,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Returns  <code>true</code> if the control (Ctrl) key modifier is activated. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_ctrl_key(javacall_handle handle,
                                        /* OUT */ javacall_bool* ret_value);

/**
 * Returns  <code>true</code> if the shift (Shift) key modifier is activated. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_shift_key(javacall_handle handle,
                                         /* OUT */ javacall_bool* ret_value);

/**
 * Returns  <code>true</code> if the alternative (Alt) key modifier is activated. 
 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_alt_key(javacall_handle handle,
                                       /* OUT */ javacall_bool* ret_value);

/**
 * Returns  <code>true</code> if the meta (Meta) key modifier is activated. 
 * <p ><b>Note:</b>  The Command key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_get_meta_key(javacall_handle handle,
                                        /* OUT */ javacall_bool* ret_value);

/**
 *  The <code>initKeyboardEvent</code> method is used to initialize the 
 * value of a <code>KeyboardEvent</code> object and has the same 
 * behavior as <code>UIEvent.initUIEvent()</code>. The value of 
 * <code>UIEvent.detail</code> remains undefined. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param type_arg  Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
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
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_init_keyboard_event(javacall_handle handle,
                                               javacall_const_utf16_string type_arg,
                                               javacall_bool can_bubble_arg,
                                               javacall_bool cancelable_arg,
                                               javacall_const_utf16_string key_identifier_arg,
                                               javacall_int32 key_location_arg,
                                               javacall_bool ctrl_key,
                                               javacall_bool alt_key,
                                               javacall_bool shift_key,
                                               javacall_bool meta_key);

/**
 *  The <code>initKeyboardEventNS</code> method is used to initialize the 
 * value of a <code>KeyboardEvent</code> object and has the same 
 * behavior as <code>UIEvent.initUIEventNS()</code>. The value of 
 * <code>UIEvent.detail</code> remains undefined. 
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * @param namespace_uri_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>UIEvent.initUIEventNS()</code>
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
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_init_keyboard_event_ns(javacall_handle handle,
                                                  javacall_const_utf16_string namespace_uri_arg,
                                                  javacall_const_utf16_string type_arg,
                                                  javacall_bool can_bubble_arg,
                                                  javacall_bool cancelable_arg,
                                                  javacall_const_utf16_string key_identifier_arg,
                                                  javacall_int32 key_location_arg,
                                                  javacall_bool ctrl_key,
                                                  javacall_bool alt_key,
                                                  javacall_bool shift_key,
                                                  javacall_bool meta_key);

/** 
 * Deletes object representing this keyboardevent
 * 
 * @param handle Pointer to the object representing this keyboardevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_keyboardevent_finalize(javacall_handle handle);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_KEYBOARDEVENT_H_ */
