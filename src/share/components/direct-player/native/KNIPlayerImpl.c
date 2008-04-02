/*
 * 
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

#include <string.h>
#include "KNICommon.h"

#include "jsrop_exceptions.h"
#include "jsr135_sync.h"

/* Global Variables ************************************************************************/

/* Externs **********************************************************************************/

extern int unicodeToNative(const jchar *ustr, int ulen, unsigned char *bstr, int blen);

/* KNI Implementation **********************************************************************/

/*  protected native int nInit (int appId, int playerId, String URI) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_PlayerImpl_nInit) {
    jint  appId = KNI_GetParameterAsInt(1);
    jint  playerId = KNI_GetParameterAsInt(2);
    jint  returnValue = 0;
    jint   URILength;
    jchar* pszURI = NULL;
    KNIPlayerInfo* pKniInfo;
    javacall_result res;

    MMP_DEBUG_STR2("+nInit application=%d, player=%d\n", appId, playerId);

    KNI_StartHandles(1);
    KNI_DeclareHandle(URI);
    
    /* Get URI object parameter */
    KNI_GetParameterAsObject(3, URI);

    /* Get URI java string */
    if (-1 == (URILength = KNI_GetStringLength(URI))) {
        pszURI = NULL;
    } else {
        pszURI = MMP_MALLOC((URILength+1) * sizeof(jchar));
        if (pszURI) {
            KNI_GetStringRegion(URI, 0, URILength, pszURI);
            pszURI[URILength] = 0;
        }
    }

    pKniInfo = (KNIPlayerInfo*)MMP_MALLOC(sizeof(KNIPlayerInfo));
LockAudioMutex();
    if (pKniInfo) {
        /* prepare kni internal information */
        pKniInfo->appId = appId;
        pKniInfo->playerId = playerId;
        pKniInfo->isAcquire = 0;
        pKniInfo->isDirectFile = JAVACALL_FALSE;
        pKniInfo->isForeground = -1;
        pKniInfo->recordState = RECORD_CLOSE;
        res = javacall_media_create(appId, playerId, pszURI, URILength, &pKniInfo->pNativeHandle); 
        if (res == JAVACALL_OK) {
            returnValue = (int)pKniInfo;
        } else if (res == JAVACALL_IO_ERROR) {
            MMP_FREE(pKniInfo);
            returnValue = -1; /* Can not create player - IO error */
        } else {
            MMP_FREE(pKniInfo);
            returnValue = 0; /* Can not create player */
        }
    } else {
        if (pKniInfo) { MMP_FREE(pKniInfo); }
    }
UnlockAudioMutex();

    if (pszURI)      { MMP_FREE(pszURI); }
    
    MMP_DEBUG_STR1("-nInit return=%d\n", returnValue);

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  protected native int nTerm ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_PlayerImpl_nTerm) {

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

/*  protected native String nGetMediaFormat(int handle); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_PlayerImpl_nGetMediaFormat) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    javacall_media_format_type mFormat = JAVACALL_MEDIA_FORMAT_UNKNOWN;

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_ReleaseHandle(stringObj);

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();            
        if (JAVACALL_OK == javacall_media_get_format(pKniInfo->pNativeHandle, &mFormat)) {
            if (mFormat != NULL) {
                KNI_NewStringUTF(mFormat, stringObj);
            }
        }
UnlockAudioMutex();            
    }
    KNI_EndHandlesAndReturnObject(stringObj);
}

/*  protected native boolean nIsHandledByDevice(int handle); */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_PlayerImpl_nIsHandledByDevice) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_TRUE;
    javacall_bool isHandled = JAVACALL_FALSE;

LockAudioMutex();            
    /* Is buffering handled by device side? */
if (pKniInfo && pKniInfo->pNativeHandle &&
       JAVACALL_OK == javacall_media_download_handled_by_device(pKniInfo->pNativeHandle,
                                                                            &isHandled)) {
        returnValue = (isHandled == JAVACALL_TRUE) ? KNI_TRUE : KNI_FALSE;
    }
UnlockAudioMutex();            

    KNI_ReturnBoolean(returnValue);
}

/*  protected native boolean nRealize(int handle, String mime); */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_PlayerImpl_nRealize) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    int mimeLength;
    jchar* pszMime = NULL;
    jboolean returnValue = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(mime);
    
    /* Get mimeType */
    KNI_GetParameterAsObject(2, mime);

    /* Get URI java string */
    if (-1 == (mimeLength = KNI_GetStringLength(mime))) {
        pszMime = NULL;
        mimeLength = 0;
    } else {
        pszMime = MMP_MALLOC((mimeLength+1) * sizeof(jchar));
        if (pszMime) {
            KNI_GetStringRegion(mime, 0, mimeLength, pszMime);
            pszMime[mimeLength] = 0;
        } else {
            mimeLength = 0;
        }
    }
    if (JAVACALL_OK == javacall_media_realize(pKniInfo->pNativeHandle, pszMime, mimeLength)) {
        returnValue = KNI_TRUE;
    }

    if (pszMime)      { MMP_FREE(pszMime); }

    KNI_EndHandles();
    KNI_ReturnBoolean(returnValue);
}

/*  private native void nSetWholeContentSize(int hNative, long contentSize) */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_PlayerImpl_nSetWholeContentSize) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    long contentSize = (long)KNI_GetParameterAsLong(2);

    if (pKniInfo && pKniInfo->pNativeHandle) {
LockAudioMutex();
        javacall_media_set_whole_content_size(pKniInfo->pNativeHandle,
                                                contentSize);
UnlockAudioMutex();            
    }
    KNI_ReturnVoid();
}

/*  protected static native int nGetJavaBufferSize(int handle); */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_PlayerImpl_nGetJavaBufferSize) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    long javaBufSize = 0;
    long firstPacketSize = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_OK == javacall_media_get_java_buffer_size(pKniInfo->pNativeHandle,
            &javaBufSize, &firstPacketSize)) {
            pKniInfo->firstPacketSize = firstPacketSize;
        } else {
            javaBufSize = 0;
            pKniInfo->firstPacketSize = -1;
        }
    }

    KNI_ReturnInt((jint)javaBufSize);
}

/*  protected static native int nGetFirstPacketSize(int handle); */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_PlayerImpl_nGetFirstPacketSize) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    long firstPacketSize = 0;

    if (pKniInfo) {
        firstPacketSize = pKniInfo->firstPacketSize;
    }
    KNI_ReturnInt((jint)firstPacketSize);
}

/*  protected static native boolean nNeedMoreDataImmediatelly(int hNative); */
KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_PlayerImpl_nNeedMoreDataImmediatelly) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean needMoreData = KNI_FALSE;

    if (pKniInfo) {
        needMoreData = pKniInfo->needMoreData;
    }
    KNI_ReturnBoolean(needMoreData);
}

/*  protected native int nBuffering ( int handle , Object buffer, int length ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_PlayerImpl_nBuffering) {

    jint handle = KNI_GetParameterAsInt(1);
    long length = (long)KNI_GetParameterAsInt(3);
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
    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (length > 0) {
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
                    returnValue = (jint)min_data_size;
                    pKniInfo->needMoreData = need_more_data;
                }
            }
        } else if (pKniInfo && pKniInfo->pNativeHandle) {
            /* Indicate end of buffering by using NULL buffer */
            length = 0;
            ret = javacall_media_do_buffering(pKniInfo->pNativeHandle, NULL, &length, &need_more_data, &min_data_size);
            if (ret == JAVACALL_OK) {
                returnValue = 0;
            }
        }
    }
UnlockAudioMutex();            

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}
