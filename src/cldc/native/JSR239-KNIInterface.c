/*
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

#include "JSR239-KNIInterface.h"

#ifdef DEBUG
#include <stdio.h>
#endif

#define PUT_DEPTH 16
#define GET_DEPTH 16
#define SCREEN_BUFFER_BPP 2


/*
 * Return the window implementation strategy for the window accessed by the
 * given javax.microedition.lcdui.Graphics object.
 *
 * Currently, STRATEGY_USE_PIXMAP and STRATEGY_USE_PBUFFER are supported
 * but STRATEGY_USE_WINDOW is not.
 */
jint
JSR239_getWindowStrategy(jobject winHandle) {
    (void)winHandle;
#ifdef DEBUG
    printf("JSR239_getWindowStrategy\n");
#endif

    return STRATEGY_USE_PIXMAP;
}


/*
 * Return a native ID for the given window that is suitable to be passed
 * to eglCreateWindowSurface.
 *
 * This function is called only when JSR239_getWindowStrategy(winHandle) has
 * returned STRATEGY_USE_WINDOW.
 */
jint
JSR239_getWindowNativeID(jobject winHandle) {
    (void)winHandle;
#ifdef DEBUG
    printf("JSR239_getWindowNativeID\n");
#endif

    return -1;
}


/* Convert a 1-8 bit quantity xyz into 8 bits of the form xyzxyzxyz... */

static void
initPixelExpansionTable(int *tab, int bits) {
    int i;

    if (bits == 0) {
        tab[0] = 0xff;
    } else if (bits == 1) {
        tab[0] = 0;
        tab[1] = 0xff;
    } else if (bits == 2) {
        tab[0] = 0;
        tab[1] = 0x33;
        tab[2] = 0xaa;
        tab[3] = 0xff;
    } else if (bits == 3) {
        for (i = 0; i < 1 << bits; i++) {
            tab[i] = i << 5;
            tab[i] |= i << 2;
            tab[i] |= i >> 1;
        }
    } else if (bits <= 8) {
        for (i = 0; i < 1 << bits; i++) {
            tab[i] = i << (8 - bits);
            tab[i] |= i >> (2*bits - 8);

#ifdef DEBUG
            /*       printf("bits = %d, tab[%d] = %d\n", bits, i, tab[i]); */
#endif
        }
    } else {
        printf("Bad # of bits: %d\n", bits);
    }
}


/*
 * Return an JSR239_Pixmap that may be used as a back buffer for the
 * window accessed by the given Canvas, GameCanvas, or Graphics.
 */
JSR239_Pixmap *
JSR239_getWindowPixmap(jobject winHandle, jint width, jint height,
                       jint pixelBytes,
                       jint redSize,
                       jint greenSize,
                       jint blueSize,
                       jint alphaSize) {

    JSR239_Pixmap *p;
    int screenBufferSize;

#ifdef DEBUG
    printf("JSR239_getWindowPixmap\n");
#endif

#ifdef DEBUG
    printf("redSize = %d\n", redSize);
    printf("greenSize = %d\n", greenSize);
    printf("blueSize = %d\n", blueSize);
    printf("alphaSize = %d\n", alphaSize);
#endif

    p = (JSR239_Pixmap *)JSR239_malloc(sizeof(JSR239_Pixmap));
    if (!p) {
        return (JSR239_Pixmap *) 0;
    }
    p->width = width;
    p->height = height;
    p->pixelBytes = pixelBytes;
    p->stride = p->width*pixelBytes;
    p->rSize = redSize;
    p->gSize = greenSize;
    p->bSize = blueSize;
    p->aSize = alphaSize;
    p->aOffset = redSize + greenSize + blueSize;
    p->rOffset = greenSize + blueSize;
    p->gOffset = blueSize;
    p->bOffset = 0;

    p->pixels = JSR239_malloc(p->height*p->stride);
    if (!p->pixels) {
        JSR239_free(p);
        return (JSR239_Pixmap *) 0;
    }

    screenBufferSize = p->width*p->height*SCREEN_BUFFER_BPP;
    p->screen_buffer = JSR239_malloc(screenBufferSize);
    if (!p->screen_buffer) {
        JSR239_free(p->pixels);
        JSR239_free(p);
        return (JSR239_Pixmap *) 0;
    }

    initPixelExpansionTable(p->rtab, p->rSize);
    initPixelExpansionTable(p->gtab, p->gSize);
    initPixelExpansionTable(p->btab, p->bSize);
    initPixelExpansionTable(p->atab, p->aSize);

    return p;
}

JSR239_Pixmap *
JSR239_getImagePixmap(jobject winHandle, jint width, jint height,
                      jint pixelBytes,
                      jint redSize, jint greenSize, jint blueSize,
                      jint alphaSize) {
#ifdef DEBUG
    printf("JSR239_getImagePixmap\n");
#endif

    return JSR239_getWindowPixmap(winHandle, width, height,
                                  pixelBytes,
                                  redSize, greenSize, blueSize, alphaSize);
}

void
JSR239_destroyPixmap(JSR239_Pixmap *pixmap) {
#ifdef DEBUG
    printf("JSR239_destroyPixmap\n");
#endif

    if (pixmap) {
        if (pixmap->pixels) {
            JSR239_free(pixmap->pixels);
        }
        if (pixmap->screen_buffer) {
            JSR239_free(pixmap->screen_buffer);
        }
        JSR239_free(pixmap);
    }
}


#ifdef DEBUG
static int debug_convert = 0;
#endif

jint
CONVERT_565_TO_8888(jshort rgb) {

    jint argb;

    int r = (rgb >> 11) & 0x1f;
    int g = (rgb >>  5) & 0x3f;
    int b = (rgb      ) & 0x1f;

#ifdef DEBUG
    if (debug_convert) {
        printf("CONVERT_565_TO_8888: rgb = 0x%x\n", rgb);
        printf("CONVERT_565_TO_8888: r = 0x%x\n", r);
        printf("CONVERT_565_TO_8888: g = 0x%x\n", g);
        printf("CONVERT_565_TO_8888: b = 0x%x\n", b);
    }
#endif

    /* Extend to 8 bits by replicating the high bits into the low bits */
    r = ((r << 3) | (r >> 2)) & 0xff;
    g = ((g << 2) | (g >> 4)) & 0xff;
    b = ((b << 3) | (b >> 2)) & 0xff;

#ifdef DEBUG
    if (debug_convert) {
        printf("CONVERT_565_TO_8888: rgb = 0x%x\n", rgb);
        printf("CONVERT_565_TO_8888: r = 0x%x\n", r);
        printf("CONVERT_565_TO_8888: g = 0x%x\n", g);
        printf("CONVERT_565_TO_8888: b = 0x%x\n", b);
    }
#endif

    argb = 0xff000000 | (r << 16) | (g << 8) | b;

#ifdef DEBUG
    if (debug_convert) {
        printf("CONVERT_565_TO_8888: argb = 0x%x\n", argb);
    }
#endif

    return argb;
}


void
copyFromScreenBuffer(JSR239_Pixmap *dst, void *sbuffer,
                     int clip_x, int clip_y, int width, int height,
                     int deltaHeight) {

    unsigned char *dstPtr;
#if GET_DEPTH == 16
    jshort *srcPtr16 = (jshort *)sbuffer;
#endif
#if GET_DEPTH == 32
    jint *srcPtr32 = (jint *)sbuffer;
#endif

    int rSize = dst->rSize;
    int gSize = dst->gSize;
    int bSize = dst->bSize;
    int aSize = dst->aSize;

    int rOffset = dst->rOffset;
    int gOffset = dst->gOffset;
    int bOffset = dst->bOffset;
    int aOffset = dst->aOffset;

    int alpha, red, green, blue, pixel;
    int x, y;

#ifdef DEBUG
    printf("aOffset = %d\n", aOffset);
    printf("rOffset = %d\n", rOffset);
    printf("gOffset = %d\n", gOffset);
    printf("bOffset = %d\n", bOffset);

    printf("aSize = %d\n", aSize);
    printf("rSize = %d\n", rSize);
    printf("gSize = %d\n", gSize);
    printf("bSize = %d\n", bSize);
#endif

    dstPtr = (unsigned char *)dst->pixels +
        dst->stride*clip_y + dst->pixelBytes*clip_x;

    dstPtr += dst->stride * deltaHeight;
    height -= deltaHeight;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int argb8888;

#ifdef DEBUG
            if ((x == 10 && y == 10) ||
                (x == 25 && y == 25) ||
                (x == 100 && y == 100) ||
                (x == 120 && y == 150)) {
                printf("\ncopyFromScreenBuffer: x = %d, y = %d\n", x, y);
            }
#endif

#if GET_DEPTH == 16
#ifdef DEBUG
            if ((x == 10 && y == 10) ||
                (x == 25 && y == 25) ||
                (x == 100 && y == 100) ||
                (x == 120 && y == 150)) {
                printf("  16-bit src = 0x%x\n", srcPtr16[x]);
            }
#endif
            argb8888 = CONVERT_565_TO_8888(srcPtr16[x]);
#endif
#if GET_DEPTH == 32
#ifdef DEBUG
            if ((x == 10 && y == 10) ||
                (x == 25 && y == 25) ||
                (x == 100 && y == 100) ||
                (x == 120 && y == 150)) {
                printf("  32-bit src = 0x%x\n", srcPtr32[x]);
            }
#endif
            argb8888 = srcPtr32[x];
#endif

            /* GL RGBA format */
            blue = (argb8888 >> 24) & 0xff;
            green = (argb8888 >> 16) & 0xff;
            red = (argb8888 >> 8) & 0xff;
            alpha = argb8888 & 0xff;

#ifdef DEBUG
            if ((x == 10 && y == 10) ||
                (x == 25 && y == 25) ||
                (x == 100 && y == 100) ||
                (x == 120 && y == 150)) {
                printf("  argb8888 = 0x%x\n", argb8888);
                printf("  alpha = 0x%x, red = 0x%x, green = 0x%x, blue = 0x%x\n",
                       alpha, red, green, blue);
            }
#endif

            alpha >>= (8 - aSize);
#if GET_DEPTH == 32
            if (aSize == 0) {
                alpha = 0xff;
            }
#endif
            alpha <<= aOffset;

            red >>= (8 - rSize);
            red <<= rOffset;

            green >>= (8 - gSize);
            green <<= gOffset;

            blue >>= (8 - bSize);
            blue <<= bOffset;

            pixel = alpha | red | green | blue;

#ifdef DEBUG
            if ((x == 10 && y == 10) ||
                (x == 25 && y == 25) ||
                (x == 100 && y == 100) ||
                (x == 120 && y == 150)) {
                printf("  after mangling:\n");
                printf("  alpha = 0x%x, red = 0x%x, green = 0x%x, blue = 0x%x\n",
                       alpha, red, green, blue);
                printf("  new pixel = 0x%x\n", pixel);
            }
#endif

            if (dst->pixelBytes == 2) {
                *((jshort *)dstPtr + x) = (jshort)pixel;
            } else if (dst->pixelBytes == 4) {
                *((jint *)dstPtr + x) = pixel;
            } else {
                printf("pixelBytes must be 2 or 4!\n");
            }
        }

#if GET_DEPTH == 16
        srcPtr16 += width;
#endif
#if GET_DEPTH == 32
        srcPtr32 += width;
#endif
        dstPtr += dst->stride;
    }
}


jshort
CONVERT_8888_TO_565(jint argb) {

    int r = (argb >> 16) & 0xff;
    int g = (argb >>  8) & 0xff;
    int b = (argb      ) & 0xff;

    r >>= 3;
    g >>= 2;
    b >>= 3;

    return (r << 11) | (g << 5) | b;
}

void
copyToScreenBuffer(JSR239_Pixmap *src, jint deltaHeight, 
                   jint clipX, jint clipY, jint clipWidth, jint clipHeight,
                   jint flipY) {

    jint width = src->width;
    jint height= src->height;

    unsigned char *srcPtr;
#if PUT_DEPTH == 16
    jshort *dstPtr16;
#endif
#if PUT_DEPTH == 32
    jint *dstPtr32;
#endif

    int aOffset = src->aOffset;
    int rOffset = src->rOffset;
    int gOffset = src->gOffset;
    int bOffset = src->bOffset;

    int aMask = (1 << src->aSize) - 1;
    int rMask = (1 << src->rSize) - 1;
    int gMask = (1 << src->gSize) - 1;
    int bMask = (1 << src->bSize) - 1;

    int red, green, blue, alpha, pixel;
    int sstride;
    int x, y;

#ifdef DEBUG
    printf("copyToScreenBuffer:\n");
    printf("width = %d\n", width);
    printf("height = %d\n", height);
    printf("src->stride = %d\n", src->stride);

    printf("aOffset = %d\n", aOffset);
    printf("rOffset = %d\n", rOffset);
    printf("gOffset = %d\n", gOffset);
    printf("bOffset = %d\n", bOffset);

    printf("aMask = %d\n", aMask);
    printf("rMask = %d\n", rMask);
    printf("gMask = %d\n", gMask);
    printf("bMask = %d\n", bMask);
#endif

    srcPtr = (unsigned char *)src->pixels +
      (flipY ? src->stride*(height - 1) : 0);
    sstride = flipY ? -src->stride : src->stride;

    srcPtr += sstride * (clipY == 0 ? deltaHeight : clipY);
    if (clipY + clipHeight > height - deltaHeight) {
        clipHeight = height - clipY - deltaHeight;
    }

#if PUT_DEPTH == 16
    dstPtr16 = (jshort *)src->screen_buffer + width * clipY;
#endif
#if PUT_DEPTH == 32
    dstPtr32 = (jint *)src->screen_buffer + width * clipY;
#endif

    for (y = clipY; y < clipY + clipHeight; y++) {
        for (x = clipX; x < clipX + clipWidth; x++) {
            if (src->pixelBytes == 2) {
                pixel = (jint)(*((jshort *)srcPtr + x));
            } else if (src->pixelBytes == 4) {
                pixel = *((jint *)srcPtr + x);
            } else {
                printf("pixelBytes must be 2 or 4!\n");
            }

            alpha = (pixel >> aOffset) & aMask;
            red   = (pixel >> rOffset) & rMask;
            green = (pixel >> gOffset) & gMask;
            blue  = (pixel >> bOffset) & bMask;

#ifdef DEBUG
            if (pixel != 0) {
                printf("\ncopyToScreenBuffer: x = %d, y = %d\n", x, y);
                printf("  original pixel = 0x%x\n", pixel);
                printf("  red = 0x%x, green = 0x%x, blue = 0x%x, alpha = 0x%x\n", red, green, blue, alpha);
            }
#endif

            alpha = src->atab[alpha];
            red   = src->rtab[red];
            green = src->gtab[green];
            blue  = src->btab[blue];

#ifdef DEBUG
            if (pixel != 0) {
                printf("  tabled alpha = 0x%x, red = 0x%x, green = 0x%x, blue = 0x%x\n",
                       alpha, red, green, blue);
            }
#endif

            /* MIDP ARGB format */
            alpha <<= 24;
            red <<= 16;
            green <<= 8;

            pixel = alpha | red | green | blue;

#ifdef DEBUG
            if (pixel != 0) {
                printf("  shifted alpha = 0x%x, red = 0x%x, green = 0x%x, blue = 0x%x\n",
                       alpha, red, green, blue);
                printf("  32-bit pixel = 0x%x\n", pixel);
            }
#endif

#if PUT_DEPTH == 16
            dstPtr16[x] = CONVERT_8888_TO_565(pixel);
#ifdef DEBUG
            if (pixel != 0) {
                printf("  16-bit pixel = 0x%x\n", dstPtr16[x]);
            }
#endif
#endif
#if PUT_DEPTH == 32
            dstPtr32[x] = pixel | 0xff000000;
#ifdef DEBUG
            if (pixel != 0) {
                printf("  32-bit pixel = 0x%x\n", dstPtr32[x]);
            }
#endif
#endif
        }

        srcPtr += sstride;
#if PUT_DEPTH == 16
        dstPtr16 += width;
#endif
#if PUT_DEPTH == 32
        dstPtr32 += width;
#endif
    }
}
