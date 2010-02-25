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
 * Cross platform functions for TextField.
 */

#include <kni.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <lfpport_textfield.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>

/** Get a DynamicCharacterArray structure from the given <tt>jobject</tt> */
#define getMidpDynamicCharacterArrayPtr(__handle) \
(unhand(struct Java_com_sun_midp_lcdui_DynamicCharacterArray, __handle))

#define BUFFER_JARRAY_HANDLE(__handle) \
	hand(getMidpDynamicCharacterArrayPtr((__handle))->buffer)

/**
 * KNI function that create native resource for current TextField.
 * Class: javax.microedition.lcdui.TextFieldLFImpl
 * Java prototype:
 * private native int createNativeResource0(int ownerId,
 *	    		String label, int layout, 
 *                      DynamicCharacterArray buffer,
 *			int constraints, String initialInputMode)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_TextFieldLFImpl_createNativeResource0() {
    
    MidpError err = KNI_OK;
    MidpItem *tfPtr = NULL;
    MidpDisplayable *ownerPtr;
    pcsl_string label = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string text = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string inputMode = PCSL_STRING_NULL_INITIALIZER;
    int maxSize, constraints, layout, textLength;

    KNI_StartHandles(4);

    KNI_DeclareHandle(labelJString);
    KNI_DeclareHandle(bufferJDCArray);
    KNI_DeclareHandle(bufferJCharArray);
    KNI_DeclareHandle(inputModeJString);

    ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, labelJString);
    layout = KNI_GetParameterAsInt(3);
    KNI_GetParameterAsObject(4, bufferJDCArray);
    constraints = KNI_GetParameterAsInt(5);
    KNI_GetParameterAsObject(6, inputModeJString);

    /* take the buffer field out of the object */
    bufferJCharArray = BUFFER_JARRAY_HANDLE(bufferJDCArray);
    
    maxSize = KNI_GetArrayLength(bufferJCharArray);
    textLength = getMidpDynamicCharacterArrayPtr(bufferJDCArray)->length;
    err = KNI_ENOMEM;
    if (PCSL_STRING_OK ==
       midp_jchar_array_to_pcsl_string(bufferJCharArray, textLength, &text)) {
        if (PCSL_STRING_OK ==
           midp_jstring_to_pcsl_string(labelJString, &label)) {
            if (PCSL_STRING_OK ==
               midp_jstring_to_pcsl_string(inputModeJString, &inputMode)) {
                err = KNI_OK;
            }
        }
    }
    KNI_EndHandles();


    if (KNI_OK == err) {
        tfPtr = MidpNewItem(ownerPtr, MIDP_TEXT_FIELD_TYPE);
        if (tfPtr == NULL) {
            err = KNI_ENOMEM;
        } else {
            err = lfpport_textfield_create(tfPtr, ownerPtr,
                                           &label, layout,
                                           &text, maxSize,
                                           constraints, &inputMode);
        }
    }
    /*cleanup:*/
    pcsl_string_free(&inputMode);
    pcsl_string_free(&text);
    pcsl_string_free(&label);

    if (err != KNI_OK) {
	    MidpDeleteItem(tfPtr);
    	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(tfPtr);
}

/**
 * KNI function that get updates of contents from native widget.
 * Class: javax.microedition.lcdui.TextFieldLFImpl
 * Java prototype:
 * private native boolean getString0(int nativeId, DynamicCharacterArray buffer)
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_lcdui_TextFieldLFImpl_getString0() {
    pcsl_string v_text;
    pcsl_string *const text=&v_text;
    jboolean newChange;
    MidpItem *tfPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    MidpError err = lfpport_textfield_get_string(text, &newChange, tfPtr);

    if (KNI_OK != err) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        GET_PCSL_STRING_DATA_AND_LENGTH(text)
        KNI_StartHandles(2);

        KNI_DeclareHandle(bufferJDCArray);
        KNI_DeclareHandle(bufferJCharArray);

        KNI_GetParameterAsObject(2, bufferJDCArray);

        bufferJCharArray = BUFFER_JARRAY_HANDLE(bufferJDCArray);
        KNI_SetRawArrayRegion(bufferJCharArray, 0,
                      text_len * sizeof(jchar), (jbyte *)text_data);
        getMidpDynamicCharacterArrayPtr(bufferJDCArray)->length = text_len;

        KNI_EndHandles();
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
        pcsl_string_free(text);
    }

    KNI_ReturnBoolean(newChange);
}

/**
 * KNI function that updates the content and caret position of the TextField's
 * native widget.
 * Class: javax.microedition.lcdui.TextFieldLFImpl
 * Java prototype:
 * private native void setString0(int nativeId, DynamicCharacterArray buffer)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_TextFieldLFImpl_setString0() {
    pcsl_string v_text;
    pcsl_string *const text=&v_text;
    MidpError err;
    pcsl_string_status perr;
    MidpItem *tfPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    KNI_StartHandles(2);

    KNI_DeclareHandle(bufferJDCArray);
    KNI_DeclareHandle(bufferJCharArray);
    
    KNI_GetParameterAsObject(2, bufferJDCArray);

    bufferJCharArray = BUFFER_JARRAY_HANDLE(bufferJDCArray);

    perr = midp_jchar_array_to_pcsl_string(bufferJCharArray,
		      getMidpDynamicCharacterArrayPtr(bufferJDCArray)->length, text);

    KNI_EndHandles();

    if (PCSL_STRING_OK != perr) {
	    err = KNI_ENOMEM;
    } else {
        err = lfpport_textfield_set_string(tfPtr, text);
        pcsl_string_free(text);
    }

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * KNI function that updates the maximum size of the TextField's native widget.
 * Class: javax.microedition.lcdui.TextFieldLFImpl
 * Java prototype:
 * private native void setMaxSize0(int nativeId, int maxSize)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_TextFieldLFImpl_setMaxSize0() {
    MidpItem *tfPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int maxSize = KNI_GetParameterAsInt(2);

    MidpError err = lfpport_textfield_set_max_size(tfPtr, maxSize);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * KNI function that queries current insert cursor position.
 * Class: javax.microedition.lcdui.TextFieldLFImpl
 * Java prototype:
 * private native int getCaretPosition0(int nativeId)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_TextFieldLFImpl_getCaretPosition0() {
    MidpItem *tfPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int caretPosition;

    MidpError err = lfpport_textfield_get_caret_position(&caretPosition, 
							 tfPtr);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(caretPosition);
}

/**
 * KNI function that updates input constraints of the TextField's native widget.
 * Class: javax.microedition.lcdui.TextFieldLFImpl
 * Java prototype:
 * private native void setConstraints0(int nativeId, int constraints)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_TextFieldLFImpl_setConstraints0() {
    MidpItem *tfPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int constraints = KNI_GetParameterAsInt(2);

    MidpError err = lfpport_textfield_set_constraints(tfPtr, constraints);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}
