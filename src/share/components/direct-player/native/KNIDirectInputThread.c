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
#include "sni.h"

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectInputThread_nWriteData) {
    jlong posToRead = 0;
    jlong curPos = 0;
    jint sizeToRead = 0;
    jint addrToWrite = 0;
    jint lenToWrite = 0;
    jfieldID fieldNativePtr;
    jfieldID fieldCurPos;
    jbyte *pDest = NULL;
    
    KNI_StartHandles(3);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle( javaBuf );
    
    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
    
    fieldNativePtr = KNI_GetFieldID( clazz, "nativePtr", "I" );
    fieldCurPos = KNI_GetFieldID( clazz, "curPos", "J" ); // Long!
    
    addrToWrite = KNI_GetIntField( instance, fieldNativePtr );
    lenToWrite = KNI_GetParameterAsInt(2);
    KNI_GetParameterAsObject(1, javaBuf);
    pDest = ( jbyte* )addrToWrite;
    KNI_GetRawArrayRegion( javaBuf, 0, lenToWrite, pDest );
    
    //Update of the current position
    curPos = KNI_GetLongField( instance, fieldCurPos ); //Long!
    curPos += lenToWrite;
    KNI_SetLongField( instance, fieldCurPos, curPos );
 
    {
        jint handle = KNI_GetParameterAsInt(3);
        KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    
        javacall_media_data_written(
        javacall_handle handle,
        javacall_int32 length,
        /*OUT*/ javacall_bool *new_request,
        /*OUT*/ javacall_int64 *new_offset,
        /*OUT*/ javacall_int32 *new_length,
        /*OUT*/ void **new_data);
    }

    
    
    

    
    KNI_EndHandles();
    KNI_ReturnVoid();
}
