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

#include <kni.h>
#include <midp_logging.h>

#include <gx_graphics.h>
#include <gxapi_constants.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"
#include "gxj_intern_image.h"

#if ENABLE_BOUNDS_CHECKS
#include <gxapi_graphics.h>
#endif

/**
 * @file
 *
 * putpixel primitive graphics. 
 */

/**
 * Create native representation for a image.
 *
 * @param jimg Java Image ROM structure to convert from
 * @param sbuf pointer to Screen buffer structure to populate
 * @param g optional Graphics object for debugging clip code.
 *	    give NULL if don't care.
 *
 * @return the given 'sbuf' pointer for convenient usage,
 *	   or NULL if the image is null.
 */
gxj_screen_buffer* gxj_get_image_screen_buffer_impl(const java_imagedata *img,
						    gxj_screen_buffer *sbuf,
						    jobject graphics) {

    /* NOTE:
     * Since this routine is called by every graphics operations
     * We use ROMStruct directly instead of macros
     * like JavaByteArray, etc, for max performance.
     */
    if (img == NULL) {
	return NULL;
    }

    sbuf->width  = img->width;
    sbuf->height = img->height;

    /* Only use nativePixelData and nativeAlphaData if
     * pixelData is null */
    if (img->pixelData != NULL) {
	sbuf->pixelData = (gxj_pixel_type *)&(img->pixelData->elements[0]);
	sbuf->alphaData = (img->alphaData != NULL)
			    ? (gxj_alpha_type *)&(img->alphaData->elements[0])
			    : NULL;
    } else {
	sbuf->pixelData = (gxj_pixel_type *)img->nativePixelData;
	sbuf->alphaData = (gxj_alpha_type *)img->nativeAlphaData;
    }

#if ENABLE_BOUNDS_CHECKS
    sbuf->g = (graphics != NULL) ? GXAPI_GET_GRAPHICS_PTR(graphics) : NULL;
#else
    (void)graphics; /* Surpress unused parameter warning */
#endif

    return sbuf;
}


/**
 * Draw triangle
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

/**
 * Premultiply color components by it's corresponding alpha component.
 *
 * Formula: Cs = Csr * As (for source pixel),
 *          Cd = Cdr * Ad (analog for destination pixel).
 *
 * @param C one of the raw color components of the pixel (Csr or Cdr in the formula).
 * @param A the alpha component of the source pixel (As or Ad in the formula).
 * @return color component in premultiplied form.
 */
#define PREMULTUPLY_ALPHA(C, A) \
    (unsigned char)( ((int)(C)) * (A) / 0xff )

/**
 * The source is composited over the destination (Porter-Duff Source Over 
 * Destination rule).
 *
 * Formula: Cr = Cs + Cd*(1-As)
 *
 * Note: the result is always equal or less than 0xff, i.e. overflow is impossible.
 *
 * @param Cs a color component of the source pixel in premultiplied form
 * @param As the alpha component of the source pixel
 * @param Cd a color component of the destination pixel in premultiplied form
 * @return a color component of the result in premultiplied form
 */
#define ADD_PREMULTIPLIEDCOLORS_SRCOVER(Cs, As, Cd) \
    (unsigned char)( ((int)(Cs)) + ((int)(Cd)) * (0xff - (As)) / 0xff )

/**
 * Combine separate source and destination color components.
 *
 * Note: all backround pixels are treated as full opaque.
 *
 * @param Csr one of the raw color components of the source pixel
 * @param As the alpha component of the source pixel
 * @param Cdr one of the raw color components of the destination pixel
 * @return a color component of the result in premultiplied form
 */
#define ADD_COLORS(Csr, As, Cdr) \
    ADD_PREMULTIPLIEDCOLORS_SRCOVER( \
            PREMULTUPLY_ALPHA(Csr, As), \
            As, \
            PREMULTUPLY_ALPHA(Cdr, 0xff) )


/**
 * Combine source and destination colors to achieve blending and transparency
 * effects.
 *
 * @param src source pixel value in 32bit ARGB format.
 * @param dst destination pixel value in 32bit RGB format.
 * @return result pixel value in 32bit RGB format.
 */
static jint alphaComposition(jint src, jint dst) {
    unsigned char As = (unsigned char)(src >> 24);

    unsigned char Rr = ADD_COLORS(
            (unsigned char)(src >> 16), As, (unsigned char)(dst >> 16) );

    unsigned char Gr = ADD_COLORS(
            (unsigned char)(src >> 8), As, (unsigned char)(dst >> 8) );

    unsigned char Br = ADD_COLORS(
            (unsigned char)src, As, (unsigned char)dst );

    /* compose RGB from separate color components */
    return (((jint)Rr) << 16) | (((jint)Gr) << 8) | Br;
}


#if (UNDER_CE)
extern void asm_draw_rgb(jint* src, int srcSpan, unsigned short* dst,
    int dstSpan, int width, int height);
#endif

/** Draw image in RGB format */
void
gx_draw_rgb(const jshort *clip,
	     const java_imagedata *dst, jint *rgbData,
             jint offset, jint scanlen, jint x, jint y,
             jint width, jint height, jboolean processAlpha) {
    int a, b, diff;
    int dataRowIndex, sbufRowIndex;

    gxj_screen_buffer screen_buffer;
    gxj_screen_buffer* sbuf = (gxj_screen_buffer*) getScreenBuffer(
      gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL));
    int sbufWidth = sbuf->width;

    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_rgb()\n");

    diff = clipX1 - x;
    if (diff > 0) {
        width -= diff;
        offset += diff;
        x = clipX1;
    }
    if (x + width > clipX2) {
        width = clipX2 - x;
    }
    diff = clipY1 - y;
    if (diff > 0) {
        height -= diff;
        offset += diff * scanlen;
        y = clipY1;
    }
    if (y + height > clipY2) {
        height = clipY2 - y;
    }
    if (width <= 0 || height <= 0) {
        return;
    }

#if (UNDER_CE)
    if (!processAlpha) {
        asm_draw_rgb(rgbData + offset, scanlen - width,
            sbuf->pixelData + sbufWidth * y + x,
            sbufWidth - width, width, height);
        return;
    }
#endif

    CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);
    dataRowIndex = 0;
    sbufRowIndex = y * sbufWidth;

    for (b = y; b < y + height;
        b++, dataRowIndex += scanlen,
        sbufRowIndex += sbufWidth) {

        for (a = x; a < x + width; a++) {
            jint value = rgbData[offset + (a - x) + dataRowIndex];
            int idx = sbufRowIndex + a;

            CHECK_PTR_CLIP(sbuf, &(sbuf->pixelData[idx]));

            if (!processAlpha || (value & 0xff000000) == 0xff000000) {
                // Pixel has no alpha or no transparency
                sbuf->pixelData[idx] = GXJ_RGB24TORGB16(value);
            } else {
                if ((value & 0xff000000) != 0) {
                    jint background = GXJ_RGB16TORGB24(sbuf->pixelData[idx]);
                    jint composition = alphaComposition(value, background);
                    sbuf->pixelData[idx] = GXJ_RGB24TORGB16(composition);
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

  /* API of the draw_roundrect requests radius of the arc at the four */
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
