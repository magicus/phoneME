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

#include "KNICommon.h"

/*  protected native int nGetVolume ( int mediaType ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectVolume_nGetVolume) {
    jint handle = KNI_GetParameterAsInt(1);
    javacall_result ret = JAVACALL_FAIL;
    long volume;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR("[kni_volume] +nGetVolume\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
        ret = javacall_media_get_volume(pKniInfo->pNativeHandle, &volume);
    } else {
        MMP_DEBUG_STR("[nGetVolume] Invalid native handle");
    }
    if (ret != JAVACALL_OK) {
        volume = -1;
    }

    KNI_ReturnInt((int)volume);
}

/*  protected native int nSetVolume ( int mediaType , int level ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectVolume_nSetVolume) {

    jint handle = KNI_GetParameterAsInt(1);
    jlong level = KNI_GetParameterAsInt(2);
    javacall_result ret = JAVACALL_FAIL;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR("[kni_volume] +nSetVolume\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
        ret = javacall_media_set_volume(pKniInfo->pNativeHandle, &level);
    } else {
        MMP_DEBUG_STR("[nSetVolume] Invalid native handle");
    }
    if (ret != JAVACALL_OK) {
        level = -1;
    }

    KNI_ReturnInt((int)level);
}

/*  protected native boolean nIsMuted ( int mediaType ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectVolume_nIsMuted) {

    jint handle = KNI_GetParameterAsInt(1);
    javacall_result ret = JAVACALL_FAIL;
    jboolean returnValue = KNI_FALSE;
    javacall_bool isMuted;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR("[kni_volume] +nIsMuted\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
        ret= javacall_media_is_mute(pKniInfo->pNativeHandle, &isMuted);
    } else {
        MMP_DEBUG_STR("[nIsMuted] Invalid native handle");
    }
    if (ret == JAVACALL_OK) {
        returnValue = (isMuted == JAVACALL_TRUE) ? KNI_TRUE : KNI_FALSE;
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native int nSetMute ( int mediaType , boolean mute ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectVolume_nSetMute) {

    jint handle = KNI_GetParameterAsInt(1);
    jboolean mute = KNI_GetParameterAsBoolean(2);
    jboolean returnValue = KNI_FALSE;
    javacall_result ret = JAVACALL_FAIL;
    javacall_bool isMuted;

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR1("[kni_volume] +nSetMute %d\n", mute);

    if (pKniInfo && pKniInfo->pNativeHandle) {
        isMuted = (mute == KNI_TRUE) ? JAVACALL_TRUE : JAVACALL_FALSE;
        ret = javacall_media_set_mute(pKniInfo->pNativeHandle, &isMuted);
    } else {
        MMP_DEBUG_STR("[nSetMute] Invalid native handle");
    }
    if (ret == JAVACALL_OK) {
        returnValue = (isMuted == JAVACALL_TRUE) ? KNI_TRUE : KNI_FALSE;
    }

    KNI_ReturnBoolean(returnValue);
}
