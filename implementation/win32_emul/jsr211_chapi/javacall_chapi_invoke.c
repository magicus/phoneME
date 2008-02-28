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


#include "javacall_chapi_invoke.h"

#include "windows.h"

/**
 * Asks NAMS to launch specified MIDlet. <BR>
 * It can be called by JVM when Java content handler should be launched
 * or when Java caller-MIDlet should be launched.
 * @param suite_id suite Id of the requested MIDlet
 * @param class_name class name of the requested MIDlet
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 *   This should be used when there is no free isolate to run content handler (e.g. in SVM mode).
 * @return result of operation.
 */
javacall_result javacall_chapi_ams_launch_midlet(int suite_id,
        javacall_const_utf16_string class_name,
        /* OUT */ javacall_bool* should_exit)
{
    (void)suite_id;
    (void)class_name;
    (void)should_exit;
    return JAVACALL_NOT_IMPLEMENTED;
}



/**
 * Sends invocation request to platform handler. <BR>
 * This is <code>Registry.invoke()</code> substitute for Java->Platform call.
 * @param invoc_id requested invocation Id for further references
 * @param handler_id target platform handler Id
 * @param invocation filled out structure with invocation params
 * @param without_finish_notification if <code>JAVACALL_TRUE</code>, then 
 *       @link javanotify_chapi_platform_finish() will not be called after 
 *       the invocation's been processed.
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 * @return result of operation.
 */
javacall_result javacall_chapi_platform_invoke(int invoc_id,
        const javacall_utf16_string handler_id, 
        javacall_chapi_invocation* invocation, 
        /* OUT */ javacall_bool* without_finish_notification, 
        /* OUT */ javacall_bool* should_exit)
{
    int res;
    (void)invoc_id;
    (void)handler_id;
    *without_finish_notification = 1;
    *should_exit = 0;

    res = (int) ShellExecuteW( NULL, invocation->action, invocation->url, NULL, NULL, SW_SHOWNORMAL);
    if ( res > 32 )
        return JAVACALL_OK;
    return JAVACALL_FAIL;
}

/*
 * Called by Java to notify platform that requested invocation processing
 * is completed by Java handler.
 * This is <code>ContentHandlerServer.finish()</code> substitute 
 * for Platform->Java call.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing. 
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 * @return result of operation.
 */
/*
javacall_result javacall_chapi_java_finish(int invoc_id, 
        javacall_const_utf16_string url,
        int argsLen, javacall_const_utf16_string* args,
        int dataLen, void* data, javacall_chapi_invocation_status status,
         javacall_bool* should_exit)
{
    (void)invoc_id;
    (void)url;
    (void)argsLen;
    (void)args;
    (void)dataLen;
    (void)data;
    (void)status;
    (void)should_exit;
    return JAVACALL_NOT_IMPLEMENTED;
}
*/