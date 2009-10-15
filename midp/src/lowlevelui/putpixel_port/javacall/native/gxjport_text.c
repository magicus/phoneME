/*
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

#include <kni.h>
#include "javacall_font.h"
#include "gxj_putpixel.h"


/**
 * Draws the first n characters to Offscreen memory image specified using the
 * current font, color.
 *
 * @param pixel color of font
 * @param clip  coordinates of the clipping area.
 *              clip[0] corresponds to clipX1.  clipX1 is the top left X
 *              coordinate of the clipping area.  clipX1 is guaranteeded
 *              to be larger or equal 0 and smaller or equal than clipX2.
 *              clip[1] corresponds to clipY1.  clipY1 is the top left Y
 *              coordinate of the clipping area.  clipY1 is guaranteeded
 *              to be larger or equal 0 and smaller or equal than clipY2
 *              clip[2] corresponds to clipX2.  clipX2 is the bottom right
 *              X coordinate of the clipping area. clipX2 is guaranteeded
 *              to be larger or equal than clipX1 and smaller or equal than
 *              dst->width.
 *              clip[3] corresponds to clipY2.  clipY2 is the bottom right
 *              Y coordinate of the clipping area.  clipY2 is guaranteeded
 *              to be larger or equal than clipY1 and smaller or equal than
 *              dst->height.
 * @param dst   pointer to gxj_screen_buffer struct
 * @param dotted
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @param x The x coordinate of the top left font coordinate
 * @param y The y coordinate of the top left font coordinate
 * @param anchor
 * @param chararray Pointer to the characters to be drawn
 * @param n The number of characters to be drawn
 *
 * @return <tt>KNI_TRUE</tt> if font rendered successfully,
 *         <tt>KNI_FALSE</tt> or negative value on error or not supported
 */
int gxjport_draw_chars(int pixel, const jshort *clip, gxj_screen_buffer *dst,
                       int dotted, int face, int style, int size,
                       int x, int y, int anchor,
                       const jchar *chararray, int n) {

    javacall_result result;

    (void)anchor;
    (void)dotted;

    result = javacall_font_set_font(face, style, size);

    if (JAVACALL_OK != result) {
        return KNI_FALSE;
    }

    result = javacall_font_draw((javacall_pixel)GXJ_RGB24TORGB16(pixel),
                                     clip[0],
                                     clip[1],
                                     clip[2],
                                     clip[3],
                                     (javacall_pixel*)dst->pixelData,
                                     dst->width,
                                     dst->height,
                                     x,
                                     y,
                                     chararray,
                                     n);
    return JAVACALL_OK == result ? KNI_TRUE : KNI_FALSE;
}

/**
 * queries for the font info structure for a given font specs
 *
 * @param face The font face to be used (Defined in <B>Font.java</B>)
 * @param style The font style to be used (Defined in
 * <B>Font.java</B>)
 * @param size The font size to be used. (Defined in <B>Font.java</B>)
 *
 * @param ascent return value of font's ascent
 * @param descent return value of font's descent
 * @param leading return value of font's leading
 *
 * @return <tt>KNI_TRUE</tt> if successful, <tt>KNI_FALSE</tt> on
 * error or not supported
 *
 */
int gxjport_get_font_info(int face, int style, int size,
                          int *ascent, int *descent, int *leading) {

    javacall_result result;

    result = javacall_font_get_info(face, style, size, ascent, descent, leading);

    return JAVACALL_OK == result ? KNI_TRUE : KNI_FALSE;
}

/**
 * returns the char width for the first n characters in charArray if
 * they were to be drawn in the font indicated by the parameters.
 *
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @param charArray The string to be measured
 * @param n The number of character to be measured
 * @return total advance width in pixels (a non-negative value)
 *         or 0 if not supported
 */
int gxjport_get_chars_width(int face, int style, int size,
                            const jchar *charArray, int n) {
    int result;

    result = javacall_font_get_width(face, style, size, charArray, n);

    return result;
}

