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
#define TRACE_RESULTBYTES

#endif

#include <assert.h>
#include <string.h>

#include <jsrop_kni.h>
#include <midpServices.h>

#include "jsr211_constants.h"
#include "javacall_memory.h"
#include "javacall_chapi_msg_exchange.h"

#ifdef _DEBUG

void memory__dump( const char * p_title, const unsigned char * p_bytes, size_t p_count ){
    static char s_xd[] = "0123456789ABCDEF";
    unsigned char a_line[ 32 * 4 + 10 ];
    printf( "%s: bytes = %p, count = %u\n", p_title, p_bytes, p_count );
    while( p_count ){
        size_t a_l = (p_count < 32)? p_count : 32, i;
        memset( a_line, ' ', sizeof(a_line) );
        p_count -= a_l;
        for( i = 0; i < a_l; i++, p_bytes++){
            a_line[ i * 3 ] = s_xd[ (*p_bytes >> 4) & 0x0F ];
            a_line[ i * 3 + 1 ] = s_xd[ *p_bytes & 0x0F ];
            a_line[ 32 * 3 + 2 + i ] = (*p_bytes < ' ')? '.' : *p_bytes;
        }
        a_line[ 32 * 4 + 2 ] = '\0';
        printf("  %s\n", a_line);
    }
}

#endif

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
        // thread is unblocked
        if( p->m_count > 0 && p->m_bytes == NULL ){
            KNI_ThrowNew(jsropOutOfMemoryError, "");
        } else {            
            // create array of bytes
            SNI_NewArray( SNI_BYTE_ARRAY, p->m_count, data );
            if( KNI_IsNullHandle(data) ){
                KNI_ThrowNew(jsropOutOfMemoryError, "");
            } else {
#ifdef TRACE_RESULTBYTES
                memory__dump( "com_sun_j2me_content_NativeMessageSender_send result", p->m_bytes, p->m_count );
#endif
                KNI_SetRawArrayRegion( data, 0, p->m_count, p->m_bytes );
            }
            if( p->m_bytes != NULL && p->m_count > 0 ) javacall_free( p->m_bytes );
            p->m_bytes = NULL;
        }
    }
    KNI_EndHandlesAndReturnObject(data);
}

void jsr211_process_msg_result( const jsr211_response_data * data ){
#ifdef TRACE_MSGEXCHANGE
    printf( "jsr211_process_msg_result( exchangeID = %d, bytes = %p, count = %d )\n", data->dataExchangeID, data->bytes, data->count );
#endif
    if( data->bytes == NULL ){
        unblockWaitingThreads( JSR211_WAIT_MSG, data->dataExchangeID, JSR211_WAIT_CANCELLED );
    } else {
        const JVMSPI_BlockedThreadInfo * ti = findThread( JSR211_WAIT_MSG, data->dataExchangeID );
        if( ti != NULL ){ 
            ReentryData * p = (ReentryData *)ti->reentry_data;
            if( p != NULL ){
                p->m_bytes = data->bytes;
                p->m_count = data->count;
            }
            unblockThread( ti );
        }
    }
}

//----------------------------------------------------------

typedef struct _Request {
    jsr211_request_data m_data;
    struct _Request * m_next;
} Request;

static Request * s_head = NULL, ** s_tail = &s_head;

int jsr211_process_msg_request( const jsr211_request_data * data ){
    Request * newR = JAVAME_MALLOC( sizeof(Request) );
    if( newR == NULL ) return( JAVACALL_FALSE );
    newR->m_data = *data;
    newR->m_next = NULL;

#ifdef TRACE_MSGEXCHANGE
    printf( "jsr211_process_msg_request( qID = %d, exchangeID = %d, msg = %d, count = %d )\n", 
                data->queueID, data->dataExchangeID, data->msg, data->count );
#endif

    // insert request to the list
    // critial section {
    *s_tail = newR;
    s_tail = &newR->m_next;
    // }

    unblockWaitingThreads( JSR211_WAIT_FOR_REQUEST, 0, JSR211_WAIT_FOR_REQUEST );
    return( JAVACALL_TRUE );
}

// int waitForRequest(); returns queueId
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_waitForRequest) {
#ifdef TRACE_MSGEXCHANGE
    printf( "waitForRequest: head = %p\n", s_head );
#endif
    if( s_head != NULL ){
        KNI_ReturnInt( s_head->m_data.queueID );
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
    if( r->m_data.count > 0 ) javacall_free( r->m_data.bytes );
    JAVAME_FREE( r );
    KNI_ReturnVoid();
}

// int getRequestMsgCode();
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_getRequestMsgCode) {
    assert( s_head != NULL );
#ifdef TRACE_MSGEXCHANGE
    printf( "getRequestMsgCode: msg = %d\n", s_head->m_data.msg );
#endif
    KNI_ReturnInt( s_head->m_data.msg );
}

// int getRequestId();
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_getRequestId) {
    assert( s_head != NULL );
#ifdef TRACE_MSGEXCHANGE
    printf( "getRequestId: id = %d\n", s_head->m_data.dataExchangeID );
#endif
    KNI_ReturnInt( s_head->m_data.dataExchangeID );
}

// byte[] getRequestBytes();
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_getRequestBytes) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    assert( s_head != NULL );
#ifdef TRACE_MSGEXCHANGE
    printf( "getRequestBytes: count = %d\n", s_head->m_data.count );
#endif
    SNI_NewArray( SNI_BYTE_ARRAY, s_head->m_data.count, data );
    if( KNI_IsNullHandle(data) ){
        KNI_ThrowNew(jsropOutOfMemoryError, "");
    } else {
        KNI_SetRawArrayRegion( data, 0, s_head->m_data.count, s_head->m_data.bytes );
    }
    KNI_EndHandlesAndReturnObject(data);
}

// void postResponse(int requestId, byte[] data);
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_content_NativeMessageReceiver_postResponse) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(data);
    KNI_GetParameterAsObject(2, data);
#ifdef TRACE_MSGEXCHANGE
    printf( "postResponse: data = %p\n", data );
#endif
    if( KNI_IsNullHandle(data) ){
        javacall_chapi_send_response( KNI_GetParameterAsInt(1), NULL, 0 );
    } else {
        javacall_chapi_send_response( KNI_GetParameterAsInt(1), SNI_GetRawArrayPointer(data), KNI_GetArrayLength(data) );
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

