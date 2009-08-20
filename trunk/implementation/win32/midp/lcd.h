/*
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

#ifndef _LCD_H_
#define _LCD_H_

#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

extern HWND midpGetWindowHandle();

/*
    Definitions to create the Lyfe Cycle of the Emulator Window
*/
#define EXMENU_ITEM_PAUSE                               1002
#define EXMENU_ITEM_RESUME                              1003
#define EXMENU_ITEM_SHUTDOWN                            1004

#define EXMENU_TEXT_MAIN                                "Life Cycle"
#define EXMENU_TEXT_SHUTDOWN                            "Send \"Shutdown\" event"
#define EXMENU_TEXT_PAUSE                               "Send \"Pause\" event"
#define EXMENU_TEXT_RESUME                              "Send \"Resume\" event"


#if !ENABLE_MULTIPLE_INSTANCES

/*
 *    The function returns JAVACALL_OK if current instance
 *    is second (another one already exist).
 */
javacall_bool isSecondaryInstance(void);

/*
 *    The function move all arguments of the current instance
 *    (see the arguments of the function 'main')
 *    to the named interprocess shared space.
 */
void enqueueInterprocessMessage(int argc, char *argv[]);

/*
 *    The function allocates memory for the array of arguments
 *    (see the arguments of the function 'main')
 *    and gets this array from the named interprocess shared space.
 *    Returns the number of arguments.
 */
int dequeueInterprocessMessage(char*** argv);

#endif

/*
 *    The function processes windows messages
 *    of the UI modal dialog box "Start TCK" to request user for
 *    TCK URL and type (trusted/untrusted) of the domain.
 *    It calls javanotify_start_tck(...) if user clicks OK.
 */
LRESULT CALLBACK start_tck_dlgproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/*
 *    The function processes windows messages
 *    of the UI modal dialog box "Debug Levels" to set the debug levels in run time
 */
LRESULT CALLBACK start_debuglevels_dlgproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#endif /* _LCD_H_ */
