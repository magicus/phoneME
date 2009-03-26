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
gx_fill_triangle(jint color, const jshort *clip, 
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

/* 
 * For A in [0..0xffff] 
 *
 *        A / 255 == A / 256 + ((A / 256) + (A % 256) + 1) / 256
 *
 */
#define div(x)  (((x) >> 8) + ((((x) >> 8) + ((x) & 0xff) + 1) >> 8))

static unsigned short alphaComposition(jint src, 
                                       unsigned short dst, 
                                       unsigned char As) {
  unsigned char Rs = (unsigned char)(src >> 16);
  unsigned char Rd = (unsigned char)
    ((((dst & 0xF800) << 5) | (dst & 0xE000)) >> 13);
  int pRr = ((int)Rs - Rd) * As + Rd * 0xff;
  unsigned char Rr = 
    (unsigned char)( div(pRr) );

  unsigned char Gs = (unsigned char)(src >> 8);
  unsigned char Gd = (unsigned char)
    (((dst & 0x07E0) >> 3) | ((dst & 0x0600) >> 9));
  int pGr = ((int)Gs - Gd) * As + Gd * 0xff;
  unsigned char Gr = 
    (unsigned char)( div(pGr) );

  unsigned char Bs = (unsigned char)(src);
  unsigned char Bd = (unsigned char)
    ((dst & 0x001F) << 3) | ((dst & 0x001C) >> 2);
  int pBr = ((int)Bs - Bd) * As + Bd * 0xff;
  unsigned char Br = 
    (unsigned char)( div(pBr) );

  /* compose RGB from separate color components */
  return ((Rr & 0xF8) << 8) + ((Gr & 0xFC) << 3) + (Br >> 3);
}


#if (UNDER_CE)
extern void asm_draw_rgb(jint* src, int srcSpan, unsigned short* dst,
    int dstSpan, int width, int height);
#endif

#define SRC_PIXEL_TO_DEST_WITH_ALPHA(pSrc, pDest) \
        src = *pSrc++;  \
        As = src >> 26; \
        if (As == 0x3F) {   \
            *pDest = GXJ_RGB24TORGB16(src);  \
        } else if (As != 0) {   \
            *pDest = alphaComposition(src, *pDest, (unsigned char)As);   \
        }   \
        pDest++

#define SRC_PIXEL_TO_DEST(pSrc, pDest) \
        src = *pSrc++;  \
        *pDest = GXJ_RGB24TORGB16(src); \
        pDest++

/** Draw image in RGB format */
void
gx_draw_rgb(const jshort *clip,
	     const java_imagedata *dst, jint *rgbData,
             jint offset, jint scanlen, jint x, jint y,
             jint width, jint height, jboolean processAlpha) {
    int diff;

    gxj_screen_buffer screen_buffer;
    gxj_screen_buffer* sbuf = (gxj_screen_buffer*) getScreenBuffer(
      gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL));
    const int sbufWidth = sbuf->width;

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

#if USE_SLOW_LOOPS
    {
      gxj_pixel_type * pdst = &sbuf->pixelData[y * sbufWidth + x];
      jint * psrc = &rgbData[offset];
      int pdst_delta = sbufWidth - width;
      int psrc_delta = scanlen - width;
      gxj_pixel_type * pdst_end = pdst + height * sbufWidth;

      if (pdst_delta < 0 || psrc_delta < 0) {
        return;
      }

      if (!processAlpha) {
	do {
	  gxj_pixel_type * pdst_stop = pdst + width;
	  
	  do {
	    jint src = *psrc++;

	    CHECK_PTR_CLIP(sbuf, pdst);

	    *pdst = GXJ_RGB24TORGB16(src);
	  } while (++pdst < pdst_stop);

	  psrc += psrc_delta;
	  pdst += pdst_delta;
	} while (pdst < pdst_end);
      } else {
	do {
	  gxj_pixel_type * pdst_stop = pdst + width;
	
	  do {
	    jint src = *psrc++;
	    unsigned char As = (unsigned char)(src >> 24);

	    CHECK_PTR_CLIP(sbuf, pdst);

	    *pdst = alphaComposition(src, *pdst, As);
	  } while (++pdst < pdst_stop);

	  psrc += psrc_delta;
	  pdst += pdst_delta;
	} while (pdst < pdst_end);
      }
    }
#else
    {
        const unsigned int width16 = width & 0xFFFFFFF0;
        const unsigned int widthRemain = width & 0xF;
        unsigned int col;
        
        gxj_pixel_type * pdst = &sbuf->pixelData[y * sbufWidth + x];
        jint * psrc = &rgbData[offset];
        int pdst_delta = sbufWidth - width;
        int psrc_delta = scanlen - width;
        gxj_pixel_type * pdst_end = pdst + height * sbufWidth;
        unsigned int  src;
        unsigned char As;

        if (pdst_delta < 0 || psrc_delta < 0) {
            return;
        }

        if (processAlpha) {
            do {
                for (col = width16; col != 0; col -= 16) {
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                }
                
                for (col = widthRemain; col != 0; col--) {
                    SRC_PIXEL_TO_DEST_WITH_ALPHA(psrc, pdst);
                }
                
                psrc += psrc_delta;
                pdst += pdst_delta;
            } while (pdst < pdst_end);
        } else {
            do {
                for (col = width16; col != 0; col -= 16) {
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                    SRC_PIXEL_TO_DEST(psrc, pdst);
                }
                
                for (col = widthRemain; col != 0; col--) {
                    SRC_PIXEL_TO_DEST(psrc,pdst);
                }

                psrc += psrc_delta;
                pdst += pdst_delta;
            } while (pdst < pdst_end);
        }
    }
#endif
}

/**
 * Obtain the color that will be final shown 
 * on the screen after the system processed it.
 */
jint
gx_get_displaycolor(jint color) {
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
gx_draw_line(jint color, const jshort *clip, 
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
gx_draw_rect(jint color, const jshort *clip, 
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
gx_fill_rect(jint color, const jshort *clip, 
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
gx_draw_roundrect(jint color, const jshort *clip, 
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
gx_fill_roundrect(jint color, const jshort *clip, 
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
gx_draw_arc(jint color, const jshort *clip, 
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
gx_fill_arc(jint color, const jshort *clip, 
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
jint
gx_get_pixel(jint rgb, int gray, int isGray) {

    REPORT_CALL_TRACE3(LC_LOWUI, "gx_getPixel(%x, %x, %d)\n",
            rgb, gray, isGray);

    /* Surpress unused parameter warnings */
    (void)gray;
    (void)isGray;

    return rgb;
}
