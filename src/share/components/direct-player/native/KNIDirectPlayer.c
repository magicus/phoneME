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

#include "jsrop_exceptions.h"
#include "jsr135_sync.h"
#include "javautil_string.h"
#include "sni.h"


/* Global Variables ************************************************************************/

static jboolean g_isRadioTunerTakenByJava = KNI_FALSE;

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
#define CHECK_ALL               (CHECK_CURRENT_PLAYER | CHECK_BUFFER)
#define CHECK_ISPLAYING         (CHECK_CURRENT_PLAYER)       
#define CHECK_CURRENT_PLAYER    (0x00000001)
#define CHECK_BUFFER            (0x00000004)
#define CHECK_ISTEMP            (0x00000008)

    if ((conditions & CHECK_ISTEMP) == CHECK_ISTEMP) {
        if (JAVACALL_TRUE == pKniInfo->isDirectFile) {
            MMP_DEBUG_STR("[KNIDirectPlayer] CHECK_ISTEMP is fail\n");
            return JAVACALL_FALSE;
        }
    }

    return JAVACALL_TRUE;
}

/* The 1st parameter is the number of the native method parameter
   that is an AsyncExecutor instance */
static void setResultAndSyncMode( KNIDECLARGS int parNum, javacall_result res );

/* KNI Implementation **********************************************************************/

/*  protected native int nClose ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nClose) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 1;
    
    MMP_DEBUG_STR("+DirectPlayer.nClose\n");
    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    
    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
LockAudioMutex();            
 
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_FAIL == javacall_media_destroy(pKniInfo->pNativeHandle)) {
            returnValue = 0;
        }
        pKniInfo->pNativeHandle = NULL;
    }
    if( pKniInfo ) {
        MMP_FREE(pKniInfo);
        pKniInfo = NULL;
    }
UnlockAudioMutex();            

    KNI_EndHandles();
    MMP_DEBUG_STR("-DirectPlayer.nClose\n");
    KNI_ReturnInt(returnValue);
}

/*  protected native void nReleaseDevice ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_nReleaseDevice) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result result = JAVACALL_FAIL;

    MMP_DEBUG_STR("+nReleaseDevice\n");

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle ) {
        result = javacall_media_deallocate( pKniInfo->pNativeHandle );
    }
UnlockAudioMutex();            

    MMP_DEBUG_STR("-nReleaseDevice\n");
    KNI_ReturnVoid();
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
    result = javacall_media_run( pKniInfo->pNativeHandle );
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
        ret = javacall_media_get_media_time(pKniInfo->pNativeHandle, &ms);
        if (ret != JAVACALL_OK) {
            ms = -1;
        }
    }
UnlockAudioMutex();

    MMP_DEBUG_STR1("-nGetMediaTime time=%d\n", ms);

    KNI_ReturnInt((jint)ms);
}

/*  protected native boolean nSetMediaTime ( int handle , long ms ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSetMediaTime) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    long ms = (long)KNI_GetParameterAsLong(2);
    javacall_result ret = JAVACALL_FAIL;

    MMP_DEBUG_STR("+nSetMediaTime\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();
        ret = javacall_media_set_media_time( pKniInfo->pNativeHandle, ms );
UnlockAudioMutex();            
    } 

    KNI_ReturnBoolean( JAVACALL_OK == ret ? KNI_TRUE : KNI_FALSE );
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
            returns_data(1, (&ms))
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
        result = javacall_media_pause( pKniInfo->pNativeHandle );
UnlockAudioMutex();
    }

    KNI_ReturnBoolean(JAVACALL_OK == result? KNI_TRUE: KNI_FALSE);
}

/*************************************************************************/

/*  protected native boolean nSwitchToForeground ( int hNative, int options ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nSwitchToForeground) {
    jboolean returnValue = KNI_FALSE;
#if ENABLE_MULTIPLE_ISOLATES
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR1("nSwitchToForeground %d\n", pKniInfo);

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

    MMP_DEBUG_STR1("nSwitchToBackground %d\n", pKniInfo);

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

static void do_finalize(KNIDECLARGS int dummy) {
    jint handle;
    jint state;
    KNIPlayerInfo* pKniInfo;
    (void)dummy;

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
        if( KNI_TRUE == KNI_GetBooleanField( instance, 
            KNI_GetFieldID( clazz, "hasTakenRadioAccess", "Z" ) ) )
        {
            g_isRadioTunerTakenByJava = KNI_FALSE;
        }
UnlockAudioMutex();
        javacall_media_destroy(pKniInfo->pNativeHandle);

        KNI_SetIntField(instance, KNI_GetFieldID(clazz, "hNative", "I"), 0);
    }

    if (pKniInfo) {
        MMP_FREE(pKniInfo);
    }

    KNI_EndHandles();
}

/* Native finalizer */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_finalize) {
    MMP_DEBUG_STR("+DirectPlayer.finalize\n");
    do_finalize(KNIPASSARGS 0);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectMIDI_finalize) {
    MMP_DEBUG_STR("+DirectMIDI.finalize\n");
    do_finalize(KNIPASSARGS 0);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectTone_finalize) {
    MMP_DEBUG_STR("+DirectTone.finalize\n");
    do_finalize(KNIPASSARGS 0);
    KNI_ReturnVoid();
}


/*  protected native boolean nPcmAudioPlayback ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nPcmAudioPlayback) {
    KNI_ReturnBoolean(KNI_FALSE);
}

static void setResultAndSyncMode( KNIDECLARGS int parNum, javacall_result res ) {

    KNI_StartHandles( 2 );
    KNI_DeclareHandle( asyncExecutor );
    KNI_DeclareHandle( clazz );
    
    KNI_GetParameterAsObject( parNum, asyncExecutor );
    
    if( KNI_IsNullHandle( asyncExecutor ) ) {
        return;
    }
    
    KNI_GetObjectClass( asyncExecutor, clazz );
    
    KNI_SetIntField( asyncExecutor, KNI_GetFieldID( clazz, "result", "I" ), ( jint )res );
    if( JAVACALL_WOULD_BLOCK == res ) {
        KNI_SetBooleanField( asyncExecutor, 
            KNI_GetFieldID( clazz, "isBlockedUntilEvent", "Z" ), KNI_TRUE );
    }
    
    KNI_EndHandles();
}

/*  private native void nPrefetch(int hNative, AsyncExecutor ae ); */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_nPrefetch) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result res = JAVACALL_FAIL;

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();
        res = javacall_media_prefetch( pKniInfo->pNativeHandle );
        setResultAndSyncMode( KNIPASSARGS 2, res );
UnlockAudioMutex();
    }
    
    
    KNI_ReturnVoid();
}

static jboolean controlSupported( KNIPlayerInfo* pKniInfo, int ctl_mask ) {

    int controls = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_player_controls(pKniInfo->pNativeHandle, &controls)) {
            if (ctl_mask == ( controls & ctl_mask )) {
                return KNI_TRUE;
            }
        }
    }

    return KNI_FALSE;
}

/*  protected native boolean nIsFramePositioningControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsFramePositioningControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_FRAME_POSITIONING);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsMetaDataControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsMetaDataControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_METADATA);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsMIDIControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsMIDIControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_MIDI);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsPitchControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsPitchControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_PITCH);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsRateControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsRateControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_RATE);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsRecordControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsRecordControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_RECORD);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsStopTimeControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsStopTimeControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_STOPTIME);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsTempoControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsTempoControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_TEMPO);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsVideoControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsVideoControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_VIDEO);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsToneControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsToneControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_TONE);
    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nIsVolumeControlSupported ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nIsVolumeControlSupported) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = controlSupported(pKniInfo,JAVACALL_MEDIA_CTRL_VOLUME);
    KNI_ReturnBoolean(returnValue);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DirectPlayer_nAcquireRadioAccess)
{
    jboolean returnValue = KNI_FALSE;
    
    LockAudioMutex();
    if( KNI_FALSE == g_isRadioTunerTakenByJava )
    {
        g_isRadioTunerTakenByJava = KNI_TRUE;
        returnValue = KNI_TRUE;
    }
    UnlockAudioMutex();
    
    KNI_ReturnBoolean( returnValue );
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_nReleaseRadioAccess)
{
    LockAudioMutex();
    g_isRadioTunerTakenByJava = KNI_FALSE;
    UnlockAudioMutex();
    
    KNI_ReturnVoid();
}

/*  protected native String nGetContentType(int handle); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetContentType)
{
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_media_format_type mFormat = JAVACALL_MEDIA_FORMAT_UNKNOWN;

    const javacall_media_configuration *cfg;
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

/*  protected native void nStartSnapshot( int handle, String imageType ); */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectPlayer_nStartSnapshot) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo*  pKniInfo = (KNIPlayerInfo*)handle;
    jint            imageTypeLength;
    jchar*          pImageTypeStr;
    javacall_result ret = JAVACALL_FAIL;
    KNI_StartHandles(1);
    KNI_DeclareHandle(imageTypeHandle);
    KNI_GetParameterAsObject(2, imageTypeHandle);
    
    MMP_DEBUG_STR("[kni_video] +nStartSnapshot\n");
    
    if (pKniInfo && pKniInfo->pNativeHandle) {
        imageTypeLength = KNI_GetStringLength(imageTypeHandle);
        pImageTypeStr = MMP_MALLOC(imageTypeLength * sizeof(jchar));
        if (pImageTypeStr) {
            KNI_GetStringRegion(imageTypeHandle, 0, imageTypeLength, pImageTypeStr);
            ret = javacall_media_start_video_snapshot( pKniInfo->pNativeHandle,
                        pImageTypeStr, imageTypeLength );
            MMP_FREE(pImageTypeStr);
            if (ret != JAVACALL_OK) {
                KNI_ThrowNew( "javax/microedition/media/MediaException",
                    "\nFailed to start Camera Snapshot\n");
            }
        }
    }
    
    KNI_EndHandles();
    
    MMP_DEBUG_STR("[kni_video] -nStartSnapshot\n");
    KNI_ReturnVoid();
}


/*  protected native byte[] nGetSnapshotData( int handle ); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetSnapshotData) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo*  pKniInfo = (KNIPlayerInfo*)handle;
    javacall_result ret = JAVACALL_FAIL;
    long dataBytes = 0;
    jbyte *tmpArray = NULL;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(returnValueHandle);

    MMP_DEBUG_STR("[kni_video] +nGetSnapshotData\n");

    ret = javacall_media_get_video_snapshot_data_size(pKniInfo->pNativeHandle, &dataBytes);
    if( JAVACALL_OK != ret || 0 >= dataBytes )
    {
        KNI_ThrowNew( "javax/microedition/media/MediaException",
            "\nFailed to get Camera Snapshot data size\n");
        goto end;
    }
    MMP_DEBUG_STR1("[kni_video] nSnapShot get data size %d\n", dataBytes);

    /* Create new Java byte array object to store snapshot data */
    SNI_NewArray(SNI_BYTE_ARRAY, dataBytes, returnValueHandle);
    if (KNI_IsNullHandle(returnValueHandle)) {
        KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        goto end;
    }
    
    tmpArray = MMP_MALLOC( dataBytes );
    if( NULL == tmpArray )
    {
        KNI_ReleaseHandle( returnValueHandle );
        KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        goto end;
    }
    
    ret = javacall_media_get_video_snapshot_data(pKniInfo->pNativeHandle, 
                      ( char* )tmpArray, dataBytes);
    if (JAVACALL_OK != ret) {
        KNI_ReleaseHandle(returnValueHandle);
        KNI_ThrowNew( "javax/microedition/media/MediaException",
            "\nFailed to get Camera Snapshot data\n");
    }
    else {
        KNI_SetRawArrayRegion(returnValueHandle, 0, dataBytes, tmpArray);
    }

    MMP_FREE( tmpArray );

    MMP_DEBUG_STR1("[kni_video] -nGetSnapshotData %d\n", returnValueHandle);

end:
    KNI_EndHandlesAndReturnObject(returnValueHandle);
}
