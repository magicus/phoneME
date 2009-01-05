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

#include <gxpport_graphics.h>
#include <lfpport_error.h>
#include <gxpport_font.h>
#include <midp_logging.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <syslog.h>
#include "lfpport_gtk.h"
#include "lfpport_font.h"


extern GtkWidget *main_canvas;
extern GtkWidget *main_window;
extern gint display_width;
extern gint display_height;
extern GdkScreen *gdk_screen;

/**
 * @file
 *
 * Stubs of primitive graphics.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define     GXPPORT_GTK_FONT_ASCENT     10
#define     GXPPORT_GTK_FONT_DESCENT    2
#define     GXPPORT_GTK_FONT_LEADING    2
#define     GXPPORT_GTK_CHARS_WIDTH     4

extern MidpError lfpport_get_font(PlatformFontPtr* fontPtr,
               int face, int style, int size);


extern GdkPixmap *current_mutable;

GdkGC *get_gc(void *dst){
    GtkWidget *form;
    GtkWidget *da;
    GdkGC *gc;

    if (dst != NULL && current_mutable == dst) {
        gc = gdk_gc_new(current_mutable);
        return gc;
    }

    form = gtk_main_window_get_current_form(main_window);
    da = gtk_object_get_user_data(form);
    if (form == NULL || da == NULL || !GTK_IS_DRAWING_AREA(da)) {
        LIMO_TRACE("%s null form or da\n", __FUNCTION__);
        return NULL;
    }

    gc = gdk_gc_new(da->window);
    return gc;
}

GdkPixmap *get_pix_map(void *dst){
    if (!dst) {
        return current_mutable;
    }
    if (!GDK_IS_PIXMAP(dst)) {
        LIMO_TRACE("%s GdkPixmap expected as dst!\n", __FUNCTION__);
        return;
    }
    return (GdkPixmap*)dst;
}

GtkWidget *get_dst_widget_for_chars(void *dst){
    GtkWidget *form;
    GtkWidget *da;

    if (!dst) {
        return main_window;
    }

    form = gtk_main_window_get_current_form(main_window);
    da = gtk_object_get_user_data(form);
    if (form == NULL || da == NULL || !GTK_IS_DRAWING_AREA(da)) {
        LIMO_TRACE("%s null form or da\n", __FUNCTION__);
        return main_window;
    }

    return da;
}

GdkRectangle create_clip_rectangle(const jshort *clip){
    GdkRectangle clipRectangle;

    clipRectangle.x = clip[0];
    clipRectangle.y = clip[1];
    clipRectangle.width = clip[2] - clip[0];
    clipRectangle.height = clip[3] - clip[1];
    return clipRectangle;
}


/**
 * Draw triangle
 *
 * @param pixel The packed pixel value
 */
void gxpport_fill_triangle(
                                 jint pixel, const jshort *clip,
                                 gxpport_mutableimage_native_handle dst, int dotted,
                                 int x1, int y1,
                                 int x2, int y2,
                                 int x3, int y3) {

    GtkForm *form;
    GtkWidget *da;
    GdkGC *gc;
    GdkColor gdkColor;
    GdkPixmap *gdk_pix_map;
    GdkLineStyle lineStyle;
    GdkPoint points[3];
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);

    gdkColor.pixel = pixel;    /* 0x00RRGGBB */
    gdkColor.red = gdkColor.green = gdkColor.blue = 0;
    gdk_gc_set_foreground(gc, &gdkColor);

    lineStyle = dotted ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID;

    points[0].x = x1;
    points[0].y = y1;
    points[1].x = x2;
    points[1].y = y2;
    points[2].x = x3;
    points[2].y = y3;

    gdk_gc_set_line_attributes(gc,
                               1,   /* line_width */
                               lineStyle,   /* line_style */
                               GDK_CAP_BUTT, /* cap_style */
                               GDK_JOIN_BEVEL); /* join_style */

    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);

    gdk_draw_polygon(gdk_pix_map,
                          gc,
                          TRUE,
                          points,
                          3);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);}

/**
 * Copy from a specify region to other region
 */
void gxpport_copy_area(
                             const jshort *clip, gxpport_mutableimage_native_handle dst,
                             int x_src, int y_src, int width, int height,
                             int x_dest, int y_dest) {

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);


    /* Suppress unused parameter warnings */
    (void)clip;
    (void)dst;
    (void)x_src;
    (void)y_src;
    (void)width;
    (void)height;
    (void)x_dest;
    (void)y_dest;
}

/**
 * Draw image in RGB format
 */
void gxpport_draw_rgb(
                        const jshort *clip,
                        gxpport_mutableimage_native_handle dst, jint *rgbData,
                        jint offset, jint scanlen, jint x, jint y,
                        jint width, jint height, jboolean processAlpha) {
    GdkPixmap *gdk_pix_map;
    GdkPixmap *src_pix_map;
    GtkWidget *da;
    GtkWidget *form;
    GdkGC *gc;
    int tfd;
    GdkColor gdkColor;
    jint nwidth, nheight;
    int length;
    GdkPixbuf *gdkPixBuf;
    guchar *pixels;
    int rowstride;
    GError *error = NULL;
    GdkImage *image;
    int i,j;
    int row;

    LIMO_TRACE(">>>%s dst=%x rgbData=%x offset=%d scanlen=%d x=%d y=%d "
               "width=%d height=%d processAlpha=%d\n", __FUNCTION__, dst, rgbData,
               offset, scanlen, x, y, width, height, processAlpha);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);

    g_usleep(15 * 1000);

    gdk_draw_rgb_32_image(gdk_pix_map,
                          gc,
                          x,
                          y,
                          width,
                          height,
                          GDK_RGB_DITHER_NONE,
                          rgbData,
                          scanlen * 4);


    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Obtain the color that will be final shown
 * on the screen after the system processed it.
 */
jint gxpport_get_displaycolor(jint color) {

    REPORT_CALL_TRACE1(LC_LOWUI, "LF:STUB:gxpport_get_displaycolor(%d)\n",
                       color);

    return color; /* No change */
}


/**
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
void gxpport_draw_line(
                             jint pixel, const jshort *clip,
                             gxpport_mutableimage_native_handle dst,
                             int dotted, int x1, int y1, int x2, int y2)
{
    GtkForm *form;
    GtkWidget *da;
    GdkGC *gc;
    GdkColor gdkColor;
    GdkPixmap *gdk_pix_map;
    GdkLineStyle lineStyle;
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);


    gdkColor.pixel = pixel;    /* 0x00RRGGBB */
    gdkColor.red = gdkColor.green = gdkColor.blue = 0;
    gdk_gc_set_foreground(gc, &gdkColor);

    lineStyle = dotted ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID;

    gdk_gc_set_line_attributes(gc,
                               1,   /* line_width */
                               lineStyle,   /* line_style */
                               GDK_CAP_BUTT, /* cap_style */
                               GDK_JOIN_BEVEL); /* join_style */

    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);
    gdk_draw_line(gdk_pix_map,
                          gc,
                          x1,
                          y1,
                          x2,
                          y2);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Draw a rectangle at (x,y) with the given width and height.
 *
 * @note x, y sure to be >=0
 *       since x,y is quan. to be positive (>=0), we don't
 *       need to test for special case anymore.
 */
void gxpport_draw_rect(
                             jint pixel, const jshort *clip,
                             gxpport_mutableimage_native_handle dst,
                             int dotted, int x, int y, int width, int height)
{
    GtkForm *form;
    GtkWidget *da;
    GdkGC *gc;
    GdkColor gdkColor;
    GdkPixmap *gdk_pix_map;
    GdkLineStyle lineStyle;
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s dst=%x\n", __FUNCTION__, dst);
    LIMO_TRACE(">>>%s clip is %d, %d, %d, %d\n", __FUNCTION__, clip[0], clip[1], clip[2], clip[3]);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);


    gdkColor.pixel = pixel;    /* 0x00RRGGBB */
    gdkColor.red = gdkColor.green = gdkColor.blue = 0;
    gdk_gc_set_foreground(gc, &gdkColor);

    lineStyle = dotted ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID;


    gdk_gc_set_line_attributes(gc,
                               1,   /* line_width */
                               lineStyle,   /* line_style */
                               GDK_CAP_BUTT, /* cap_style */
                               GDK_JOIN_BEVEL); /* join_style */

    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);

    gdk_draw_rectangle(gdk_pix_map,
                          gc,
                          FALSE,
                          x, y,
                          width,
                          height);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Fill a rectangle at (x,y) with the given width and height.
 */
void gxpport_fill_rect(
                             jint pixel, const jshort *clip,
                             gxpport_mutableimage_native_handle dst,
                             int dotted, int x, int y, int width, int height) {
    GtkForm *form;
    GtkWidget *da;
    GdkGC *gc;
    GdkColor gdkColor;
    GdkPixmap *gdk_pix_map;
    GdkLineStyle lineStyle;
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s dst=%x x=%d y=%d width=%d height=%d\n",
               __FUNCTION__, dst, x, y, width, height);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);

    gdkColor.pixel = pixel;    /* 0x00RRGGBB */
    gdkColor.red = gdkColor.green = gdkColor.blue = 0;
    gdk_gc_set_foreground(gc, &gdkColor);

    lineStyle = dotted ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID;

    gdk_gc_set_line_attributes(gc,
                               1,   /* line_width */
                               lineStyle,   /* line_style */
                               GDK_CAP_BUTT, /* cap_style */
                               GDK_JOIN_BEVEL); /* join_style */

    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);

    LIMO_TRACE("%s clip.x=%d clip.y=%d clip.width=%d clip.height=%d\n",
               __FUNCTION__, clip[0], clip[1], clip[2], clip[3]);

    gdk_draw_rectangle(gdk_pix_map,
                          gc,
                          TRUE,
                          x, y,
                          width,
                          height);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void gxpport_draw_roundrect(
                                  jint pixel, const jshort *clip,
                                  gxpport_mutableimage_native_handle dst,
                                  int dotted,
                                  int x, int y, int width, int height,
                                  int arcWidth, int arcHeight)
{

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)arcWidth;
    (void)arcHeight;
}

/**
 * Fill a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void gxpport_fill_roundrect(
                                  jint pixel, const jshort *clip,
                                  gxpport_mutableimage_native_handle dst,
                                  int dotted,
                                  int x, int y, int width, int height,
                                  int arcWidth, int arcHeight)
{

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)arcWidth;
    (void)arcHeight;
}

/**
 *
 * Draw an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle>
 * degrees.  arcAngle may not be negative.
 *
 * @note: check for width, height <0 is done in share layer
 */
void gxpport_draw_arc(
                            jint pixel, const jshort *clip,
                            gxpport_mutableimage_native_handle dst,
                            int dotted, int x, int y, int width, int height,
                            int startAngle, int arcAngle)
{
    GtkForm *form;
    GtkWidget *da;
    GdkGC *gc;
    GdkColor gdkColor;
    GdkPixmap *gdk_pix_map;
    GdkLineStyle lineStyle;
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);

    gdkColor.pixel = pixel;    /* 0x00RRGGBB */
    gdkColor.red = gdkColor.green = gdkColor.blue = 0;
    gdk_gc_set_foreground(gc, &gdkColor);

    lineStyle = dotted ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID;

    gdk_gc_set_line_attributes(gc,
                               1,   /* line_width */
                               lineStyle,   /* line_style */
                               GDK_CAP_BUTT, /* cap_style */
                               GDK_JOIN_BEVEL); /* join_style */

    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);

    gdk_draw_arc(gdk_pix_map,
                      gc,
                      FALSE,
                      x,
                      y,
                      width,
                      height,
                      startAngle,
                      arcAngle);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Fill an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle>
 * degrees.  arcAngle may not be negative.
 */
void gxpport_fill_arc(
                            jint pixel, const jshort *clip,
                            gxpport_mutableimage_native_handle dst,
                            int dotted, int x, int y, int width, int height,
                            int startAngle, int arcAngle)
{
    GtkForm *form;
    GtkWidget *da;
    GdkGC *gc;
    GdkColor gdkColor;
    GdkPixmap *gdk_pix_map;
    GdkLineStyle lineStyle;
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    gc = get_gc(dst);
    gdk_pix_map = get_pix_map(dst);

    gdkColor.pixel = pixel;    /* 0x00RRGGBB */
    gdkColor.red = gdkColor.green = gdkColor.blue = 0;
    gdk_gc_set_foreground(gc, &gdkColor);

    lineStyle = dotted ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID;


    gdk_gc_set_line_attributes(gc,
                               1,   /* line_width */
                               lineStyle,   /* line_style */
                               GDK_CAP_BUTT, /* cap_style */
                               GDK_JOIN_BEVEL); /* join_style */

    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);

    gdk_draw_arc(gdk_pix_map,
                      gc,
                      TRUE,
                      x,
                      y,
                      width,
                      height,
                      startAngle,
                      arcAngle);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Return the pixel value.
 */
jint gxpport_get_pixel(
                             jint rgb, int gray, int isGray) {

    REPORT_CALL_TRACE3(LC_LOWUI, "LF:STUB:gxpport_getPixel(%x, %x, %d)\n",
                       rgb, gray, isGray);

    /* Suppress unused parameter warnings */
    (void)gray;
    (void)isGray;

    return rgb;
}

/*
 * Draws the first n characters specified using the current font,
 * color, and anchor point.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java declaration:
 * <pre>
 *     drawString(Ljava/lang/String;III)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param face The font face to be used (Defined in <B>Font.java</B>)
 * @param style The font style to be used (Defined in
 * <B>Font.java</B>)
 * @param size The font size to be used. (Defined in <B>Font.java</B>)
 * @param x The x coordinate of the anchor point
 * @param y The y coordinate of the anchor point
 * @param anchor The anchor point for positioning the text
 * @param chararray Pointer to the characters to be drawn
 * @param n The number of characters to be drawn
 */
void gxpport_draw_chars(
                              jint pixel, const jshort *clip,
                              gxpport_mutableimage_native_handle dst,
                              int dotted,
                              int face, int style, int size,
                              int x, int y, int anchor,
                              const jchar *charArray, int n) {
    gchar text_buf[MAX_TEXT_LENGTH];
    MidpError status;
    PlatformFontPtr fontPtr;
    GdkColor gdkColor;
    GtkWidget *dstWidget;
    GtkWidget *form;
    GdkPixmap *gdk_pix_map;
    PangoRenderer *renderer;
    PangoMatrix matrix = PANGO_MATRIX_INIT;
    PangoContext *context;
    PangoLayout *layout;
    PangoFontDescription *desc;
    GdkGC *gc;
    int text_len;
    int i;
    GdkRectangle clipRectangle;

    LIMO_TRACE(">>>%s dst=%x x=%d y=%d anchor=%d n=%d\n",
               __FUNCTION__, dst, x, y, anchor, n);

    status = lfpport_get_font(&desc,
                              face,
                              style,
                              size);
    if (status != KNI_OK) {
        LIMO_TRACE("%s lfpport_get_font returned error status %d\n", __FUNCTION__, status);
        return;
    }

    //set color
    gdkColor.pixel = pixel;

    //translate text
    for (i = 0; i < n; i++) {
        text_buf[i] = charArray[i];
    }

    gc = get_gc(dst);
    gdk_gc_set_foreground(gc, &gdkColor);
    clipRectangle = create_clip_rectangle(clip);
    gdk_gc_set_clip_rectangle(gc, &clipRectangle);
    gdk_pix_map = get_pix_map(dst);
    dstWidget = get_dst_widget_for_chars(dst);

    /* Get the default renderer for the screen, and set it up for drawing  */
    renderer = gdk_pango_renderer_get_default(gdk_screen);
    gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER (renderer), gdk_pix_map);
    gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER (renderer), gc);
    LIMO_TRACE("%s renderer=%x\n", __FUNCTION__, renderer);
    /* Create a PangoLayout, set the font and text */
    context = gtk_widget_create_pango_context(dstWidget);
    layout = pango_layout_new (context);

    LIMO_TRACE("%s context=%x layout=%x text_buf=%s\n", __FUNCTION__, context, layout, text_buf);
    pango_layout_set_text(layout, text_buf, n);
    pango_layout_set_font_description (layout, desc);

    pango_context_set_matrix(context, &matrix);
    pango_layout_context_changed(layout);
    pango_renderer_draw_layout(renderer, layout,
                  x * PANGO_SCALE,
                  y * PANGO_SCALE);

    g_object_unref(layout);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Obtains the ascent, descent and leading info for the font indicated.
 *
 * @param face The face of the font (Defined in <B>Font.java</B>)
 * @param style The style of the font (Defined in <B>Font.java</B>)
 * @param size The size of the font (Defined in <B>Font.java</B>)
 * @param ascent The font's ascent should be returned here.
 * @param descent The font's descent should be returned here.
 * @param leading The font's leading should be returned here.
 */
void gxpport_get_fontinfo(
  int face, int style, int size,
  int *ascent, int *descent, int *leading) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_getFontInfo()\n");

    /* Suppress unused parameter warnings */
    (void)face;
    (void)size;
    (void)style;

    /* Return hard-coded values */
    *ascent = GXPPORT_GTK_FONT_ASCENT;
    *descent = GXPPORT_GTK_FONT_DESCENT;
    *leading = GXPPORT_GTK_FONT_LEADING;
}

/**
 * Gets the advance width for the first n characters in charArray if
 * they were to be drawn in the font indicated by the parameters.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java declaration:
 * <pre>
 *     charWidth(C)I
 * </pre>
 *
 * @param face The font face to be used (Defined in <B>Font.java</B>)
 * @param style The font style to be used (Defined in
 * <B>Font.java</B>)
 * @param size The font size to be used. (Defined in <B>Font.java</B>)
 * @param charArray The string to be measured
 * @param n The number of character to be measured
 * @return The total advance width in pixels (a non-negative value)
 */
extern int gxpport_get_charswidth(
                 int face, int style, int size,
                 const jchar *charArray, int n) {

    GtkWidget *da;
    PangoLayout *layout;
    PangoContext *context;
    MidpError status;
    PangoFontDescription *desc;
    gchar text_buf[MAX_TEXT_LENGTH];
    int width, height;
    int i;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    //translate text
    for (i = 0; i < n; i++) {
        text_buf[i] = charArray[i];
    }

    status = lfpport_get_font(&desc, face, style, size);
    if (status != KNI_OK) {
        return 0;
    }

    da = gtk_drawing_area_new(); /* at this point main window might have not been created yet */
    gtk_widget_set_size_request(da, display_width, display_height);
    gtk_widget_show(da);

    context = gtk_widget_create_pango_context(da);
    layout = pango_layout_new(context);
    pango_layout_set_text(layout, text_buf, n);
    pango_layout_set_font_description (layout, desc);
    pango_layout_get_pixel_size (layout, &width, &height);
    g_object_unref(layout);
    g_object_unref (context);
    LIMO_TRACE("%s width=%d height=%d\n", __FUNCTION__, width, height);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return width;
}

