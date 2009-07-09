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
 * Alert's cross platform functions and data structure.
 */

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>

#include <lfpport_alert.h>
#include <lfp_registry.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>

#include <gxp_image.h>

/**
 * KNI function to create native dialog for Alert.
 * CLASS: javax.microedition.lcdui.AlertLFImpl
 * PROTOTYPE: private native int createNativeResource0(String title,
 * 						       String tickerText,
 *						       int type)
 * RETURN: pointer to Alert's MidpDisplayable structure
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_AlertLFImpl_createNativeResource0() {
    MidpDisplayable *alertPtr = NULL;
    pcsl_string title = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string tickerText = PCSL_STRING_NULL_INITIALIZER;
    MidpError err;
    MidpComponentType type;
    jboolean stringsObtained = 0;

    /* Get parameters */
    KNI_StartHandles(2);
    KNI_DeclareHandle(titleJString);
    KNI_DeclareHandle(tickerTextJString);
    KNI_GetParameterAsObject(1, titleJString);
    KNI_GetParameterAsObject(2, tickerTextJString);
    type = KNI_GetParameterAsInt(3) + MIDP_NULL_ALERT_TYPE;

    if(PCSL_STRING_OK == midp_jstring_to_pcsl_string(titleJString,&title)) {
        if(PCSL_STRING_OK == midp_jstring_to_pcsl_string(tickerTextJString,&tickerText)) {
            stringsObtained = 1;
        }
    }

    KNI_EndHandles();

    if (!stringsObtained) {
	    err = KNI_ENOMEM;
    } else {
        /* Allocate MidpDisplayable structure */
        alertPtr = MidpNewDisplayable(type);
        if (alertPtr == NULL) {
            err = KNI_ENOMEM;
        } else {
            /* Call platform dependent function to fill in
             * MidpDisplayable structure */
            err = lfpport_alert_create(alertPtr, &title, &tickerText, type);
            /* Gauge pointer will be set later
             * in setNativeContents0() function */
        }
    }
    /*cleanup:*/
    pcsl_string_free(&tickerText);
    pcsl_string_free(&title);

    if (err != KNI_OK) {
	    MidpDeleteDisplayable(alertPtr); /* NULL is acceptable */
	    KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(alertPtr);
}

/**
 * KNI function to show native dialog for Alert.
 * CLASS: javax.microedition.lcdui.AlertLFImpl
 * PROTOTYPE: private native void showNativeResource0(int nativeId)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_AlertLFImpl_showNativeResource0() {
    MidpError err;

    /* Get parameters */
    MidpDisplayable *alertPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);

    err = alertPtr->frame.show(&alertPtr->frame);
    if (err == KNI_OK) {
	/* Becomes the new screen that receives events */
	MidpCurrentScreen = &alertPtr->frame;
    } else {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * KNI function to populate native Alert dialog with image, gauge and text.
 * CLASS: javax.microedition.lcdui.AlertLFImpl
 * PROTOTYPE: private native boolean setNativeContents0(int nativeId,
 *						       Image img,
 *						       int[] indicatorBounds,
 * 						       String text)
 * PARAM: indicatorBounds: a 4 integer array for indicator gauge
 *			   [0] : OUT x coordinate in alert dialog
 *			   [1] : OUT y coordinate in alert dialog
 *			   [2] : IN/OUT width of the gauge, in pixels
 *			   [3] : IN/OUT height of the gauge, in pixels
 * RETURN: true if content requires scrolling
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_lcdui_AlertLFImpl_setNativeContents0() {
    MidpDisplayable *alertPtr;
    unsigned char* imgPtr;
    jboolean isContentScroll;
    int gaugeBounds[4];
    int *bounds;

    KNI_StartHandles(3);

    KNI_DeclareHandle(gaugeBoundsJArray);
    KNI_DeclareHandle(imageJObject);

    /* Get parameters */
    alertPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, imageJObject);
    KNI_GetParameterAsObject(3, gaugeBoundsJArray);
    GET_PARAMETER_AS_PCSL_STRING(4, text) {
        MidpError err;
        if (KNI_IsNullHandle(gaugeBoundsJArray) == KNI_TRUE) {
            bounds = NULL;
        } else {
            gaugeBounds[0] = KNI_GetIntArrayElement(gaugeBoundsJArray, 0);
            gaugeBounds[1] = KNI_GetIntArrayElement(gaugeBoundsJArray, 1);
            gaugeBounds[2] = KNI_GetIntArrayElement(gaugeBoundsJArray, 2);
            gaugeBounds[3] = KNI_GetIntArrayElement(gaugeBoundsJArray, 3);
            bounds = gaugeBounds;
        }

        if (KNI_IsNullHandle(imageJObject) == KNI_TRUE) {
            imgPtr = NULL;
        } else {
            imgPtr = gxp_get_imagedata(imageJObject);
        }

        /* Call platform dependent setContent() function */
        err = lfpport_alert_set_contents(alertPtr,
                         imgPtr, bounds, &text);

        if (err == KNI_OK) {
            /* Update the gauge bounds array in Java */
            if (KNI_IsNullHandle(gaugeBoundsJArray) == KNI_FALSE) {
                KNI_SetIntArrayElement(gaugeBoundsJArray, 0, gaugeBounds[0]);
                KNI_SetIntArrayElement(gaugeBoundsJArray, 1, gaugeBounds[1]);
                KNI_SetIntArrayElement(gaugeBoundsJArray, 2, gaugeBounds[2]);
                KNI_SetIntArrayElement(gaugeBoundsJArray, 3, gaugeBounds[3]);
            }

            /* query for scrolling */
            err = lfpport_alert_need_scrolling(&isContentScroll,
                               alertPtr);
        }

        if (err != KNI_OK) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    } RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    KNI_ReturnBoolean(isContentScroll);
}
