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

/** \file jvmspi.h
 * Callback functions to be implemented by the JVM client software.
 * (i.e. e.g. your main program or your native profile framework).
 *
 * The following section defines a callback SPI for the JVM to invoke
 * profile-specific support services. For example, a GUI-based profile
 * may implement print_raw() such that the VM output is logged in a
 * "console" dialog box.
 *
*/

#ifndef _JVM_CONFIG_H_
#include "jvmconfig.h"
#endif

#ifndef _JVMSPI_H_
#define _JVMSPI_H_

#ifndef _JAVASOFT_KNI_H_
#include "kni.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void JVMSPI_PrintRaw(const char* s);

/** 
 * Called from the VM when it wants to exit *abnormally*.
 * It will NOT get called on normal exit (i.e. JVM_Start() returns).
 * This callback must terminate the VM process. This callback must not
 * return to its caller.
 */
void JVMSPI_Exit(int code);

typedef void *JVMSPI_ThreadID;

/*
 * This structure describes a Java LWT thread that's blocked
 */
typedef struct {
    JVMSPI_ThreadID thread_id;
    void        *reentry_data;
    int          reentry_data_size;
} JVMSPI_BlockedThreadInfo;

/*
 * This function is called by the VM periodically. It has to check if
 * any of the blocked threads are ready for execution, and call
 * SNI_UnblockThread() on those threads that are ready.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until an event happens, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the events sources but do not block. Return to the
 *       caller immediately regardless of the status of the event sources.
 *  -1 = Do not timeout. Block until an event happens.
 */
void JVMSPI_CheckEvents(JVMSPI_BlockedThreadInfo *blocked_threads,
                        int blocked_threads_count,
                        jlong timeout);

/* This function is called from within System.exit(). If this function
 * returns KNI_FALSE, System.exit() throws a SecurityException and
 * will NOT exit the VM. */
jboolean JVMSPI_CheckExit(void);

/**
 * Display a message that explains how to invoke the system
 * and what it's possible valid parameters are.
 */
void JVMSPI_DisplayUsage(char *message);

char *JVMSPI_GetSystemProperty(char *property_name);
void  JVMSPI_FreeSystemProperty(char *property_value);
void  JVMSPI_SetSystemProperty(char *property_name, char *property_value);

#if ENABLE_JAVA_DEBUGGER
void JVMSPI_DebuggerNotification(jboolean);
#endif

#if ENABLE_METHOD_TRAPS
void JVMSPI_MethodTrap(int trap_action, int trap_handle);
#endif

#if ENABLE_DYNAMIC_RESTRICTED_PACKAGE
/*
 * This function is called before loading an application class. If the
 * return value is true, the class is considered to be in a restricted
 * package and the VM will refuse to load it.
 *
 * <pkg_name>    name of the package. This string is NOT 0-terminated.
 * <name_length> number of UTF8 characters in <pkg_name>.
 *
 * For example, if the class com.foobar.pkg.Class is being loaded,
 * <pkg_name> = "com/foobar/pkg" and <name_length> = 14.
 */
jboolean JVMSPI_IsRestrictedPackage(const char* pkg_name, int name_length);
#endif

#ifdef __cplusplus
}
#endif

#endif
