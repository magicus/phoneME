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
 * Shared cross platform functions for all Displayables.
 */


#include <kni.h>

#include <midpString.h>
#include <midpError.h>
#include <midpMalloc.h>
#include <lfpport_displayable.h>
#include "lfp_intern_registry.h"

#include <midpUtilKni.h>
/**
 * Cached field ID to be accessible in finalizer.
 */
static jfieldID nativeFieldId = 0;

/**
 * Static class initializer.
 * Java Prototype: private static native void initialize();
 * Class: javax.microedition.lcdui.DisplayableLFImpl
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DisplayableLFImpl_initialize0(void)
{
    KNI_StartHandles(1);
    KNI_DeclareHandle(classHandle);
    KNI_GetClassPointer(classHandle);

    nativeFieldId = KNI_GetFieldID(classHandle, "nativeId", "I");

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * KNI function to set current Displayable's title.
 * Function: private native void setTitle0(int nativeId, String title)
 * Class: javax.microedition.lcdui.DisplayableLFImpl
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DisplayableLFImpl_setTitle0() {
    MidpDisplayable * displayablePtr;

    KNI_StartHandles(1);

    displayablePtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_PCSL_STRING(2, title) {
        /* NULL and empty strings are acceptable. */
    	MidpError err = displayablePtr->setTitle(displayablePtr, &title);

        if (err != KNI_OK) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    } RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    KNI_ReturnVoid();
}

/**
 * KNI native implementation to start the ticker
 * Function: void setTicker0(int nativeId, String text)
 * Class: javax.microedition.lcdui.DisplayableLFImpl
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DisplayableLFImpl_setTicker0() {
    MidpDisplayable * displayablePtr;

    KNI_StartHandles(1);
    displayablePtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_PCSL_STRING(2, text_str)

        displayablePtr->setTicker(displayablePtr, &text_str);

    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * KNI function that hides native Displayable and release its resource.
 * Java Prototype: void deleteNativeResource0(int nativeId)
 * CLASS:         javax.microedition.lcdui.DisplayableLFImpl
 * @param  nativeId Id of previously created Form native resource
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DisplayableLFImpl_deleteNativeResource0() {

    MidpDisplayable *p = (MidpDisplayable *)KNI_GetParameterAsInt(1);

    /* free platform dependent resource and MidpDisplayable structure */
    MidpDeleteDisplayable(p);

    KNI_ReturnVoid();
}

/**
 * Native finalizer.
 * Java Prototype: private native void finalize();
 * Class: javax.microedition.lcdui.DisplayableLFImpl
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DisplayableLFImpl_finalize() {

    jint nativeId;
    MidpDisplayable *p;
    KNI_StartHandles(1);
    KNI_DeclareHandle(thisHandle);

    KNI_GetThisPointer(thisHandle);
    nativeId = KNI_GetIntField(thisHandle, nativeFieldId);
    
    if (nativeId != 0) {
        p = (MidpDisplayable *)nativeId;
        MidpDeleteDisplayable(p);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
