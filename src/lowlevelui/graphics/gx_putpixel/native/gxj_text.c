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

#include <string.h>
#include <kni.h>
#include <midp_logging.h>
#include <midp_properties_port.h>

#include <gxapi_constants.h>
#include <gxjport_text.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"

/*
 * @file
 *
 * draws the first n characters specified using the current font,
 * color, and anchor point.
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
 * @param direction text output direction (LEFT_TO_RIGHT | RIGHT_TO_LEFT)
 * @param chararray Pointer to the characters to be drawn
 * @param n The number of characters to be drawn
 */
static void
drawString(jint pixel, const jshort *clip,
	      const java_imagedata *dst, int dotted, 
	      int face, int style, int size,
	      int x, int y, int anchor, int direction,
	      const jchar *charArray, int n) {

    int xDest;
    int yDest;
    int charsWidth;
    int charsHeight;
    int fontAscent;
    int fontDescent;
    int fontLeading;
    gxj_screen_buffer screen_buffer;
    gxj_screen_buffer *dest =
      gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
    dest = (gxj_screen_buffer *)getScreenBuffer(dest);

    REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_chars_impl()\n");

    if (n <= 0) {
        /* nothing to do */
        return;
    }

    xDest = x;
    yDest = y;  

    if (anchor & (RIGHT | HCENTER | BOTTOM | BASELINE)) {
        gx_get_fontinfo(face, style, size, &fontAscent, &fontDescent, &fontLeading);
        charsWidth = gx_get_charswidth(face, style, size, charArray, n);
        charsHeight = fontAscent + fontDescent + fontLeading;

        if (anchor & RIGHT) {
            xDest -= charsWidth;
        }

        if (anchor & HCENTER) {
            xDest -= charsWidth / 2;
        }

        if (anchor & BOTTOM) {
            yDest -= charsHeight;
        }

        if (anchor & BASELINE) {
            yDest -= charsHeight - fontDescent;
        }
    }

    // Request platform to draw chars on its own
    (void)gxjport_draw_chars(pixel, clip, dest, dotted, face, style, size,
                             xDest, yDest, anchor, direction, charArray, n);
}

void
gx_draw_chars(jint pixel, const jshort *clip,
	      const java_imagedata *dst, int dotted,
	      int face, int style, int size,
	      int x, int y, int anchor,
	      const jchar *charArray, int n) {

    /* In the case of right-to-left output the string with mixed content should be
     * analyzed for sequences of characters with the same direction. Each sequence
     * substring should be rendered with proper coordinates, anchor and direction.
     */

    /* Get locale to detect whether right-to-left output is needed */
    // TODO: move this check to the place where inicialization is performed
    const char* locale = getSystemProperty("microedition.locale");
    int direction = (locale != NULL && strcmp(locale, "he-IL") == 0) ?
               RIGHT_TO_LEFT : LEFT_TO_RIGHT;

    drawString(pixel, clip, dst, dotted, face, style, size,
        x, y, anchor, direction, charArray, n);
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
void
gx_get_fontinfo(int face, int style, int size, 
		int *ascent, int *descent, int *leading) {
    REPORT_CALL_TRACE(LC_LOWUI, "gx_get_fontinfo()\n");

    (void)gxjport_get_font_info(face, style, size,
                                ascent, descent, leading);
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
int
gx_get_charswidth(int face, int style, int size, 
		  const jchar *charArray, int n) {
    int width;

    REPORT_CALL_TRACE(LC_LOWUI, "gx_get_charswidth()\n");

    width = gxjport_get_chars_width(face, style, size, charArray, n);
    return width;
}
