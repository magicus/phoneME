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

#include <stdio.h>
#include <string.h>
#include "KNICommon.h"

/*********************************************************
 * KNI function implementation                           *
 *********************************************************/

/*  private native int nPlayTone ( int appId, int note , int dur , int vol ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_NativeTonePlayer_nPlayTone) {
    jint appId = KNI_GetParameterAsInt(1);
    jint note = KNI_GetParameterAsInt(2);
    jint dur = KNI_GetParameterAsInt(3);
    jint vol = KNI_GetParameterAsInt(4);
    jboolean returnValue = KNI_TRUE;

    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 100;
    }

    if (note >= 0 && note <= 127) {
        if (JAVACALL_FAIL == javacall_media_play_tone(appId, note, dur, vol)) {
            returnValue = KNI_FALSE;
        }
    }
    KNI_ReturnBoolean(returnValue);
}

/*  private native int nStopTone ( int appId) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_NativeTonePlayer_nStopTone) {
    jint appId = KNI_GetParameterAsInt(1);
    jboolean returnValue = KNI_TRUE;

    if (JAVACALL_FAIL == javacall_media_stop_tone(appId)) {
        returnValue = KNI_FALSE;
    }
    KNI_ReturnBoolean(returnValue);
}

/*************************************************************************/

/* Native finalizer */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_NativeTonePlayer_finalize) {

/* commeted out because of appId is unknown */
/*    javacall_media_stop_tone();*/
    KNI_ReturnVoid();
}
