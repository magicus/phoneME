/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <string.h>
#include <stdio.h>
#include <win32app_export.h>

#include <windows.h>

/**
 * @file
 *
 * Posts window messages to a running MIDP OI emulator.
 */

static char* USAGE =
"postTestMsg <wParam> [<lParam>]\n\n"
"  wParam:\n\n"
"    -1 = Shutdown the MIDP system.\n"
"     1 = Start an installed MIDlet, lParam selects a suite by index.\n"
"     2 = Pause a MIDlet, lParam is the suite's index.\n"
"     3 = Resume a MIDlet, lParam is the suite's index.\n"
"     4 = Destroy a MIDlet, lParam is the suite's index.\n"
"     5 = Put a MIDlet in the foreground, lParam is the suite's index.\n"
"     6 = Start an internal MIDlet, lParam selects a rommized MIDlet.\n\n";


/**
 * Accepts 1 or 2 arguments.
 * The first argument will be the message's wParam, default 0.
 * The second argument will be the message's lParam, default 0.
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 */
int main(int argc, char* argv[]) {
    WPARAM wParam = -1;
    LPARAM lParam = 0;
    HWND hwnd;

    switch (argc) {
    case 0:
    case 1:
        fprintf(stderr, "Too few arguments given\n\n");
        fprintf(stderr, USAGE);
        return -1;

    case 3:
        lParam = atol(argv[2]);
        /* Fall through */
    case 2:
        wParam = atol(argv[1]);
        break;

    default:
        fprintf(stderr, "Too many arguments given\n\n");
        fprintf(stderr, USAGE);
        return -1;
    }

    hwnd = FindWindow(MAIN_WINDOW_CLASS_NAME, NULL);
    if (NULL == hwnd) {
        fprintf(stderr, "FindWindow failed (%d)\n", GetLastError());
        return -1;
    }

    if (PostMessage(hwnd, WM_TEST, wParam, lParam) == 0) {
        fprintf(stderr, "PostMessage failed (%d)\n", GetLastError());
        return -1;
    }

    return 0;
}
