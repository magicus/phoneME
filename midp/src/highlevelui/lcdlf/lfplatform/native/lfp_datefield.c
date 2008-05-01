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
 * Cross platform functions for DateField.
 */

#include <kni.h>

#include <midpMalloc.h>
#include <midpError.h>
#include <lfpport_datefield.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>

/**
 * KNI function that create native resource for current DateField.
 *
 * The native widget created by this function must take into account
 * label and body locations, using Item methods.
 * The implementation must create a native date/time widget that can be
 * displayed as date only, time only, or date/time modes.
 *
 * The displayMode is a two bit mask in which the MSB is the date, and the
 * LSB (ie. binary 10 is date only, 01 is time only, 11 is both).
 *
 * As the datetime parameter coming from java is in milliseconds since the
 * epoch 00:00 1/1/1970, it is needed to translate this value to the native
 * date/time representation. (for example, QT uses the same epoch, but in
 * resolution of seconds rather than milliseconds).
 *
 *
 * Class: javax.microedition.lcdui.DateFieldLFImpl
 * Java prototype:
 * 	private native int createNativeResource0 ( int ownerId , 
 * 	String label , int layout, long datetime , int displayMode,
 *	String timeZone);
 * @param datetime in milliseconds since 00:00 1/1/1970
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_DateFieldLFImpl_createNativeResource0() {

    MidpDisplayable *ownerPtr;
    pcsl_string label;
    pcsl_string timezone;
    pcsl_string_status rc1, rc2;
    jint layout;
    jint displayMode;
    jlong datetime;

    long qtime = 0;

    MidpError err = KNI_OK;
    MidpItem *dfPtr = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(labelHandle);
    KNI_DeclareHandle(timezoneHandle);

    ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, labelHandle);
    layout = KNI_GetParameterAsInt(3);
    datetime = KNI_GetParameterAsLong(4); /* Long occupies two parameters */
    displayMode = KNI_GetParameterAsInt(6);
    KNI_GetParameterAsObject(7, timezoneHandle);

    rc1 = midp_jstring_to_pcsl_string(labelHandle, &label);
    rc2 = midp_jstring_to_pcsl_string(timezoneHandle, &timezone);
    
    KNI_EndHandles();

    /* NULL and empty strings are acceptable */
    if (PCSL_STRING_OK != rc1 || PCSL_STRING_OK != rc2) {
        err = KNI_ENOMEM;
        goto cleanup;
    }

    dfPtr = MidpNewItem(ownerPtr, MIDP_DATE_FIELD_TYPE);
    if (dfPtr == NULL) {
        err = KNI_ENOMEM;
        goto cleanup;
    }

    qtime = (long)(datetime/1000); /* qt date works in seconds */

    err = lfpport_datefield_create(dfPtr, ownerPtr, &label, layout,
				   displayMode, qtime, &timezone);

cleanup:

    pcsl_string_free(&label);
    pcsl_string_free(&timezone);

    if (err != KNI_OK) {
        MidpDeleteItem(dfPtr);
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(dfPtr);

}

/**
 * 
 * The datetime parameter coming from java is in milliseconds since the
 * epoch 00:00 1/1/1970. It is needed to translate this value to the native
 * date/time representation. (for example, QT uses the same epoch, but in
 * resolution of seconds rather than milliseconds).
 *
 * private native void setDate0 ( int nativeId , long date ) ; 
 * @param date in milliseconds since 00:00 1/1/1970
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DateFieldLFImpl_setDate0() {
    MidpItem *dfPtr = NULL;
    MidpError err;

    jint nativeId = KNI_GetParameterAsInt(1);
    jlong date = KNI_GetParameterAsLong(2); /* Long occupies two parameters */

    dfPtr = (MidpItem *)nativeId;

    err = lfpport_datefield_set_date(dfPtr,(long)(date/1000));

    if (err != KNI_OK) {
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/** 
 * Request the item to change mode.
 *
 * The displayMode is a two bit mask in which the MSB is the date, and the
 * LSB (ie. binary 10 is date only, 01 is time only, 11 is both).
 *
 * After this function call returns, the Form will query the item for
 * new size, than set size of this item (and other items affected from the
 * change) and update location.
 *
 * The implementation must update its contents, hide and/or show parts of 
 * itself, and be prepared to return the correct sizes.
 *
 * It is not necessary to perform inner layout at this stage, as it will occur
 * later.
 *
 * private native void setInputMode0 ( int nativeId , int mode ) ; 
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_DateFieldLFImpl_setInputMode0() {
    MidpItem *dfPtr = NULL;
    MidpError err;

    jint nativeId = KNI_GetParameterAsInt(1);
    jint mode = KNI_GetParameterAsInt(2);

    dfPtr = (MidpItem *)nativeId;

    err = lfpport_datefield_set_input_mode(dfPtr,(int)mode);

    if (err != KNI_OK) {
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * The datetime parameter returned to java must be in milliseconds since the
 * epoch 00:00 1/1/1970. 
 * It is needed to translate the native value to the java date/time
 * representation. (for example, QT uses the same epoch, but in
 * resolution of seconds rather than milliseconds, so it is needed to multiply
 * the value by 1000 to pass to java).
 *
 * private native long getDate0 ( int nativeId ) ; 
 * @return date in milliseconds since 00:00 1/1/1970
 */
KNIEXPORT KNI_RETURNTYPE_LONG
Java_javax_microedition_lcdui_DateFieldLFImpl_getDate0() {
    MidpItem *dfPtr = NULL;
    MidpError err;

    jint nativeId = KNI_GetParameterAsInt(1);

    jlong returnValue = 0L ;
    
    dfPtr = (MidpItem *)nativeId;

    err = lfpport_datefield_get_date((long*)(void*)&returnValue, dfPtr);

    if (err != KNI_OK) {
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    returnValue *= 1000;

    KNI_ReturnLong(returnValue);
}

