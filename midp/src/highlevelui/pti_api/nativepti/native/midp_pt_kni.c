/*
 *  
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
 * @file 
 *
 * Manage predictive text KNI Support
 */

#include <string.h>
#include <kni.h>
#include <sni.h>
#include <midpUtilKni.h>
#include <pt_impl.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>
#include <midpError.h>
#include <midpMalloc.h>

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
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_midp_chameleon_input_PTDictionaryImpl_ptInitLibrary0() {
    jint handle = 0;

    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(1, lang);
    handle = ptInitLibrary(&lang);
    RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();
    KNI_ReturnInt(handle);
}

/**
 * Create a new iterator instance. 
 *
 * @param lib library handle. If library handle is not valid 0 is returned
 * as an iterator handle.
 *
 * @return handle of new iterator. If some error occurs 0 is returned
 * as an iterator handle. 
 */
KNIEXPORT KNI_RETURNTYPE_INT 
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptNewIterator0() {
    jint lib;
    lib = KNI_GetParameterAsInt(1);
    KNI_ReturnInt(ptOpenIterator(lib));
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
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptClear0() {
    jint handle;
    handle = KNI_GetParameterAsInt(1);
    KNI_ReturnBoolean(ptClear(handle));
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
KNIEXPORT KNI_RETURNTYPE_BOOLEAN 
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptAddKey0() {
    jint handle = 0;
    jint keyCode;
    keyCode = KNI_GetParameterAsInt(2);
    handle = KNI_GetParameterAsInt(1);
    KNI_ReturnBoolean(ptAddKey(handle, keyCode));
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
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptDeleteKey0() {
    jint handle;
    handle = KNI_GetParameterAsInt(1);
    KNI_ReturnBoolean(ptDeleteKey(handle));
}

/**
 * Return the current predictive text completion option
 *
 * @param handle the handle of the iterator. If iterator handle is not valid
 * function returns null.
 * @param outStringLen max size of the outArray. If size is equal or less than
 * 0 function returns null    
 *
 * @return next element in the iteration
 */
KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptNextCompletionOption0() {
    jint result = 0;
    jint handle = 0;
    jint outStringLen;
    jchar nextCompletion[256] = {0};
    jsize resultLength;
    KNI_StartHandles(1);
    KNI_DeclareHandle(string);
    handle = (int)KNI_GetParameterAsInt(1);
    outStringLen = (int)KNI_GetParameterAsInt(2) - 1;
   
    result = ptNextCompletionOption(handle, nextCompletion, outStringLen);
    if (result == KNI_TRUE) {
        for (resultLength = 0; nextCompletion[resultLength]; resultLength++) {}
        KNI_NewString(nextCompletion, resultLength, string);
    } else {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }
    KNI_EndHandlesAndReturnObject(string);
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
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptHasCompletionOption0() {
    jint handle;
    handle = KNI_GetParameterAsInt(1);
    KNI_ReturnBoolean(ptHasCompletionOption(handle)); 
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
KNIEXPORT KNI_RETURNTYPE_BOOLEAN 
Java_com_sun_midp_chameleon_input_PTIteratorImpl_ptRenewCompletionOptions0() {
    jint handle;
    handle = KNI_GetParameterAsInt(1);
    KNI_ReturnBoolean(ptRenewCompletionOptions(handle));
}
