/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * @file javacall_invoke.h
 * @ingroup CHAPI
 * @brief Content handlers executor interface for JSR-211 CHAPI
 */


/**
 * @defgroup CHAPI JSR-211 Content Handler API (CHAPI)
 *
 *  The following API definitions are required by JSR-211.
 *  These APIs are not required by standard JTWI implementations.
 *
 * @{
 */

#ifndef __JAVACALL_INVOKE_H
#define __JAVACALL_INVOKE_H

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

typedef void* native_invocation_handle;


#define NATIVE_INVOCATION_STATUS_ERROR 0xFEFEFEFEL

native_invocation_handle native_handler_exec_invoke(const unsigned short* content_handler_id, const char* action, const char* url);

// should be implemented by client module
int native_handler_exec_invocation_finished(native_invocation_handle invoc,int exitcode);

void native_handler_exec_wait(native_invocation_handle invoc);

void native_handler_exec_cleanup_monitor(native_invocation_handle invoc);


#ifdef __cplusplus
}
#endif/*__cplusplus*/

#endif //__JAVACALL_NATIVE_HANDLERS_EXEC_H