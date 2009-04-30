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

#define TRACE_MSGEXCHANGE

#endif

#include <assert.h>
#include <memory.h>

#include <jsrop_kni.h>
#include <midpServices.h>

#include "jsr211_constants.h"
#include <javacall_chapi_msg_exchange.h>

typedef struct {
  MidpReentryData  m_midpRD;
  unsigned char *  m_bytes;
  size_t           m_count;
} ReentryData;

// byte[] send(int queueId, int msgCode, byte[] data) throws IOException;
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_NativeMessageSender_send) {
    ReentryData* p = (ReentryData*)SNI_GetReentryData(NULL);
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    KNI_GetParameterAsObject(3, data);

    if( p == NULL ){
        int dataExchangeID;
        jsize bytesCount = KNI_GetArrayLength(data);
        jbyte * bytes = JAVAME_MALLOC( bytesCount );
        if( bytes == NULL ){
            KNI_ThrowNew(jsropOutOfMemoryError, "");
        } else {
            KNI_GetRawArrayRegion(data, 0, bytesCount, bytes);
            if( JAVACALL_OK != javacall_chapi_post_message( KNI_GetParameterAsInt(1), KNI_GetParameterAsInt(2), 
                                                                bytes, bytesCount, &dataExchangeID ) ){
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
            if( KNI_IsNullHandle(data) ){
                KNI_ThrowNew(jsropOutOfMemoryError, "");
            } else {
                KNI_SetRawArrayRegion( data, 0, p->m_count, p->m_bytes );
            }
        }
    }
    KNI_EndHandlesAndReturnObject(data);
}

javacall_result javanotify_chapi_process_msg_result( int dataExchangeID, const unsigned char * bytes, size_t count ){
#ifdef TRACE_MSGEXCHANGE
    printf( "javanotify_chapi_process_msg_result( exchangeID = %d, count = %d )\n", dataExchangeID, count );
#endif
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

//----------------------------------------------------------

typedef struct _Request {
    int m_qID;
    int m_requestID;
    int m_msg;
    unsigned char * m_data;
    size_t m_count;
    struct _Request * m_next;
} Request;

static Request * s_head = NULL, ** s_tail = &s_head;

javacall_result javanotify_chapi_process_msg_request( int queueID, int dataExchangeID, 
                                                int msg, const unsigned char * bytes, size_t count ){
    Request * newR = JAVAME_MALLOC( sizeof(Request) );
    if( newR == NULL ) return( JAVACALL_FAIL );
    newR->m_qID = queueID;
    newR->m_requestID = dataExchangeID;
    newR->m_msg = msg;
    newR->m_count = count;
    newR->m_data = JAVAME_MALLOC( count );
    if( newR->m_data == NULL ){
        JAVAME_FREE( newR );
        return( JAVACALL_FAIL );
    }
    memcpy( newR->m_data, bytes, newR->m_count );
    newR->m_next = NULL;

#ifdef TRACE_MSGEXCHANGE
    printf( "javanotify_chapi_process_msg_request( qID = %d, exchangeID = %d, msg = %d, count = %d )\n", queueID, dataExchangeID, msg, count );
#endif

    // insert request to the list
    // critial section {
    *s_tail = newR;
    s_tail = &newR->m_next;
    // }

    unblockWaitingThreads( JSR211_WAIT_FOR_REQUEST, 0, JSR211_WAIT_FOR_REQUEST );
    return( JAVACALL_OK );
}

// int waitForRequest(); returns queueId
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_waitForRequest) {
#ifdef TRACE_BLOCKING
    printf( "waitForRequest: head = %p\n", s_head );
#endif
    if( s_head != NULL ){
        KNI_ReturnInt( s_head->m_qID );
    }
    // block thread
    blockThread( JSR211_WAIT_FOR_REQUEST, 0 );
    KNI_ReturnInt(-1);
}

// void nextRequest();
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_nextRequest) {
    Request * r = s_head;
#ifdef TRACE_BLOCKING
    printf( "nextRequest: head = %p\n", s_head );
#endif
    assert( s_head != NULL );
    // crirical section {
    if( s_tail == &s_head->m_next )
        s_tail = &s_head;
    s_head = r->m_next;
    // }
    JAVAME_FREE( r->m_data );
    JAVAME_FREE( r );
    KNI_ReturnVoid();
}

// int getRequestMsgCode();
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_getRequestMsgCode) {
    assert( s_head != NULL );
    KNI_ReturnInt( s_head->m_msg );
}

// int getRequestId();
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_getRequestId) {
    assert( s_head != NULL );
    KNI_ReturnInt( s_head->m_requestID );
}

// byte[] getRequestBytes();
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_getRequestBytes) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    assert( s_head != NULL );
    SNI_NewArray( SNI_BYTE_ARRAY, s_head->m_count, data );
    if( KNI_IsNullHandle(data) ){
        KNI_ThrowNew(jsropOutOfMemoryError, "");
    } else {
        KNI_SetRawArrayRegion( data, 0, s_head->m_count, s_head->m_data );
    }
    KNI_EndHandlesAndReturnObject(data);
}

// void postResponse(int requestId, byte[] data);
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_postResponse) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    KNI_GetParameterAsObject(2, data);
    if( KNI_IsNullHandle(data) ){
        javacall_chapi_send_response( KNI_GetParameterAsInt(1), NULL, 0 );
    } else {
        javacall_chapi_send_response( KNI_GetParameterAsInt(1), SNI_GetRawArrayPointer(data), KNI_GetArrayLength(data) );
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

