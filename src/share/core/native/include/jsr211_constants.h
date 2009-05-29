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

#ifndef JSR211_CONSTANTS_H
#define JSR211_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

#include <sni.h>

/**
 * Status code for threads waiting for registry store access and 
 * invocation events
 */
typedef enum {
  JSR211_WAIT_OK          = 0x0001
, JSR211_WAIT_CANCELLED   = 0x0002
, JSR211_WAIT_MSG         = 0x0004
, JSR211_WAIT_FOR_REQUEST = 0x0008
} jsr211_wait_status;

void blockThread( jsr211_wait_status status, int blockID );
jboolean isThreadCancelled( void );

const JVMSPI_BlockedThreadInfo * findThread( jsr211_wait_status status, int blockID );
void unblockThread( const JVMSPI_BlockedThreadInfo * p );
void unblockWaitingThreads( jsr211_wait_status status, int blockID, jsr211_wait_status newStatus );

/* invocation statuses: */
/*
 * This Invocation was just constructed and is being initialized.
 */
#define STATUS_INIT 1

/*
 * This Invocation is a new request and is being handled by the content handler.
 */
#define STATUS_ACTIVE 2

/*
 * This Invocation has been invoked and is waiting to be complete.
 */
#define STATUS_WAITING 3

/*
 * This Invocation is on hold until a chained Invocation is completed.
 */
#define STATUS_HOLD 4

/*
 * The content handler successfully completed processing the Invocation.
 */
#define STATUS_OK 5

/*
 * The processing of the Invocation was cancelled by the ContentHandler.
 */
#define STATUS_CANCELLED 6

/*
 * The content handler failed to correctly process the Invocation request.
 */
#define STATUS_ERROR 7

/*
 * The processing of the Invocation has been initiated and will
 * continue. This status is only appropriate when the content
 * handler can not provide a response when it is finished.
 */
#define STATUS_INITIATED 8

/*
 * The DISPOSE status is used with {@link #setStatus setStatus}
 * to discard the native Invocation. It must not overlap with
 * Status values defined in the Invocation class and must match
 * STATUS_DISPOSE defined in invocStore.c and InvocationImpl.
 */
#define STATUS_DISPOSE 100

/* strutures */

typedef struct {
    int queueID;
    int msg;
    int dataExchangeID;
    unsigned char * bytes;
    unsigned int count;
} jsr211_request_data;

typedef struct {
    int dataExchangeID;
    unsigned char * bytes;
    unsigned int count;
} jsr211_response_data;


#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif // JSR211_CONSTANTS_H
