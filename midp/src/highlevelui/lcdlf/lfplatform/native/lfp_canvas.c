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
 * Canvas' cross platform functions and data structure.
 */

#include <kni.h>

#include <midpMalloc.h>
#include <midpError.h>
#include <lfpport_canvas.h>
#include <lfp_registry.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>

/**
 * KNI function to create and show native window for Canvas.
 * CLASS: javax.microedition.lcdui.CanvasLFImpl
 * PROTOTYPE: private native int createNativeResource0(String title,
 * 						       String tickerText);
 * RETURN: pointer to Canvas' MidpDisplayable structure
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_CanvasLFImpl_createNativeResource0() {
    MidpDisplayable *canvasPtr;
    MidpError err = KNI_ENOMEM;

    /* Allocate MidpDisplayable structure */
    canvasPtr = MidpNewDisplayable(MIDP_CANVAS_TYPE);
    if (canvasPtr != NULL) {
        /* Get parameters */
        KNI_StartHandles(2);

        GET_PARAMETER_AS_PCSL_STRING(1, title)
        GET_PARAMETER_AS_PCSL_STRING(2, tickerText)

    /* Call platform dependent function to fill in MidpDisplayable structure */
        err = lfpport_canvas_create(canvasPtr, &title, &tickerText);

        RELEASE_PCSL_STRING_PARAMETER
        RELEASE_PCSL_STRING_PARAMETER
        KNI_EndHandles();
    }

    if (err != KNI_OK) {
        MidpDeleteDisplayable(canvasPtr); /* NULL is acceptable */
        KNI_ThrowNew(midpOutOfMemoryError, NULL); /* ok if invoked twice */
    } else {
        MidpCurrentScreen = &canvasPtr->frame;
    }

    KNI_ReturnInt(canvasPtr);
}
