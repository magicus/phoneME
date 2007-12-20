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

#include <midpError.h>
#include <midpEventUtil.h>

#include <gx_graphics.h>
#include <gxapi_constants.h>
#include <gxapi_graphics.h>
#include "gxapi_intern_graphics.h"

#ifdef UNDER_CE
#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"
#include "gxj_intern_image.h"
extern void fast_rect_8x8(void*first_pixel, int ypitch, int pixel);
#endif

/**
 * Get a C structure representing the given <tt>ImageData</tt> class.
 */
#define GET_IMAGEDATA_PTR_FROM_GRAPHICS(handle) \
  GXAPI_GET_GRAPHICS_PTR(handle)->img != NULL ? \
  GXAPI_GET_GRAPHICS_PTR(handle)->img->imageData : \
  (java_imagedata*)NULL

/**
 * Gets the clipping region of the given graphics object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 * @param ARRAY native <tt>jshort</tt> array to save the clip data
 */
#define GET_CLIP(G, ARRAY) \
    ARRAY[0] = GXAPI_GET_GRAPHICS_PTR(G)->clipX1, \
    ARRAY[1] = GXAPI_GET_GRAPHICS_PTR(G)->clipY1, \
    ARRAY[2] = GXAPI_GET_GRAPHICS_PTR(G)->clipX2, \
    ARRAY[3] = GXAPI_GET_GRAPHICS_PTR(G)->clipY2


/**
 * Translate the pixel location according to the translation of
 * the given graphics object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 * @param X variable representing the <tt>x</tt> coordinate to be translated;
 *        this macro sets the value of X
 * @param Y variable representing the <tt>y</tt> coordinate to be translated;
 *        this macro sets the value of Y
 */
#define TRANSLATE(G, X, Y)  \
    (X) += GXAPI_GET_GRAPHICS_PTR((G))->transX, \
    (Y) += GXAPI_GET_GRAPHICS_PTR((G))->transY

/**
 * @file
 * Implementation of Java native methods for the <tt>Graphics</tt> class.
 */

/**
 * Draws a straight line between the given coordinates using the
 * current color, stroke style, and clipping data.
 * <p>
 * Java declaration:
 * <pre>
 *     drawLine(IIII)V
 * </pre>
 *
 * @param x1 The x coordinate of the start of the line
 * @param y1 The y coordinate of the start of the line
 * @param x2 The x coordinate of the end of the line
 * @param y2 The y coordinate of the end of the line
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawLine) {
    int y2 = KNI_GetParameterAsInt(4);
    int x2 = KNI_GetParameterAsInt(3);
    int y1 = KNI_GetParameterAsInt(2);
    int x1 = KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);

    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

        TRANSLATE(thisObject, x1, y1);
        TRANSLATE(thisObject, x2, y2);

        GET_CLIP(thisObject, clip);

        gx_draw_line(GET_PIXEL(thisObject), 
                     clip, 
                     GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                     GET_LINESTYLE(thisObject), 
                     x1, y1, x2, y2);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws the outline of the specified rectangle using the current
 * color and stroke style. According to the MIDP specification,
 * if either the <tt>width</tt> or <tt>height</tt> are negative,
 * no rectangle is drawn.
 * <p>
 * Java declaration:
 * <pre>
 *     drawRect(IIII)V
 * </pre>
 *
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawRect) {
    int h = KNI_GetParameterAsInt(4);
    int w = KNI_GetParameterAsInt(3);
    int y = KNI_GetParameterAsInt(2);
    int x = KNI_GetParameterAsInt(1);

    /*
     * @note { Spec verify step: "If either width or height
     * is less than zero, nothing is drawn." }
     */
    if ((w >= 0) && (h >= 0)) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(thisObject);
        
        KNI_GetThisPointer(thisObject);
        
        if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            TRANSLATE(thisObject, x, y);
          
            GET_CLIP(thisObject, clip);

            gx_draw_rect(GET_PIXEL(thisObject),
                         clip,
                         GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                         GET_LINESTYLE(thisObject),
                         x, y, w, h);
        }

        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

/**
 * Fills the specified rectangle using the current color and
 * stroke style. According to the MIDP specification, if either
 * the <tt>width</tt> or <tt>height</tt> are negative, no
 * rectangle is drawn.
 * <p>
 * Java declaration:
 * <pre>
 *     fillRect(IIII)V
 * </pre>
 *
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_fillRect) {
    int h = KNI_GetParameterAsInt(4);
    int w = KNI_GetParameterAsInt(3);
    int y = KNI_GetParameterAsInt(2);
    int x = KNI_GetParameterAsInt(1);

    /*
     * @note { Spec verify step: "If either width or height
     * is zero or less, nothing is drawn." }
     */
    if ((w >= 0) && (h >= 0)) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(thisObject);
        
        KNI_GetThisPointer(thisObject);

        if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            TRANSLATE(thisObject, x, y);

            GET_CLIP(thisObject, clip);

            gx_fill_rect(GET_PIXEL(thisObject),
                         clip,
                         GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                         GET_LINESTYLE(thisObject),
                         x, y, w, h);
        }

        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

/**
 * Draws the outline of the specified rectangle, with rounded corners,
 * using the current color and stroke style. According to the MIDP
 * specification, if either the <tt>width</tt> or <tt>height</tt>
 * are negative, no rectangle is drawn.
 * <p>
 * Java declaration:
 * <pre>
 *     drawRoundRect(IIIIII)V
 * </pre>
 *
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 * @param arcWidth The horizontal diameter of the arc at the four corners
 * @param arcHeight The vertical diameter of the arc at the four corners
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawRoundRect) {
    int arcHeight = KNI_GetParameterAsInt(6);
    int  arcWidth = KNI_GetParameterAsInt(5);
    int         h = KNI_GetParameterAsInt(4);
    int         w = KNI_GetParameterAsInt(3);
    int         y = KNI_GetParameterAsInt(2);
    int         x = KNI_GetParameterAsInt(1);

    /*
     * @note { Spec verify step: "If either width or height
     * is less than zero, nothing is drawn." }
     */
    if ((w >= 0) && (h >= 0)) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(thisObject);
        
        KNI_GetThisPointer(thisObject);

        if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            TRANSLATE(thisObject, x, y);

            GET_CLIP(thisObject, clip);

            gx_draw_roundrect(GET_PIXEL(thisObject),
                              clip,
                              GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                              GET_LINESTYLE(thisObject),
                              x, y, w, h, arcWidth, arcHeight);
        }
        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

/**
 * Fills the specified rectangle, with rounded corners, using
 * the current color and stroke style. According to the MIDP
 * specification, if either the <tt>width</tt> or <tt>height</tt>
 * are negative, no rectangle is drawn.
 * <p>
 * Java declaration:
 * <pre>
 *     fillRoundRect(IIIIII)V
 * </pre>
 *
 * @param x The x coordinate of the rectangle to be drawn
 * @param y The y coordinate of the rectangle to be drawn
 * @param width The width of the rectangle to be drawn
 * @param height The height of the rectangle to be drawn
 * @param arcWidth The horizontal diameter of the arc at the four corners
 * @param arcHeight The vertical diameter of the arc at the four corners
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_fillRoundRect) {
    int arcHeight = KNI_GetParameterAsInt(6);
    int  arcWidth = KNI_GetParameterAsInt(5);
    int         h = KNI_GetParameterAsInt(4);
    int         w = KNI_GetParameterAsInt(3);
    int         y = KNI_GetParameterAsInt(2);
    int         x = KNI_GetParameterAsInt(1);

    /*
     * @note { Spec verify step: "If either width or height
     * is zero or less, nothing is drawn." }
     */
    if ((w >= 0) && (h >= 0)) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(thisObject);
        
        KNI_GetThisPointer(thisObject);

        if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            TRANSLATE(thisObject, x, y);

            GET_CLIP(thisObject, clip);

            gx_fill_roundrect(GET_PIXEL(thisObject),
                              clip,
                              GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                              GET_LINESTYLE(thisObject),
                              x, y, w, h, arcWidth, arcHeight);
        }

        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

/**
 * Draws the outline of the specified circular or elliptical arc
 * segment using the current color and stroke style. According
 * to the MIDP specification, if either the <tt>width</tt> or
 * <tt>height</tt> are negative, no arc is drawn.
 * <p>
 * Java declaration:
 * <pre>
 *     drawArc(IIIIII)V
 * </pre>
 *
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
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawArc) {
    int   arcAngle = KNI_GetParameterAsInt(6);
    int startAngle = KNI_GetParameterAsInt(5);
    int          h = KNI_GetParameterAsInt(4);
    int          w = KNI_GetParameterAsInt(3);
    int          y = KNI_GetParameterAsInt(2);
    int          x = KNI_GetParameterAsInt(1);

    /*
     * @note { Spec verify step: "If either width or height
     * is less than zero, nothing is drawn." }
     */
    if ((w >= 0) && (h >= 0)) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(thisObject);
        
        KNI_GetThisPointer(thisObject);

        if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {        
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            TRANSLATE(thisObject, x, y);
        
#ifdef PLATFORM_SUPPORT_CCW_ARC_ONLY
            /* this block transfer any negative number of
             * start angle or arc angle to positive and
             * always counter-clockwise.
             *
             * Optimization: This whole block can skip if 
             * native platform support negative arc input.
             */
            if (arcAngle < 0) {
                startAngle += arcAngle;
                arcAngle = -arcAngle;
            }
            startAngle = (startAngle + 360) % 360;
#endif

            GET_CLIP(thisObject, clip);

            gx_draw_arc(GET_PIXEL(thisObject),
                        clip,
                        GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                        GET_LINESTYLE(thisObject),
                        x, y, w, h, startAngle, arcAngle);
        }

        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

/**
 * Fills the specified circular or elliptical arc segment using the
 * current color and stroke style. According to the MIDP specification,
 * if either the <tt>width</tt> or <tt>height</tt> are negative, no
 * arc is drawn.
 * <p>
 * Java declaration:
 * <pre>
 *     fillArc(IIIIII)V
 * </pre>
 *
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
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_fillArc) {
    int   arcAngle = KNI_GetParameterAsInt(6);
    int startAngle = KNI_GetParameterAsInt(5);
    int          h = KNI_GetParameterAsInt(4);
    int          w = KNI_GetParameterAsInt(3);
    int          y = KNI_GetParameterAsInt(2);
    int          x = KNI_GetParameterAsInt(1);

    /*
     * @note { Spec verify step: "If either width or height
     * is less than zero, nothing is drawn." }
     */
    if ((w >= 0) && (h >= 0)) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(thisObject);
        
        KNI_GetThisPointer(thisObject);

        if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            TRANSLATE(thisObject, x, y);
        
#ifdef PLATFORM_SUPPORT_CCW_ARC_ONLY
            /* Please see above for explanation */
            if (arcAngle < 0) {
                startAngle += arcAngle;
                arcAngle = -arcAngle;
            }
            startAngle = (startAngle + 360) % 360;
#endif

            GET_CLIP(thisObject, clip);

            gx_fill_arc(GET_PIXEL(thisObject),
                        clip,
                        GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                        GET_LINESTYLE(thisObject),
                        x, y, w, h, startAngle, arcAngle);
        }

        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

/**
 * Fills the specified triangle using the current color and stroke
 * style.
 * <p>
 * Java declaration:
 * <pre>
 *     fillTriangle(IIIIII)V
 * </pre>
 *
 * @param x1 The x coordinate of the first vertices
 * @param y1 The y coordinate of the first vertices
 * @param x2 The x coordinate of the second vertices
 * @param y2 The y coordinate of the second vertices
 * @param x3 The x coordinate of the third vertices
 * @param y3 The y coordinate of the third vertices
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_fillTriangle) {
    int         y3 = KNI_GetParameterAsInt(6);
    int         x3 = KNI_GetParameterAsInt(5);
    int         y2 = KNI_GetParameterAsInt(4);
    int         x2 = KNI_GetParameterAsInt(3);
    int         y1 = KNI_GetParameterAsInt(2);
    int         x1 = KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);

    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

        TRANSLATE(thisObject, x1, y1);
        TRANSLATE(thisObject, x2, y2);
        TRANSLATE(thisObject, x3, y3);
      
        GET_CLIP(thisObject, clip);

        gx_fill_triangle(GET_PIXEL(thisObject),
                         clip,
                         GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                         GET_LINESTYLE(thisObject),
                         x1, y1, x2, y2, x3, y3);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws the specified <tt>String</tt> using the current font and color.
 * <p>
 * Java declaration:
 * <pre>
 *     drawString(Ljava/lang/String;III)V
 * </pre>
 *
 * @param str The <tt>String</tt> to be drawn
 * @param x The x coordinate of the anchor point
 * @param y The y coordinate of the anchor point
 * @param anchor The anchor point for positioning the text
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawString) {
    int anchor = KNI_GetParameterAsInt(4);
    int      y = KNI_GetParameterAsInt(3);
    int      x = KNI_GetParameterAsInt(2);
    int strLen;

    KNI_StartHandles(3);

    KNI_DeclareHandle(str);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(font);

    KNI_GetParameterAsObject(1, str);
    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        strLen = KNI_GetStringLength(str);
        if (strLen < 0) {
            KNI_ThrowNew(midpNullPointerException, NULL);
        } else if (!check_anchor(anchor, VCENTER)) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else {
            int      face, style, size;
            _JavaString *jstr;
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            GET_FONT(thisObject, font);
        
            DECLARE_FONT_PARAMS(font);
        
            TRANSLATE(thisObject, x, y);
        
            jstr = GET_STRING_PTR(str);
        
            GET_CLIP(thisObject, clip);

            gx_draw_chars(GET_PIXEL(thisObject),
                          clip,
                          GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                          GET_LINESTYLE(thisObject),
                          face, style, size, x, y, anchor, 
                          jstr->value->elements + jstr->offset,
                          strLen);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws a portion of the specified <tt>String</tt> using the
 * current font and color.
 * <p>
 * Java declaration:
 * <pre>
 *     drawSubstring(Ljava/lang/String;III)V
 * </pre>
 *
 * @param str The <tt>String</tt> to be drawn
 * @param offset Zero-based index of first character in the substring
 * @param length Length of the substring
 * @param x The x coordinate of the anchor point
 * @param y The y coordinate of the anchor point
 * @param anchor The anchor point for positioning the text
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawSubstring) {
    int anchor = KNI_GetParameterAsInt(6);
    int      y = KNI_GetParameterAsInt(5);
    int      x = KNI_GetParameterAsInt(4);
    int length = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int strLen;

    KNI_StartHandles(3);

    KNI_DeclareHandle(str);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(font);

    KNI_GetParameterAsObject(1, str);
    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        strLen = KNI_GetStringLength(str);
        if (strLen < 0) {
            KNI_ThrowNew(midpNullPointerException, NULL);
        } else if (   (offset < 0) 
                      || (offset > strLen) 
                      || (length < 0)
                      || (length > strLen)
                      || ((offset + length) < 0)
                      || ((offset + length) > strLen)) {
            KNI_ThrowNew(midpStringIndexOutOfBoundsException, NULL);
        } else if (!check_anchor(anchor, VCENTER)) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else if (length != 0) {
            int      face, style, size;
            _JavaString *jstr;

            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */
        
            GET_FONT(thisObject, font);
        
            DECLARE_FONT_PARAMS(font);
        
            TRANSLATE(thisObject, x, y);
        
            jstr = GET_STRING_PTR(str);
        
            GET_CLIP(thisObject, clip);

            gx_draw_chars(GET_PIXEL(thisObject),
                          clip,
                          GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                          GET_LINESTYLE(thisObject),
                          face, style, size, x, y, anchor,
                          jstr->value->elements + (jstr->offset + offset),
                          length);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws the specified character using the current font and color.
 * <p>
 * Java declaration:
 * <pre>
 *     drawChar(CIII)V
 * </pre>
 *
 * @param ch The character to be drawn
 * @param x The x coordinate of the anchor point
 * @param y The y coordinate of the anchor point
 * @param anchor The anchor point for positioning the text
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawChar) {
    int anchor = KNI_GetParameterAsInt(4);
    int      y = KNI_GetParameterAsInt(3);
    int      x = KNI_GetParameterAsInt(2);
    jchar  c = KNI_GetParameterAsShort(1);

    KNI_StartHandles(2);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(font);

    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        if (!check_anchor(anchor, VCENTER)) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else {
            int      face, style, size;
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            GET_FONT(thisObject, font);

            DECLARE_FONT_PARAMS(font);

            TRANSLATE(thisObject, x, y);

            GET_CLIP(thisObject, clip);

            gx_draw_chars(GET_PIXEL(thisObject),
                          clip,
                          GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                          GET_LINESTYLE(thisObject),
                          face, style, size, x, y, anchor, &c, 1);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws the specified characters using the current font and color.
 * <p>
 * Java declaration:
 * <pre>
 *     drawChars([CIIIII)V
 * </pre>
 *
 * @param data The array of characters to be drawn
 * @param offset Zero-based index of first character to be drawn
 * @param length Number of characters to be drawn
 * @param x The x coordinate of the anchor point
 * @param y The y coordinate of the anchor point
 * @param anchor The anchor point for positioning the text
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawChars) {
    int anchor = KNI_GetParameterAsInt(6);
    int      y = KNI_GetParameterAsInt(5);
    int      x = KNI_GetParameterAsInt(4);
    int length = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int chLen;

    KNI_StartHandles(3);

    KNI_DeclareHandle(ch);
    KNI_DeclareHandle(thisObject);
    KNI_DeclareHandle(font);

    KNI_GetParameterAsObject(1, ch);
    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        chLen = KNI_GetArrayLength(ch);
        if (chLen < 0) {
            KNI_ThrowNew(midpNullPointerException, NULL);
        } else if (   (offset < 0) 
                      || (offset > chLen) 
                      || (length < 0)
                      || (length > chLen)
                      || ((offset + length) < 0)
                      || ((offset + length) > chLen)) {
            KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
        } else if (!check_anchor(anchor, VCENTER)) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else if (length != 0) {
            int      face, style, size;
            jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

            GET_FONT(thisObject, font);
        
            DECLARE_FONT_PARAMS(font);
        
            TRANSLATE(thisObject, x, y);
        
            GET_CLIP(thisObject, clip);
      
            gx_draw_chars(GET_PIXEL(thisObject),
                          clip,
                          GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                          GET_LINESTYLE(thisObject),
                          face, style, size, x, y, anchor,
                          &(JavaCharArray(ch)[offset]),
                          length);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws the specified pixels from the given data array. The array
 * consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     drawRGB([IIIIIIIZ)V
 * </pre>
 *
 * @param rgbData The array of argb pixels to draw
 * @param offset Zero-based index of first argb pixel to be drawn
 * @param scanlen Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          region to draw
 * @param y The y coordinate of the upper left corner of the
 *          region to draw
 * @param width The width of the target region
 * @param height The height of the target region
 * @param processAlpha If <tt>true</tt>, alpha channel bytes will
 *                     be used, otherwise, alpha channel bytes will
 *                     be ignored
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_drawRGB) {
    jboolean processAlpha = KNI_GetParameterAsBoolean(8);
    jint height = KNI_GetParameterAsInt(7);
    jint width = KNI_GetParameterAsInt(6);
    jint y = KNI_GetParameterAsInt(5);
    jint x = KNI_GetParameterAsInt(4);
    jint scanlen = KNI_GetParameterAsInt(3);
    jint offset = KNI_GetParameterAsInt(2);
    jint buflen;
    jint *rgbBuffer;
    long min, max, l_scanlen, l_height, l_tmpexp;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {
        if (KNI_IsNullHandle(rgbData)) {
            KNI_ThrowNew(midpNullPointerException, NULL);
        } else {
	
            buflen = KNI_GetArrayLength(rgbData);
	
            /* According to the spec., this function can be
             * defined as operation P(a,b) = rgbData[ offset +
             * (a-x) + (b-y)* scanlength] where x <= a < x + width
             * AND y <= b < y + height.
             *
             * We do not need to check every index value and its
             * corresponding array access violation. We only need
             * to check for the min/max case. Detail explanation
             * can be found in the design doc.
             *
             * - To translate "<" to "<=", we minus one from height
             * and width (the ceiling operation), for all cases
             * except when height or width is zero.
             * - To avoid overflow (or underflow), we cast the
             * variables scanlen and height to long first */

            l_scanlen = (long) scanlen;
            l_height  = (long) height - 1;
            l_tmpexp  = (height == 0) ? 0 : l_height * l_scanlen ;
	
            /* Find the max/min of the index for rgbData array */
            max = offset + ((width==0) ? 0 : (width-1)) 
                + ((scanlen<0) ? 0 : l_tmpexp);
            min = offset + ((scanlen<0) ? l_tmpexp : 0);
	
            if ((max >= buflen) || (min < 0) || (max < 0) || (min >= buflen)) {
                KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
            } else {
	  
                if ((0 == scanlen || 
                     0 == width   || 
                     0 == height)) {
	    
                    /* Valid values, but nothing to render. */
	    
                } else {

                    jshort clip[4]; /* Defined in Graphics.java as 4 shorts */
            
                    rgbBuffer = JavaIntArray(rgbData);
            
                    TRANSLATE(thisObject, x, y);

                    GET_CLIP(thisObject, clip);
            
                    gx_draw_rgb(clip,
                        GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject), 
                        rgbBuffer, offset, scanlen, x, y, width, 
                        height, processAlpha);
                }

            }
        }
    }
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Copies the specified region of the current <tt>Graphics</tt> object
 * to the specified destination within the same <tt>Graphics</tt> object.
 * <p>
 * Java declaration:
 * <pre>
 *     doCopyArea(IIIIII)V
 * </pre>
 *
 * @param x_src The x coordinate of the upper-left corner of the
 *              source region
 * @param y_src The y coordinate of the upper-left corner of the
 *              source region
 * @param width The width of the source region
 * @param height The height of the source region
 * @param x_dest The x coordinate of the upper-left corner of the
 *               destination region
 * @param y_dest The y coordinate of the upper-left corner of the
 *               destination region
 * @param anchor The anchor point for positioning the copied region
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Graphics_doCopyArea) {
    int anchor = KNI_GetParameterAsInt(7);
    int y_dest = KNI_GetParameterAsInt(6); 
    int x_dest = KNI_GetParameterAsInt(5);
    int height = KNI_GetParameterAsInt(4);
    int width  = KNI_GetParameterAsInt(3);
    int y_src  = KNI_GetParameterAsInt(2);
    int x_src  = KNI_GetParameterAsInt(1);
    jshort gfx_width = 0;
    jshort gfx_height = 0;

    KNI_StartHandles(1); 
    KNI_DeclareHandle(thisObject);

    KNI_GetThisPointer(thisObject);

    if (GRAPHICS_OP_IS_ALLOWED(thisObject)) {

        gfx_width  = (int)GXAPI_GET_GRAPHICS_PTR(thisObject)->maxWidth;
        gfx_height = (int)GXAPI_GET_GRAPHICS_PTR(thisObject)->maxHeight;

        TRANSLATE(thisObject, x_src, y_src); 
      
        if((height < 0) || (width < 0) || (x_src < 0) || (y_src < 0) ||
           ((x_src + width) > gfx_width) || ((y_src + height) > gfx_height)) {
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else {
	
            TRANSLATE(thisObject, x_dest, y_dest);
	
            if (normalize_anchor(&x_dest, &y_dest, width, height, anchor)) {
                jshort clip[4]; /* Defined in Graphics.java as 4 shorts */
                GET_CLIP(thisObject, clip);

                gx_copy_area(clip,
                             GET_IMAGEDATA_PTR_FROM_GRAPHICS(thisObject),
                             x_src, y_src, width, height, 
                             x_dest, y_dest);
            }
        }
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets a specific pixel value.
 * <p>
 * Java declaration:
 * <pre>
 *     getPixel(IIZ)I
 * </pre>
 *
 * @param rgb compact rgb representation
 * @param gray gray scale
 * @param isGray use gray scale
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Graphics_getPixel) {
    int isGray = KNI_GetParameterAsBoolean(3);
    int   gray = KNI_GetParameterAsInt(2);
    int    rgb = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(gx_get_pixel(rgb, gray, isGray));
}

/**
 * Maps the specified RGB value to the actual RGB value displayed
 * on device.
 * <p>
 * Java declaration:
 * <pre>
 *     getDisplayColor(I)I
 * </pre>
 *
 * @param color The RGB value to get the display mapping
 * @return The RGB value used to display this color on device
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Graphics_getDisplayColor) {
    int color = KNI_GetParameterAsInt(1);
    KNI_ReturnInt(gx_get_displaycolor(color));
}

/**
 * Draws the specified image by using the anchor point.
 * The image can be drawn in different positions relative to
 * the anchor point by passing the appropriate position constants.
 * See <a href="#anchor">anchor points</a>.
 *
 * <p>If the source image contains transparent pixels, the corresponding
 * pixels in the destination image must be left untouched.  If the source
 * image contains partially transparent pixels, a compositing operation
 * must be performed with the destination pixels, leaving all pixels of
 * the destination image fully opaque.</p>
 *
 * <p>If <code>img</code> is the same as the destination of the Graphics
 * object, the result is undefined.  For copying areas within an
 * <code>Image</code>, {@link #copyArea copyArea} should be used instead.
 * </p>
 *
 * @param img the specified Image to be drawn
 * @param x the x coordinate of the anchor point
 * @param y the y coordinate of the anchor point
 * @param anchor the anchor point for positioning the image
 * @throws IllegalArgumentException if <code>anchor</code>
 * is not a legal value
 * @throws NullPointerException if <code>g</code> is <code>null</code>
 * @see Image
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Graphics_render) {
    jboolean success = KNI_TRUE;

    int anchor = KNI_GetParameterAsInt(4);
    int y      = KNI_GetParameterAsInt(3);
    int x      = KNI_GetParameterAsInt(2);

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(g);
    KNI_DeclareHandle(gImg);

    KNI_GetParameterAsObject(1, img);
    KNI_GetThisPointer(g);

    if (GRAPHICS_OP_IS_ALLOWED(g)) {
        /* null checking is handled by the Java layer, but test just in case */
        if (KNI_IsNullHandle(img)) {
            success = KNI_FALSE; //KNI_ThrowNew(midpNullPointerException, NULL);
        } else {
  	    const java_imagedata * srcImageDataPtr =
	      IMGAPI_GET_IMAGE_PTR(img)->imageData;

            IMGAPI_GET_IMAGE_PTR(gImg) =
	      (struct Java_javax_microedition_lcdui_Image *)
	      (GXAPI_GET_GRAPHICS_PTR(g)->img);
            if (KNI_IsSameObject(gImg, img) || !check_anchor(anchor,0)) {
                success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
            } else if (!normalize_anchor(&x, &y, srcImageDataPtr->width, 
					       srcImageDataPtr->height, 
					       anchor)) {
                success = KNI_FALSE;//KNI_ThrowNew(midpIllegalArgumentException, NULL);
            } else {
	        jshort clip[4]; /* Defined in Graphics.java as 4 shorts */
	        const java_imagedata * dstMutableImageDataPtr = 
		  GET_IMAGEDATA_PTR_FROM_GRAPHICS(g);

                TRANSLATE(g, x, y);
		GET_CLIP(g, clip);

		gx_render_image(srcImageDataPtr, dstMutableImageDataPtr,
				clip, x, y);

            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(success);
}

/**
 * Renders the given region of the given  <tt>Image</tt> onto the
 * this <tt>Graphics</tt> object.
 * <p>
 * Java declaration:
 * <pre>
 *     renderRegion(Ljavax/microedition/lcdui/Image;IIIIIIII)V
 * </pre>
 *
 * @param img The <tt>Image</tt> object to be drawn
 * @param x_src The x coordinate of the upper-left corner of the
 *              source region
 * @param y_src The y coordinate of the upper-left corner of the
 *              source region
 * @param width The width of the source region
 * @param height The height of the source region
 * @param transform The transform to apply to the selected region.
 * @param x_dest The x coordinate of the destination anchor point
 * @param y_dest The y coordinate of the destination anchor point
 * @param anchor The anchor point for positioning the destination
 *               <tt>Image</tt>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Graphics_renderRegion) {
    int anchor    = KNI_GetParameterAsInt(9);
    int y_dest    = KNI_GetParameterAsInt(8);
    int x_dest    = KNI_GetParameterAsInt(7);
    int transform = KNI_GetParameterAsInt(6);
    int height    = KNI_GetParameterAsInt(5);
    int width     = KNI_GetParameterAsInt(4);
    int y_src     = KNI_GetParameterAsInt(3);
    int x_src     = KNI_GetParameterAsInt(2);
    jboolean success = KNI_TRUE;

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(g);
    KNI_DeclareHandle(gImg);

    KNI_GetParameterAsObject(1, img);
    KNI_GetThisPointer(g);
    
    if (GRAPHICS_OP_IS_ALLOWED(g)) {
      if (KNI_IsNullHandle(img)) {
        /* null checking is performed in the Java code, but check just in case */
        success = KNI_FALSE; //KNI_ThrowNew(midpNullPointerException, NULL);
      } else if ((transform < 0) || (transform > 7)) {
        success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
      } else if (!normalize_anchor(&x_dest, &y_dest, width, height, anchor)) {
        success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
      } else {
	const java_imagedata * srcImageDataPtr = 
	  IMGAPI_GET_IMAGE_PTR(img)->imageData;
        jint img_width = srcImageDataPtr->width;
        jint img_height = srcImageDataPtr->height;

        IMGAPI_GET_IMAGE_PTR(gImg) = 
	  (struct Java_javax_microedition_lcdui_Image *)
	                      (GXAPI_GET_GRAPHICS_PTR(g)->img);
        if (KNI_IsSameObject(gImg, img) || 
           (height < 0) || (width < 0) || (x_src < 0) || (y_src < 0) ||
           ((x_src + width) > img_width) || 
           ((y_src + height) > img_height)) {
          success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else {
	  jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

	  const java_imagedata * dstMutableImageDataPtr = 
	    GET_IMAGEDATA_PTR_FROM_GRAPHICS(g);

	  TRANSLATE(g, x_dest, y_dest);
	  GET_CLIP(g, clip);

	  gx_render_imageregion(srcImageDataPtr, dstMutableImageDataPtr,
				clip, 
				x_src, y_src, 
				width, height,
				x_dest, y_dest, 
				transform);
        }
      }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(success);
}
