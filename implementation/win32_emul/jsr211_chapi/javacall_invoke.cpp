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


/**
 * @file
 * @brief Content Handler Invocation stubs.
 */

#include "stdio.h"
#include "string.h"

#include "javacall_chapi_invoke.h"
#include "javacall_chapi_registry.h"

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#include "javacall_invoke.h"

static char volatile timeToQuit;

static DWORD WINAPI InvocationRequestListenerProc( LPVOID lpParam ){
    HANDLE pipe = INVALID_HANDLE_VALUE;
    while( 1 ){
        pipe = ::CreateNamedPipe(
            TEXT(PIPENAME),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            PIPE_UNLIMITED_INSTANCES,
            sizeof( InvocationMsg ),  //DWORD nOutBufferSize,
            sizeof( InvocationMsg ),  //DWORD nInBufferSize,
            1000,               //DWORD nDefaultTimeOut,
            0 );
        assert( pipe != INVALID_HANDLE_VALUE );

        InvocationMsg msg;
        DWORD numberOfBytesRead;
        int ret;

        do {
            ret = ::ReadFile( pipe, &msg, sizeof( msg ), &numberOfBytesRead, 0 );
            ::Sleep( 100 );
        } while ( !timeToQuit && ret == 0 );

        if ( timeToQuit )
            break;

        if ( ret == 0 )
            continue;

        javacall_chapi_invocation inv;
        inv.url = (javacall_utf16_string) msg.getMsgPtr();
        inv.type = (javacall_utf16_string) NULL;
        inv.action = (javacall_utf16_string) L"open";
        inv.invokingAppName = (javacall_utf16_string) L"jsr211 demo";  
        inv.invokingAuthority = (javacall_utf16_string) NULL; 
        inv.username = (javacall_utf16_string) NULL;
        inv.password = (javacall_utf16_string) NULL;
        inv.argsLen  = 0;
        inv.args     = NULL;
        inv.dataLen  = 0; 
        inv.data     = NULL;
        inv.responseRequired = 0;

        printf( "\nURL to process '%ls'", inv.url );

        // determine handlerID
        javacall_utf16_string value = (javacall_utf16_string)wcsrchr( (const wchar_t *)inv.url, L'.' );
        if( value == NULL )
            continue;
        printf( "\nsuffix '%ls'", value );
        javacall_utf16 buffer[ 0x100 ]; // demo
        int pos = 0, len = sizeof(buffer)/sizeof(buffer[0]);
        int rc = javacall_chapi_enum_handlers_by_suffix(value, &pos, buffer, &len);
        printf( ", pos = %X", pos );
       	javacall_chapi_enum_finish(pos);
        if( rc != 0 )
            continue;

        printf( "\ncontent handler ID '%ls'", buffer );
        javanotify_chapi_java_invoke( buffer, &inv, 1 );

        ::CloseHandle( pipe );
        pipe = INVALID_HANDLE_VALUE;
    }
    ::CloseHandle( pipe );
    return 0;
}

javacall_result javacall_chapi_java_finish(
    int invoc_id, 
    javacall_const_utf16_string url,
    int argsLen, javacall_const_utf16_string* args,
    int dataLen, void* data, javacall_chapi_invocation_status status,
    /* OUT */ javacall_bool* should_exit)
{
    *should_exit = (javacall_bool) 0;
    return JAVACALL_OK;
}

extern "C" void InitPlatform2JavaInvoker(){
    timeToQuit = 0;
    HANDLE InvocationRequestListenerThread;
    InvocationRequestListenerThread = CreateThread( 
        NULL,              // default security attributes
        0,                 // use default stack size  
        InvocationRequestListenerProc,        // thread function 
        0,                 // argument to thread function 
        0,                 // use default creation flags 
        NULL );            // do not return thread identifier 
    assert( InvocationRequestListenerThread != NULL );
    CloseHandle( InvocationRequestListenerThread );
}


extern "C" void DeInitPlatform2JavaInvoker(){
    timeToQuit++;
}
