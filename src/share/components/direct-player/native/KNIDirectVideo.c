/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#include "jsrop_exceptions.h"
#include "jsr135_sync.h"

/*  private native int nGetWidth ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetWidth) {

    jint handle = KNI_GetParameterAsInt(1);
    jint returnValue = 0;
    long width;
    long height;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result result = JAVACALL_FAIL;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_get_video_size(pKniInfo->pNativeHandle, &width, &height),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_VIDEO_SIZE_GOTTEN,
            returns_data(2, (&width, &height))
        );
        if (result == JAVACALL_OK) {
            returnValue = width;
        }
    }


    MMP_DEBUG_STR2("[kni_video] -nGetWidth %d ret %d\n", width, returnValue);

    KNI_ReturnInt(returnValue);
}

/*  private native int nGetHeight ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetHeight) {

    jint handle = KNI_GetParameterAsInt(1);
    jint returnValue = 0;
    long width;
    long height;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result result = JAVACALL_FAIL;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_get_video_size(pKniInfo->pNativeHandle, &width, &height),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_VIDEO_SIZE_GOTTEN,
            returns_data(2, (&width, &height))
        );
        if (result == JAVACALL_OK) {
            returnValue = height;
        }
    }

    MMP_DEBUG_STR2("[kni_video] -nGetHeight %d ret %d\n", height, returnValue);

    KNI_ReturnInt(returnValue);
}

/*  private native int nSetLocation ( int handle , int x , int y , int w , int h ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSetLocation) {

    jint handle = KNI_GetParameterAsInt(1);
    jint x = KNI_GetParameterAsInt(2);
    jint y = KNI_GetParameterAsInt(3);
    jint w = KNI_GetParameterAsInt(4);
    jint h = KNI_GetParameterAsInt(5);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    jboolean returnValue = KNI_FALSE;
    javacall_result result;

    MMP_DEBUG_STR4("[kni_video] +nSetLocation %d %d %d %d\n", x, y, w, h);

    if (pKniInfo && pKniInfo->pNativeHandle) {
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_set_video_location(pKniInfo->pNativeHandle, x, y, w, h),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_VIDEO_LOCATION_SET,
            returns_no_data
        );
        if (result == JAVACALL_OK) {
            returnValue = KNI_TRUE;
        }
    }

    MMP_DEBUG_STR1("[kni_video] -nSetLocation ret %d\n", returnValue);

    KNI_ReturnBoolean(returnValue);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN 
KNIDECL(com_sun_mmedia_DirectPlayer_nSetFullScreenMode) {
    jint     handle  = KNI_GetParameterAsInt(1);
    jboolean fscreen = KNI_GetParameterAsBoolean(2);

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR1("[kni_video] +nSetFullScreen(%d)\n", fscreen);

    if (pKniInfo && pKniInfo->pNativeHandle &&
        JAVACALL_OK == javacall_media_set_video_full_screen_mode(pKniInfo->pNativeHandle, 
                           (KNI_TRUE == fscreen ? JAVACALL_TRUE : JAVACALL_FALSE))) 
    {
        returnValue = KNI_TRUE;
    }

    MMP_DEBUG_STR1("[kni_video] +nSetFullScreen=%d\n", returnValue);

    KNI_ReturnBoolean(returnValue);
}

/*  private native boolean nSetVisible ( int handle, boolean visible ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSetVisible) {

    jint handle = KNI_GetParameterAsInt(1);
    jboolean visible = KNI_GetParameterAsBoolean(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result result = JAVACALL_FALSE;

    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR1("[kni_video] +nSetVisible %d\n", visible);

    if (pKniInfo && pKniInfo->pNativeHandle) {
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_set_video_visible(pKniInfo->pNativeHandle, 
                (KNI_TRUE == visible ? JAVACALL_TRUE : JAVACALL_FALSE)),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_VIDEO_VISIBILITY_SET,
            returns_no_data
        );
        if (result == JAVACALL_OK) {
            returnValue = KNI_TRUE;
        }
    }

    MMP_DEBUG_STR1("[kni_video] -nSetVisible ret %d\n", returnValue);

    KNI_ReturnBoolean(returnValue);
}

/*  private native boolean nSetColorKey (boolean on, int colorKey) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSetColorKey) {

    jint handle = KNI_GetParameterAsInt(1);
    jboolean isOn = KNI_GetParameterAsBoolean(2);
    jint color = KNI_GetParameterAsInt(3);
    javacall_result ret = JAVACALL_FAIL;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    javacall_pixel lcd_color 
        = (javacall_pixel)RGB2PIXELTYPE((color & 0xFF0000) >> 16,
                                        (color & 0x00FF00) >> 8,
                                        (color & 0x0000FF));

    MMP_DEBUG_STR2("[kni_video] +nSetColorKey on=%d colorKey=%d\n", isOn, color);

    if (pKniInfo && pKniInfo->pNativeHandle ) {
        ret = javacall_media_set_video_color_key(pKniInfo->pNativeHandle, 
					KNI_TRUE == isOn ? JAVACALL_TRUE : JAVACALL_FALSE, lcd_color);
    }

    MMP_DEBUG_STR1("[kni_video] -nSetColorKey ret %d\n", ret);

    KNI_ReturnBoolean(JAVACALL_SUCCEEDED(ret) ? KNI_TRUE : KNI_FALSE);  
}

