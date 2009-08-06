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

/*  protected native void nSetSequence ( int handle, Object buffer ); */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectTone_nSetSequence)
{
    jsize length;
    void* ptr;
    jint handle = KNI_GetParameterAsInt(1);
    javacall_result r;

    KNIPlayerInfo*  pKniInfo = (KNIPlayerInfo*)handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);
    KNI_GetParameterAsObject(2, buffer);

    length = KNI_GetArrayLength(buffer);

    r = javacall_media_tone_alloc_buffer(pKniInfo->pNativeHandle,length,&ptr);

    if( JAVACALL_OK == r ) {
        KNI_GetRawArrayRegion(buffer, 0, (int)length, (jbyte*)ptr);
        javacall_media_tone_sequence_written(pKniInfo->pNativeHandle);
    } else {
        // TODO: KNI_ThrowNew(...);
    }

    KNI_EndHandles();

    KNI_ReturnVoid();
}

