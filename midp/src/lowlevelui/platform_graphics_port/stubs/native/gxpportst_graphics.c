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

#include <gxpport_graphics.h>
#include <gxpport_font.h>
#include <midp_logging.h>

/**
 * @file
 *
 * Stubs of primitive graphics. 
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Draw triangle
 *
 * @param pixel The packed pixel value
 */
extern void gxpport_fill_triangle(
  jint pixel, const jshort *clip,
  gxpport_mutableimage_native_handle dst, int dotted,
  int x1, int y1,
  int x2, int y2,
  int x3, int y3) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_fill_triangle()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)x3;
    (void)y3;
}

/**
 * Copy from a specify region to other region
 */
extern void gxpport_copy_area(
  const jshort *clip, gxpport_mutableimage_native_handle dst,
  int x_src, int y_src, int width, int height, 
  int x_dest, int y_dest) {
    
    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_copy_area()\n");

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
extern void gxpport_draw_rgb(
  const jshort *clip, 
  gxpport_mutableimage_native_handle dst, jint *rgbData, 
  jint offset, jint scanlen, jint x, jint y, 
  jint width, jint height, jboolean processAlpha) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_drawRGB()\n");

    /* Suppress unused parameter warnings */
    (void)clip;
    (void)dst;
    (void)rgbData;
    (void)offset;
    (void)scanlen;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)processAlpha;
}

/**
 * Obtain the color that will be final shown 
 * on the screen after the system processed it.
 */
extern jint gxpport_get_displaycolor(jint color) {

    REPORT_CALL_TRACE1(LC_LOWUI, "LF:STUB:gxpport_get_displaycolor(%d)\n", 
		       color);
    
    return color; /* No change */
}


/**
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
extern void gxpport_draw_line(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted, int x1, int y1, int x2, int y2)
{

    REPORT_CALL_TRACE(LC_LOWUI, "gxpport_draw_line()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

/**
 * Draw a rectangle at (x,y) with the given width and height.
 *
 * @note x, y sure to be >=0
 *       since x,y is quan. to be positive (>=0), we don't
 *       need to test for special case anymore.
 */
extern void gxpport_draw_rect(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted, int x, int y, int width, int height)
{

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_draw_rect()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

/**
 * Fill a rectangle at (x,y) with the given width and height.
 */
extern void gxpport_fill_rect(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted, int x, int y, int width, int height) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_fill_rect()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

/**
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
extern void gxpport_draw_roundrect(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted,
  int x, int y, int width, int height,
  int arcWidth, int arcHeight)
{

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_draw_roundrect()\n");

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
extern void gxpport_fill_roundrect(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst, 
  int dotted,
  int x, int y, int width, int height,
  int arcWidth, int arcHeight)
{

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_fill_roundrect()\n");
    
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
extern void gxpport_draw_arc(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted, int x, int y, int width, int height, 
  int startAngle, int arcAngle)
{

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_draw_arc()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)startAngle;
    (void)arcAngle;
}

/**
 * Fill an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
extern void gxpport_fill_arc(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted, int x, int y, int width, int height, 
  int startAngle, int arcAngle)
{

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_fill_arc()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)startAngle;
    (void)arcAngle;
}

/**
 * Return the pixel value.
 */
extern jint gxpport_get_pixel(
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
extern void gxpport_draw_chars(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted,
  int face, int style, int size,
  int x, int y, int anchor, 
  const jchar *charArray, int n) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_drawChars()\n");

    /* Suppress unused parameter warnings */
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)face;
    (void)size;
    (void)style;
    (void)x;
    (void)y;
    (void)anchor;
    (void)charArray;
    (void)n;
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
extern void gxpport_get_fontinfo(
  int face, int style, int size,
  int *ascent, int *descent, int *leading) {

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_getFontInfo()\n");

    /* Suppress unused parameter warnings */
    (void)face;
    (void)size;
    (void)style;

    /* Not yet implemented */
    *ascent = 0;
    *descent = 0;
    *leading = 0;
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

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:gxpport_charsWidth()\n");

    /* Suppress unused parameter warnings */
    (void)face;
    (void)size;
    (void)style;
    (void)charArray;
    (void)n;

    /* Not yet implemented */
    return 0;
}
