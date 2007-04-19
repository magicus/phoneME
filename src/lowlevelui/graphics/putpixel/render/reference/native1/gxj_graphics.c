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

#include <kni.h>
#include <midp_logging.h>

#include <gx_graphics.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"
#include "gxj_intern_image.h"

/**
 * @file
 *
 * putpixel primitive graphics. 
 */

/**
 * Draw triangle
 *
 * @param pixel The packed pixel value
 */
void
gx_fill_triangle(int color, const jshort *clip, 
		  const java_imagedata *dst, int dotted, 
                  int x1, int y1, int x2, int y2, int x3, int y3) {
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  REPORT_CALL_TRACE(LC_LOWUI, "gx_fill_triangle()\n");

  /* Surpress unused parameter warnings */
  (void)dotted;

  fill_triangle(sbuf, GXJ_RGB24TORGB16(color), 
		clip, x1, y1, x2, y2, x3, y3);
}

/**
 * Copy from a specify region to other region
 */
void
gx_copy_area(const jshort *clip, 
	      const java_imagedata *dst, int x_src, int y_src, 
              int width, int height, int x_dest, int y_dest) {
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  copy_imageregion(sbuf, sbuf, clip, x_dest, y_dest, width, height,
		   x_src, y_src, 0);
}

/** Draw image in RGB format */
void
gx_draw_rgb(const jshort *clip,
	     const java_imagedata *dst, jint *rgbData,
             jint offset, jint scanlen, jint x, jint y,
             jint width, jint height, jboolean processAlpha) {
    int a, b;
    int sbufWidth;

    int dataRowIndex = 0;
    int sbufRowIndex = 0;

    gxj_screen_buffer screen_buffer;
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];
    gxj_screen_buffer *sbuf =
        gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
    sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);
    sbufWidth = sbuf->width;

    REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_rgb()\n");

    CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);
    sbufRowIndex = y * sbufWidth;

    for (b = y; b < y + height;
        b++, dataRowIndex += scanlen,
        sbufRowIndex += sbufWidth) {

        if (b >= clipY2) return;
        if (b <  clipY1) continue;

        for (a = x; a < x + width; a++) {

            int value = rgbData[offset + (a - x) + dataRowIndex];
            int idx = sbufRowIndex + a;

            if (a >= clipX2) break;
            if (a < clipX1) {
                // JAVA_TRACE("drawRGB:A OutOfBounds %d   %d %d %d %d\n", idx,
                //     a, b, sbuf->width, sbuf->height);
            } else {
                CHECK_PTR_CLIP(sbuf, &(sbuf->pixelData[idx]));
                if (!processAlpha || ((value & 0xff000000) == 0xff000000)) {
                    sbuf->pixelData[idx] = GXJ_RGB24TORGB16(value);
                } else {
                    unsigned int xA =
                        GXJ_XAAA8888_FROM_ARGB8888((unsigned int)value);
                    unsigned int XAInv =
                        (unsigned int)(((unsigned int)(0xFFFFFFFF)) - xA);
                    sbuf->pixelData[idx] =
                        GXJ_RGB24TORGB16((xA & value) |
                            (XAInv & GXJ_RGB16TORGB24(sbuf->pixelData[idx])));
                }
            }
        } /* loop by rgb data columns */
    } /* loop by rgb data rows */
}



/**
 * Obtain the color that will be final shown 
 * on the screen after the system processed it.
 */
int
gx_get_displaycolor(int color) {
    int newColor = GXJ_RGB16TORGB24(GXJ_RGB24TORGB16(color));

    REPORT_CALL_TRACE1(LC_LOWUI, "gx_getDisplayColor(%d)\n", color);

    /*
     * JAVA_TRACE("color %x  -->  %x\n", color, newColor);
     */

    return newColor;
}


/**
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
void
gx_draw_line(int color, const jshort *clip, 
	      const java_imagedata *dst, int dotted, 
              int x1, int y1, int x2, int y2)
{
  int lineStyle = (dotted ? DOTTED : SOLID);
  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);
  
  REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_line()\n");
  
  draw_clipped_line(sbuf, pixelColor, lineStyle, clip, x1, y1, x2, y2);
}

/**
 * Draw a rectangle at (x,y) with the given width and height.
 *
 * @note x, y sure to be >=0
 *       since x,y is quan. to be positive (>=0), we don't
 *       need to test for special case anymore.
 */
void 
gx_draw_rect(int color, const jshort *clip, 
	      const java_imagedata *dst, int dotted, 
              int x, int y, int width, int height)
{

  int lineStyle = (dotted ? DOTTED : SOLID);
  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_rect()\n");

  draw_roundrect(pixelColor, clip, sbuf, lineStyle, x,  y, 
		 width, height, 0, 0, 0);
}


#if (UNDER_ADS || UNDER_CE) || (defined(__GNUC__) && defined(ARM))
extern void fast_pixel_set(unsigned * mem, unsigned value, int number_of_pixels);
#else
void fast_pixel_set(unsigned * mem, unsigned value, int number_of_pixels)
{
   int i;
   gxj_pixel_type* pBuf = (gxj_pixel_type*)mem;

   for (i = 0; i < number_of_pixels; ++i) {
      *(pBuf + i) = (gxj_pixel_type)value;
   }
}
#endif

void fastFill_rect(unsigned short color, gxj_screen_buffer *sbuf, int x, int y, int width, int height, int cliptop, int clipbottom) {
	int screen_horiz=sbuf->width;
	unsigned short* raster;

    if (width<=0) {return;}
	if (x > screen_horiz) { return; }
	if (y > sbuf->height) { return; }
	if (x < 0) { width+=x; x=0; }
	if (y < cliptop) { height+=y-cliptop; y=cliptop; }
	if (x+width  > screen_horiz) { width=screen_horiz - x; }
	if (y+height > clipbottom) { height= clipbottom - y; }


	raster=sbuf->pixelData + y*screen_horiz+x;
	for(;height>0;height--) {
		fast_pixel_set((unsigned *)raster, color, width);
		raster+=screen_horiz;
	}
}

/**
 * Fill a rectangle at (x,y) with the given width and height.
 */
void 
gx_fill_rect(int color, const jshort *clip, 
	      const java_imagedata *dst, int dotted, 
              int x, int y, int width, int height) {

  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);


  if ((clipX1==0)&&(clipX2==sbuf->width)&&(dotted!=DOTTED)) {
    fastFill_rect(pixelColor, sbuf, x, y, width, height, clipY1, clipY2 );
    return;
  }

  
  REPORT_CALL_TRACE(LC_LOWUI, "gx_fill_rect()\n");

  draw_roundrect(pixelColor, clip, sbuf, dotted?DOTTED:SOLID, 
		 x, y, width, height, 1, 0, 0);
}

/**
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void 
gx_draw_roundrect(int color, const jshort *clip, 
		   const java_imagedata *dst, int dotted, 
                   int x, int y, int width, int height,
                   int arcWidth, int arcHeight)
{
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_roundrect()\n");

  //API of the draw_roundrect requests radius of the arc at the four
  draw_roundrect(pixelColor, clip, sbuf, lineStyle, 
		 x, y, width, height,
		 0, arcWidth >> 1, arcHeight >> 1);
}

/**
 * Fill a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
void 
gx_fill_roundrect(int color, const jshort *clip, 
		   const java_imagedata *dst, int dotted, 
                   int x, int y, int width, int height,
                   int arcWidth, int arcHeight)
{
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  REPORT_CALL_TRACE(LC_LOWUI, "gx_fillround_rect()\n");

  draw_roundrect(pixelColor, clip, sbuf, lineStyle, 
		 x,  y,  width,  height,
		 1, arcWidth >> 1, arcHeight >> 1);
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
void 
gx_draw_arc(int color, const jshort *clip, 
	     const java_imagedata *dst, int dotted, 
             int x, int y, int width, int height,
             int startAngle, int arcAngle)
{
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  draw_arc(pixelColor, clip, sbuf, lineStyle, x, y, 
	   width, height, 0, startAngle, arcAngle);
}

/**
 * Fill an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
void 
gx_fill_arc(int color, const jshort *clip, 
	     const java_imagedata *dst, int dotted, 
             int x, int y, int width, int height,
             int startAngle, int arcAngle)
{
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_pixel_type pixelColor = GXJ_RGB24TORGB16(color);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = 
      gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

  REPORT_CALL_TRACE(LC_LOWUI, "gx_fill_arc()\n");

  draw_arc(pixelColor, clip, sbuf, lineStyle, 
	   x, y, width, height, 1, startAngle, arcAngle);
}

/**
 * Return the pixel value.
 */
int
gx_get_pixel(int rgb, int gray, int isGray) {

    REPORT_CALL_TRACE3(LC_LOWUI, "gx_getPixel(%x, %x, %d)\n",
            rgb, gray, isGray);

    /* Surpress unused parameter warnings */
    (void)gray;
    (void)isGray;

    return rgb;
}
