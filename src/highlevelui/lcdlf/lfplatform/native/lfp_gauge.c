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
 * Cross platform functions for Gauge.
 */

#include <kni.h>

#include <midpMalloc.h>
#include <midpError.h>
#include <lfpport_gauge.h>
#include "lfp_intern_registry.h"

#include <midpUtilKni.h>

/**
 * KNI function that create native resource for current Gauge.
 * Class: javax.microedition.lcdui.GaugeLFImpl
 * Java prototype:
 * private native int createNativeResource0(int ownerId,
 *	    		String label, int layout, 
 *                      boolean interactive,
 *			int maxValue, int initialValue)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_GaugeLFImpl_createNativeResource0() {
    
    MidpError err = KNI_OK;
    MidpItem *gaugePtr = NULL;
    MidpDisplayable *ownerPtr;
    int maxValue, initialValue, layout;
    jboolean interactive;

    KNI_StartHandles(1);

    ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_PCSL_STRING(2, label)
    layout = KNI_GetParameterAsInt(3);
    interactive = KNI_GetParameterAsBoolean(4);
    maxValue = KNI_GetParameterAsInt(5);
    initialValue = KNI_GetParameterAsInt(6);
    {
        gaugePtr = MidpNewItem(ownerPtr,
                   interactive ? MIDP_INTERACTIVE_GAUGE_TYPE
                           : MIDP_NON_INTERACTIVE_GAUGE_TYPE);
        if (gaugePtr == NULL) {
            err = KNI_ENOMEM;
        } else {
            err = lfpport_gauge_create(gaugePtr, ownerPtr, &label, layout,
                           interactive, maxValue, initialValue);
        }
        /* cleanup: */
        if (err != KNI_OK) {
            MidpDeleteItem(gaugePtr);
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    }
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    KNI_ReturnInt(gaugePtr);
}

/**
 * KNI function that updates the current and/or maximum values of the Gauge's native widget.
 * Class: javax.microedition.lcdui.GaugeLFImpl
 * Java prototype:
 * private native void setValue0(int nativeId, int value, int maxValue)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_GaugeLFImpl_setValue0() {
    MidpItem *gaugePtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int value = KNI_GetParameterAsInt(2);
    int maxValue = KNI_GetParameterAsInt(3);

    MidpError err = lfpport_gauge_set_value(gaugePtr, value, maxValue);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}
