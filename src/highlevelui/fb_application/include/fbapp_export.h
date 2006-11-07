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
 * Returns the file descriptor for reading the keyboard. 
 */
extern int fbapp_get_keyboard_fd();

/**
 * Returns the type of the frame buffer device.
 */
extern int fbapp_get_fb_device_type();

#ifdef __cplusplus
}
#endif

#endif /* _FBAPP_APPLICATION_H_ */
