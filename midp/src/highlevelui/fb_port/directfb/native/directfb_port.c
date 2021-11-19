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

/**
 * @file
 * Implementation of the porting layer for DirectFB application
 */

#include <kni.h>
#include <midpMalloc.h>
#include <gxj_putpixel.h>
#include <directfbapp_export.h>

/** System offscreen buffer */
gxj_screen_buffer gxj_system_screen_buffer;

/** Return invalid file descriptor on keyboard device request */
int getKeyboardFd() {
    return -1;
}
/** Return invalid file descriptor on mouse device request */
int getMouseFd() {
    return -1;
}

/** Initializes the fbapp_ native resources */
void initScreenBuffer(int width, int height) {
    gxj_system_screen_buffer.width = width;
    gxj_system_screen_buffer.height = height;
    gxj_system_screen_buffer.pixelData = NULL;
    gxj_system_screen_buffer.alphaData = NULL;
}

/** Connect to frame buffer and enable direct fb application events */
void connectFrameBuffer() {
    gxj_system_screen_buffer.pixelData =
        (gxj_pixel_type *)directfbapp_open_window();
    gxj_system_screen_buffer.alphaData = NULL;
    gxj_system_screen_buffer.width = directfbapp_get_screen_width();
    gxj_system_screen_buffer.height = directfbapp_get_screen_height();

    directfbapp_enable_events();
}

/** Clear screen content */
void clearScreen() {
    // TODO: Stubbed implementation
}

/**
  * Change screen orientation to landscape or portrait,
  * depending on the current screen mode
  */
void reverseScreenOrientation() {
    // TODO: Stubbed implementation
}

/**
 * Resizes system screen buffer to fit the screen dimensions.
 * Call after frame buffer is initialized.
 */
void resizeScreenBuffer(int width, int height) {
    // TODO: Stubbed implementation
    (void)width; (void)height;
}

/** Get x-coordinate of screen origin */
int getScreenX(int rotatedScreen) {
    (void)rotatedScreen;
    return 0;
}

/** Get y-coordinate of screen origin */
int getScreenY(int rotatedScreen) {
    (void)rotatedScreen;
    return 0;
}

/** Refresh screen with offscreen buffer content */
void refreshScreenNormal(int x1, int y1, int x2, int y2) {
    gxj_system_screen_buffer.pixelData =
        (gxj_pixel_type *)directfbapp_refresh(x1, y1, x2, y2);
}

/** Refresh screen with offscreen buffer content */
void refreshScreenRotated(int x1, int y1, int x2, int y2) {
    // TODO: Stubbed implementation
    (void)x1; (void)x2;
    (void)y1; (void)y2;
}

/** Free allocated resources and restore system state */
void finalizeFrameBuffer() {
    directfbapp_finalize();
}
