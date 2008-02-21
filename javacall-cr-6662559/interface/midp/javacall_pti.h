/*
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
 */ 
#ifndef __JAVACALL_PTI_H
#define __JAVACALL_PTI_H

/**
 * @file javacall_pti.h
 * @ingroup Input
 * @brief Javacall interfaces for PTI input method
 */

#include "javacall_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup PTIOptional PTI Predictive Input Support API
 * @ingroup Input
 *
 * PTI is a codename for predictive text entry for small mobile devices. 
 * By using PTI input, users can enter words (presumably) with a single key press for each character. 
 * For example, to write the word ¡°how¡±, user can press once ¡°4¡±, ¡°6¡± and ¡°9¡± , 
 * eliminating the need for repeated key presses: ¡°4¡±,¡±4¡±,¡±6¡±,¡±6¡±¡±6¡±, and ¡°9¡± 
 * in standard text entry mode. 
 *
 * PTI is just another text input mode, similar to upper-case text entry mode, lowercase text entry mode, 
 * numerical text entry mode and foreign language text entry mode (Hebrew, Greek etc.)
 * See site http://www.pti.com/  for additional information.
 *
 * PTI Input mechanism is not mandatory for JTWI compliance. However, 
 * it may improve handset user's experience. PTI API assume that the handset device already has PTI dictionary 
 * functionality, hence the API is used to expose the dictionaries for Java's use.
 * @{
 */

/**
 * @enum javacall_pti_keycode
 * @brief PTI input keycode
 */
typedef enum {
    /** PTI Key 1 */
    JAVACALL_PTI_KEYCODE_1=49, //'1'
    /** PTI Key 2 */
    JAVACALL_PTI_KEYCODE_2=50, //'2'
    /** PTI Key 3 */
    JAVACALL_PTI_KEYCODE_3=51, //'3'
    /** PTI Key 4 */
    JAVACALL_PTI_KEYCODE_4=52, //'4'
    /** PTI Key 5 */
    JAVACALL_PTI_KEYCODE_5=53, //'5'
    /** PTI Key 6 */
    JAVACALL_PTI_KEYCODE_6=54, //'6',
    /** PTI Key 7 */
    JAVACALL_PTI_KEYCODE_7=55, //'7'
    /** PTI Key 8 */
    JAVACALL_PTI_KEYCODE_8=56, //'8'
    /** PTI Key 9 */
    JAVACALL_PTI_KEYCODE_9=57, //'9'
    /** PTI Key 0 */
    JAVACALL_PTI_KEYCODE_0=48  //'0'
} javacall_pti_keycode;

/**
 * @enum javacall_pti_dictionary
 * 
 * @brief Language Dictionary for PTI input
 */
typedef enum {
    /** PTI dictionary for English */
    JAVACALL_PTI_DICTIONARY_ENGLISH=101,
    /** PTI dictionary for Spanish */
    JAVACALL_PTI_DICTIONARY_SPANISH=102
} javacall_pti_dictionary;

/**
 * Called 1st time PTI library is accessed.
 * This function may perform specific PTI initialization functions
 * @retval JAVACALL_OK if support is available
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_pti_init(void);


/**
 * create a new PTI iterator instance 
 * Language dictionary must be set default locale language
 *
 * @return handle of new PTI iterator or <tt>0</tt> on error
 */
javacall_handle javacall_pti_open(void);

/**
 * delete a PTI iterator. 
 *
 * @retval JAVACALL_OK if successful
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_pti_close(javacall_handle handle);

/**
 * Set a dictionary for a PTI iterator. 
 * All newly created iterators are defaultly set to defualt locale language
 * this function is called explicitly
 *
 * @param handle the handle of the iterator 
 * @param dictionary the dictionary for the text prediction
 *
 * @retval JAVACALL_OK if language is supported
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_pti_set_dictionary(javacall_handle handle, 
                                         javacall_pti_dictionary);


/**
 * advance PTI iterator using the next key code
 *
 * @param handle the handle of the iterator 
 * @param keyCode the next key (representing one of the keys '0'-'9')
 *
 * @return JAVACALL_OK if successful, or JAVACALL_FAIL on error
 */
javacall_result javacall_pti_add_key(javacall_handle handle, 
                                    javacall_pti_keycode keyCode);

/**
 * Backspace the iterator one key 
 * If current string is empty, has no effect.
 *
 * @param handle the handle of the iterator 
 * @return JAVACALL_OK if successful, or JAVACALL_FAIL on error
 */
javacall_result javacall_pti_backspace(javacall_handle handle);

/**
 * Clear all text from the PTI iterator 
 *
 * @param handle the handle of the iterator 
 * @return JAVACALL_OK if successful, or JAVACALL_FAIL on error
 */
javacall_result javacall_pti_clear_all(javacall_handle handle);

/**
 * return the current PTI completion option
 *
 * @param handle the handle of the iterator 
 * @param outString the array to be filled with UNICODE string 
 * @param outStringLen max size of the outArray 
 *
 * @return number of chars returned if successful, or <tt>0</tt> otherwise 
 */
int javacall_pti_completion_get_next(
                    javacall_handle     handle, 
                    javacall_utf16*   outString, 
                    int                 outStringLen);


/**
 * see if further completion options exist for the current PTI entry
 *
 * @param handle the handle of the iterator 
 *
 * @retval JAVACALL_OK if more completion options exist
 * @retval JAVACALL_FAIL if no more completion options exist
 */
javacall_result javacall_pti_completion_has_next(javacall_handle handle);

/**
 * reset completion options for for the current PTI entry
 * After this call, the function javacall_pti_completion_get_next() will 
 * return all completion options starting from 1st option
 *
 * @param handle the handle of the iterator 
 * @return JAVACALL_OK  if successful, JAVACALL_FAIL otherwise 
 */
javacall_result javacall_pti_completion_rewind(javacall_handle handle);


/** @} */


#ifdef __cplusplus
} //extern "C"
#endif

#endif




