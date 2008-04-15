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

#include "jsrop_exceptions.h"
#include "jsr135_sync.h"
#include "javautil_string.h"

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
    javacall_result result = JAVACALL_FAIL;

    MMP_DEBUG_STR("+nAcquireDevice\n");

LockAudioMutex();
    if (pKniInfo && pKniInfo->pNativeHandle) {
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_acquire_device(pKniInfo->pNativeHandle),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_DEVICE_ACQUIRED,
            returns_no_data
        );
        if (result == JAVACALL_OK) {
            pKniInfo->isAcquire = JAVACALL_TRUE;
            returnValue = KNI_TRUE;
        }
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
    javacall_result result;
    
    MMP_DEBUG_STR("+nStart\n");

    if (NULL == pKniInfo || NULL == pKniInfo->pNativeHandle) {
        KNI_ReturnBoolean(KNI_FALSE);
    }

LockAudioMutex();            
    JAVACALL_MM_ASYNC_EXEC(
        result,
        javacall_media_start(pKniInfo->pNativeHandle),
        pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_STARTED,
        returns_no_data
    );
UnlockAudioMutex();            

    if (JAVACALL_OK != result) {
        KNI_ReturnBoolean(KNI_FALSE);
    }
    
    KNI_ReturnBoolean(KNI_TRUE);
}

/*  protected native boolean nStop ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nStop) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result result;
    
    MMP_DEBUG_STR("+nStop\n");

    if (NULL == pKniInfo || NULL == pKniInfo->pNativeHandle || 
            JAVACALL_TRUE != jmmpCheckCondition(pKniInfo, CHECK_ISPLAYING)) {
        REPORT_ERROR(LC_MMAPI, "nStop fail cause we are not in playing\n");
        KNI_ReturnBoolean(KNI_FALSE);
    }

LockAudioMutex();            
    JAVACALL_MM_ASYNC_EXEC(
        result,
        javacall_media_stop(pKniInfo->pNativeHandle),
        pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_STOPPED,
        returns_no_data
    );
UnlockAudioMutex();            

    if (JAVACALL_OK != result) {
        KNI_ReturnBoolean(KNI_FALSE);
    }
    
    KNI_ReturnBoolean(KNI_TRUE);
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

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();

        JAVACALL_MM_ASYNC_EXEC(
            ret,
            javacall_media_set_time(pKniInfo->pNativeHandle, &ms),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_TIME_SET,
            returns_data((&ms))
        );

UnlockAudioMutex();            
    } else {
        ms = -1;
        REPORT_ERROR(LC_MMAPI, "nSetMediaTime fail\n");
    }
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

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();

        JAVACALL_MM_ASYNC_EXEC(
            ret,
            javacall_media_get_duration(pKniInfo->pNativeHandle, &ms),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_DURATION_GOTTEN,
            returns_data((&ms))
        );
        
UnlockAudioMutex();            
    }
    
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
    javacall_result result = JAVACALL_FAIL;

    MMP_DEBUG_STR("+nPause\n");  

    if (NULL == pKniInfo || NULL == pKniInfo->pNativeHandle || 
            JAVACALL_TRUE != jmmpCheckCondition(pKniInfo, CHECK_ISPLAYING)) {
        REPORT_ERROR(LC_MMAPI, "nPause fail cause is not in playing\n\n");
        KNI_ReturnBoolean(KNI_FALSE);
    } else {

LockAudioMutex();            
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_pause(pKniInfo->pNativeHandle),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_PAUSED,
            returns_no_data
        );
UnlockAudioMutex();
    }

    KNI_ReturnBoolean(JAVACALL_OK == result? KNI_TRUE: KNI_FALSE);
}

/*  protected native boolean nResume ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nResume) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result result = JAVACALL_FAIL;

    MMP_DEBUG_STR("+nResume\n");  

    if (!pKniInfo || !pKniInfo->pNativeHandle || JAVACALL_TRUE != jmmpCheckCondition(pKniInfo, CHECK_ISPLAYING)) {
        REPORT_ERROR(LC_MMAPI, "nResume fail cause is not in playing\n\n");
        KNI_ReturnBoolean(KNI_FALSE);
    } else {

LockAudioMutex();            
        JAVACALL_MM_ASYNC_EXEC(
            result,
            javacall_media_resume(pKniInfo->pNativeHandle),
            pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_RESUMED,
            returns_no_data
        );
UnlockAudioMutex();
    }

    KNI_ReturnBoolean(JAVACALL_OK == result? KNI_TRUE: KNI_FALSE);
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

/*  private native boolean nPrefetch(int hNative); */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nPrefetch) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();
        if (JAVACALL_OK == javacall_media_prefetch(pKniInfo->pNativeHandle)) {
            pKniInfo->isAcquire = JAVACALL_TRUE;
            returnValue = KNI_TRUE;
        }
UnlockAudioMutex();            
    }
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsFramePositioningControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsFramePositioningControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_FRAME_POSITIONING) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsMetaDataControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsMetaDataControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_METADATA) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsMIDIControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsMIDIControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_MIDI) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsPitchControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsPitchControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_PITCH) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsRateControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsRateControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_RATE) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsRecordControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsRecordControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_RECORD) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsStopTimeControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsStopTimeControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_STOPTIME) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsTempoControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsTempoControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_TEMPO) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsVideoControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsVideoControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_VIDEO) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsToneControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsToneControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_TONE) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsVolumeControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsVolumeControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;

    int controls;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (controls & JAVACALL_MEDIA_CTRL_VOLUME) {
                returnValue = KNI_TRUE;
            }
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/*  protected native String nGetContentType(int handle); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetContentType)
{
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_media_format_type mFormat = JAVACALL_MEDIA_FORMAT_UNKNOWN;

    javacall_media_configuration *cfg;
    javacall_media_caps *caps;

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_ReleaseHandle(stringObj);

    if (pKniInfo && pKniInfo->pNativeHandle) {
        LockAudioMutex();            

        if( JAVACALL_OK == javacall_media_get_format(pKniInfo->pNativeHandle, &mFormat) ) {
            if( NULL != mFormat ) {
                if( JAVACALL_OK == javacall_media_get_configuration(&cfg) ) {
                    for( caps = cfg->mediaCaps; 
                         caps != NULL && caps->mediaFormat != NULL;
                         caps++ ) {
                        if( javautil_string_equals( caps->mediaFormat, mFormat ) ) {
                            const char* ct = caps->contentTypes;
                            KNI_NewStringUTF(ct, stringObj);
                        }
                    }
                }
            }
        }

        UnlockAudioMutex();            
    }
    KNI_EndHandlesAndReturnObject(stringObj);
}

