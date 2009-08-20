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

/* KNI Implementation **********************************************************************/

/*  protected native int nInit (int appId, int playerId, String URI) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_HighLevelPlayer_nInit) {
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
        pKniInfo->isDirectFile = JAVACALL_FALSE;
        pKniInfo->isForeground = -1;
        pKniInfo->recordState = RECORD_CLOSE;
        pKniInfo->isClosed = KNI_FALSE;
        res = javacall_media_create(appId, playerId, pszURI, URILength, &pKniInfo->pNativeHandle); 
        if (res == JAVACALL_OK) {
            returnValue = (int)pKniInfo;
        } else if (res == JAVACALL_IO_ERROR) {
            MMP_FREE(pKniInfo);
            KNI_ThrowNew( "java/io/IOException",
                "\nUnable to create native player\n" );
            returnValue = -1; /* Can not create player - IO error */
        } 
        else if ( JAVACALL_NO_AUDIO_DEVICE == res )
        {
            MMP_FREE(pKniInfo);
            KNI_ThrowNew( "javax/microedition/media/MediaException",
"\nNo audio device found. Please check your audio driver settings\n" );
            returnValue = 0; /* Can not create player */
        }
        else {
            MMP_FREE(pKniInfo);
            KNI_ThrowNew( "javax/microedition/media/MediaException",
            "\nUnable to create native player\n" );
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
KNIDECL(com_sun_mmedia_HighLevelPlayer_nTerm) {

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
 
    if (pKniInfo && pKniInfo->pNativeHandle && KNI_FALSE == pKniInfo->isClosed ) {
        if (JAVACALL_FAIL == javacall_media_close(pKniInfo->pNativeHandle)) {
            returnValue = 0;
        }
        pKniInfo->isClosed = KNI_TRUE;
    }
UnlockAudioMutex();            

    if (pKniInfo) {
        MMP_FREE(pKniInfo);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  protected native String nGetMediaFormat(int handle); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_HighLevelPlayer_nGetMediaFormat) {
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
KNIDECL(com_sun_mmedia_HighLevelPlayer_nIsHandledByDevice) {
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
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_HighLevelPlayer_nRealize) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    int mimeLength;
    jchar* pszMime = NULL;
    javacall_result status = JAVACALL_FAIL;

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
    JAVACALL_MM_ASYNC_EXEC(
        status,
        javacall_media_realize(pKniInfo->pNativeHandle, pszMime, mimeLength),
        pKniInfo->pNativeHandle, pKniInfo->appId, pKniInfo->playerId, JAVACALL_EVENT_MEDIA_REALIZED,
        returns_no_data
    );
    if ( JAVACALL_NO_AUDIO_DEVICE == status ) {
        KNI_ThrowNew( "javax/microedition/media/MediaException",
"\nNo audio device found. Please check your audio driver settings\n" );
    }
    else if(status != JAVACALL_OK) {
        KNI_ThrowNew( "javax/microedition/media/MediaException",
            "\nCannot realize\n" );
    }
    

    if (pszMime)      { MMP_FREE(pszMime); }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
