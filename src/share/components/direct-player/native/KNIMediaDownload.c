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


/*  private native void nSetWholeContentSize(int hNative, long contentSize) */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_MediaDownload_nSetWholeContentSize) {
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
KNIDECL(com_sun_mmedia_MediaDownload_nGetJavaBufferSize) {
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
KNIDECL(com_sun_mmedia_MediaDownload_nGetFirstPacketSize) {
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
KNIDECL(com_sun_mmedia_MediaDownload_nNeedMoreDataImmediatelly) {
    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean needMoreData = KNI_FALSE;

    if (pKniInfo) {
        needMoreData = pKniInfo->needMoreData;
    }
    KNI_ReturnBoolean(needMoreData);
}

/*  protected native int nBuffering ( int handle , Object buffer, int offset, int total, int length ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_MediaDownload_nBuffering) {

    jint handle = KNI_GetParameterAsInt(1);
    long offset = (long)KNI_GetParameterAsInt(3);
    long total = (long)KNI_GetParameterAsInt(4);
    long length = (long)KNI_GetParameterAsInt(5);
    jint returnValue = -1;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jbyte *nBuffer;
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
                if ((offset + length) <= total) {
                    KNI_GetRawArrayRegion(bufferHandle, offset, (int)length, (jbyte*)nBuffer);
                } else {
                    KNI_GetRawArrayRegion(bufferHandle, offset, (int)(total-offset), (jbyte*)nBuffer);
                    KNI_GetRawArrayRegion(bufferHandle, 0, (int)offset+length-total, (jbyte*)&nBuffer[total-offset]);
                }
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
