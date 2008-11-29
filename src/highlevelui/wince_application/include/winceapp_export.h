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

#ifndef _WINCEAPP_EXPORT_H_
#define _WINCEAPP_EXPORT_H_

/**
 * @file
 * @ingroup wince_application
 */

#include <windows.h>

#include <midpEvents.h>
#include <midpServices.h>

#include <gxapi_graphics.h>
#include <imgapi_image.h>

/* The MIDP windows procedure must put its results here. */
extern MidpReentryData* pSignalResult;
extern MidpEvent* pMidpEventResult;
extern int appManagerRequestWaiting;

/**
 * Main window class name.
 * Shared by NAMS testing program to post test events.
 */
#define MAIN_WINDOW_CLASS_NAME "SunJWCWindow"

/**
 * Window Event types
 */
#define WM_DEBUGGER      (WM_USER)
#define WM_HOST_RESOLVED (WM_USER + 1)
#define WM_NETWORK       (WM_USER + 2)
#define WM_TEST          (WM_USER + 3)

#define EVENT_TIMER_ID (1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the Wince native resources.
 */
extern void winceapp_init();

/**
 * Finalize the Wince native resources.
 */
extern void winceapp_finalize();

/**
 * Refresh the given area.  For double buffering purposes.
 */
extern void winceapp_refresh(int x, int y, int w, int h);

/**
 * Returns the handle of the main application window.
 */
extern HWND winceapp_get_window_handle();

/**
 * Return screen width
 */
extern int winceapp_get_screen_width();

/**
 * Return screen height
 */
extern int winceapp_get_screen_height();

/**
 * Invert screen rotation flag
 */
extern jboolean winceapp_reverse_orientation();

/**
 * Handle clamshell event
 */
extern void winceapp_hadle_clamshell_event();

/**
 * Get screen rotation flag
 */
extern jboolean winceapp_get_reverse_orientation();

/**
 * Remembers the handle of the main application window.
 */
extern void winceapp_set_window_handle(HWND hwnd);

extern LRESULT CALLBACK winceapp_wndproc(HWND hwnd, UINT msg, WPARAM wp,
                                         LPARAM lp);

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
extern jboolean winceapp_direct_flush(const java_graphics *g,
                                      const java_imagedata *offscreen_buffer,
                                      int h);

extern BOOL sendMidpKeyEvent(MidpEvent* event, int size);

#ifdef __cplusplus
}
#endif

#endif /* _WINCEAPP_EXPORT_H_ */
