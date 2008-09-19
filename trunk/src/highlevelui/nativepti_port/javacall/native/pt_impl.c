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

/**
 * @file pt_impl.c
 *
 * Manage predictive text Support
 */

#include <pcsl_string.h>
#include <kni.h>
#include <pt_impl.h>
#include <javacall_pti.h>

/**
 * Called 1st time predictive text library is accessed.
 * it calls platform specific predictive text initialization functions.
 * If library has been already initialized it's just initialized once again. 
 * It can not guarantee the returned handler will be identical to the
 * previous one. 
 *
 * @param lang the language used to select the library. If the language is
 * NULL or it's not supported this function returns handle for the default
 * language library. 
 * @return the handle of the library. If some error occurs 0 is returned
 * as a library handle. 
 */
jint ptInitLibrary(const pcsl_string * lang) {

    javacall_result result = javacall_pti_init(); 

    (void)lang; //##
    return 1;
}

/**
 * Clear all text from the predictive text iterator 
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns false
 *
 * @return true if iterator has been cleared successfully otherwise false.
 * If function returns false the iterator state is not changed.  
 */
jboolean ptClear(jint handle) {

    javacall_result result = javacall_pti_clear_all((javacall_handle)handle);
    return (result == JAVACALL_OK) ? KNI_TRUE : KNI_FALSE;
}

/**
 * Open a new iterator for the particular labrary. The implementation can
 * support a limited amount of active iterators. If limit was exceeded 0
 * is returned as an iterator handle.
 *
 * @param lib library handle. If library handle is not valid 0 is returned
 * as an iterator handle.
 *
 * @return handle of new iterator. If some error occurs 0 is returned
 * as an iterator handle. 
 */
jint ptOpenIterator(jint lib) {

    /* default dictionary language is English, change to any if necessary */
    javacall_handle handle = javacall_pti_open();

    (void)lib; //##
    return (jint)handle;
}

/**
 * Close the iterator. 
 *
 * @param handle iterator handle. If iterator handle is not valid false
 * is returned
 *
 * @return true if iterator has been closed successfully otherwise false.
 */
jboolean ptCloseIterator(jint handle) {

    javacall_result result = javacall_pti_close((javacall_handle)handle);
    return (result == JAVACALL_OK) ? KNI_TRUE : KNI_FALSE;
}
 
/**
 * Advances the predictive text iterator using the next key code
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns false. 
 *
 * @param keyCode the next key ('0'-'9'). If keyCode is not valid function
 * returns false. 
 *
 * @return true if key code has been added successfully otherwise false.
 * If function returns false the iterator state is not changed.  
 */
jboolean ptAddKey(jint handle, jint keyCode) {

    javacall_result result = javacall_pti_add_key((javacall_handle)handle, (javacall_pti_keycode)keyCode);
    return (result == JAVACALL_OK) ? KNI_TRUE : KNI_FALSE;
}

/**
 * Backspace the iterator one key 
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns false. 
 *
 * @return true if key has been deleted successfully otherwise false.
 * If function returns false the iterator state is not changed.  
 */
jboolean ptDeleteKey(jint handle) {

    javacall_result result = javacall_pti_backspace((javacall_handle) handle);
    return (result == JAVACALL_OK) ? KNI_TRUE : KNI_FALSE;
}

/**
 * return the current predictive text completion option
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns false. 
 * @param outArray the byte array to copy the ASCII chars to 
 * @param outStringLen max size of the outArray. If size is equal or less than
 * 0 function returns false
 *
 * @return true if successful, false otherwise. If function returns false,
 * outArray is set as NULL
 */
jboolean ptNextCompletionOption(jint handle, jchar *outString,
                                int outStringLen) {

    int numChars = javacall_pti_completion_get_next(
        (javacall_handle)     handle, 
        (javacall_unicode*)   outString, 
        outStringLen);

    return (numChars > 0) ? KNI_TRUE : KNI_FALSE;
}

/**
 * See if exist further completion options for the current
 * predictive text entry
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns false. 
 *
 * @return true if more completion options exist, false otherwise
 */
jboolean ptHasCompletionOption(jint handle) {

    javacall_result result = javacall_pti_completion_has_next((javacall_handle)handle);
    return (result == JAVACALL_OK) ? KNI_TRUE : KNI_FALSE;
}

/**
 * Reset completion options for for the current predictive text entry
 * After this call, ptNextCompletionOption() will return all
 * completion options starting from 1st one.
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns false. 
 * @return true if iterator has been reset successfully otherwise false.
 * If function returns false the iterator state is not changed.  
 */
jboolean ptRenewCompletionOptions(jint handle) {

    javacall_result result = javacall_pti_completion_rewind((javacall_handle)handle);
    return (result == JAVACALL_OK) ? KNI_TRUE : KNI_FALSE;
}
