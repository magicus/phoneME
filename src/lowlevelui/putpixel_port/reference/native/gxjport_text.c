/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
#include <midp_properties_port.h>

#include <gxapi_constants.h>
#include <gxjport_text.h>
#include <gxj_putpixel.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"
#include "putpixel_font_bitmap.h"

/**
 * @file
 *
 * given character code c, return the bitmap table where
 * the encoding for this character is stored.
 */
static pfontbitmap selectFontBitmap(jchar c, pfontbitmap* pfonts) {
    int i=1;
    unsigned char c_hi = (c>>8) & 0xff;
    unsigned char c_lo = c & 0xff;
    do {
        if ( c_hi == pfonts[i][FONT_CODE_RANGE_HIGH]
          && c_lo >= pfonts[i][FONT_CODE_FIRST_LOW]
          && c_lo <= pfonts[i][FONT_CODE_LAST_LOW]
        ) {
            return pfonts[i];
        }
        i++;
    } while (i <= (int) pfonts[0]);
    /* the first table must cover the range 0-nn */
    return pfonts[1];
}

/**
 * @file
 *
 * putpixel primitive character drawing.
 */
unsigned char BitMask[8] = {0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};
static void drawChar(gxj_screen_buffer *sbuf, jchar c0,
		     gxj_pixel_type pixelColor, int x, int y,
		     int xSource, int ySource, int xLimit, int yLimit,
		     pfontbitmap* pfonts,
		     int fontWidth, int fontHeight) {
    int xDest;
    int yDest;
    int xDestLimit;
    int yDestLimit;
    unsigned long byteIndex;
    int bitOffset;
    unsigned long pixelIndex;
    unsigned long pixelIndexLineInc;
    unsigned char bitmapByte;
    unsigned char const * fontbitmap =
        selectFontBitmap(c0,pfonts) + FONT_DATA;
    jchar const c = (c0 & 0xff) -
        fontbitmap[FONT_CODE_FIRST_LOW-FONT_DATA];
    unsigned long mapLen =
        ((fontbitmap[FONT_CODE_LAST_LOW-FONT_DATA]
        - fontbitmap[FONT_CODE_FIRST_LOW-FONT_DATA]
        + 1) * fontWidth * fontHeight + 7) >> 3;
    unsigned long const firstPixelIndex = c * fontHeight * fontWidth;
    unsigned char const * const mapend = fontbitmap + mapLen;

    int destWidth = sbuf->width;
    gxj_pixel_type *dest = sbuf->pixelData + y*destWidth + x;
    int destInc = destWidth - xLimit + xSource;

    pixelIndex = firstPixelIndex + (ySource * fontWidth) + xSource;
    pixelIndexLineInc = fontWidth - (xLimit - xSource);
    byteIndex = pixelIndex / 8;
    fontbitmap += byteIndex;
    bitOffset = pixelIndex % 8;
    bitmapByte = *fontbitmap;
    yDestLimit = y + yLimit - ySource;
    xDestLimit = x + xLimit - xSource;

    // The clipping should be applied here already, so
    // we use optimal access to destination buffer with
    // no extra checks

    if (fontbitmap < mapend) {
        for (yDest = y; yDest < yDestLimit;
                yDest++, bitOffset+=pixelIndexLineInc, dest += destInc) {
            for (xDest = x; xDest < xDestLimit;
                    xDest++, bitOffset++, dest++) {
                if (bitOffset >= 8) {
                    fontbitmap += bitOffset / 8;
                    if (fontbitmap >= mapend) {
                        break;
                    }
                    bitOffset %= 8;
                    bitmapByte = *fontbitmap;
                }

                /* we don't draw "background" pixels, only foreground */
                if ((bitmapByte & BitMask[bitOffset]) != 0) {
                    *dest = pixelColor;
                }
            }
        }
    }
}


/**
 * Draws the first n characters to Offscreen memory image specified using the
 * current font, color.
 *
 * @param pixel color of font
 * @param clip  coordinates of the clipping area.
 *              clip[0] corresponds to clipX1.  clipX1 is the top left X
 *              coordinate of the clipping area.  clipX1 is guaranteeded
 *              to be larger or equal 0 and smaller or equal than clipX2.
 *              clip[1] corresponds to clipY1.  clipY1 is the top left Y
 *              coordinate of the clipping area.  clipY1 is guaranteeded
 *              to be larger or equal 0 and smaller or equal than clipY2
 *              clip[2] corresponds to clipX2.  clipX2 is the bottom right
 *              X coordinate of the clipping area. clipX2 is guaranteeded
 *              to be larger or equal than clipX1 and smaller or equal than
 *              dst->width.
 *              clip[3] corresponds to clipY2.  clipY2 is the bottom right
 *              Y coordinate of the clipping area.  clipY2 is guaranteeded
 *              to be larger or equal than clipY1 and smaller or equal than
 *              dst->height.
 * @param dest  pointer to gxj_screen_buffer struct
 * @param dotted
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @param x The x coordinate of the top left font coordinate
 * @param y The y coordinate of the top left font coordinate
 * @param anchor
 * @param direction text output direction (LEFT_TO_RIGHT | RIGHT_TO_LEFT)
 * @param charArray Pointer to the characters to be drawn
 * @param n The number of characters to be drawn
 *
 * @return <tt>KNI_TRUE</tt> if font rendered successfully,
 *         <tt>KNI_FALSE</tt> or negative value on error or not supported
 */
int gxjport_draw_chars(int pixel, const jshort *clip, gxj_screen_buffer *dest,
                       int dotted, int face, int style, int size,
                       int xDest, int yDest, int anchor, int direction,
                       const jchar *charArray, int n) {
    int i;
    int xStart;
    int width;
    int yLimit;
    int nCharsToSkip = 0;
    int charToDraw;
    int charToStop;

    int widthRemaining;
    int yCharSource;
    int fontWidth;
    int fontHeight;
    gxj_pixel_type pixelColor;
    int clipX1 = clip[0];
    int clipY1 = clip[1];
    int clipX2 = clip[2];
    int clipY2 = clip[3];
    int diff;

//    int fontAscent;
//    int fontDescent;
//    int fontLeading;
//    int result;
//    gxj_screen_buffer screen_buffer;

    fontWidth = FontBitmaps[1][FONT_WIDTH];
    fontHeight = FontBitmaps[1][FONT_HEIGHT];
//    fontDescent = FontBitmaps[1][FONT_DESCENT];

    width = fontWidth * n;
    yLimit = fontHeight;

    xStart = 0;
    yCharSource = 0;

    /*
     * Don't let a bad clip origin into the clip code or the may be
     * over or under writes of the destination buffer.
     *
     * Don't change the clip array that was passed in.
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

    /* Apply the clip region to the destination region */
    diff = clipX1 - xDest;
    if (diff > 0) {
        xStart += diff % fontWidth;
        width -= diff;
        xDest += diff;
        nCharsToSkip = diff / fontWidth;
    }

    diff = (xDest + width) - clipX2;
    if (diff > 0) {
        width -= diff;
        n -= diff/fontWidth;
    }

    diff = (yDest + fontHeight) - clipY2;
    if (diff > 0) {
        yLimit -= diff;
    }

    diff = clipY1 - yDest;
    if (diff > 0) {
        yCharSource += diff;
        yDest += diff;
    }

    if (width <= 0 || yCharSource >= yLimit || nCharsToSkip >= n) {
        /* Nothing to do. */
        return;
    }

    widthRemaining = width;
    pixelColor = GXJ_RGB24TORGB16(pixel);

    switch (direction) {
        case RIGHT_TO_LEFT:
            charToDraw = n - nCharsToSkip - 1;
            charToStop = -1;
            break;

        case LEFT_TO_RIGHT:
        default:
            charToDraw = nCharsToSkip;
            charToStop = n;
            break;
    }

    if (xStart != 0) {
        int xLimit;
        int startWidth;
        if (width > fontWidth) {
            startWidth = fontWidth - xStart;
            xLimit = fontWidth;
        } else {
            startWidth = width;
            xLimit = xStart + width;
        }

        /* Clipped, draw the right part of the first char. */
        drawChar(dest, charArray[charToDraw], pixelColor, xDest, yDest,
                 xStart, yCharSource, xLimit, yLimit,
                 FontBitmaps, fontWidth, fontHeight);
        charToDraw += direction;
        xDest += startWidth;
        widthRemaining -= startWidth;
    }

    /* Draw all the fully wide chars. */
    for (i = charToDraw; i != charToStop && widthRemaining >= fontWidth;
         i+=direction, xDest += fontWidth, widthRemaining -= fontWidth) {

        drawChar(dest, charArray[i], pixelColor, xDest, yDest,
                 0, yCharSource, fontWidth, yLimit,
                 FontBitmaps, fontWidth, fontHeight);
    }

    if (i != charToStop && widthRemaining > 0) {
        /* Clipped, draw the left part of the last char. */
        drawChar(dest, charArray[i], pixelColor, xDest, yDest,
                 0, yCharSource, widthRemaining, yLimit,
                 FontBitmaps, fontWidth, fontHeight);
    }

    return KNI_TRUE;
}

/**
 * queries for the font info structure for a given font specs
 *
 * @param face The font face to be used (Defined in <B>Font.java</B>)
 * @param style The font style to be used (Defined in
 * <B>Font.java</B>)
 * @param size The font size to be used. (Defined in <B>Font.java</B>)
 *
 * @param ascent return value of font's ascent
 * @param descent return value of font's descent
 * @param leading return value of font's leading
 *
 * @return <tt>KNI_TRUE</tt> if successful, <tt>KNI_FALSE</tt> on
 * error or not supported
 *
 */
int gxjport_get_font_info(int face, int style, int size,
                          int *ascent, int *descent, int *leading) {

    *ascent  = FontBitmaps[1][FONT_ASCENT];
    *descent = FontBitmaps[1][FONT_DESCENT];
    *leading = FontBitmaps[1][FONT_LEADING];

    return KNI_TRUE;
}

/**
 * returns the char width for the first n characters in charArray if
 * they were to be drawn in the font indicated by the parameters.
 *
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @param charArray The string to be measured
 * @param n The number of character to be measured
 * @return total advance width in pixels (a non-negative value)
 *         or 0 if not supported
 */
int gxjport_get_chars_width(int face, int style, int size,
                            const jchar *charArray, int n) {

    return n * FontBitmaps[1][FONT_WIDTH];
}

