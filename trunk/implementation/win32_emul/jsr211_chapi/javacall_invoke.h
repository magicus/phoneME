/*
 *
 *
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

#ifndef JAVACALL_INVOKE_H
#define JAVACALL_INVOKE_H

// To eliminate VC 8.0+ warnings
#define _CRT_SECURE_NO_DEPRECATE 

#include <windows.h>

#define PIPENAME "\\\\.\\pipe\\EmulJavaInvokerPipe"
#define INV_MSG_BUFF_SIZE 256

#if defined( _DEBUG ) && defined( _X86_ ) 
#define assert( a ) if ( !(a) ) __asm int 3
#else
#include <assert.h>
#endif

// NOTE! shouldn't contain virtual functions
class InvocationMsg{
public:
    InvocationMsg();
    
    /// returns non zero if incoming string too long
    int setMsg( wchar_t* );
    const wchar_t *getMsgPtr();

private:
    wchar_t mMsg[INV_MSG_BUFF_SIZE];
};


inline InvocationMsg::InvocationMsg(){
    *mMsg = 0;
}

inline int InvocationMsg::setMsg( wchar_t *s ){
    if ( wcslen( s ) >= ( sizeof( mMsg ) / sizeof( *mMsg ) ) )
        return -1;
    wcscpy( mMsg, s );
    return 0;
}

inline const wchar_t * InvocationMsg::getMsgPtr(){
    return mMsg;
}

#endif