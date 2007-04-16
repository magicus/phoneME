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



#include "javacall_pti.h"

/**
 * Called 1st time PTI library is accessed.
 * This function may perform specific PTI initialization functions
 * @retval JAVACALL_OK if support is available
 * @retval JAVACALL_FAIL on error
 */
    javacall_result javacall_pti_init(void){
        return JAVACALL_FAIL;
    }


/**
 * create a new PTI iterator instance 
 * Language dictionary must be set default locale language
 *
 * @return handle of new PTI iterator or <tt>0</tt> on error
 */
javacall_handle javacall_pti_open(void) {
    return 0;
}

/**
 * delete a PTI iterator. 
 *
 * @retval JAVACALL_OK if successful
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_pti_close(javacall_handle handle) {
    return JAVACALL_FAIL;
}

/**
 * Set a dictionary for a PTI iterator. 
 * All newly created iterators are defaultly set to defualt locale language
 * this function is called explicitly
 *
 * @retval JAVACALL_OK if language is supported
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_pti_set_dictionary(javacall_handle handle, 
                                           javacall_pti_dictionary dictionary){
    return JAVACALL_FAIL;
}


/**
 * advance PTI iterator using the next key code
 *
 * @param handle the handle of the iterator 
 * @param keyCode the next key (representing one of the keys '0'-'9')
 *
 * @return JAVACALL_OK if successful, or JAVACALL_FAIL on error
 */
javacall_result javacall_pti_add_key(javacall_handle handle, 
                                    javacall_pti_keycode keyCode){
    return JAVACALL_FAIL;
}

/**
 * Backspace the iterator one key 
 * If current string is empty, has no effect.
 *
 * @param handle the handle of the iterator 
 * @return JAVACALL_OK if successful, or JAVACALL_FAIL on error
 */
javacall_result javacall_pti_backspace(javacall_handle handle){
    return JAVACALL_FAIL;
}

/**
 * Clear all text from the PTI iterator 
 *
 * @param handle the handle of the iterator 
 * @return JAVACALL_OK if successful, or JAVACALL_FAIL on error
 */
javacall_result javacall_pti_clear_all(javacall_handle handle){
    return JAVACALL_FAIL;
}

/**
 * return the current PTI completion option
 *
 * @param handle the handle of the iterator 
 * @param outArray the array to be filled with UNICODE string 
 * @param outStringLen max size of the outArray 
 *
 * @return number of chars returned if successful, or <tt>0</tt> otherwise 
 */
int javacall_pti_completion_get_next(
                    javacall_handle     handle, 
                    javacall_utf16*   outString, 
                    int                 outStringLen){
    return 0;
}


/**
 * see if further completion options exist for the current PTI entry
 *
 * @param handle the handle of the iterator 
 *
 * @retval JAVACALL_OK if more completion options exist
 * @retval JAVACALL_FAIL if no more completion options exist
 */
javacall_result javacall_pti_completion_has_next(javacall_handle handle){
    return JAVACALL_FAIL;
}

/**
 * reset completion options for for the current PTI entry
 * After this call, the function javacall_pti_completion_get_next() will 
 * return all completion options starting from 1st option
 *
 * @param handle the handle of the iterator 
 * @return JAVACALL_OK  if successful, JAVACALL_FAIL otherwise 
 */
javacall_result javacall_pti_completion_rewind(javacall_handle handle){
    return JAVACALL_FAIL;
}







