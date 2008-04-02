/*
 *  
 *
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

#include <gxapi_constants.h>
#include <gxjport_text.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"
#include "gxj_intern_font_bitmap.h"


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

/*
 * Draws the first n characters specified using the current font,
 * color, and anchor point.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java declaration:
 * <pre>
 *     drawString(Ljava/lang/String;III)V
 * </pre>
 *
 * @param pixel Device-dependent pixel value
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
void
gx_draw_chars(jint pixel, const jshort *clip, 
	      const java_imagedata *dst, int dotted, 
	      int face, int style, int size,
	      int x, int y, int anchor, const jchar *charArray, int n) {
    int i;
    int xStart;
    int xDest;
    int yDest;
    int width;
    int yLimit;
    int nCharsToSkip = 0;
    int widthRemaining;
    int yCharSource;
    int charsWidth;
    int charsHeight;
    int fontWidth;
    int fontHeight;
    int fontAscent;
    int fontDescent;
    int fontLeading;
    int clipX1 = clip[0];
    int clipY1 = clip[1];
    int clipX2 = clip[2];
    int clipY2 = clip[3];
    int diff;
    int result;
    gxj_screen_buffer screen_buffer;
    gxj_screen_buffer *dest = 
      gxj_get_image_screen_buffer_impl(dst, &screen_buffer, NULL);
    dest = (gxj_screen_buffer *)getScreenBuffer(dest);

    REPORT_CALL_TRACE(LC_LOWUI, "LCDUIdrawChars()\n");

    if (n <= 0) {
        /* nothing to do */
        return;
    }

    gx_get_fontinfo(face, style, size, &fontAscent, &fontDescent, &fontLeading);
    charsWidth = gx_get_charswidth(face, style, size, charArray, n);
    charsHeight = fontAscent + fontDescent + fontLeading;

    xDest = x;
    if (anchor & RIGHT) {
        xDest -= charsWidth;
    }

    if (anchor & HCENTER) {
        xDest -= charsWidth / 2;
    }

    yDest = y;  
    if (anchor & BOTTOM) {
        yDest -= charsHeight;
    }

    if (anchor & BASELINE) {
        yDest -= charsHeight - fontDescent;
    }

    result = gxjport_draw_chars(pixel, clip, dest, dotted, face, style, size,
                                xDest, yDest, anchor, charArray, n);
    if (result == KNI_TRUE) { 
        return;
    }

    fontWidth = FontBitmaps[1][FONT_WIDTH];
    fontHeight = FontBitmaps[1][FONT_HEIGHT];
    fontDescent = FontBitmaps[1][FONT_DESCENT];

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
        drawChar(dest, charArray[nCharsToSkip], GXJ_RGB24TORGB16(pixel), xDest, yDest,
                 xStart, yCharSource, xLimit, yLimit,
                 FontBitmaps, fontWidth, fontHeight);
        nCharsToSkip++;
        xDest += startWidth;
        widthRemaining -= startWidth;
    }

    /* Draw all the fully wide chars. */
    for (i = nCharsToSkip; i < n && widthRemaining >= fontWidth;
         i++, xDest += fontWidth, widthRemaining -= fontWidth) {

        drawChar(dest, charArray[i], GXJ_RGB24TORGB16(pixel), xDest, yDest,
                 0, yCharSource, fontWidth, yLimit,
                 FontBitmaps, fontWidth, fontHeight);
    }

    if (i < n && widthRemaining > 0) {
        /* Clipped, draw the left part of the last char. */
        drawChar(dest, charArray[i], GXJ_RGB24TORGB16(pixel), xDest, yDest,
                 0, yCharSource, widthRemaining, yLimit,
                 FontBitmaps, fontWidth, fontHeight);
    }
}

/**
 * Obtains the ascent, descent and leading info for the font indicated.
 *
 * @param face The face of the font (Defined in <B>Font.java</B>)
 * @param style The style of the font (Defined in <B>Font.java</B>)
 * @param size The size of the font (Defined in <B>Font.java</B>)
 * @param ascent The font's ascent should be returned here.
 * @param descent The font's descent should be returned here.
 * @param leading The font's leading should be returned here.
 */
void
gx_get_fontinfo(int face, int style, int size, 
		int *ascent, int *descent, int *leading) {
    int result;

    REPORT_CALL_TRACE(LC_LOWUI, "LCDUIgetFontInfo()\n");

    result = gxjport_get_font_info(face, style, size,
                                   ascent, descent, leading);
    if (result == KNI_TRUE) { 
        return;
    }

    *ascent  = FontBitmaps[1][FONT_ASCENT];
    *descent = FontBitmaps[1][FONT_DESCENT];
    *leading = FontBitmaps[1][FONT_LEADING];
}

/**
 * Gets the advance width for the first n characters in charArray if
 * they were to be drawn in the font indicated by the parameters.
 *
 * <p>
 * <b>Reference:</b>
 * Related Java declaration:
 * <pre>
 *     charWidth(C)I
 * </pre>
 *
 * @param face The font face to be used (Defined in <B>Font.java</B>)
 * @param style The font style to be used (Defined in
 * <B>Font.java</B>)
 * @param size The font size to be used. (Defined in <B>Font.java</B>)
 * @param charArray The string to be measured
 * @param n The number of character to be measured
 * @return The total advance width in pixels (a non-negative value)
 */
int
gx_get_charswidth(int face, int style, int size, 
		  const jchar *charArray, int n) {
    int width;

    REPORT_CALL_TRACE(LC_LOWUI, "LCDUIcharsWidth()\n");

    width = gxjport_get_chars_width(face, style, size, charArray, n); 
    if (width > 0) {
        return width;
    }

    return n * FontBitmaps[1][FONT_WIDTH];
}
