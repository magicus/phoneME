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

#ifndef _WIN32APP_EXPORT_H_
#define _WIN32APP_EXPORT_H_

/**
 * @defgroup highui_winapp Windows Application External Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_winapp
 *
 * @brief win32 application exported native interface
 */

#include <windows.h>

#include <midpEvents.h>
#include <midpServices.h>

#include <gxapi_graphics.h>
#include <imgapi_image.h>

/**
 * @name WndProc globals
 * The MIDP windows procedure puts its results here.
 * @{
 */
extern MidpReentryData* pSignalResult;
extern MidpEvent* pMidpEventResult;
extern int appManagerRequestWaiting;
/** @} */

/**
 * Main window class name.
 * Shared by NAMS testing program to post test events.
 */
#define MAIN_WINDOW_CLASS_NAME "SunJWCWindow"

/**
 * @name Main Window Event Types
 * @{
 */
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)
#define WM_TEST          (WM_USER + 3)
/** @} */

/** The event loop timer handle */
#define EVENT_TIMER_ID (1)

#define EMULATOR_WIDTH  (241 + 8)
#define EMULATOR_HEIGHT (635 + 24)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the Win32 native resources.
 */
extern void win32app_init();

/**
 * Finalize the Win32 native resources.
 */
extern void win32app_finalize();

/**
 * Refresh the given area.  For double buffering purposes.
 */
extern void win32app_refresh(int x, int y, int w, int h);

/**
 * Return screen width
 */
extern int win32app_get_screen_width();

/**
 * Return screen height
 */
extern int win32app_get_screen_height();

/**
 * Invert screen rotation flag
 */
extern jboolean win32app_reverse_orientation();

/**
 * Handle clamshell event
 */
extern void win32app_handle_clamshell_event();

/**
 * Get screen rotation flag
 */
extern jboolean win32app_get_reverse_orientation();


/**
 * Returns the handle of the main application window.
 */
extern HWND win32app_get_window_handle();

/**
 * Flushes the offscreen buffer directly to the device screen.
 * The size of the buffer flushed is defined by offscreen buffer width
 * and passed in height. 
 * Offscreen_buffer must be aligned to the top-left of the screen and
 * its width must be the same as the device screen width.
 * @param graphics The Graphics instance associated with the screen.
 * @param offscreen_buffer The offscreen buffer to be flushed
 * @param h The height to be flushed
 * @return KNI_TRUE if direct_flush was successful, KNI_FALSE - otherwise
 */
extern jboolean win32app_direct_flush(const java_graphics *g, 
		    	              const java_imagedata *offscreen_buffer, 
                                      int h);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32APP_EXPORT_H_ */
