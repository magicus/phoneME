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

/**
 * \file
 * Immutable image functions that needed to be implemented for each port.
 */
#include <kni.h>
#include <string.h>

#include <midpMalloc.h>
#include <midp_logging.h>

#include <gxapi_constants.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_image.h"
#include "gxj_intern_putpixel.h"

static void clipped_blit(gxj_screen_buffer* dst, int dstX, int dstY,
			 gxj_screen_buffer* src, const jshort *clip);

extern void unclipped_blit(unsigned short *dstRaster, int dstSpan,
			   unsigned short *srcRaster, int srcSpan,
			   int height, int width, gxj_screen_buffer *dst);

#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
/* 
 * For A in [0..0xffff] 
 *
 *        A / 255 == A / 256 + ((A / 256) + (A % 256) + 1) / 256
 *
 */
#define div(x)  (((x) >> 8) + ((((x) >> 8) + ((x) & 0xff) + 1) >> 8))

/* return value of src OVER dst, where:
 *     src and dst are an 8888 RGBA or ABGR pixel in an int (OpenGL ES format or inverted OpenGL ES format)
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
static jint alphaComposition(jint src, jint dst) { 

    int Rs, Bs, Gs, As, Rd, Bd, Gd, Ad, ONE_MINUS_As;

    /* Note src values are not premultiplied */
#if ENABLE_RGBA8888_PIXEL_FORMAT
    As = src & 0xff;
    if (As == 0xff) {
        /* src is opaque, so no blend and no premultiplication are necessary */
        return src;
    }
    src = src >> 8;
    Bs = src & 0xff;
    src = src >> 8;
    Gs = src & 0xff;
    Rs = (src >> 8) & 0xff;
#elif ENABLE_ABGR8888_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
    Rs = src & 0xff;
    src = src >> 8;
    Gs = src & 0xff;
    src = src >> 8;
    Bs = src & 0xff;
    As = (src >> 8) & 0xff;
    if (As == 0xff) {
        /* src is opaque, so no blend and no premultiplication are necessary */
        return src;
    }
#endif

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
#endif /* ENABLE_32BITS_PIXEL_FORMAT

/**
 * Renders the contents of the specified mutable image
 * onto the destination specified.
 *
 * @param srcImagePtr         pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to the clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 */
void
draw_image(gxj_screen_buffer *imageSBuf,
	     gxj_screen_buffer *gSBuf,
	     const jshort *clip,
	     jint x_dest, jint y_dest) {
  gxj_screen_buffer *destSBuf = getScreenBuffer(gSBuf);
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];

  REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:MutableImage_render_Image()\n");

  CHECK_SBUF_CLIP_BOUNDS(destSBuf, clip);

  if (imageSBuf->alphaData == NULL) {
    if (x_dest >= clipX1 && y_dest >= clipY1 &&
       (x_dest + imageSBuf->width) <= clipX2 &&
       (y_dest + imageSBuf->height) <= clipY2) {
#if ENABLE_DYNAMIC_PIXEL_FORMAT
      if (pp_enable_32bit_mode) {
          unclipped_blit((unsigned short *)&destSBuf->pixelData[y_dest*destSBuf->width+x_dest],
    		        destSBuf->width<<2,
		        (unsigned short *)&imageSBuf->pixelData[0], imageSBuf->width<<2,
		        imageSBuf->height, imageSBuf->width<<2, destSBuf);

      } else {
          unclipped_blit((unsigned short *)&((gxj_pixel16_type*)(destSBuf->pixelData))[y_dest*destSBuf->width+x_dest],
		        destSBuf->width<<1,
		        (unsigned short *)&imageSBuf->pixelData[0], imageSBuf->width<<1,
		        imageSBuf->height, imageSBuf->width<<1, destSBuf);

      }
#elif ENABLE_32BITS_PIXEL_FORMAT
      unclipped_blit((unsigned short *)&destSBuf->pixelData[y_dest*destSBuf->width+x_dest],
		    destSBuf->width<<2,
		    (unsigned short *)&imageSBuf->pixelData[0], imageSBuf->width<<2,
		    imageSBuf->height, imageSBuf->width<<2, destSBuf);
#else
      unclipped_blit(&destSBuf->pixelData[y_dest*destSBuf->width+x_dest],
		    destSBuf->width<<1,
		    &imageSBuf->pixelData[0], imageSBuf->width<<1,
		    imageSBuf->height, imageSBuf->width<<1, destSBuf);
#endif

    } else {
      clipped_blit(destSBuf, x_dest, y_dest, imageSBuf, clip);
    }
  } else {
    copy_imageregion(imageSBuf, destSBuf,
		     clip, x_dest, y_dest,
		     imageSBuf->width, imageSBuf->height,
		     0, 0, 0);
  }
}

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param srcImagePtr         pointer to source image
 * @param graphicsDestination pointer to destination graphics object
 * @param clip                pointer to the clip
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 */
void
draw_imageregion(gxj_screen_buffer *imageSBuf,
		 gxj_screen_buffer *gSBuf,
		 const jshort *clip,
		 jint x_dest, jint y_dest,
		 jint width, jint height,
		 jint x_src, jint y_src,
		 jint transform) {
  gxj_screen_buffer *dstSBuf = getScreenBuffer(gSBuf);

  REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:MutableImage_render_imageRegion()\n");

  CHECK_SBUF_CLIP_BOUNDS(dstSBuf, clip);

  copy_imageregion(imageSBuf, dstSBuf,
                  clip, x_dest, y_dest, width, height, x_src, y_src, transform);
}


static void
clipped_blit(gxj_screen_buffer* dst, int dstX, int dstY, gxj_screen_buffer* src, const jshort *clip) {
  int width, height;        /* computed width and height */
  int startX; int startY;   /* x,y into the dstRaster */
  int negY, negX;           /* x,y into the srcRaster */
  int diff;
  gxj_pixel_type* srcRaster;
  gxj_pixel_type* dstRaster;
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];

  if ((dstX >= clipX2) || (dstY >= clipY2))
      return;

  if (dstX < 0) {
    startX = 0;
    negX   = -dstX;
  } else {
    startX = dstX;
    negX   = 0;
  }
  if (dstY < 0) {
    startY = 0;
    negY   = -dstY;
  } else {
    startY = dstY;
    negY   = 0;
  }

  width = src->width - negX;
  /* clip left edge */
  if ((diff=clipX1-startX) > 0) {
    negX   += diff;
    width  -= diff;
    startX  = clipX1;
  }
  /* clip right edge */
  if ((diff=clipX2-startX) < width) {
    width = diff;
  }
  if (width <= 0)
    return;

  height = src->height - negY;
  /* clip top edge */
  if ((diff=clipY1-startY) > 0) {
    negY   += diff;
    height -= diff;
    startY  = clipY1;
   }
  /* clip bottom edge */
  if ((diff=clipY2-startY) < height) {
    height = diff;
  }
  if (height <= 0)
    return;

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
#endif
#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
    srcRaster = src->pixelData + (negY ? (negY   * src->width) : 0) + negX;
    dstRaster = dst->pixelData +         (startY * dst->width)      + startX;

    unclipped_blit((unsigned short *)dstRaster, dst->width<<2,
                   (unsigned short *)srcRaster, src->width<<2, height, width<<2, dst);
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
  } else {
    srcRaster = (gxj_pixel_type*)(((gxj_pixel16_type*)src->pixelData) + (negY ? (negY   * src->width) : 0) + negX);
    dstRaster = (gxj_pixel_type*)(((gxj_pixel16_type*)dst->pixelData) +         (startY * dst->width)      + startX);
#else
    srcRaster = src->pixelData + (negY ? (negY   * src->width) : 0) + negX;
    dstRaster = dst->pixelData +         (startY * dst->width)      + startX;
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT

    unclipped_blit((unsigned short *)dstRaster, dst->width<<1,
                   (unsigned short *)srcRaster, src->width<<1, height, width<<1, dst);
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
  }
#endif
}



/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param src                 pointer to source screen buffer
 * @param dest                pointer to destination screen buffer
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param transform           transform to be applied to the region
 */
void
create_transformed_imageregion(gxj_screen_buffer* src, gxj_screen_buffer* dest, jint src_x, jint src_y,
                             jint width, jint height, jint transform) {
  int srcX;
  int srcY;
  int xStart;
  int yStart;
  int xIncr;
  int yIncr;
  int destX;
  int destY;
  int yCounter;
  int xCounter;
  int srcIdx, dstIdx;
  int srcYwidth, destYwidth;

  /* set dimensions of image being created,
     depending on transform */
  if (transform & TRANSFORM_INVERTED_AXES) {
    dest->width  = height;
    dest->height = width;
  } else {
    dest->width  = width;
    dest->height = height;
  }

  if (transform & TRANSFORM_Y_FLIP) {
    yStart = height-1;
    yIncr = -1;
  } else {
    yStart = 0;
    yIncr = +1;
  }

  if (transform & TRANSFORM_X_FLIP) {
    xStart = width-1;
    xIncr = -1;
  } else {
    xStart = 0;
    xIncr = +1;
  }

#if ENABLE_DYNAMIC_PIXEL_FORMAT
  if (pp_enable_32bit_mode) {
#endif
  for (srcY = src_y, destY = yStart, yCounter = 0;
       yCounter < height;
       srcY++, destY+=yIncr, yCounter++) {

    srcYwidth = srcY * src->width;
    destYwidth = destY * dest->width;

    for (srcX = src_x, destX = xStart, xCounter = 0;
         xCounter < width;
         srcX++, destX+=xIncr, xCounter++) {

      if ( transform & TRANSFORM_INVERTED_AXES ) {
        dstIdx = destX * dest->width + destY;
        srcIdx = srcYwidth /*srcY*src->width*/ + srcX;

        dest->pixelData[dstIdx] = src->pixelData[srcIdx];

        if (src->alphaData != NULL) {
            dest->alphaData[dstIdx] = src->alphaData[srcIdx];
        }
      } else {
        dstIdx = destYwidth /*destY * dest->width*/ + destX;
        srcIdx = srcYwidth /*srcY*src->width*/ + srcX;

        dest->pixelData[dstIdx] = src->pixelData[srcIdx];
        if (src->alphaData != NULL) {
            dest->alphaData[dstIdx] = src->alphaData[srcIdx];
        }
      }
    } /*for x*/
  } /* for y */
#if ENABLE_DYNAMIC_PIXEL_FORMAT
  } else {
#endif
  for (srcY = src_y, destY = yStart, yCounter = 0;
       yCounter < height;
       srcY++, destY+=yIncr, yCounter++) {

    srcYwidth = srcY * src->width;
    destYwidth = destY * dest->width;

    for (srcX = src_x, destX = xStart, xCounter = 0;
         xCounter < width;
         srcX++, destX+=xIncr, xCounter++) {

      if ( transform & TRANSFORM_INVERTED_AXES ) {
        dstIdx = destX * dest->width + destY;
        srcIdx = srcYwidth /*srcY*src->width*/ + srcX;

        ((gxj_pixel16_type*)dest->pixelData)[dstIdx] =
            ((gxj_pixel16_type*)src->pixelData)[srcIdx];

        if (src->alphaData != NULL) {
            ((gxj_pixel16_type*)dest->alphaData)[dstIdx] =
                ((gxj_pixel16_type*)src->alphaData)[srcIdx];
        }
      } else {
        dstIdx = destYwidth /*destY * dest->width*/ + destX;
        srcIdx = srcYwidth /*srcY*src->width*/ + srcX;

        ((gxj_pixel16_type*)dest->pixelData)[dstIdx] =
            ((gxj_pixel16_type*)src->pixelData)[srcIdx];
        if (src->alphaData != NULL) {
            ((gxj_pixel16_type*)dest->alphaData)[dstIdx] =
                ((gxj_pixel16_type*)src->alphaData)[srcIdx];
        }
      }
    } /*for x*/
  } /* for y */
#if ENABLE_DYNAMIC_PIXEL_FORMAT
  }
#endif
}

/**
 * Renders the contents of the specified region of this
 * mutable image onto the destination specified.
 *
 * @param src                 pointer to source screen buffer
 * @param dest                pointer to destination screen buffer
 * @param clip                pointer to structure holding the dest clip
 *                              [x, y, width, height]
 * @param x_dest              x-coordinate in the destination
 * @param y_dest              y-coordinate in the destination
 * @param width               width of the region
 * @param height              height of the region
 * @param x_src               x-coord of the region
 * @param y_src               y-coord of the region
 * @param transform           transform to be applied to the region
 */
void
copy_imageregion(gxj_screen_buffer* src, gxj_screen_buffer* dest, const jshort *clip,
		jint x_dest, jint y_dest,
                jint width, jint height, jint x_src, jint y_src,
                jint transform) {
    int clipX1 = clip[0];
    int clipY1 = clip[1];
    int clipX2 = clip[2];
    int clipY2 = clip[3];
    int diff;
    gxj_screen_buffer newSrc;

    /*
     * Don't let a bad clip origin into the clip code or the may be
     * over or under writes of the destination buffer.
     */
    if (clipX1 < 0) {
        clipX1 = 0;
    }

    if (clipY1 < 0) {
        clipY1 = 0;
    }

    diff = clipX2 - dest->width;
    if (diff > 0) {
        clipX2 -= diff;
    }

    diff = clipY2 - dest->height;
    if (diff > 0) {
        clipY2 -= diff;
    }

    if (clipX1 >= clipX2 || clipY1 >= clipY2) {
        /* Nothing to do. */
        return;
    }

    /*
     * Don't let any bad source numbers into the transform or copy,
     * clip any pixels outside of the source buffer to prevent over or
     * under reading the source buffer.
     */
    if (x_src < 0) {
        width += x_src;
        x_src = 0;
    }

    if (y_src < 0) {
        height += y_src;
        y_src = 0;
    }

    diff = (x_src + width) - src->width;
    if (diff > 0) {
        width -= diff;
    }

    diff = (y_src + height) - src->height;
    if (diff > 0) {
        height -= diff;
    }

    if (width <= 0 || height <= 0) {
        /* Nothing to do. */
        return;
    }

    /*
     * check if the source and destination are the same image,
     * or a transform is needed
     */
    newSrc.pixelData = NULL;
    newSrc.alphaData = NULL;
    if (dest == src || transform != 0) {
        int pixelSize, imageSize;

        pixelSize = width * height;

        /*
         * create a new image that is a copy of the region with transform
         * applied
         */
#if ENABLE_DYNAMIC_PIXEL_FORMAT
        if (pp_enable_32bit_mode) {
            imageSize = pixelSize * sizeof (gxj_pixel32_type);
        } else {
            imageSize = pixelSize * sizeof (gxj_pixel16_type);
        }
#elif ENABLE_32BITS_PIXEL_FORMAT
        imageSize = pixelSize * sizeof (gxj_pixel32_type);
#else 
        imageSize = pixelSize * sizeof (gxj_pixel16_type);
#endif

        newSrc.pixelData =
            (gxj_pixel_type *)midpMalloc(imageSize);
        if (newSrc.pixelData == NULL) {
            REPORT_ERROR(LC_LOWUI, "Out of memory error, copyImageRegion (pixelData)\n"); 
            return ; 
        }
        if (src->alphaData != NULL) {
            newSrc.alphaData =
                (gxj_alpha_type *)midpMalloc(pixelSize * sizeof (gxj_alpha_type));
            if (newSrc.alphaData == NULL) {
                midpFree(newSrc.pixelData);
                REPORT_ERROR(LC_LOWUI, "Out of memory error, copyImageRegion (Alpha)\n");
                return ;
            }
        }
        
        create_transformed_imageregion(src, &newSrc, x_src, y_src, width,
				       height, transform);
        
        /* set the new image as the source */
        src = &newSrc;
        x_src = 0;
        y_src = 0;

        if (transform & TRANSFORM_INVERTED_AXES) {
            // exchange the width and height
            width = src->width;
            height = src->height;
        }
    }

    /* Apply the clip region to the destination region */
    diff = clipX1 - x_dest;
    if (diff > 0) {
        x_src += diff;
        width -= diff;
        x_dest = clipX1;
    }

    diff = clipY1 - y_dest;
    if (diff > 0) {
        y_src += diff;
        height -= diff;
        y_dest = clipY1;
    }

    diff = (x_dest + width) - clipX2;
    if (diff > 0) {
        width -= diff;
    }

    diff = (y_dest + height) - clipY2;
    if (diff > 0) {
        height -= diff;
    }

    if (width > 0) {
        int rowsCopied;
        int destWidthDiff = dest->width - width;
        int srcWidthDiff = src->width - width;
        int r1, g1, b1, a2, a3, r2, b2, g2;

#if ENABLE_DYNAMIC_PIXEL_FORMAT
    if (pp_enable_32bit_mode) {
#endif
#if ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
        gxj_pixel32_type* pDest = dest->pixelData + (y_dest * dest->width) + x_dest;
        gxj_pixel32_type* pSrc = src->pixelData + (y_src * src->width) + x_src;
        gxj_pixel32_type* limit;

        if (src->alphaData != NULL) {
            unsigned char *pSrcAlpha = src->alphaData + (y_src * src->width) + x_src;

            /* copy the source to the destination */
            for (rowsCopied = 0; rowsCopied < height; rowsCopied++) {
                for (limit = pDest + width; pDest < limit; pDest++, pSrc++, pSrcAlpha++) {
                    if ((*pSrcAlpha) == 0xFF) {
                        CHECK_PTR_CLIP(dest, pDest);
                        *pDest = *pSrc;
                    }
                    else if (*pSrcAlpha > 0x3) {
                        *pDest = alphaComposition(*pSrc, *pDest);
                    }
                }

                pDest += destWidthDiff;
                pSrc += srcWidthDiff;
                pSrcAlpha += srcWidthDiff;
            }
        } else {
            /* copy the source to the destination */
            for (rowsCopied = 0; rowsCopied < height; rowsCopied++) {
                for (limit = pDest + width; pDest < limit; pDest++, pSrc++) {
                    CHECK_PTR_CLIP(dest, pDest);
                    *pDest = *pSrc;
                }

                pDest += destWidthDiff;
                pSrc += srcWidthDiff;
            }
        }
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
    } else {
#endif
#if !ENABLE_32BITS_PIXEL_FORMAT || ENABLE_DYNAMIC_PIXEL_FORMAT
        gxj_pixel16_type* pDest = (gxj_pixel16_type*)dest->pixelData + (y_dest * dest->width) + x_dest;
        gxj_pixel16_type* pSrc = (gxj_pixel16_type*)src->pixelData + (y_src * src->width) + x_src;
        gxj_pixel16_type* limit;

        if (src->alphaData != NULL) {
            unsigned char *pSrcAlpha = src->alphaData + (y_src * src->width) + x_src;

            /* copy the source to the destination */
            for (rowsCopied = 0; rowsCopied < height; rowsCopied++) {
                for (limit = pDest + width; pDest < limit; pDest++, pSrc++, pSrcAlpha++) {
                    if ((*pSrcAlpha) == 0xFF) {
                        CHECK_PTR_CLIP(dest, pDest);
                        *pDest = *pSrc;
                    }
                    else if (*pSrcAlpha > 0x3) {
                        r1 = (*pSrc >> 11);
                        g1 = ((*pSrc >> 5) & 0x3F);
                        b1 = (*pSrc & 0x1F);

                        r2 = (*pDest >> 11);
                        g2 = ((*pDest >> 5) & 0x3F);
                        b2 = (*pDest & 0x1F);

                        a2 = *pSrcAlpha >> 2;
                        a3 = *pSrcAlpha >> 3;

                        r1 = (r1 * a3 + r2 * (31 - a3)) >> 5;
                        g1 = (g1 * a2 + g2 * (63 - a2)) >> 6;
                        b1 = (b1 * a3 + b2 * (31 - a3)) >> 5;

                        *pDest = (gxj_pixel16_type)((r1 << 11) | (g1 << 5) | (b1));
                    }
                }

                pDest += destWidthDiff;
                pSrc += srcWidthDiff;
                pSrcAlpha += srcWidthDiff;
            }
        } else {
            /* copy the source to the destination */
            for (rowsCopied = 0; rowsCopied < height; rowsCopied++) {
                for (limit = pDest + width; pDest < limit; pDest++, pSrc++) {
                    CHECK_PTR_CLIP(dest, pDest);
                    *pDest = *pSrc;
                }

                pDest += destWidthDiff;
                pSrc += srcWidthDiff;
            }
        }
#endif
#if ENABLE_DYNAMIC_PIXEL_FORMAT
    }
#endif

    }

    if (newSrc.pixelData != NULL) {
        midpFree(newSrc.pixelData);
    }

    if (newSrc.alphaData != NULL) {
        midpFree(newSrc.alphaData);
    }
}


/**
 * Draws the specified image at the given coordinates.
 *
 * <p>If the source image contains transparent pixels, the corresponding
 * pixels in the destination image must be left untouched.  If the source
 * image contains partially transparent pixels, a compositing operation
 * must be performed with the destination pixels, leaving all pixels of
 * the destination image fully opaque.</p>
 *
 * @param srcImageDataPtr the source image to be rendered
 * @param dstMutableImageDataPtr the mutable target image to be rendered to
 * @param clip the clip of the target image
 * @param x the x coordinate of the anchor point
 * @param y the y coordinate of the anchor point
 */
void gx_render_image(const java_imagedata * srcImageDataPtr,
		      const java_imagedata * dstMutableImageDataPtr,
		      const jshort * clip,
		      jint x, jint y) {
  gxj_screen_buffer srcSBuf;
  gxj_screen_buffer dstSBuf;

  gxj_screen_buffer * psrcSBuf =
    gxj_get_image_screen_buffer_impl(srcImageDataPtr, &srcSBuf, NULL);
  gxj_screen_buffer * pdstSBuf =
    getScreenBuffer(gxj_get_image_screen_buffer_impl(dstMutableImageDataPtr, &dstSBuf, NULL));

  draw_image(psrcSBuf, pdstSBuf, clip, x, y);
}

/**
 * Renders the given region of the source image onto the destination image
 * at the given coordinates.
 *
 * @param srcImageDataPtr the source image to be rendered
 * @param dstMutableImageDataPtr the mutable destination image to be rendered to
 * @param x_src The x coordinate of the upper-left corner of the
 *              source region
 * @param y_src The y coordinate of the upper-left corner of the
 *              source region
 * @param width The width of the source region
 * @param height The height of the source region
 * @param x_dest The x coordinate of the upper-left corner of the destination region
 * @param y_dest The y coordinate of the upper-left corner of the destination region
 * @param transform The transform to apply to the selected region.
 */
extern void gx_render_imageregion(const java_imagedata * srcImageDataPtr,
				  const java_imagedata * dstMutableImageDataPtr,
				  const jshort * clip,
				  jint x_src, jint y_src,
				  jint width, jint height,
				   jint x_dest, jint y_dest,
				   jint transform) {
    gxj_screen_buffer srcSBuf;
    gxj_screen_buffer dstSBuf;

    gxj_screen_buffer * psrcSBuf =
      gxj_get_image_screen_buffer_impl(srcImageDataPtr, &srcSBuf, NULL);
    gxj_screen_buffer * pdstSBuf =
      getScreenBuffer(gxj_get_image_screen_buffer_impl(dstMutableImageDataPtr,
						       &dstSBuf, NULL));

    draw_imageregion(psrcSBuf, pdstSBuf,
		     clip,
		     x_dest, y_dest,
		     width, height,
		     x_src, y_src,
		     transform);
}
