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

#ifndef _GX_GRAPHICS_H_
#define _GX_GRAPHICS_H_


/**
 * @file
 * @ingroup lowui_gx
 *
 * @brief Porting api for graphics library
 */

#include <imgapi_image.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Translate a RGB or Gray Scale to device-dependent pixel value. 
 *
 * <p>
 * Related Java platform declaration:
 * <pre>
 *     getPixel(IIZ)I
 * </pre>
 *
 * @param rgb    compact RGB representation
 * @param gray   gray scale
 * @param isGray use gray scale
 */
extern jint gx_get_pixel(jint rgb, int gray, int isGray);

/**
 * Draws a straight line between the given coordinates using the
 * current color, stroke style, and clipping data.
 *
 * <p>
 * Related Java platform declaration:
 * <pre>
 *     drawLine(IIII)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted Stroke style
 * @param x1 x coordinate of the start point
 * @param y1 y coordinate of the start point
 * @param x2 x coordinate of the end point
 * @param y2 y coordinate of the end point
 */
extern void gx_draw_line(jint pixel, const jshort *clip, 
			 const java_imagedata *dst,
                         int dotted, int x1, int y1, int x2, int y2);

/**
 * Draws the outline of the specified rectangle using the current
 * color and stroke style at (x,y) position.
 *
 * <p>
 * Related Java platform declaration:
 * <pre>
 *     drawRect(IIII)V
 * </pre>
 *
 * @note There is no need to check for negative (or zero) of
 * width and height since they are already checked before MIDP
 * runtime calls this function.
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted Stroke style
 * @param x x coordinate of the rectangle
 * @param y y coordinate of the rectangle
 * @param width  Width of the rectangle
 * @param height Height of the rectangle
 */
extern void gx_draw_rect(jint pixel, const jshort *clip, 
	        	 const java_imagedata *dst,
                         int dotted, int x, int y, int width, int height);

/**
 * Fills the specified rectangle using the current color and
 * stroke style at (x,y) position.
 *
 * @note There is no need to check for negative (or zero) of width and
 * height since they are already checked before MIDP runtime calls
 * this function.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     fillRect(IIII)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 */
extern void gx_fill_rect(jint pixel, const jshort *clip, 
			 const java_imagedata *dst,
                         int dotted, int x, int y, int width, int height);

/**
 * Draws the outline of the specified rectangle, with rounded
 * corners, using the current color and stroke style.
 *
 * @note There is no need to check for negative (or zero) of
 * width and height since they are already checked before MIDP
 * runtime calls this function.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     drawRoundRect(IIIIII)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 * @param arcWidth The horizontal diameter of the arc at the four corners
 * @param arcHeight The vertical diameter of the arc at the four corners
 */
extern void gx_draw_roundrect(jint pixel, const jshort *clip, 
			       const java_imagedata *dst,
                               int dotted,
                               int x, int y, int width, int height,
                               int arcWidth, int arcHeight);

/**
 * Fills the outline of the specified rectangle, with rounded corners,
 * using the current color and stroke style.
 * 
 * @note There is no need to check for negative (or zero) of
 * width and height since they are already checked before MIDP
 * runtime calls this function.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     fillRoundRect(IIIIII)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 * @param arcWidth The horizontal diameter of the arc at the four corners
 * @param arcHeight The vertical diameter of the arc at the four corners
 */
extern void gx_fill_roundrect(jint pixel, const jshort *clip, 
			      const java_imagedata *dst, 
                              int dotted,
                              int x, int y, int width, int height,
                              int arcWidth, int arcHeight);

/**
 * Draws the outline of the specified circular or elliptical arc
 * segment using the current color and stroke style.
 *
 * The portion of the arc to be drawn starts at startAngle (with
 * 0 at the 3 o'clock position) and proceeds counterclockwise by
 * <arcAngle> degrees. Variable arcAngle may not be negative.
 *
 * @note There is no need to check for negative (or zero) of
 * width and height since they are already checked before MIDP.
 * If your platform supports drawing arc only counterclockwise
 * only, you should defined <B>PLATFORM_SUPPORT_CCW_ARC_ONLY</B>
 * to be true.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     drawArc(IIIIII)V
 * </pre>
 * 
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param x The x coordinate of the upper-left corner of the arc
 *          to be drawn
 * @param y The y coordinate of the upper-left corner of the arc
 *          to be drawn
 * @param width The width of the arc to be drawn
 * @param height The height of the arc to be drawn
 * @param startAngle The beginning angle
 * @param arcAngle The angular extent of the arc, relative to
 *                 <tt>startAngle</tt>
 */
extern void gx_draw_arc(jint pixel, const jshort *clip, 
			const java_imagedata *dst,
                        int dotted, int x, int y, int width, int height, 
                        int startAngle, int arcAngle);

/**
 * Fills the specified circular or elliptical arc segment using the
 * current color and stroke style.
 *
 * The portion of the arc to be drawn starts at startAngle (with
 * 0 at the 3 o'clock position) and proceeds counterclockwise by
 * <arcAngle> degrees. Variable arcAngle may not be negative.
 *
 * @note There is no need to check for negative (or zero) of
 * width and height since they are already checked before MIDP.
 * If your platform supports drawing arc only counterclockwise
 * only, you should defined <B>PLATFORM_SUPPORT_CCW_ARC_ONLY</B>
 * to be true.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     fillArc(IIIIII)V
 * </pre>
 * 
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param x The x coordinate of the upper-left corner of the arc
 *          to be drawn
 * @param y The y coordinate of the upper-left corner of the arc
 *          to be drawn
 * @param width The width of the arc to be drawn
 * @param height The height of the arc to be drawn
 * @param startAngle The beginning angle
 * @param arcAngle The angular extent of the arc, relative to
 *                 <tt>startAngle</tt>
 */
extern void gx_fill_arc(jint pixel, const jshort *clip, 
			const java_imagedata *dst,
                        int dotted, int x, int y, int width, int height, 
                        int startAngle, int arcAngle);

/**
 * Fills the specified triangle using the current color and stroke
 * style with the specify vertices (x1,y1), (x2,y2), and (x3,y3).
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     fillTriangle(IIIIII)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param dotted The stroke style to be used
 * @param x1 The x coordinate of the first vertex
 * @param y1 The y coordinate of the first vertex
 * @param x2 The x coordinate of the second vertex
 * @param y2 The y coordinate of the second vertex
 * @param x3 The x coordinate of the third vertex
 * @param y3 The y coordinate of the third vertex
 */
extern void gx_fill_triangle(jint pixel, const jshort *clip,
                             const java_imagedata *dst, int dotted,
                             int x1, int y1,
                             int x2, int y2,
                             int x3, int y3);

/**
 * Draws the first n characters specified using the current font,
 * color, and anchor point.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     drawString(Ljava/lang/String;III)V
 * </pre>
 *
 * @param pixel Device-dependent pixel color value
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
extern void gx_draw_chars(jint pixel, const jshort *clip, 
			  const java_imagedata *dst,
                          int dotted,
                          int face, int style, int size,
                          int x, int y, int anchor, 
                          const jchar *chararray, int n);


/**
 * Copies the specified region of the given image data to a new
 * destination, locating its anchor point at (x, y).
 *
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param x_src The x coordinate of the upper-left corner of the
 *        image source
 * @param y_src The y coordinate of the upper-left corner of the
 *        image source
 * @param width The width of the image in the source image
 * @param height The height of the image in the source image
 * @param x_dest The x coordinate of the upper-left corner of the
 *        image to be drawn
 * @param y_dest The y coordinate of the upper-left corner of the
 *        image to be drawn
 */
extern void gx_copy_area(const jshort *clip, const java_imagedata *dst,
                         int x_src, int y_src, int width, int height, 
                         int x_dest, int y_dest);

/**
 * Draws the specified pixels from the given data array. The
 * array consists of values in the form of 0xAARRGGBB.  It's
 * upper left corner will be located at (x,y).
 *
 * <p>
 * <b>Reference:</b>
 * Related Java platform declaration:
 * <pre>
 *     drawRGB([IIIIIIIZ)V
 * </pre>
 *
 * @param clip Clipping information
 * @param dst Platform dependent destination information
 * @param rgbData The array of ARGB pixels to draw
 * @param offset Zero-based index of first ARGB pixel to be drawn
 * @param scanlen Number of intervening pixels between pixels in
 *        the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *        region to draw
 * @param y The y coordinate of the upper left corner of the
 *        region to draw
 * @param width The width of the target region
 * @param height The height of the target region
 * @param processAlpha If <tt>true</tt>, alpha channel bytes
 *        should be used, otherwise, alpha channel bytes will
 *        be ignored
 */
extern void gx_draw_rgb(const jshort *clip, 
			const java_imagedata *dst, jint *rgbData, 
                        jint offset, jint scanlen, jint x, jint y, 
                        jint width, jint height, jboolean processAlpha);

/**
 * Return the displayed RGB value of a given RGB pixel in both 0xRRGGBB format.
 * For example, on systems where blue is only 5 bits, the value is
 * rounded down slightly. On 8-bit systems, the color is identical.
 *
 * @param color Java platform RGB color
 *
 * @return Device-dependent pixel color value but in Java platform color size
 */
extern jint gx_get_displaycolor(jint color);

#ifdef __cplusplus
}
#endif

#endif /* _GX_GRAPHICS_H_ */
