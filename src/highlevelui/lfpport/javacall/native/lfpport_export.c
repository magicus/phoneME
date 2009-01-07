/*
 *
 *
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


/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Platform Look and Feel Port exported native interface
 */


#include <gxapi_graphics.h>
#include <imgapi_image.h>
#include <lfpport_export.h>
#include "lfpport_gtk.h"
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gint display_width;
extern gint display_height;
extern GtkWidget *main_window;
extern GtkLabel  *ticker;
extern GMainLoop *main_loop;
extern GdkPixmap *current_mutable;
extern gint tmpFlag;

#define LFPPORT_SCREEN_HEIGHT       320
#define LFPPORT_SCREEN_WIDTH        240

 /**
 * Refresh the given area.  For double buffering purposes.
 */
void lfpport_refresh(int x, int y, int w, int h){
    guchar *pixels;
    int rowstride;
    GdkRectangle clipRectangle;
    GdkGC *gc;
    GdkRectangle rect;
    GtkWidget *form;
    GtkWidget *da;

    LIMO_TRACE(">>>%s x=%d y=%d w=%d h=%d\n", __FUNCTION__, x, y, w, h);

//     clipRectangle.x = 0;
//     clipRectangle.y = 0;
//     clipRectangle.width = w;
//     clipRectangle.height = h;

//     rect.x = x;
//     rect.y = y;
//     rect.width = w;
//     rect.height = h;

//    gc = main_window->style->black_gc;


//    gdk_gc_set_clip_rectangle(gc, &clipRectangle);

//     form = gtk_main_window_get_current_form(main_window);
//     da = gtk_object_get_user_data(form);
//    LIMO_TRACE("%s da=%x\n", __FUNCTION__, da);

//    gdk_draw_drawable(main_window->window,
//    gdk_draw_drawable(da->window,
//              gc,
//              current_mutable,
//              x,   /* x */
//              y,   /* y */
//              x,
//              y,
//              w,   /* width */
//              h);  /* height */

//
//     gdk_window_invalidate_rect(da, &rect, FALSE);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * set the screen mode either to fullscreen or normal.
 *
 * @param mode The screen mode
 */
void lfpport_set_fullscreen_mode(jboolean mode){
    LIMO_TRACE(">>>%s mode=%d\n", __FUNCTION__, mode);
    (void)mode;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Change screen orientation flag
 */
jboolean lfpport_reverse_orientation(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return 0;
}

/**
 * Get screen orientation flag
 */
jboolean lfpport_get_reverse_orientation(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return 0;
}

/**
 * Return screen width
 */
int lfpport_get_screen_width(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s returning %d\n", __FUNCTION__, display_width);
    return display_width;
}

/**
 *  Return screen height
 */
int lfpport_get_screen_height(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s returning %d\n", __FUNCTION__, display_height);
    return display_height;
}

/**
 * Resets native resources when foreground is gained by a new display.
 */
void lfpport_gained_foreground(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Initializes the window system.
 */
void lfpport_ui_init(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    stub_func();
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Finalize the window system.
 */
void lfpport_ui_finalize(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_destroy(main_window);
    gtk_main_quit();
    //g_main_loop_quit(main_loop);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Flushes the offscreen buffer directly to the device screen.
 * The size of the buffer flushed is defined by offscreen buffer width
 * and passed in height.
 * Offscreen_buffer must be aligned to the top-left of the screen and
 * its width must be the same as the device screen width.
 * @param graphics The Graphics handle associated with the screen.
 * @param offscreen_buffer The ImageData handle associated with
 *                         the offscreen buffer to be flushed
 * @param height The height to be flushed
 * @return KNI_TRUE if direct_flush was successful, KNI_FALSE - otherwise
 */
jboolean lfpport_direct_flush(const java_graphics *g,
		  	      const java_imagedata *offscreen_buffer, int h){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return 0;
}

#ifdef __cplusplus
}
#endif

