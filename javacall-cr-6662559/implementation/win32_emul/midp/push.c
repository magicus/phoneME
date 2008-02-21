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
#ifdef __cplusplus
extern "C" {
#endif


#include "javacall_push.h"

/**
 * Popup native dialog and block till user provides a reply:
 * either permit or deny.
 * If user permits to run Java, then device focus needs to be passed to
 * the Java task.
 *
 * @param midletName UNICODE name of midlet to launch
 * @param midletNameLen length of name of midlet to launch
 * @param midletSuite UNICODE midlet suite to launch
 * @param midletSuiteLen length of midlet suite to launch
 *
 * @return <tt>JAVACALL_TRUE</tt> if permit, <tt>JAVACALL_FALSE</tt> if deny or on error
 *
 */

#define _UNICODE
#define   UNICODE
#include <Windows.h>
#include <stdio.h>

static TCHAR message[256], caption[] = L"Confirm";

javacall_bool javacall_push_show_request_launch_java(
        const javacall_utf16* midletName,  int midletNameLen,
        const javacall_utf16* midletSuite, int midletSuiteLen)  {

    wcsncpy (message, L"Do you want to launch midlet ", 256);
    wcsncpy (message + wcslen(message), midletName, midletNameLen);
    wcsncpy (message + wcslen(message), L" in midlet suite ", 256);
    wcsncpy (message + wcslen(message), midletSuite, midletSuiteLen);

    return IDYES == MessageBox(NULL, message, caption, MB_YESNO  | MB_ICONQUESTION | MB_TOPMOST)?
      JAVACALL_TRUE: JAVACALL_FALSE;
}


/** @} */


#ifdef __cplusplus
} //extern "C"
#endif
