/*
 *  
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

#ifndef _FBAPP_EXPORT_H_
#define _FBAPP_EXPORT_H_

/**
 * @defgroup highui_fbapp Linux/framebuffer application library
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_fbapp
 *
 * @brief Linux/framebuffer application exported native interface
 */

#include <fbapp_device_type.h>
#include <java_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the FB native resources.
 */
extern void fbapp_init();

/**
 * Finalize the FB native resources.
 */
extern void fbapp_finalize();

/**
 * Refresh the given area.  For double buffering purposes.
 */
extern void fbapp_refresh(int x, int y, int w, int h);

/**
 * Invert screen orientation flag
 */
extern jboolean fbapp_reverse_orientation();

/*
 * Return screen orientation flag
 */
extern jboolean fbapp_get_reverse_orientation();

/**
 * Set full screen mode on/off
 */
extern void fbapp_set_fullscreen_mode(int mode);

/**
 * Returns the file descriptor for reading the keyboard. 
 */
#ifndef DIRECTFB
extern int fbapp_get_keyboard_fd();
#else
/**
 * Checks for events from keyboard. Gotten event must be retrieved 
 * by <code>fbapp_get_event</code>.
 */
extern int fbapp_event_is_waiting();

/**
 * Retrieves next event from queue. Must be called when 
 * <code>fbapp_event_is_waiting</code> returned true.
 */
extern void fbapp_get_event(void*);

/**
 * Closes application window.
 */
extern void fbapp_close_window();

#endif
/**
 * Returns the file descriptor for reading the mouse. 
 */
extern int fbapp_get_mouse_fd();

/**
 * Returns the type of the frame buffer device.
 */
extern int fbapp_get_fb_device_type();

extern int get_screen_width();

extern int get_screen_height();


#ifdef __cplusplus
}
#endif

#endif /* _FBAPP_APPLICATION_H_ */
