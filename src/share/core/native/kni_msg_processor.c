/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#ifdef _DEBUG

#include <stdio.h>

#define TRACE_BLOCKING

#endif

#include <jsrop_kni.h>
#include <midpServices.h>

#include "jsr211_constants.h"

javacall_result javacall_chapi_post_message( int msgCode, const unsigned char * bytes, size_t bytesCount, int * dataExchangeID ){}

typedef struct {
  MidpReentryData  m_midpRD;
  unsigned char *  m_bytes;
  size_t           m_count;
} ReentryData;

// byte[] send(int msgCode, byte[] data) throws IOException;
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_NativeMessageProcessor_send) {
    ReentryData* p = (ReentryData*)SNI_GetReentryData(NULL);
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    KNI_GetParameterAsObject(2, data);

    if( p == NULL ){
        int dataExchangeID;
        jsize bytesCount = KNI_GetArrayLength(data);
        jbyte * bytes = JAVAME_MALLOC( bytesCount );
        if( bytes == NULL ){
            KNI_ThrowNew(jsropOutOfMemoryError, "");
        } else {
            KNI_GetRawArrayRegion(data, 0, bytesCount, bytes);
            if( JAVACALL_OK != javacall_chapi_post_message( KNI_GetParameterAsInt(1), bytes, bytesCount, &dataExchangeID ) ){
                KNI_ThrowNew(jsropIOException, "javacall_chapi_post_message failed");
            } else {
                p = (ReentryData*)SNI_AllocateReentryData(sizeof(ReentryData));
                p->m_bytes = NULL;
                p->m_count = 0;
                blockThread( JSR211_WAIT_MSG, dataExchangeID );
            }
            JAVAME_FREE( bytes );
        }
    } else if( isThreadCancelled() ){
        KNI_ThrowNew(jsropIOException, "data exchange failed");
    } else {
        if( p->m_count > 0 && p->m_bytes == NULL ){
            KNI_ThrowNew(jsropOutOfMemoryError, "");
        } else {            
            // create array of bytes
            SNI_NewArray( SNI_BYTE_ARRAY, p->m_count, data );
            KNI_SetRawArrayRegion( data, 0, p->m_count, p->m_bytes );
        }
    }

    KNI_EndHandlesAndReturnObject(data);
}

javacall_result javanotify_chapi_process_msg_result( int dataExchangeID, const unsigned char * bytes, size_t count ){
    if( bytes == NULL ){
        unblockWaitingThreads( JSR211_WAIT_MSG, dataExchangeID, JSR211_WAIT_CANCELLED );
    } else {
        const JVMSPI_BlockedThreadInfo * ti = findThread( JSR211_WAIT_MSG, dataExchangeID );
        if( ti != NULL ){ 
            ReentryData * p = (ReentryData *)ti->reentry_data;
            if( p != NULL ){
                p->m_count = count;
                if( count != 0 ){
                    // create copy of bytes
                    p->m_bytes = JAVAME_MALLOC( p->m_count );
                    if( p->m_bytes != NULL ){
                        memcpy( p->m_bytes, bytes, p->m_count );
                    }
                }
            }
            unblockThread( ti );
        }
    }
    return( JAVACALL_OK );
}

javacall_result javanotify_chapi_process_msg( int dataExchangeID, const unsigned char * bytes, size_t count ){
    return( JAVACALL_OK );
}
