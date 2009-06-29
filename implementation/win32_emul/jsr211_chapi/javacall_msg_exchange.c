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

#include "javacall_chapi_msg_exchange.h"

javacall_result javacall_chapi_post_message( int queueId, int msgCode, const unsigned char * bytes, size_t bytesCount, int * dataExchangeID ){
    static int freeDEID = 1;
    do {
        *dataExchangeID = freeDEID++;
    } while( *dataExchangeID == 0 );
    return( javanotify_chapi_process_msg_request( queueId, *dataExchangeID, msgCode, bytes, bytesCount ) );
}

javacall_result javacall_chapi_send_response( int dataExchangeID, const unsigned char * bytes, size_t bytesCount ){
    return( javanotify_chapi_process_msg_result( dataExchangeID, bytes, bytesCount ) );
}

