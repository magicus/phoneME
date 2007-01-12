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

/**
 * \file
 * Immutable image functions that needed to be implemented for each port.
 */
#include <kni.h>
#include <string.h>

#include <midpMalloc.h>
#include <midp_logging.h>

#include <gxutl_graphics.h>
#include <gx_image.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_image.h"
#include "gxj_intern_putpixel.h"
#include "gxj_intern_image_decode.h"

#if ENABLE_JPEG
#include <jpegdecoder.h>
#endif

#define CT_PALETTE  0x01
#define CT_COLOR    0x02
#define CT_ALPHA    0x04

typedef struct _imgDst {
  imageDstData   super;
  gxj_screen_buffer*           vdc;
  jboolean       hasAlpha;
  jboolean       hasColorMap;
  jboolean       hasTransMap;
  long           cmap[256];
  /*
   * IMPL NOTE: investigate if 'unsigned char' type could be used for storing
   * transparency map instead of 'long' type.
   */
  long           tmap[256]; 
} _imageDstData, *_imageDstPtr;

void initImageDst(_imageDstPtr dst);

static void clipped_blit(gxj_screen_buffer* dst, int dstX, int dstY,
			 gxj_screen_buffer* src, const jshort *clip);

extern void unclipped_blit(unsigned short *dstRaster, int dstSpan,
			   unsigned short *srcRaster, int srcSpan,
			   int height, int width, gxj_screen_buffer * dst);

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
      unclipped_blit(&destSBuf->pixelData[y_dest*destSBuf->width+x_dest],
		    destSBuf->width<<1,
		    &imageSBuf->pixelData[0], imageSBuf->width<<1,
		    imageSBuf->height, imageSBuf->width<<1,destSBuf);
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

/**
 * Decodes the given input data into a storage format used by immutable
 * images.  The input data should be a PNG image.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param creationErrorPtr pointer to the status of the decoding
 *         process. This function sets creationErrorPtr's value.
 */
int
decode_png
(unsigned char* srcBuffer, int length, gxj_screen_buffer *image,
 gxutl_native_image_error_codes* creationErrorPtr) {

    _imageDstData dstData;
    imageSrcPtr src = NULL;

    REPORT_CALL_TRACE(LC_LOWUI,
                     "LF:decode_PNG()\n");

    /* Create the image from the buffered data */
    initImageDst(&dstData);

    dstData.vdc = image;
    /* what about (image->pixelData == NULL &&
                   image->width > 0 &&
                   image->height > 0) ? */
    if ((src = create_imagesrc_from_data((char **)(void*)&srcBuffer,
                                             length)) == NULL) {
      *creationErrorPtr = GXUTL_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
    } else if (!decode_png_image(src, (imageDstData *)(&dstData))) {
      *creationErrorPtr = GXUTL_NATIVE_IMAGE_DECODING_ERROR;
    } else {
      *creationErrorPtr = GXUTL_NATIVE_IMAGE_NO_ERROR;

      /* dstData.hasAlpha == KNI_FALSE */
    }

    if(src != NULL) {
        midpFree(src);
    }

    return dstData.hasAlpha;
}

#if ENABLE_JPEG
#define RGB565_PIXEL_SIZE sizeof(gxj_pixel_type)
/**
 * TBD:
 * a). use imageSrcPtr & imageDstPtr instead of direct array pointers
 * b). move to a special file, like decode_png_image() in gxj_png_decode.c
 */
//static bool decode_jpeg_image(imageSrcPtr src, imageDstPtr dst)
static int decode_jpeg_image(char* inData, int inDataLen,
    char* outData, int outDataWidth, int outDataHeight)
{
    int result = FALSE;

    void *info = JPEG_To_RGB_init();
    if (info) {
        int width, height;
        if (JPEG_To_RGB_decodeHeader(info, inData, inDataLen,
            &width, &height) != 0) {
            if ((width < outDataWidth) || (height < outDataHeight)) {
                /*
                 * TBD:
                 * actual jpeg image size is smaller that prepared buffer
                 * (and requested image size),
                 * So initialize unused areas:
                 * right part = {width, 0, outDataWidth, height}
                 * & bottom part = {0, height, outDataWidth, outDataHeight}
                 * with default color (0,0,0) ?
                 */
            }

            if (JPEG_To_RGB_decodeData2(info, outData,
                RGB565_PIXEL_SIZE, 0, 0, outDataWidth, outDataHeight) != 0) {
                result = TRUE;
            }
        }

        JPEG_To_RGB_free(info);
    }
    return result;
}
#endif

/**
 * Decodes the given input data into a storage format used by
 * images.  The input data should be a JPEG image.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param creationErrorPtr pointer to the status of the decoding
 *         process. This function sets creationErrorPtr's value.
 */
void
decode_jpeg
(unsigned char* srcBuffer, int length, gxj_screen_buffer *image,
 gxutl_native_image_error_codes* creationErrorPtr) {

#if ENABLE_JPEG
    _imageDstData dstData;
    imageSrcPtr src = NULL;

    REPORT_CALL_TRACE(LC_LOWUI,
                     "LF:decodeJPEG()\n");

    /* Create the image from the buffered data */
    initImageDst(&dstData);

    dstData.vdc = image;
    /* what about (image->pixelData == NULL &&
                   image->width > 0 &&
                   image->height > 0) ? */
    if (image->pixelData == NULL) {
        ((imageDstPtr)&dstData)->setSize(
            ((imageDstPtr)&dstData), image->width, image->height);
        /*
        image->pixelData = pcsl_mem_malloc(
            image->width * image->height * RGB565_PIXEL_SIZE);
        */
    }

    if ((src = create_imagesrc_from_data((char **)(void *)&srcBuffer,
        length)) == NULL) {
        *creationErrorPtr = GXUTL_NATIVE_IMAGE_OUT_OF_MEMORY_ERROR;
    } else if (decode_jpeg_image((char*)srcBuffer, length,
        (char*)(image->pixelData),
        image->width, image->height) != FALSE) {
        *creationErrorPtr = GXUTL_NATIVE_IMAGE_NO_ERROR;
    } else {
        *creationErrorPtr = GXUTL_NATIVE_IMAGE_DECODING_ERROR;
    }

    if(src != NULL) {
        midpFree(src);
    }

#else
    (void)srcBuffer;
    (void)length;
    (void)image;
    *creationErrorPtr = GXUTL_NATIVE_IMAGE_UNSUPPORTED_FORMAT_ERROR;
#endif
}

/**
 * Image Decoder call back to set the color map of the decoded image
 *
 *  @param self pointer to a structure to hold decoded image structures.
 *  @param map pointer to the color map of the decoded image.
 *  @param length length of the color map.
 */
static void
setImageColormap(imageDstPtr self, long *map, int length) {
  _imageDstPtr p = (_imageDstPtr)self;

  REPORT_CALL_TRACE(LC_LOWUI,
                    "LF:STUB:setImageColormap()\n");

  p->hasColorMap = KNI_TRUE;
  memcpy(p->cmap, map, length * sizeof(long));
}

/**
 * Image Decoder call back to set the transparency map of the decoded image
 *
 *  @param self pointer to a structure to hold decoded image structures.
 *  @param map pointer to the transparency map of the decoded image.
 *  @param length length of the transparency map.
 *  @param palLength length of the color map.
 */
static void
setImageTransparencyMap(imageDstPtr self, unsigned char *map,
        int length, int palLength) {
    /*
     * This function is used for color type 3 (indexed-color) only,
     * in this case, the tRNS chunk contains a series of _one-byte_ alpha
     * values, corresponding to entries in the PLTE chunk.
     * For all other color types, transparency map is absent or has a fixed
     * length.
     */

    _imageDstPtr p = (_imageDstPtr)self;
    int i;

    REPORT_CALL_TRACE(LC_LOWUI, "LF:STUB:setImageTransparencyMap()\n");

    /*
     * The tRNS chunk shall not contain more alpha values than there are
     * palette entries, but a tRNS chunk may contain fewer values than there are
     * palette entries. In this case, the alpha value for all remaining palette
     * entries is assumed to be 255.
     */
    for (i = 0; i < length; i++) {
        p->tmap[i] = map[i];
    }
    if (length >= 0) { 
        for (i = length; i < palLength; i++) {
            p->tmap[i] = 0xFF;
        }
    }

    p->hasTransMap = KNI_TRUE;
}

/**
 * Image Decoder call back to set the width and height of the decoded image
 *
 *  @param self pointer to a structure to hold decoded image structures.
 *  @param width the width of the decoded image
 *  @param height the height of the decoded image
 */
static void
setImageSize(imageDstPtr self, int width, int height) {
  _imageDstPtr p = (_imageDstPtr)self;

    REPORT_CALL_TRACE(LC_LOWUI,
                      "LF:STUB:setImageSize()\n");

    if (p->vdc->pixelData == NULL) {
        p->vdc->width = width;
        p->vdc->height = height;

        p->vdc->pixelData = (gxj_pixel_type *)midpMalloc(width*height*sizeof(gxj_pixel_type));
    } else {
        if (p->vdc->width != width || p->vdc->height != height) {
            /* JAVA_TRACE("IMAGE DIMENSION IS INCORRECT!!\n"); */
        }
    }

    if (p->vdc->alphaData == NULL) {
        p->vdc->alphaData = (unsigned char *)
            midpMalloc(width*height*sizeof(unsigned char));
    }
}

/**
 * Image Decoder call back to set the pixels of the decoded image
 *
 *  @param self pointer to a structure to hold decoded image structures.
 *  @param y the y coordinate of the line where the pixel belong
 *  @param pixels the pixel data for the line
 *  @param pixelType the type of the pixel data
 */
static void
sendPixelsColor(imageDstPtr self, int y, uchar *pixels, int pixelType) {
  _imageDstPtr p = (_imageDstPtr)self;
  int x;

  REPORT_CALL_TRACE(LC_LOWUI,
                    "LF:STUB:sendPixelsColor()\n");

  if (p->vdc->pixelData == NULL || p->vdc->alphaData == NULL) {
    return;
  }

  if ((pixelType == CT_COLOR) ||              /* color triplet */
      (pixelType == (CT_COLOR | CT_ALPHA))) { /* color triplet with alpha */
    for (x = 0; x < p->vdc->width; ++x) {
      int r = pixels[0] >> 3;
      int g = pixels[1] >> 2;
      int b = pixels[2] >> 3;
      int alpha = 0xff;

      if (pixelType & CT_ALPHA) {
        alpha = pixels[3];
        pixels++;
      } else if (p->hasTransMap) {
        alpha = pixels[3];
        pixels++;
      }
      pixels += 3;

      // p->vdc->pixelData[y*p->vdc->width + x] = (r<<16) + (g<<8) + b;
      p->vdc->pixelData[y*p->vdc->width + x] = GXJ_RGB2PIXEL(r, g, b);
      p->vdc->alphaData[y*p->vdc->width + x] = alpha;
      if (alpha != 0xff) {
          p->hasAlpha = KNI_TRUE;
      }
    }
  } else { /* indexed color */
    for (x = 0; x < p->vdc->width; ++x) {
      int cmapIndex = *pixels++;

      int color = p->cmap[cmapIndex];

      int r = ((color >> 16) & 0xff) >> 3;
      int g = ((color >>  8) & 0xff) >> 2;
      int b = ((color >>  0) & 0xff) >> 3;

      int alpha = 0xff;

      if (r < 0) r = 0; else if (r > 0xff) r = 0xff;
      if (g < 0) g = 0; else if (g > 0xff) g = 0xff;
      if (b < 0) b = 0; else if (b > 0xff) b = 0xff;

      if ((pixelType & (CT_ALPHA | CT_COLOR)) == CT_ALPHA) {
        alpha = *pixels++;
      } else if (p->hasTransMap) {
        if ((pixelType & CT_COLOR) == 0) { /* grayscale */
          alpha = *pixels++;
        } else { /* indexed color */
          alpha = p->tmap[cmapIndex];
        }
      }

      // p->vdc->pixelData[y*p->vdc->width + x] = (r<<16) + (g<<8) + b;
      p->vdc->pixelData[y*p->vdc->width + x] = GXJ_RGB2PIXEL(r, g, b);
      p->vdc->alphaData[y*p->vdc->width + x] = alpha;
      if (alpha != 0xff) {
          p->hasAlpha = KNI_TRUE;
      }
    }
  }
}

/**
 * Initialize the given image destination structure
 *
 *  @param p pointer to the image destination to initialize
 */
void
initImageDst(_imageDstPtr p) {
  REPORT_CALL_TRACE(LC_LOWUI,
                    "LF:STUB:initImageDst()\n");

  p->super.ptr = p;

  p->super.depth            = 8;
  p->super.setColormap      = setImageColormap;
  p->super.setTransMap      = setImageTransparencyMap;
  p->super.setSize          = setImageSize;
  p->super.sendPixels       = sendPixelsColor;
  p->hasColorMap            = KNI_FALSE;
  p->hasTransMap            = KNI_FALSE;
  p->vdc                    = NULL;
  p->hasAlpha               = KNI_FALSE;
}

static void
clipped_blit(gxj_screen_buffer* dst, int dstX, int dstY, gxj_screen_buffer* src, const jshort *clip) {
  int width, height;        /* computed width and height */
  int startX; int startY;   /* x,y into the dstRaster */
  int negY, negX;           /* x,y into the srcRaster */
  int diff;
  unsigned short* srcRaster;
  unsigned short* dstRaster;
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

  srcRaster = src->pixelData + (negY ? (negY   * src->width) : 0) + negX;
  dstRaster = dst->pixelData +         (startY * dst->width)      + startX;

  unclipped_blit(dstRaster, dst->width<<1,
		 srcRaster, src->width<<1, height, width<<1,dst);
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

  for (srcY = src_y, destY = yStart, yCounter = 0;
       yCounter < height;
       srcY++, destY+=yIncr, yCounter++) {

    int srcYwidth = srcY * src->width;
    int destYwidth = destY * dest->width;

    for (srcX = src_x, destX = xStart, xCounter = 0;
         xCounter < width;
         srcX++, destX+=xIncr, xCounter++) {

      if ( transform & TRANSFORM_INVERTED_AXES ) {
          dest->pixelData[destX * dest->width + destY] =
            src->pixelData[srcYwidth /*srcY*src->width*/ + srcX];
        if (src->alphaData != NULL) {
            dest->alphaData[destX * dest->width + destY] =
                src->alphaData[srcYwidth /*srcY*src->width*/ + srcX];
        }

      } else {
        dest->pixelData[destYwidth /*destY * dest->width*/ + destX] =
                   src->pixelData[srcYwidth /*srcY*src->width*/ + srcX];
        if (src->alphaData != NULL) {
            dest->alphaData[destYwidth /*destY * dest->width*/ + destX] =
                src->alphaData[srcYwidth /*srcY*src->width*/ + srcX];
        }
      }
    } /*for x*/
  } /* for y */
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
        /*
         * create a new image that is a copy of the region with transform
         * applied
         */
        newSrc.pixelData =
            (gxj_pixel_type *)midpMalloc(width * height * sizeof (gxj_pixel_type));
        if (newSrc.pixelData == NULL) {
            REPORT_ERROR(LC_LOWUI, "Out of memory error, copyImageRegion (pixelData)\n"); 
            return ; 
        }
        if (src->alphaData != NULL) {
            newSrc.alphaData =
                (gxj_alpha_type *)midpMalloc(width * height * sizeof (gxj_alpha_type));
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
        gxj_pixel_type* pDest = dest->pixelData + (y_dest * dest->width) + x_dest;
        gxj_pixel_type* pSrc = src->pixelData + (y_src * src->width) + x_src;
        gxj_pixel_type* limit;
        int destWidthDiff = dest->width - width;
        int srcWidthDiff = src->width - width;
        int r1, g1, b1, a2, a3, r2, b2, g2;

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

                        *pDest = (gxj_pixel_type)((r1 << 11) | (g1 << 5) | (b1));
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
    }

    if (newSrc.pixelData != NULL) {
        midpFree(newSrc.pixelData);
    }

    if (newSrc.alphaData != NULL) {
        midpFree(newSrc.alphaData);
    }
}

/**
 * Gets an ARGB integer array from this <tt>ImageData</tt>. The
 * array consists of values in the form of 0xAARRGGBB.
 *
 * @param imageData The ImageData to read the ARGB data from
 * @param rgbBuffer The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlength Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
void gx_get_argb(const java_imagedata * srcImageDataPtr,
		 jint * rgbBuffer,
		 jint offset,
		 jint scanlength,
		 jint x, jint y, jint width, jint height,
		 gxutl_native_image_error_codes * errorPtr) {
  gxj_screen_buffer sbuf;
  if (gxj_get_image_screen_buffer_impl(srcImageDataPtr, &sbuf, NULL) != NULL) {
    // rgbData[offset + (a - x) + (b - y) * scanlength] = P(a, b);
    // P(a, b) = rgbData[offset + (a - x) + (b - y) * scanlength]
    // x <= a < x + width
    // y <= b < y + height
    int a, b, pixel, alpha;

    if (sbuf.alphaData != NULL) {
      for (b = y; b < y + height; b++) {
	for (a = x; a < x + width; a++) {
	  pixel = sbuf.pixelData[b*sbuf.width + a];
	  alpha = sbuf.alphaData[b*sbuf.width + a];
	  rgbBuffer[offset + (a - x) + (b - y) * scanlength] =
	    (alpha << 24) + GXJ_RGB16TORGB24(pixel);
	}
      }
    } else {
      for (b = y; b < y + height; b++) {
	for (a = x; a < x + width; a++) {
	  pixel = sbuf.pixelData[b*sbuf.width + a];
	  rgbBuffer[offset + (a - x) + (b - y) * scanlength] =
	    GXJ_RGB16TORGB24(pixel) | 0xFF000000;
	}
      }
    }
  }

  * errorPtr = GXUTL_NATIVE_IMAGE_NO_ERROR;
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
