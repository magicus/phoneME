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

#include <windows.h>
#include "KNICommon.h"
#include "sni.h"

static void get_data_request_params( KNIDECLARGS javacall_handle handle );

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectInputThread_nWriteData) {
    jint jHandle = KNI_GetParameterAsInt(3);
    javacall_handle handle = NULL;
    jint lenToWrite = 0;
    jbyte *pDest = NULL;
        
    
    if( 0 == jHandle )
    {
        KNI_ReturnVoid();
    }
    
    if( NULL == ( handle = (( KNIPlayerInfo* )jHandle)->pNativeHandle) )
    {
        KNI_ReturnVoid();
    }

    lenToWrite = KNI_GetParameterAsInt(2);
    if( 0 >= lenToWrite )
    {
        KNI_ReturnVoid();
    }
    
    KNI_StartHandles(3);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle( javaBuf );
    
    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);

    /* notify JavaCall that the data is ready,
       AND get the destination buffer address */
    javacall_media_data_ready( handle, lenToWrite, &pDest );
    {
        char msg[256];
        sprintf( msg, "KNIDirectInputThread: data ready, handle %8X", handle );
        OutputDebugString( msg );
    }
    
    /* copy the data */
    if( NULL != pDest )
    {
        KNI_GetParameterAsObject(1, javaBuf);
        KNI_GetRawArrayRegion( javaBuf, 0, lenToWrite, pDest );
    }
    
    /* Notify JavaCall that the data is written */
    {
        javacall_bool new_request = JAVACALL_FALSE;
                
        javacall_media_data_written( handle, &new_request );
        
        if( JAVACALL_TRUE == new_request )
        {
            get_data_request_params( KNIPASSARGS handle );
        }
        else {
            KNI_SetIntField( instance, 
                KNI_GetFieldID( clazz, "lenToRead", "I" ), 0 );
        }
    }

    /* Update of the current position */
    {
        jfieldID fieldCurPos;
        jlong curPos = 0;
        fieldCurPos = KNI_GetFieldID( clazz, "curPos", "J" ); /* Long! */
        curPos = KNI_GetLongField( instance, fieldCurPos ); /* Long! */
        curPos += lenToWrite;
        KNI_SetLongField( instance, fieldCurPos, curPos ); /* Long! */
    }
 
    KNI_EndHandles();
    KNI_ReturnVoid();
}

static void get_data_request_params( KNIDECLARGS javacall_handle handle )
{
    javacall_int64 offset;
    javacall_int32 length;
    javacall_result res = JAVACALL_FAIL;
    
    if( JAVACALL_OK != javacall_media_get_data_request( handle, &offset, 
        &length ) )
    {
        length = 0;
    }
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);

    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
    
    KNI_SetLongField( instance, KNI_GetFieldID( clazz, "posToRead", "J" ), 
                        ( jlong )offset );
    KNI_SetIntField( instance, KNI_GetFieldID( clazz, "lenToRead", "I" ),
                        ( jint )length );

    KNI_EndHandles();
}


KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectInputThread_nGetRequestParams) {
    jint jHandle = KNI_GetParameterAsInt(1);
    javacall_handle handle = NULL;
    
    if( 0 == jHandle )
    {
        KNI_ReturnVoid();
    }
    
    if( NULL == ( handle = (( KNIPlayerInfo* )jHandle)->pNativeHandle) )
    {
        KNI_ReturnVoid();
    }
    
    get_data_request_params( KNIPASSARGS handle );
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectInputThread_nNotifyEndOfStream) {
    jint jHandle = KNI_GetParameterAsInt(1);
    javacall_handle handle = NULL;
    jlong pos = -1;
    
    if( 0 == jHandle )
    {
        KNI_ReturnVoid();
    }
    
    if( NULL == ( handle = (( KNIPlayerInfo* )jHandle)->pNativeHandle) )
    {
        KNI_ReturnVoid();
    }
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);

    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
    
    pos = KNI_GetLongField( instance, KNI_GetFieldID( clazz, "posToRead", "J" ));
    
    //notify End Of Stream
    javacall_media_stream_length( handle, JAVACALL_TRUE, ( javacall_int64 )pos );
    
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}

