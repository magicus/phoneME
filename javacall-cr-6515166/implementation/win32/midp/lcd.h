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

#ifndef _LCD_H_
#define _LCD_H_

#ifndef RGB
#define RGB(r, g, b)   ( b +(g << 5)+ (r << 11) )
#endif

/** Separate colors are 8 bits as in Java RGB */
#define GET_RED_FROM_PIXEL(P)   (((P) >> 8) & 0xF8)
#define GET_GREEN_FROM_PIXEL(P) (((P) >> 3) & 0xFC)
#define GET_BLUE_FROM_PIXEL(P)  (((P) << 3) & 0xF8)

#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)

extern HWND midpGetWindowHandle();

/*
    Definitions to create the Lyfe Cycle of the Emulator Window
*/
#define EXMENU_ITEM_QUIT				1000

#define EXMENU_ITEM_START				1001
#define EXMENU_ITEM_PAUSE				1002
#define EXMENU_ITEM_RESUME				1003
#define EXMENU_ITEM_SHUTDOWN			1004
#define EXMENU_ITEM_START_TCK			1005
#define EXMENU_ITEM_INTERNAL_PAUSE			1006
#define EXMENU_ITEM_INTERNAL_RESUME			1007
/*
    Definitions to create the VSCL Menu of the Emulator Window
*/
#define EXMENU_ITEM_FLIP_OPEN				1008
#define EXMENU_ITEM_FLIP_CLOSE			    1009
#define EXMENU_ITEM_INCOMING_CALL   	    1010
#define EXMENU_ITEM_CALL_DROPPED    	    1011

#define EXMENU_ITEM_DEBUG_LEVELS    	    1012


#define EXMENU_TEXT_MAIN				"Life Cycle"
#define EXMENU_TEXT_START				"Send \"Start\" event..."
#define EXMENU_TEXT_SHUTDOWN			"Send \"Shutdown\" event..."
#define EXMENU_TEXT_PAUSE				"Send \"Pause\" event..."
#define EXMENU_TEXT_RESUME				"Send \"Resume\" event..."
#define EXMENU_TEXT_START_TCK   		"Start TCK..."
#define EXMENU_TEXT_INTERNAL_PAUSE			"Send \"Internal Pause\" event..."
#define EXMENU_TEXT_INTERNAL_RESUME			"Send \"Internal Resume\" event..."

#define EXMENU_TEXT_VSCL				"VSCL"
#define EXMENU_TEXT_FLIP_OPEN			"Send \"Flip Open\" event..."
#define EXMENU_TEXT_FLIP_CLOSE			"Send \"Flip Close\" event..."
#define EXMENU_TEXT_INCOMING_CALL		"Send \"Incoming Call\" event..."
#define EXMENU_TEXT_CALL_DROPPED		"Send \"Call Dropped\" event..."

#define EXMENU_TEXT_QUIT				"Quit"

#define EXMENU_TEXT_DEBUG_LEVELS	    "Debug"

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
