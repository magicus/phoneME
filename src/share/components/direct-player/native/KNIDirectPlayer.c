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
#include "sni.h"

#include "jsrop_exceptions.h"
#include "jsr135_sync.h"

/* Global Variables ************************************************************************/

javacall_int64 g_currentPlayer = -1;

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

#if 0
    if ((conditions & CHECK_CURRENT_PLAYER) == CHECK_CURRENT_PLAYER) {
        if (pKniInfo->uniqueId != g_currentPlayer) {
            MMP_DEBUG_STR("[KNIDirectPlayer] CHECK_CURRENT_PLAYER fail\n");
            return JAVACALL_FALSE;
        }
    }
#endif

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
        CVMD_gcSafeExec(_ee, {
            if (JAVACALL_FAIL == javacall_media_close(pKniInfo->pNativeHandle)) {
                returnValue = 0;
            }
        });
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
    CVMD_gcSafeExec(_ee, {
        if (pKniInfo && JAVACALL_OK == javacall_media_acquire_device(pKniInfo->pNativeHandle)) {
            pKniInfo->isAcquire = JAVACALL_TRUE;
            returnValue = KNI_TRUE;
        }
    });
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
        CVMD_gcSafeExec(_ee, {
            javacall_media_release_device(pKniInfo->pNativeHandle);
        });
        pKniInfo->isAcquire = JAVACALL_FALSE;
    }
UnlockAudioMutex();            

    KNI_ReturnVoid();
}

/*  protected native int nBuffering ( int handle , Object buffer, long length ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nBuffering) {

    jint handle = KNI_GetParameterAsInt(1);
    jlong length = KNI_GetParameterAsLong(3);
    jint returnValue = -1;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(bufferHandle);
    KNI_GetParameterAsObject(2, bufferHandle);
    
LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle && length > 0) {
        int ret;
        jbyte *tmpArray = MMP_MALLOC(length);
        if (tmpArray != NULL) {
            MMP_DEBUG_STR2("+nBuffering length=%d offset=%d\n", length, pKniInfo->offset);
            KNI_GetRawArrayRegion(bufferHandle, 0, length, (jbyte*)tmpArray);
            CVMD_gcSafeExec(_ee, {
                ret = javacall_media_do_buffering(pKniInfo->pNativeHandle, 
                    (const char*)tmpArray, (int)length, pKniInfo->offset);
            });
            MMP_FREE(tmpArray);
            if (ret > 0) {
                pKniInfo->offset += ret;
                returnValue = ret;
            }
        } else {
            REPORT_ERROR(LC_MMAPI, "[kni_player] nBuffering allocation fail\n");                
            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        }
    } else if (pKniInfo && pKniInfo->pNativeHandle) {
        /* Indicate end of buffering by using NULL buffer */
        CVMD_gcSafeExec(_ee, {
            returnValue = javacall_media_do_buffering(pKniInfo->pNativeHandle, NULL, -1, -1);
        });
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
        CVMD_gcSafeExec(_ee, {
            if (JAVACALL_OK == javacall_media_clear_buffer(pKniInfo->pNativeHandle)) {
                pKniInfo->offset = 0;   /* reset offset value */
                returnValue = KNI_TRUE;
            } else {
                REPORT_ERROR(LC_MMAPI, "javacall_media_clear_buffer return fail");
            }
        });
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
    CVMD_gcSafeExec(_ee, {
        if (pKniInfo && pKniInfo->pNativeHandle && JAVACALL_OK == javacall_media_start(pKniInfo->pNativeHandle)) {
            g_currentPlayer = pKniInfo->uniqueId;    /* set the current player */
            returnValue = KNI_TRUE;
        }
    });
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
        CVMD_gcSafeExec(_ee, {
            if (JAVACALL_OK == javacall_media_stop(pKniInfo->pNativeHandle)) {
                returnValue = KNI_TRUE;
            }
        });
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
    jint returnValue = -1;
LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        CVMD_gcSafeExec(_ee, {
            returnValue = javacall_media_get_time(pKniInfo->pNativeHandle);
        });
    }
UnlockAudioMutex();            

    MMP_DEBUG_STR1("-nGetMediaTime time=%d\n", returnValue);

    KNI_ReturnInt(returnValue);
}

/*  protected native int nSetMediaTime ( int handle , long ms ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nSetMediaTime) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jlong ms = KNI_GetParameterAsLong(2);
    jint returnValue = -1;

    MMP_DEBUG_STR("+nSetMediaTime\n");

LockAudioMutex();            
    if (pKniInfo) {
        CVMD_gcSafeExec(_ee, {
            returnValue = javacall_media_set_time(pKniInfo->pNativeHandle, (int)ms);
        });
    } else {
        REPORT_ERROR(LC_MMAPI, "nSetMediaTime fail\n");
    }
UnlockAudioMutex();            

    KNI_ReturnInt(returnValue);
}

/*  protected native int nGetDuration ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetDuration) {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = -1;

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        CVMD_gcSafeExec(_ee, {
            returnValue = javacall_media_get_duration(pKniInfo->pNativeHandle);
        });
    }
UnlockAudioMutex();            

    MMP_DEBUG_STR1("-nGetDuration duration=%d\n", returnValue);

    KNI_ReturnInt(returnValue);
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
        CVMD_gcSafeExec(_ee, {
            if (JAVACALL_OK == javacall_media_pause(pKniInfo->pNativeHandle)) {
                returnValue = KNI_TRUE;
            }
        });
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
        CVMD_gcSafeExec(_ee, {
            if (JAVACALL_OK == javacall_media_resume(pKniInfo->pNativeHandle)) {
                returnValue = KNI_TRUE;
            }
        });
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

LockAudioMutex();            
    /* Is buffering handled by device side? */
    CVMD_gcSafeExec(_ee, {
        if (pKniInfo && pKniInfo->pNativeHandle &&
            JAVACALL_OK == javacall_media_protocol_handled_by_device(pKniInfo->pNativeHandle)) {
            returnValue = KNI_FALSE;
        }
    });
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
    jint options = KNI_GetParameterAsInt(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR2("nSwitchToForeground %d %d\n", pKniInfo, options);

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (1 != pKniInfo->isForeground) {
            pKniInfo->isForeground = 1;
            CVMD_gcSafeExec(_ee, {
                if (JAVACALL_SUCCEEDED(javacall_media_to_foreground(
                    pKniInfo->pNativeHandle, options))) {
                    returnValue = KNI_TRUE;
                }
            });
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
    jint options = KNI_GetParameterAsInt(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR2("nSwitchToBackground %d %d\n", pKniInfo, options);

LockAudioMutex();            
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (0 != pKniInfo->isForeground) {
            pKniInfo->isForeground = 0;
            CVMD_gcSafeExec(_ee, {
                if (JAVACALL_SUCCEEDED(javacall_media_to_background(
                    pKniInfo->pNativeHandle, options))) {
                    returnValue = KNI_TRUE;
                }
            });
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
            CVMD_gcSafeExec(_ee, {
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
            });
            pKniInfo->recordState = RECORD_CLOSE;
            //KNI_SetBooleanField(record_instance, KNI_GetFieldID(clazz, "recording", "Z"), KNI_FALSE);
        }
UnlockAudioMutex();            
#endif
        /* Stop playing, delete cache, release device and terminate library */ 
LockAudioMutex();            
        CVMD_gcSafeExec(_ee, {
            if (STARTED == state) {
                MMP_DEBUG_STR("stopped by finalizer\n");
                javacall_media_stop(pKniInfo->pNativeHandle);
                javacall_media_clear_buffer(pKniInfo->pNativeHandle);
            }
            javacall_media_release_device(pKniInfo->pNativeHandle);
            javacall_media_close(pKniInfo->pNativeHandle);
        });
UnlockAudioMutex();            
        CVMD_gcSafeExec(_ee, {
            javacall_media_destroy(pKniInfo->pNativeHandle);
        });

        KNI_SetIntField(instance, KNI_GetFieldID(clazz, "hNative", "I"), 0);
    }

    if (pKniInfo) {
        MMP_FREE(pKniInfo);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#ifdef ENABLE_CDC
static char print_buf_[100000];
static int print_buf_i_ = 0;
static int print_inited_ = 0;
static CVMMutex print_mutex_;
static CVMCondVar print_condvar_;

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectPlayer_nGetDebugLines) {

    if (!print_inited_) {
        CVMmutexInit(&print_mutex_);
        CVMcondvarInit(&print_condvar_, &print_mutex_);
        print_inited_ = 1;
    }
    KNI_StartHandles(1);
    KNI_DeclareHandle(returnValueHandle);
    CVMmutexLock(&print_mutex_);
    if (print_buf_i_ == 0) {
        CVMD_gcSafeExec(_ee, {
            CVMcondvarWait(&print_condvar_, &print_mutex_, 0);
        });
    }
    if (print_buf_i_ > 0) {
        SNI_NewArray(SNI_BYTE_ARRAY, (long)print_buf_i_, returnValueHandle);
    
        if (KNI_IsNullHandle(returnValueHandle)) {
            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        } else {
            KNI_SetRawArrayRegion(returnValueHandle, 0, (long)print_buf_i_, (jbyte*)print_buf_);
        }
        print_buf_i_ = 0;
    }
    CVMmutexUnlock(&print_mutex_);
    KNI_EndHandlesAndReturnObject(returnValueHandle);
}


static int write_to_log(const void *buf, CVMUint32 nBytes) {
    static char full_buf_msg_[] = "<<<\n**** Static buffer is FULL\n";

    if (!print_inited_) {
        CVMmutexInit(&print_mutex_);
        CVMcondvarInit(&print_condvar_, &print_mutex_);
        print_inited_ = 1;
    }
    CVMmutexLock(&print_mutex_);
    if (print_buf_i_ + nBytes < sizeof print_buf_ - 1) {
        memcpy(print_buf_ + print_buf_i_, buf, nBytes);
        print_buf_i_ += nBytes;
        print_buf_[print_buf_i_++] = 0;
    } else {
        char *p = print_buf_ + sizeof print_buf_ - sizeof full_buf_msg_ - 1 - 1;
        memcpy(p, full_buf_msg_, sizeof full_buf_msg_);
        print_buf_i_ = (p - print_buf_) + sizeof full_buf_msg_;
    }
    CVMcondvarNotifyAll(&print_condvar_);
    CVMmutexUnlock(&print_mutex_);
}

int perror(const char *p) {
    return write_to_log(p, strlen(p));
}
int fprintf(FILE *fp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (fp == stdout || fp == stderr) {
        char buf[2048];
        vsprintf(buf, fmt, args);
        va_end(args);
        return write_to_log(buf, strlen(buf));
    } else {
        fprintf(stderr, "Bad descriptor\n");
    }
    return 0;
}
int printf(const char *fmt, ...) {
    va_list args;
    char buf[2048];
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    return write_to_log(buf, strlen(buf));
}
#endif

