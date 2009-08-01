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
#include "gxj_putpixel.h"

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
#if ENABLE_DYNAMIC_PIXEL_FORMAT
        if (pp_enable_32bit_mode) {
   	    sbuf->pixelData = (gxj_pixel_type *)img->nativePixelData32;
        } else {
   	    sbuf->pixelData = (gxj_pixel_type *)img->nativePixelData16;
        }
#else
	sbuf->pixelData = (gxj_pixel_type *)img->nativePixelData;
#endif
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
  gxj_pixel_type pixelColor;
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
      pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
      pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

  REPORT_CALL_TRACE(LC_LOWUI, "gx_fill_triangle()\n");

  /* Surpress unused parameter warnings */
  (void)dotted;

  fill_triangle(sbuf, pixelColor, clip, x1, y1, x2, y2, x3, y3);
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

#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
/* return value of src OVER dst, where:
 *     src is an 8888 ARGB pixel in an int (LCDUI format)
 *     dst is an 8888 RGBA or ABGR pixel in an int (OpenGL ES format or inverted OpenGL ES format)
 *     OVER is the Source Over Destination composite rule
 * Note, we cannot assume that the destination alpha coming
 * into this function is 1.0, as LCDUI normally assumes for
 * a drawing target, since we are accumulating partial frame
 * results for compositing to the screen through OpenGL ES.
 * We will maintain the putpixel destination buffer in
 * premultiplied alpha format, since this will eliminate
 * 3 divides in the computation, and OpenGL ES allows us
 * to use premultiplied pixel values in a texture.
 * The incoming src pixels will not be in premultiplied
 * format, so we must premultiply them if the alpha value
 * is not 1.0.
 * Reference: 
 * http://graphics.stanford.edu/courses/cs248-01/comp/comp.html
 * In particular:
 * http://graphics.stanford.edu/courses/cs248-01/comp/comp.html#The compositing algebra
 * http://graphics.stanford.edu/courses/cs248-01/comp/comp.html#Premultiplied alpha
 */
static jint alphaComposition32(jint src, jint dst) { 

    int Rs, Bs, Gs, As, Rd, Bd, Gd, Ad, ONE_MINUS_As;

    /* Note src values are not premultiplied */
    Bs = src & 0xff;
    src = src >> 8;
    Gs = src & 0xff;
    src = src >> 8;
    Rs = src & 0xff;
    As = (src >> 8) & 0xff;

    if (As == 0xff) {
        /* src is opaque, so no blend and no premultiplication are necessary */
#if ENABLE_RGBA8888_PIXEL_FORMAT
        return((Rs << 24) | (Gs << 16) | (Bs << 8) | 0xff);
#elif ENABLE_ABGR8888_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
        return(0xff000000 | (Bs << 16) | (Gs << 8) | Rs);
#endif
    }

    /* Note dst vales are already premultiplied */
#if ENABLE_RGBA8888_PIXEL_FORMAT
    Ad = dst & 0xff;
    dst = dst >> 8;
    Bd = dst & 0xff;
    dst = dst >> 8;
    Gd = dst & 0xff;
    Rd = (dst >> 8) & 0xff;
#elif ENABLE_ABGR8888_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
    Rd = dst & 0xff;
    dst = dst >> 8;
    Gd = dst & 0xff;
    dst = dst >> 8;
    Bd = dst & 0xff;
    Ad = (dst >> 8) & 0xff;
#endif

    ONE_MINUS_As = 0xff - As;
    Rd = Rs * As + Rd * ONE_MINUS_As; /* premultiply Rs and blend with Rd */
    Rd = div(Rd);                     /* scale back to 0-255 */
    Gd = Gs * As + Gd * ONE_MINUS_As;
    Gd = div(Gd);
    Bd = Bs * As + Bd * ONE_MINUS_As;
    Bd = div(Bd);
    Ad = As * 0xff + Ad * ONE_MINUS_As; /* must scale As appropriately */
    Ad = div(Ad);

#if ENABLE_RGBA8888_PIXEL_FORMAT
    return((Rd << 24) | (Gd << 16) | (Bd << 8) | Ad);
#elif ENABLE_ABGR8888_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
    return((Ad << 24) | (Bd << 16) | (Gd << 8) | Rd);
#endif
}
#endif

#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
static unsigned short alphaComposition16(jint src, 
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
#endif

#if (UNDER_CE)
extern void asm_draw_rgb(jint* src, int srcSpan, unsigned short* dst,
    int dstSpan, int width, int height);
#endif

#if ENABLE_DYNAMIC_PIXEL_FORMAT

#define SRC_PIXEL_TO_DEST_WITH_ALPHA_32(pSrc, pDest) \
        src = *pSrc++;  \
        As = src >> 24; \
        if (As == 0xFF) {   \
            *pDest = GXJ_MIDPTOOPAQUEPIXEL_32(src);  \
        } else if (As != 0) {   \
            *pDest = alphaComposition32(src, *pDest);   \
        }   \
        pDest++

#define SRC_PIXEL_TO_DEST_WITH_ALPHA_16(pSrc, pDest) \
        src = *pSrc++;  \
        As = src >> 24; \
        if (As == 0xFF) {   \
            *pDest = GXJ_MIDPTOOPAQUEPIXEL_16(src);  \
        } else if (As != 0) {   \
            *pDest = alphaComposition16(src, *pDest, (unsigned char)As);   \
        }   \
        pDest++

#define SRC_PIXEL_TO_DEST_32(pSrc, pDest) \
        src = *pSrc++;  \
        *pDest = GXJ_MIDPTOOPAQUEPIXEL_32(src); \
        pDest++

#define SRC_PIXEL_TO_DEST_16(pSrc, pDest) \
        src = *pSrc++;  \
        *pDest = GXJ_MIDPTOOPAQUEPIXEL_16(src); \
        pDest++

#elif ENABLE_32BITS_PIXEL_FORMAT

#define SRC_PIXEL_TO_DEST_WITH_ALPHA(pSrc, pDest) \
        src = *pSrc++;  \
        As = src >> 24; \
        if (As == 0xFF) {   \
            *pDest = GXJ_MIDPTOOPAQUEPIXEL(src);  \
        } else if (As != 0) {   \
            *pDest = alphaComposition32(src, *pDest);   \
        }   \
        pDest++

#define SRC_PIXEL_TO_DEST_WITH_ALPHA_32(pSrc, pDest) SRC_PIXEL_TO_DEST_WITH_ALPHA(pSrc, pDest)
#define SRC_PIXEL_TO_DEST_32(pSrc, pDest) SRC_PIXEL_TO_DEST(pSrc, pDest)

#else

#define SRC_PIXEL_TO_DEST_WITH_ALPHA(pSrc, pDest) \
        src = *pSrc++;  \
        As = src >> 24; \
        if (As == 0xFF) {   \
            *pDest = GXJ_MIDPTOOPAQUEPIXEL(src);  \
        } else if (As != 0) {   \
            *pDest = alphaComposition16(src, *pDest, (unsigned char)As);   \
        }   \
        pDest++

#define SRC_PIXEL_TO_DEST_WITH_ALPHA_16(pSrc, pDest) SRC_PIXEL_TO_DEST_WITH_ALPHA(pSrc, pDest)
#define SRC_PIXEL_TO_DEST_16(pSrc, pDest) SRC_PIXEL_TO_DEST(pSrc, pDest)

#endif


#if !ENABLE_DYNAMIC_PIXEL_FORMAT
#define SRC_PIXEL_TO_DEST(pSrc, pDest) \
        src = *pSrc++;  \
        *pDest = GXJ_MIDPTOOPAQUEPIXEL(src); \
        pDest++
#endif

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
#warning: slow loops are not adopted for dynamic pixel format
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

	    *pdst = GXJ_MIDPTOOPAQUEPIXEL(src);

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

#if ENABLE_32BITS_PIXEL_FORMAT
	    *pdst = alphaComposition32(src, *pdst);
#else
	    *pdst = alphaComposition16(src, *pdst, As);
#endif
	  } while (++pdst < pdst_stop);

	  psrc += psrc_delta;
	  pdst += pdst_delta;
	} while (pdst < pdst_end);
      }
    }
#else /* USE_SLOW_LOOPS */
    {
        const unsigned int width16 = width & 0xFFFFFFF0;
        const unsigned int widthRemain = width & 0xF;
        unsigned int col;
        
        jint * psrc = &rgbData[offset];
        int pdst_delta = sbufWidth - width;
        int psrc_delta = scanlen - width;
        unsigned int  src;
        unsigned char As;

        if (pdst_delta < 0 || psrc_delta < 0) {
            return;
        }

        if (processAlpha) {
#if ENABLE_DYNAMIC_PIXEL_FORMAT
          if (pp_enable_32bit_mode) {
#endif
#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
            gxj_pixel32_type * pdst = &sbuf->pixelData[y * sbufWidth + x];
            gxj_pixel32_type * pdst_end = pdst + height * sbufWidth;

            do {
                for (col = width16; col != 0; col -= 16) {
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                }
                
                for (col = widthRemain; col != 0; col--) {
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_32(psrc, pdst);
                }
                
                psrc += psrc_delta;
                pdst += pdst_delta;
            } while (pdst < pdst_end);
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
          } else {
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
            gxj_pixel16_type * pdst = &((gxj_pixel16_type*)sbuf->pixelData)[y * sbufWidth + x];
            gxj_pixel16_type * pdst_end = pdst + height * sbufWidth;

            do {
                for (col = width16; col != 0; col -= 16) {
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                }
                
                for (col = widthRemain; col != 0; col--) {
                    SRC_PIXEL_TO_DEST_WITH_ALPHA_16(psrc, pdst);
                }
                
                psrc += psrc_delta;
                pdst += pdst_delta;
            } while (pdst < pdst_end);
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
          }
#endif
        } else { /* processAlpha */
#if ENABLE_DYNAMIC_PIXEL_FORMAT
          if (pp_enable_32bit_mode) {
#endif
#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
            gxj_pixel32_type * pdst = &sbuf->pixelData[y * sbufWidth + x];
            gxj_pixel32_type * pdst_end = pdst + height * sbufWidth;

            do {
                for (col = width16; col != 0; col -= 16) {
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                    SRC_PIXEL_TO_DEST_32(psrc, pdst);
                }
                
                for (col = widthRemain; col != 0; col--) {
                    SRC_PIXEL_TO_DEST_32(psrc,pdst);
                }

                psrc += psrc_delta;
                pdst += pdst_delta;
            } while (pdst < pdst_end);
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
          } else {
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
            gxj_pixel16_type * pdst = &((gxj_pixel16_type*)sbuf->pixelData)[y * sbufWidth + x];
            gxj_pixel16_type * pdst_end = pdst + height * sbufWidth;

            do {
                for (col = width16; col != 0; col -= 16) {
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                    SRC_PIXEL_TO_DEST_16(psrc, pdst);
                }
                
                for (col = widthRemain; col != 0; col--) {
                    SRC_PIXEL_TO_DEST_16(psrc,pdst);
                }

                psrc += psrc_delta;
                pdst += pdst_delta;
            } while (pdst < pdst_end);
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
          }
#endif
        } /* processAlpha */
    }
#endif /* USE_SLOW_LOOPS */
}

/**
 * Obtain the color that will be final shown 
 * on the screen after the system processed it.
 */
jint
gx_get_displaycolor(jint color) {
#if ENABLE_DYNAMIC_PIXEL_FORMAT
    int newColor;
    if (pp_enable_32bit_mode) {
        newColor = color;
    } else {
        newColor = GXJ_PIXELTOOPAQUEMIDP_16(GXJ_MIDPTOOPAQUEPIXEL_16(color));
    }
#elif ENABLE_32BITS_PIXEL_FORMAT
    int newColor = color;
#else
    int newColor = GXJ_PIXELTOOPAQUEMIDP(GXJ_MIDPTOOPAQUEPIXEL(color));
#endif

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
  gxj_pixel_type pixelColor;
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif
  
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
  gxj_pixel_type pixelColor;
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

  REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_rect()\n");

  draw_roundrect(pixelColor, clip, sbuf, lineStyle, x,  y, 
		 width, height, 0, 0, 0);
}


#if (UNDER_ADS || UNDER_CE) || (defined(__GNUC__) && defined(ARM))

#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
extern void fast_pixel_set_32(unsigned * mem, unsigned value, int number_of_pixels);
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
extern void fast_pixel_set_16(unsigned * mem, unsigned value, int number_of_pixels);
#endif

#else
#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
void fast_pixel_set_32(unsigned * mem, unsigned value, int number_of_pixels)
{
   int i;
   gxj_pixel_type* pBuf = (gxj_pixel_type*)mem;

   for (i = 0; i < number_of_pixels; ++i) {
      *(pBuf + i) = (gxj_pixel_type)value;
   }
}
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
void fast_pixel_set_16(unsigned * mem, unsigned value, int number_of_pixels)
{
   int i;
   gxj_pixel16_type* pBuf = (gxj_pixel16_type*)mem;

   for (i = 0; i < number_of_pixels; ++i) {
      *(pBuf + i) = (gxj_pixel16_type)value;
   }
}
#endif

#endif

void fastFill_rect(gxj_pixel_type color, gxj_screen_buffer *sbuf, int x, int y, int width, int height, int cliptop, int clipbottom) {
	int screen_horiz=sbuf->width;

        if (width<=0) {return;}
	if (x > screen_horiz) { return; }
	if (y > sbuf->height) { return; }
	if (x < 0) { width+=x; x=0; }
	if (y < cliptop) { height+=y-cliptop; y=cliptop; }
	if (x+width  > screen_horiz) { width=screen_horiz - x; }
	if (y+height > clipbottom) { height= clipbottom - y; }


        {
#if ENABLE_DYNAMIC_PIXEL_FORMAT
        if (pp_enable_32bit_mode) {
#endif
#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
  	    gxj_pixel_type* raster=sbuf->pixelData + y*screen_horiz+x;

	    for(;height>0;height--) {
		    fast_pixel_set_32((unsigned *)raster, color, width);
		    raster+=screen_horiz;
	    }
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
        } else {
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
  	    gxj_pixel16_type* raster=((gxj_pixel16_type*)sbuf->pixelData) + y*screen_horiz+x;

	    for(;height>0;height--) {
		    fast_pixel_set_16((unsigned *)raster, color, width);
		    raster+=screen_horiz;
	    }
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
        }
#endif
        }
}

/**
 * Fill a rectangle at (x,y) with the given width and height.
 */
void 
gx_fill_rect(jint color, const jshort *clip, 
	      const java_imagedata *dst, int dotted, 
              int x, int y, int width, int height) {

  gxj_screen_buffer screen_buffer;
  gxj_pixel_type pixelColor;
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

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
  gxj_pixel_type pixelColor;
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

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
  gxj_pixel_type pixelColor;
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

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
  gxj_pixel_type pixelColor;
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

  REPORT_CALL_TRACE(LC_LOWUI, "gx_draw_arc()\n");

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
  gxj_pixel_type pixelColor;
  int lineStyle = (dotted?DOTTED:SOLID);
  gxj_screen_buffer screen_buffer;
  gxj_screen_buffer *sbuf = 
      gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
  sbuf = (gxj_screen_buffer *)getScreenBuffer(sbuf);

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_32(color);
  } else {
    pixelColor = GXJ_MIDPTOOPAQUEPIXEL_16(color);
  }
#else
  pixelColor = GXJ_MIDPTOOPAQUEPIXEL(color);
#endif

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
