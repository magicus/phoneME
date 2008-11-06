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

#ifndef _DIRECTFBAPP_EXPORT_H_
#define _DIRECTFBAPP_EXPORT_H_

/**
 * @defgroup highui_directfbapp Linux/DirectFB application library
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_directfbapp
 *
 * @brief Exported interface of the Linux/DirectDB application
 */
#ifdef __cplusplus
extern "C" {
#endif

/** Opens application window and returns back buffer for painting */
extern char *directfbapp_open_window();

/** Closes application window */
extern void directfbapp_close_window();

/**
 * Refreshs screen with offscreen buffer content and
 * returns back buffer buffer for painting
 */
extern char *directfbapp_refresh(int x1, int y1, int x2, int y2);

/** Enables events listening for DirectFB application */
extern void directfbapp_enable_events();

/**
 * Checks for events from keyboard. Gotten event must be retrieved 
 * by <code>directfbapp_get_event</code>.
 */
extern int directfbapp_event_is_waiting();

/**
 * Retrieves next event from queue. Must be called when 
 * <code>directfbapp_event_is_waiting</code> returned true.
 */
extern void directfbapp_get_event(void*);

/** Gets screen width of the DirectFB application */
extern int directfbapp_get_screen_width();

/** Gets screen height of the DirectFB application */
extern int directfbapp_get_screen_height();

/** Finalizes native resources of the DirectFB application */
extern void directfbapp_finalize();

#ifdef __cplusplus
}
#endif

#endif /* _DIRECTFBAPP_EXPORT_H_ */
