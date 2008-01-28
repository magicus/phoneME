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

#include <string.h>
#include "KNICommon.h"

#include "jsrop_exceptions.h"
#include "jsr135_sync.h"

/* Global Variables ************************************************************************/

/* Externs **********************************************************************************/

extern int unicodeToNative(const jchar *ustr, int ulen, unsigned char *bstr, int blen);

/* Internal utility implementation **********************************************************/

/**
 * Check current condition
 * 
 * @param hLIB
 * @return  If condition is good return TRUE else return FALSE
 */
static javacall_bool jmmpCheckCondition(KNIPlayerInfo* pKniInfo, int conditions)
{
/* Check conditions flag */
#define CHECK_ALL               (CHECK_CURRENT_PLAYER | CHECK_ACQUIRE_RESOURCE | CHECK_BUFFER)
#define CHECK_ISPLAYING         (CHECK_CURRENT_PLAYER | CHECK_ACQUIRE_RESOURCE)       
#define CHECK_CURRENT_PLAYER    (0x00000001)
#define CHECK_ACQUIRE_RESOURCE  (0x00000002)
#define CHECK_BUFFER            (0x00000004)
#define CHECK_ISTEMP            (0x00000008)

    if ((conditions & CHECK_ISTEMP) == CHECK_ISTEMP) {
        if (JAVACALL_TRUE == pKniInfo->isDirectFile) {
            MMP_DEBUG_STR("[KNIDirectPlayer] CHECK_ISTEMP is fail\n");
            return JAVACALL_FALSE;
        }
    }

    if ((conditions & CHECK_ACQUIRE_RESOURCE) == CHECK_ACQUIRE_RESOURCE) {
        if (!pKniInfo->isAcquire) {
            MMP_DEBUG_STR("[KNIDirectPlayer] CHECK_ACQUIRE_RESOURCE fail: not acquire\n");
            return JAVACALL_FALSE;
        }
    }

    return JAVACALL_TRUE;
}

/* KNI Implementation **********************************************************************/

/*  protected native int nInit (int appId, int playerId, String mimeType, String URI, long contentLength ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nInit) {
    jint  appId = KNI_GetParameterAsInt(1);
    jint  playerId = KNI_GetParameterAsInt(2);
    jint  returnValue = 0;
    jint   mimeLength, URILength;
    jchar* pszMimeType = NULL;
    jchar* pszURI = NULL;
    KNIPlayerInfo* pKniInfo;
    javacall_result res;

    MMP_DEBUG_STR2("+nInit isolate=%d, player=%d\n", isolateId, playerId);

    KNI_StartHandles(2);
    KNI_DeclareHandle(mimeType);
    KNI_DeclareHandle(URI);
    
    /* Get mimeType and URI object parameter */
    KNI_GetParameterAsObject(3, mimeType);
    KNI_GetParameterAsObject(4, URI);

    /* Get mime type java string */
    mimeLength = KNI_GetStringLength(mimeType);
    pszMimeType = MMP_MALLOC(mimeLength * sizeof(jchar));
    if (pszMimeType) {
        KNI_GetStringRegion(mimeType, 0, mimeLength, pszMimeType);
    }

    /* Get URI java string */
    if (-1 == (URILength = KNI_GetStringLength(URI))) {
        pszURI = NULL;
    } else {
        pszURI = MMP_MALLOC(URILength * sizeof(jchar));
        if (pszURI) {
            KNI_GetStringRegion(URI, 0, URILength, pszURI);
        }
    }

    pKniInfo = (KNIPlayerInfo*)MMP_MALLOC(sizeof(KNIPlayerInfo));
LockAudioMutex();
    if (pKniInfo && pszMimeType /* && pszURI */) {
        /* prepare kni internal information */
        pKniInfo->appId = appId;
        pKniInfo->playerId = playerId;
        pKniInfo->isAcquire = 0;
        pKniInfo->offset = 0;
        pKniInfo->hBuffer = 0;
        pKniInfo->isDirectFile = JAVACALL_FALSE;
        pKniInfo->isForeground = -1;
        pKniInfo->recordState = RECORD_CLOSE;
        res = javacall_media_create(appId, playerId, pszURI, URILength, &pKniInfo->pNativeHandle); 
        if (res == JAVACALL_FAIL) {
            MMP_FREE(pKniInfo);
        } else {
            returnValue = (int)pKniInfo;
        }
    } else {
        if (pKniInfo) { MMP_FREE(pKniInfo); }
    }
UnlockAudioMutex();            

    if (pszMimeType) { MMP_FREE(pszMimeType); }
    if (pszURI)      { MMP_FREE(pszURI); }
    
    MMP_DEBUG_STR1("-nInit return=%d\n", returnValue);

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  protected native int nTerm ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nTerm) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 1;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    
    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
LockAudioMutex();            
 
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_FAIL == javacall_media_close(pKniInfo->pNativeHandle)) {
            returnValue = 0;
        }
    }
UnlockAudioMutex();            

    if (pKniInfo) {
        MMP_FREE(pKniInfo);
        KNI_SetIntField(instance, KNI_GetFieldID(clazz, "hNative", "I"), 0);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  protected native boolean nAcquireDevice ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nAcquireDevice) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR("+nAcquireDevice\n");

LockAudioMutex();
    if (pKniInfo && JAVACALL_OK == javacall_media_acquire_device(pKniInfo->pNativeHandle)) {
        pKniInfo->isAcquire = JAVACALL_TRUE;
        returnValue = KNI_TRUE;
    }
UnlockAudioMutex();
    KNI_ReturnBoolean(returnValue);
}

/*  protected native void nReleaseDevice ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_nReleaseDevice) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR("+nReleaseDevice\n");

LockAudioMutex();            
    if (pKniInfo) {
        javacall_media_release_device(pKniInfo->pNativeHandle);
        pKniInfo->isAcquire = JAVACALL_FALSE;
    }
UnlockAudioMutex();            

    KNI_ReturnVoid();
}

/*  protected native int nBuffering ( int handle , Object buffer, long length ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nBuffering) {

    jint handle = KNI_GetParameterAsInt(1);
    long length = (long)KNI_GetParameterAsLong(3);
    jint returnValue = -1;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    void *nBuffer;
    long nBufferSize;
    javacall_bool need_more_data;
    long min_data_size;
    javacall_result ret;

    KNI_StartHandles(1);
    KNI_DeclareHandle(bufferHandle);
    KNI_GetParameterAsObject(2, bufferHandle);
    
LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle && length > 0) {
        int ret;
        ret = javacall_media_get_buffer_address(pKniInfo->pNativeHandle, &nBuffer, &nBufferSize);
        if ((ret == JAVACALL_OK) && (nBuffer != NULL)) {
            if (nBufferSize < length) {
                length = nBufferSize;
            }
            MMP_DEBUG_STR1("+nBuffering length=%d\n", length);
            KNI_GetRawArrayRegion(bufferHandle, 0, (int)length, (jbyte*)nBuffer);
            ret = javacall_media_do_buffering(pKniInfo->pNativeHandle, 
                    (const void*)nBuffer, &length, &need_more_data, &min_data_size);
            if (ret == JAVACALL_OK) {
                returnValue = (jint)length;
            }
        }
    } else if (pKniInfo && pKniInfo->pNativeHandle) {
        /* Indicate end of buffering by using NULL buffer */
        ret = javacall_media_do_buffering(pKniInfo->pNativeHandle, NULL, 0, &need_more_data, &min_data_size);
        if (ret == JAVACALL_OK) {
            returnValue = 0;
        }
    }
UnlockAudioMutex();            

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  protected native boolean nFlushBuffer ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nFlushBuffer) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

LockAudioMutex();            
    /* If it is not temp buffer just return true */
    if (pKniInfo && JAVACALL_TRUE == jmmpCheckCondition(pKniInfo, CHECK_ISTEMP)) {
        if (JAVACALL_OK == javacall_media_clear_buffer(pKniInfo->pNativeHandle)) {
            pKniInfo->offset = 0;   /* reset offset value */
            returnValue = KNI_TRUE;
        } else {
            REPORT_ERROR(LC_MMAPI, "javacall_media_clear_buffer return fail");
        }
    } else {
        REPORT_ERROR(LC_MMAPI, "nFlushBuffer fail cause we are not using temp buffer");
    }
UnlockAudioMutex();            

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nStart ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nStart) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;
    
    MMP_DEBUG_STR("+nStart\n");
    
LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle && JAVACALL_OK == javacall_media_start(pKniInfo->pNativeHandle)) {
        returnValue = KNI_TRUE;
    }
UnlockAudioMutex();            

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nStop ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nStop) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR("+nStop\n");

LockAudioMutex();            
    if (pKniInfo && JAVACALL_TRUE == jmmpCheckCondition(pKniInfo, CHECK_ISPLAYING)) {
        if (JAVACALL_OK == javacall_media_stop(pKniInfo->pNativeHandle)) {
            returnValue = KNI_TRUE;
        }
    } else {
        REPORT_ERROR(LC_MMAPI, "nStop fail cause we are not in playing\n");
    }
UnlockAudioMutex();            
    
    KNI_ReturnBoolean(returnValue);
}

/*  protected native int nGetMediaTime ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetMediaTime) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result ret;
    long ms = -1;
LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        ret = javacall_media_get_time(pKniInfo->pNativeHandle, &ms);
        if (ret != JAVACALL_OK) {
            ms = -1;
        }
    }
UnlockAudioMutex();            

    MMP_DEBUG_STR1("-nGetMediaTime time=%d\n", ms);

    KNI_ReturnInt((jint)ms);
}

/*  protected native int nSetMediaTime ( int handle , long ms ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nSetMediaTime) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    long ms = (long)KNI_GetParameterAsLong(2);
    javacall_result ret = JAVACALL_FAIL;

    MMP_DEBUG_STR("+nSetMediaTime\n");

LockAudioMutex();            
    if (pKniInfo) {
        ret = javacall_media_set_time(pKniInfo->pNativeHandle, &ms);
    } else {
        REPORT_ERROR(LC_MMAPI, "nSetMediaTime fail\n");
    }
UnlockAudioMutex();            
    if (ret != JAVACALL_OK) {
        ms = -1;
    }

    KNI_ReturnInt((jint)ms);
}

/*  protected native int nGetDuration ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetDuration) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    long ms;
    javacall_result ret = JAVACALL_FAIL;

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        ret = javacall_media_get_duration(pKniInfo->pNativeHandle, &ms);
    }
UnlockAudioMutex();            
    if (ret != JAVACALL_OK) {
        ms = -1;
    }

    MMP_DEBUG_STR1("-nGetDuration duration=%d\n", ms);

    KNI_ReturnInt((jint)ms);
}

/*  protected native boolean nPause ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nPause) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR("+nPause\n");  

LockAudioMutex();            
    if (pKniInfo && JAVACALL_TRUE == jmmpCheckCondition(pKniInfo, CHECK_ISPLAYING)) {
        if (JAVACALL_OK == javacall_media_pause(pKniInfo->pNativeHandle)) {
            returnValue = KNI_TRUE;
        }
    } else {
        REPORT_ERROR(LC_MMAPI, "nPause fail cause is not in playing\n");    
    }
UnlockAudioMutex();            

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nResume ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nResume) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    MMP_DEBUG_STR("+nResume\n");  

LockAudioMutex();            
    if (pKniInfo && JAVACALL_TRUE == jmmpCheckCondition(pKniInfo, CHECK_ISPLAYING)) {
        if (JAVACALL_OK == javacall_media_resume(pKniInfo->pNativeHandle)) {
            returnValue = KNI_TRUE;
        }    
    } else {
        REPORT_ERROR(LC_MMAPI, "nResume fail cause is not in playing\n");    
    }
UnlockAudioMutex();            

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsNeedBuffering ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsNeedBuffering) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_TRUE;
    javacall_bool isHandled = JAVACALL_FALSE;

LockAudioMutex();            
    /* Is buffering handled by device side? */
    if (pKniInfo && pKniInfo->pNativeHandle &&
        JAVACALL_OK == javacall_media_download_handled_by_device(pKniInfo->pNativeHandle,
                                                                            &isHandled)) {
        returnValue = (isHandled == JAVACALL_TRUE) ? KNI_FALSE : KNI_TRUE;
    }
UnlockAudioMutex();            

    KNI_ReturnBoolean(returnValue);
}

/*************************************************************************/

/*  protected native boolean nSwitchToForeground ( int hNative, int options ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSwitchToForeground) {
    jboolean returnValue = KNI_FALSE;
#if ENABLE_MULTIPLE_ISOLATES
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR2("nSwitchToForeground %d %d\n", pKniInfo, options);

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (1 != pKniInfo->isForeground) {
            pKniInfo->isForeground = 1;
            if (JAVACALL_SUCCEEDED(javacall_media_to_foreground(
                pKniInfo->pNativeHandle, pKniInfo->appId))) {
                returnValue = KNI_TRUE;
            }
        } else {
            returnValue = KNI_TRUE;
        }
    }
UnlockAudioMutex();            
#endif
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nSwitchToBackground ( int hNative, int options ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSwitchToBackground) {
    jboolean returnValue = KNI_FALSE;
#if ENABLE_MULTIPLE_ISOLATES
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR2("nSwitchToBackground %d %d\n", pKniInfo, options);

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (0 != pKniInfo->isForeground) {
            pKniInfo->isForeground = 0;
            if (JAVACALL_SUCCEEDED(javacall_media_to_background(
                pKniInfo->pNativeHandle, pKniInfo->appId))) {
                returnValue = KNI_TRUE;
            }
        } else {
            returnValue = KNI_TRUE;
        }
    }
UnlockAudioMutex();            
#endif
    KNI_ReturnBoolean(returnValue);
}

/*************************************************************************/

/* Native finalizer */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_finalize) {
    jint handle;
    jint state;
    KNIPlayerInfo* pKniInfo;
#ifdef ENABLE_MEDIA_RECORD
    KNI_StartHandles(3);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(record_instance);
#else
    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
#endif
    
    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
    
    /* Get field of this object */
    handle = KNI_GetIntField(instance, KNI_GetFieldID(clazz, "hNative", "I"));
    state = KNI_GetIntField(instance, KNI_GetFieldID(clazz, "state", "I"));

    MMP_DEBUG_STR2("+finalize handle=%d state=%d\n", handle, state);

    pKniInfo = (KNIPlayerInfo*)handle;

    if (pKniInfo && pKniInfo->pNativeHandle) {
#ifdef ENABLE_MEDIA_RECORD        
        /* Get record control instance and clazz */
        KNI_GetObjectField(instance, 
                       KNI_GetFieldID(clazz, "recordControl", "Lcom/sun/mmedia/DirectRecord;"), 
                       record_instance);
LockAudioMutex();            
        if (KNI_FALSE == KNI_IsNullHandle(record_instance)) {
            MMP_DEBUG_STR1("stop recording by finalizer recordState=%d\n", pKniInfo->recordState);
            switch(pKniInfo->recordState) {
            case RECORD_START:
                javacall_media_stop_recording(pKniInfo->pNativeHandle);
                /* NOTE - Intentionally omit break */
            case RECORD_STOP:
                javacall_media_reset_recording(pKniInfo->pNativeHandle);
                /* NOTE - Intentionally omit break */
            case RECORD_RESET:
            case RECORD_COMMIT:
            case RECORD_INIT:
                javacall_media_close_recording(pKniInfo->pNativeHandle);
                break;
            }
            pKniInfo->recordState = RECORD_CLOSE;
            //KNI_SetBooleanField(record_instance, KNI_GetFieldID(clazz, "recording", "Z"), KNI_FALSE);
        }
UnlockAudioMutex();            
#endif
        /* Stop playing, delete cache, release device and terminate library */ 
LockAudioMutex();            
        if (STARTED == state) {
            MMP_DEBUG_STR("stopped by finalizer\n");
            javacall_media_stop(pKniInfo->pNativeHandle);
            javacall_media_clear_buffer(pKniInfo->pNativeHandle);
        }
        javacall_media_release_device(pKniInfo->pNativeHandle);
        javacall_media_close(pKniInfo->pNativeHandle);
UnlockAudioMutex();            
        javacall_media_destroy(pKniInfo->pNativeHandle);

        KNI_SetIntField(instance, KNI_GetFieldID(clazz, "hNative", "I"), 0);
    }

    if (pKniInfo) {
        MMP_FREE(pKniInfo);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  protected native boolean nPcmAudioPlayback ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nPcmAudioPlayback) {
    KNI_ReturnBoolean(KNI_FALSE);
}
