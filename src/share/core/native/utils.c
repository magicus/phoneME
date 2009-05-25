/*
 * 
 *
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

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

#include <stddef.h>
#include <assert.h>

#ifdef _DEBUG
#include <stdio.h>

#define TRACE_BLOCKING
#endif

#include <sni.h>

#include <midpServices.h>

#include "jsr211_constants.h"

/**
 * Block this thread until unblocked.
 * Initialize the reentry data needed to unblock this thread later.
 * Only the waitingFor setting is used.
 * The status is set to STATUS_OK.
 * If canceled the status will be set to cancelled.
 */
void blockThread( jsr211_wait_status status, int blockID ) {
    /* Initialize the re-entry data so later this thread can
     * be unblocked.
     */
    MidpReentryData* p = (MidpReentryData*)(SNI_GetReentryData(NULL));
    if (p == NULL) {
        p = (MidpReentryData*)SNI_AllocateReentryData(sizeof(MidpReentryData));
    }
#ifdef TRACE_BLOCKING
    printf( "blockThread(%d, %d), ~id = %p\n", status, blockID, p );
#endif
    p->waitingFor = JSR211_SIGNAL;
    p->status = (int)status;
    p->pResult = (void *)blockID;
    SNI_BlockThread();
}

/**
 * Check if this blocked thread was cancelled.
 */
jboolean isThreadCancelled( void ) {
    MidpReentryData* p = (MidpReentryData*)(SNI_GetReentryData(NULL));
#ifdef TRACE_BLOCKING
    printf( "isThreadCancelled: ~id = %p, %s\n", p, (p != NULL && p->status == JSR211_WAIT_CANCELLED)? "yes" : "no" );
#endif
    return (p != NULL && p->status == JSR211_WAIT_CANCELLED);
}

void * enumWaitingThreads( int(* filter)(const MidpReentryData *, void *), void * (*processor)(const JVMSPI_BlockedThreadInfo *, void *), void * data ){
    void * result = NULL;
    int n, i;
    JVMSPI_BlockedThreadInfo * blocked_threads = SNI_GetBlockedThreads(&n);
#ifdef TRACE_BLOCKING
    printf( "enumWaitingThreads: blocked_threads count = %d\n", n );
#endif
    for (i = 0; result == NULL && i < n; i++) {
        MidpReentryData * p = (MidpReentryData *)blocked_threads[i].reentry_data;
#ifdef TRACE_BLOCKING
        if( p != NULL && p->waitingFor == JSR211_SIGNAL )
            printf( "\tThread[%d]: (%d, %d), ~id = %p\n", i, p->status, p->pResult, p );
#endif
        if( !(*filter)( p, data ) ) continue;
        result = (*processor)( &blocked_threads[i], data );
    }
    return( result );
}

static int isThreadStatus( const MidpReentryData * p, jsr211_wait_status status ){
    return( p != NULL && p->waitingFor == JSR211_SIGNAL && p->status == status );
}

static struct ftData {
    jsr211_wait_status m_status;
    int m_blockID;
};
static int ftFilter(const MidpReentryData * p, void * data) {
    struct ftData * d = (struct ftData *)data;
    return( isThreadStatus( p, d->m_status ) && p->pResult == (void *)d->m_blockID );
}
static void * ftProcessor(const JVMSPI_BlockedThreadInfo * p, void * data) {
    return( (void *)p );
}

const JVMSPI_BlockedThreadInfo * findThread( jsr211_wait_status status, int blockID ) {
    struct ftData data; // = { status, blockID };
    data.m_status = status;
    data.m_blockID = blockID;
    assert( blockID != 0 );
    return( (JVMSPI_BlockedThreadInfo *)enumWaitingThreads( ftFilter, ftProcessor, &data ) );
}

void unblockThread( const JVMSPI_BlockedThreadInfo * p ){
    JVMSPI_ThreadID id = p->thread_id;
    if (id != NULL) {
#ifdef TRACE_BLOCKING
        MidpReentryData * mrd = (MidpReentryData *)p->reentry_data;
        if( mrd != NULL )
            printf( "unblockThread: (%d, %d), ~id = %p\n", mrd->status, mrd->pResult, mrd );
#endif
        SNI_UnblockThread(id);
    }
}

static struct uwtData {
    struct ftData ft;
    jsr211_wait_status m_newStatus;
};
static int uwtFilter(const MidpReentryData * p, void * data) {
    struct uwtData * d = (struct uwtData *)data;
    return( isThreadStatus( p, d->ft.m_status ) && (d->ft.m_blockID == 0 || p->pResult == (void *)d->ft.m_blockID) );
}
static void * uwtProcessor(const JVMSPI_BlockedThreadInfo * ti, void * data) {
    MidpReentryData * p = ti->reentry_data;
    struct uwtData * d = (struct uwtData *)data;
    p->status = d->m_newStatus;
    unblockThread( ti );
    return( NULL );
}

/**
 * Scan the block thread data for every thread blocked
 * for a JSR211_SIGNAL block type with INVOKE status and unblock it.
 * For now, every blocked thread is awoken; it should 
 * check if the thread is blocked for the requested application,
 * classname.
 *
 */
void unblockWaitingThreads( jsr211_wait_status status, int blockID, jsr211_wait_status newStatus ) {
    struct uwtData data; // = { { status, blockID }, newStatus };
    data.ft.m_status = status;
    data.ft.m_blockID = blockID;
    data.m_newStatus = newStatus;
    enumWaitingThreads( uwtFilter, uwtProcessor, &data );
}

#ifdef __cplusplus
}
#endif/*__cplusplus*/
