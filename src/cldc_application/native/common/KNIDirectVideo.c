/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
#include "commonKNIMacros.h"
#include "sni.h"
#include "lcdlf_export.h"
#include "midpServices.h"
#include "midpError.h"
#include "javacall_multimedia.h"

#include "jsrop_exceptions.h"

/*  private native int nGetWidth ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectVideo_nGetWidth) {

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
KNIDECL(com_sun_mmedia_DirectVideo_nGetHeight) {

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
KNIDECL(com_sun_mmedia_DirectVideo_nSetLocation) {

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

/*  protected native byte [ ] nSnapShot ( String imageType ) ; */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectVideo_nSnapShot) {
    
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo*  pKniInfo = (KNIPlayerInfo*)handle;
    jint            imageTypeLength;
    jchar*          pImageTypeStr;
    javacall_result ret = JAVACALL_FAIL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(imageTypeHandle);
    KNI_DeclareHandle(returnValueHandle);
    KNI_GetParameterAsObject(2, imageTypeHandle);

    MMP_DEBUG_STR("[kni_video] +nSnapShot\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
        imageTypeLength = KNI_GetStringLength(imageTypeHandle);
        pImageTypeStr = MMP_MALLOC(imageTypeLength * sizeof(jchar));
        if (pImageTypeStr) {
            KNI_GetStringRegion(imageTypeHandle, 0, imageTypeLength, pImageTypeStr);
            JAVACALL_MM_ASYNC_EXEC(
                ret,
                javacall_media_start_video_snapshot(pKniInfo->pNativeHandle, pImageTypeStr, imageTypeLength),
                pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
                returns_no_data
            );
            MMP_FREE(pImageTypeStr);
            if (ret == JAVACALL_OK) {
                long dataBytes;
                
                ret = javacall_media_get_video_snapshot_data_size(pKniInfo->pNativeHandle, &dataBytes);
                if (JAVACALL_OK == ret) {
                    MMP_DEBUG_STR1("[kni_video] nSnapShot get data size %d\n", dataBytes);
        
                    if (dataBytes > 0) {
                        /* Create new Java byte array object to store snapshot data */
                        SNI_NewArray(SNI_BYTE_ARRAY, dataBytes, returnValueHandle);
                        if (KNI_IsNullHandle(returnValueHandle)) {
                            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                        } else {
                            ret = javacall_media_get_video_snapshot_data(pKniInfo->pNativeHandle, 
                                              (char*)JavaByteArray(returnValueHandle), dataBytes);
                            if (JAVACALL_OK != ret) {
                                KNI_ReleaseHandle(returnValueHandle);
                            }
                        }
                    } else {
                        MMP_DEBUG_STR("[kni_video] FATAL - javacall_media_get_video_snapshot_data_size return OK with 0\n");
                    }
                }
            }
        }
    }

    MMP_DEBUG_STR1("[kni_video] -nSnapShot %d\n", returnValueHandle);

    KNI_EndHandlesAndReturnObject(returnValueHandle);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN 
KNIDECL(com_sun_mmedia_DirectVideo_nSetFullScreenMode) {
    jint     handle  = KNI_GetParameterAsInt(1);
    jboolean fscreen = KNI_GetParameterAsBoolean(2);

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR1("[kni_video] +nSetFullScreen(%d)\n", fscreen);

    if (pKniInfo && pKniInfo->pNativeHandle &&
        JAVACALL_OK == javacall_media_set_video_fullscreenmode(pKniInfo->pNativeHandle, 
                           (KNI_TRUE == fscreen ? JAVACALL_TRUE : JAVACALL_FALSE))) 
    {
        returnValue = KNI_TRUE;
    }

    MMP_DEBUG_STR1("[kni_video] +nSetFullScreen=%d\n", returnValue);

    KNI_ReturnBoolean(returnValue);
}

/*  private native boolean nSetVisible ( int handle, boolean visible ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectVideo_nSetVisible) {

    jint handle = KNI_GetParameterAsInt(1);
    jboolean visible = KNI_GetParameterAsBoolean(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR1("[kni_video] +nSetVisible %d\n", visible);

    if (pKniInfo && pKniInfo->pNativeHandle &&
        JAVACALL_OK == javacall_media_set_video_visible(pKniInfo->pNativeHandle, 
                           (KNI_TRUE == visible ? JAVACALL_TRUE : JAVACALL_FALSE))) 
    {
        returnValue = KNI_TRUE;
    }

    MMP_DEBUG_STR1("[kni_video] -nSetVisible ret %d\n", returnValue);

    KNI_ReturnBoolean(returnValue);
}

/*  private native int nGetScreenHeight ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectVideo_nGetScreenHeight() {
    KNI_ReturnInt(lcdlf_get_screen_height());
}

/*  private native int nGetScreenWidth ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectVideo_nGetScreenWidth() {
    KNI_ReturnInt(lcdlf_get_screen_width());  
}

/*  private native int nSetAlpha (boolean on, int color) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectVideo_nSetAlpha() {

    jint handle = KNI_GetParameterAsInt(1);
    jboolean isOn = KNI_GetParameterAsBoolean(2);
    jint color = KNI_GetParameterAsInt(3);
    javacall_result ret = JAVACALL_FAIL;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR2("[kni_video] +nSetAlpha on=%d alpha=%d\n", isOn, color);

    if (pKniInfo && pKniInfo->pNativeHandle ) {
        ret = javacall_media_set_video_color_key(pKniInfo->pNativeHandle, 
					KNI_TRUE == isOn ? JAVACALL_TRUE : JAVACALL_FALSE, 
                                         (javacall_pixel)color);
    }

    MMP_DEBUG_STR1("[kni_video] -nSetAlpha ret %d\n", ret);

    KNI_ReturnInt(JAVACALL_SUCCEEDED(ret) ? 1 : 0);  
}

