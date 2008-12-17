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

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include <windows.h>
#include <ddraw.h>

#include <kni.h>
#include <midp_logging.h>
#include <pcsl_memory.h>

#include <gxapi_constants.h>

#include "gxj_intern_graphics.h"
#include "gxj_intern_putpixel.h"
#include "gxj_intern_font_bitmap.h"

#include "gx_font.h"
#include "gxj_putpixel.h"
#include "font_internal.h"

/*
 * WinCE font notes:
 *
 * We use native fonts from WinCE. This makes it possible to support
 * localized fonts such as CJK characters. Essentially, all text output
 * is done via ExtTextOut(). Character widths are calculated using 
 * GetTextExtentPoint32W().
 *
 * However, the WinCE APIs are very slow, especially when we have to switch
 * between drawing to the MIDP screen and a MIDP off-screen bitmap. In order
 * to have good JBenchmark1 & Jbenchmark2 scores, we use the following
 * techniques:
 *
 * (1) We cache ASCII font bitmaps and render them directly. This
 *     means we don't have to call getScreenBufferHDC() unless you are
 *     drawing text outside of the ASCII range.
 * (2) We JIT-compile an ARM routine for each ASCII character. This routine
 *     can be used when the text is not clipped. This routine is faster
 *     because it can set pixels without having to read the font bitmap.
 *     The Text test in JB2 shows a progress from 180fps to 230fps with
 *     this optimization. On the i-mate KJAM, the JIT routines occupy about
 *     15KB of RAM. They may take up more space on a VGA device.
 *
 * To disable WinCE native fonts, set USE_NATIVE_FONT=0 in this
 * file.
 */

#define WIN32_PLATFORM_PSPC 1
//#define JWC_WINCE_USE_DIRECT_DRAW ENABLE_WINCE_DIRECT_DRAW

#ifdef WIN32_PLATFORM_PSPC
// comment this line to use JAVA font
#define USE_NATIVE_FONT 0
#else // smartphone, we use Java font now and may find a suitable font for it later.
#define USE_NATIVE_FONT 0
#endif

#include <windows.h>
#include <windowsx.h>

extern HDC getScreenBufferHDC(gxj_pixel_type *buffer, int width, int height);

extern LPDIRECTDRAW g_pDD;

// variable used at caching hdc for fast font display
// pDDS and cachedHDC must both be NULL or both be non-NULL
static LPDIRECTDRAWSURFACE pFontDDS = NULL;
static HDC cachedFontHDC = NULL;
static gxj_pixel_type *cachedFontBuffer;

/** Separate colors are 8 bits as in Java RGB */
#define GET_RED_FROM_PIXEL(P)   (((P) >> 8) & 0xF8)
#define GET_GREEN_FROM_PIXEL(P) (((P) >> 3) & 0xFC)
#define GET_BLUE_FROM_PIXEL(P)  (((P) << 3) & 0xF8)

/* convert color to 16bit color */
#define RGB24TORGB16(x) (((( x ) & 0x00F80000) >> 8) + \
                         ((( x ) & 0x0000FC00) >> 5) + \
			 ((( x ) & 0x000000F8) >> 3) )

/* convert color to 16bit color */
#define RGB16TORGB24(x) (((( x ) << 8) & 0x00F80000) | \
                         ((( x ) << 5) & 0x0000FC00) | \
                         ((( x ) << 3) & 0x000000F8) )

// pfontbitmap
#define TheFontBitmap     FontBitmaps[1]
#define TheFontBitmapSize 2024 // IMPL_NOTE: must not be hardcoded!

static int *compile_glyph(gxj_pixel_type *buf, int bufWidth, int fontWidth,
                          int height, int *scratch_code);

#define MIN_FAST_GLYPH  32    // inclusive
#define MAX_FAST_GLYPH  127   // exclusive
#define NUM_FAST_GLYPHS (MAX_FAST_GLYPH - MIN_FAST_GLYPH)
#define FASTIDX(i) (((int)i) - MIN_FAST_GLYPH)
#define IS_FAST_GLYPH(c) ((MIN_FAST_GLYPH <= (c)) && ((c) < MAX_FAST_GLYPH))

typedef int (*compiled_glyph_drawer)(gxj_pixel_type *dest, int color, 
                                     int linePitch);

typedef struct {
    int ascent;
    int descent;
    int leading;
    int maxWidth;

    // If thie font is a raster font, then we can use compiled_glyph_drawer
    // and cache bitmaps.
    int is_raster;

    // The native FONT associated with this MIDP font.
    HFONT hfont;

    // The widths of the "fast" glyphs.
    unsigned char widths[NUM_FAST_GLYPHS];

    // The *bit* offset of the bitmap of each glyph.
    int bit_offsets[NUM_FAST_GLYPHS];

    // The compiled glyph drawers, for optimal font drawing speed.
    // Note: the default font (WinCE system font) should have all
    // glyphs compiled. This costs about 15KB of code. For all other
    // fonts, consider compiling only the glyphs for "JBenchmark".
    compiled_glyph_drawer drawers[NUM_FAST_GLYPHS];

    // The bitmaps for the "fast" glyphs.
    unsigned char *bitmaps;

} FastFontInfo;

// Cached information about the system font. We only cache the font bitmaps
// and widths of the printable ASCII characters (0x20 ~ 0x7e). We use
// WinCE API to query and draw the other characters.
//
// FIXME: we should support other font styles, such as italic, monospace
// and bold.
static FastFontInfo systemFontInfo;

// A counter to check the size of the JIT code.
static int totalcode = 0;

extern int gfFontInit = 0;

static int CvtRef(int refPoint)
{
    int v = 0;

    switch (refPoint & (TOP | BOTTOM | BASELINE)) {
    case BOTTOM:
        v |= TA_BOTTOM;
	break;

    case BASELINE:
        v |= TA_BASELINE;
	break;

    case TOP:
    default:
        v |= TA_TOP;
	break;
    }

    switch (refPoint & (LEFT | RIGHT | HCENTER)) {
    case RIGHT:
        v |= TA_RIGHT;
	break;

    case HCENTER:
        v |= TA_CENTER;
	break;

    case LEFT:
    default:
        v |= TA_LEFT;
        break;
    }

    return v;
}

#define MAX_GLYPH_WIDTH 40
#define MAX_GLYPH_HEIGHT 40
#define MAX_CODE 1000

#define R0 0
#define R1 1
#define R2 2
#define LR 14
#define PC 15
#define AL 0x0e

static int add_imm(int reg, int value) {
    int opcode;
    if (value > 0) {
        opcode = 0x04; // add
    } else {
        opcode = 0x02; // sub
        value = -value;
    }
    return (int)(AL << 28 | 1 << 25 | opcode << 21 | reg << 16 | reg << 12 |
                 value);
}

static int strh(int reg_value, int reg_addr, int offset) {
    return (int)(AL << 28 | 1 << 24 | 1 << 23 | 2 << 21 |
                 reg_addr << 16 | 
                 reg_value << 12 |
                 (offset & 0xf0) << 4 |
                 0x0b << 4 | (offset & 0x0f));
}

static int strh_preindexed(int reg_value, int reg_addr, int offset) {
    return (int)(AL << 28 | 1 << 24 | 1 << 23 | 3 << 21 |
                 reg_addr << 16 | 
                 reg_value << 12 |
                 (offset & 0xf0) << 4 |
                 0x0b << 4 | (offset & 0x0f));
}

static int mov(int reg_dst, int reg_src) {
    return (int)(AL << 28 | 0x0d << 21 | reg_dst << 12 | reg_src);
}

static int mov_imm(int reg_dst, int imm) {
    return (int)(AL << 28 | 1 << 25 | 0x0d << 21 | reg_dst << 12 | imm);
}

static int add(int reg, int reg_rm) {
    return (int)(AL << 28 | 0x04 << 21 | reg << 16 | reg << 12 |reg_rm);
}

// This is a feeble attemp to deal with anti-aliased fonts. It doesn't
// really work because in some cases the 'important' pixels in an anti-
// aliased character may not reach the level of intensity required by
// the formular below.
static int is_pixel_set(gxj_pixel_type pixel) {
    int r = GET_RED_FROM_PIXEL(pixel);
    int g = GET_GREEN_FROM_PIXEL(pixel);
    int b = GET_BLUE_FROM_PIXEL(pixel);
    if ((r + g + b) < (128 * 3 + 40)) {
        return 1;
    } else {
        return 0;
    }
}

int cache_bitmap_and_compile(FastFontInfo* info, HFONT font, int height,
                             int maxWidth, int totalWidth)
{
    int buf_bytes = sizeof(gxj_pixel_type) * maxWidth * height;
    gxj_pixel_type *buf = (gxj_pixel_type*)pcsl_mem_malloc(buf_bytes);
    HDC hdc;
    int i, x, y, max_codes;
    int *scratch_code;
    int bitmap_bytes = (totalWidth * height + 7) / 8;
    unsigned char *bitmap = (unsigned char*)pcsl_mem_malloc(bitmap_bytes);
    unsigned int bitbyte = 0;
    int bits = 0;

    if (info == NULL) {
        return 0;
    }

    hdc = getScreenBufferHDC(buf, maxWidth, height);
    if (hdc == NULL) {
        pcsl_mem_free(buf);
        return 0;
    }

    info->bitmaps = bitmap;

    // In the worst case, for each scan-line we need <maxWidth> instructions
    // plus 2 instructions to wrap to the next line. We also need two
    // more instructions to return from the function.
    max_codes = (maxWidth + 2) * height + 2 + /*slack*/ 40;
    scratch_code = (int*)pcsl_mem_malloc(sizeof(int) * max_codes);

    SelectObject(hdc, font);
    SetTextColor(hdc, 0x0);
    SetBkMode(hdc, TRANSPARENT);

    for (i=MIN_FAST_GLYPH; i<MAX_FAST_GLYPH; i++) {
        TCHAR c = (TCHAR)i;
        int fontWidth;
        int *func;

        memset(buf, 0x0ff, buf_bytes);
        ExtTextOut(hdc, 0, 0, 0, NULL, &c, 1, NULL);

        fontWidth = info->widths[FASTIDX(i)];
        func = compile_glyph(buf, maxWidth, fontWidth, height, scratch_code);
        info->drawers[FASTIDX(i)] = (compiled_glyph_drawer)func;

        for (y=0; y < height; y++) {
            gxj_pixel_type *b = &buf[y * maxWidth];
            for (x=0; x<fontWidth; x++) {
                if (is_pixel_set(b[x])) { // the pixel is set
                    bitbyte |= (1 << (7-bits));
                }
                bits ++;
                if (bits == 8) {
                    *bitmap++ = (unsigned char)bitbyte;
                    bits = 0;
                    bitbyte = 0;
                }
            }
        }
    }

    if (bits != 0) {
        *bitmap++ = (unsigned char)bitbyte;
    }

    pcsl_mem_free(scratch_code);
    pcsl_mem_free(buf);
    FlushInstructionCache(GetCurrentProcess(), 0, 0);

    return 1;
}

static int *compile_glyph(gxj_pixel_type *buf, int bufWidth, int fontWidth,
                          int height, int *scratch_code) {
    int x, y, maxY;
    int nextcode = 0;
    int *func;

    // (1) Find the last non-empty line. This save a few instructions
    //     for the glyphs that have no descending pixels.
    for (maxY = height-1; maxY >= 0; maxY --) {
        gxj_pixel_type *p = &buf[maxY * bufWidth];
        int empty = 1;
        for (x=0; x<fontWidth; x++) {
            if (p[x] == 0) {
                empty = 0;
                break;
            }
        }
        if (!empty) {
            break;
        }
    }

    for (y=0; y<=maxY; y++) {
        int cursor = 0;
        gxj_pixel_type *b = &buf[y * bufWidth];

        for (x=0; x<fontWidth; x++) {
            if (is_pixel_set(b[x])) { // this pixel is set.
                int distance;

                if (cursor == 0) {
                    // Check if this scanline has a single pixel. If so,
                    // we can save one instruction.
                    int k;
                    for (k=x+1; k<fontWidth; k++) {
                        if (is_pixel_set(b[k])) {
                            break;
                        }
                    }
                    if (k == fontWidth) {
                        // This scanline has only one pixel
                        scratch_code[nextcode++] = strh(R1, R0, x*2);
                        break;
                    }
                }

                distance = x - cursor;
                scratch_code[nextcode++] = strh_preindexed(R1, R0, distance*2);
                cursor = x;
            }
        }
        if (cursor > 0) {
            scratch_code[nextcode++]= add_imm(R0, -cursor * sizeof(gxj_pixel_type));
        }
        if (y != maxY) {
            scratch_code[nextcode++] = add(R0, R2);
        }
    }

    scratch_code[nextcode++] = mov_imm(R0, fontWidth);
    scratch_code[nextcode++] = mov(PC, LR);

    func = (int*)pcsl_mem_malloc(sizeof(int) * nextcode);
    memcpy(func, scratch_code, sizeof(int) * nextcode);
    return func;
}

static int init_font(FastFontInfo *info, HFONT hfont, int is_raster) {
    HWND hwnd = (HWND)winceapp_get_window_handle();
    HDC hdc = GetDC(hwnd);
    HFONT old;
    TEXTMETRIC metric;
    int i, height, totalWidth, maxWidth;

    if (info == NULL) {
        return 0;
    }

    if (hdc == NULL) {
        printf("init_font() ERROR! hdc is NULL!\n");
        return 0;
    }

    info->hfont = hfont;
    info->is_raster = is_raster;
    old = SelectObject(hdc, info->hfont);

    // (1) Get the basic font information
    if (!GetTextMetrics(hdc, &metric)) {
        printf("init_font() ERROR! 0x%X calling GetTextMetrics()\n",
               GetLastError());
        return 0;
    }

    info->ascent  = metric.tmAscent;
    info->descent = metric.tmDescent;
    info->leading = metric.tmExternalLeading;

    height = metric.tmAscent + metric.tmDescent;
    totalWidth = 0;
    maxWidth = 0;

    // (2) Get the widths of each glyph, and also compute its bit offset.
    for (i=MIN_FAST_GLYPH; i<MAX_FAST_GLYPH; i++) {
        int width;
        TCHAR c = (TCHAR)i;
        SIZE size;

        if (!GetTextExtentPoint32W(hdc, &c, 1, &size)) {
            printf("init_font() ERROR! 0x%X calling GetTextExtentPoint32W()\n", GetLastError());
        }
        width = (int)size.cx;

        info->widths[FASTIDX(i)] = (unsigned char)width;
        info->bit_offsets[FASTIDX(i)] = height * totalWidth;
        totalWidth += width;

        if (maxWidth < width) {
            maxWidth = width;
        }
    }

    SelectObject(hdc, old);
    ReleaseDC(hwnd, hdc);

    if (maxWidth & 0x01) maxWidth++; //make it couple, so, the buf inisde  cache_bitmap_and_compile() will be 4-bytes aligned.
    info->maxWidth = maxWidth;

    if (is_raster) {
        // (3) Cache the font bitmap and compile the hot glyphs
        return cache_bitmap_and_compile(info, hfont, height, maxWidth,
                                        totalWidth);
    } else {
        return 1;
    }
}

static unsigned char BitMask[8] = {0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};

static void drawCharImpl(gxj_screen_buffer *sbuf,
       jchar c,
       gxj_pixel_type pixelColor,
       int x, int y,
       int xSource, int ySource, int xLimit, int yLimit,
       unsigned char *fontbitmap, unsigned long mapLen,
       int fontWidth, int fontHeight) {
   unsigned int byte1, byte2;
   register unsigned int bitmask;
   int N;
   unsigned long byteIndex;
   int bitOffset, xSteps, yDestLimit;
   unsigned long pixelIndex;
   unsigned long firstPixelIndex;
   register gxj_pixel_type *ptr;
   int sbufWidth;
   gxj_pixel_type *sbufPixelData;
   int dstMask;

   if (c == ' ') {
       return; // hack -- let assume that space is always empty in all
               // charsets.
   }

#if USE_NATIVE_FONT
   if (IS_FAST_GLYPH(c)) {
       firstPixelIndex = systemFontInfo.bit_offsets[FASTIDX(c)];
       fontbitmap = systemFontInfo.bitmaps;
       fontWidth = systemFontInfo.widths[FASTIDX(c)];
   } else {
       return;
   }
#else
   firstPixelIndex = (FONT_DATA * 8) + (c * fontHeight * fontWidth);
   if ((firstPixelIndex / 8) >= mapLen) {
       /* this character is not supported. */
       return;
   }
#endif

   if (fontbitmap == NULL) {
       return;
   }

   sbufWidth = sbuf->width;
   sbufPixelData = sbuf->pixelData;

   pixelIndex = firstPixelIndex + ySource * fontWidth + xSource;
   pixelIndex -= fontWidth;

   /* Number of pixels to draw in X direction (1-16) */
   xSteps = xLimit - xSource;

   /*
    * This mask is to check if all the relevant bits in bitmask
    * are zero. If so, we can skip the (unrolled) inner loop altogether.
    * This happens frequently with lower-case ASCII characters.
    */
   dstMask = ((int)(1 << 31)) >> (16 + xSteps - 1);
   dstMask &= 0x0000ffff;

   yDestLimit = y + (yLimit - ySource);
   N = y * sbufWidth + x + xSteps - 1;

   for (; y < yDestLimit; y++, N += sbufWidth) {
       ptr = &sbufPixelData[N];

       pixelIndex += fontWidth;
       byteIndex = pixelIndex / 8;

       byte1 = (unsigned int)fontbitmap[byteIndex];
       byte2 = (unsigned int)fontbitmap[byteIndex+1];

       bitOffset = pixelIndex % 8;
       bitmask = (byte1 << (8 + bitOffset)) | (byte2 << bitOffset);

#define UNROLLED_STEP(n) \
       if (bitmask & (1 << n)) { \
          *ptr = pixelColor; \
       } \
       ptr --

       if ((dstMask & bitmask) != 0) {
           switch (xSteps) {
           case 16: UNROLLED_STEP( 0);
           case 15: UNROLLED_STEP( 1);
           case 14: UNROLLED_STEP( 2);
           case 13: UNROLLED_STEP( 3);
           case 12: UNROLLED_STEP( 4);
           case 11: UNROLLED_STEP( 5);
           case 10: UNROLLED_STEP( 6);
           case  9: UNROLLED_STEP( 7);
           case  8: UNROLLED_STEP( 8);
           case  7: UNROLLED_STEP( 9);
           case  6: UNROLLED_STEP(10);
           case  5: UNROLLED_STEP(11);
           case  4: UNROLLED_STEP(12);
           case  3: UNROLLED_STEP(13);
           case  2: UNROLLED_STEP(14);
           case  1: UNROLLED_STEP(15);
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
 * @param charArray Pointer to the characters to be drawn
 * @param n The number of characters to be drawn
 */
void
gx_port_draw_chars(jint pixel, const jshort *clip, 
                   gxj_screen_buffer *dest,
                   int dotted,
                   int face, int style, int size,
                   int x, int y, int anchor, 
                   const jchar *charArray, int n) {
    int i;
    int xStart;
    int xDest;
    int yDest;
    int width;
    int yLimit;
    int nCharsToSkip = 0;
    int xCharSource;
    int widthRemaining;
    int yCharSource;
    int fontWidth;
    int fontHeight;
    int fontDescent;
    int clipX1 = clip[0];
    int clipY1 = clip[1];
    int clipX2 = clip[2];
    int clipY2 = clip[3];
    int diff;

    REPORT_CALL_TRACE(LC_LOWUI, "LCDUIdrawChars()\n");

    if (n <= 0) {
        /* nothing to do */
        return;
    }

    if (dest == NULL) {
        printf("ERROR: gx_port_draw_chars(): dest == NULL!\n");
        return;
    }

#if USE_NATIVE_FONT
    {
        HDC hdc;
        FastFontInfo *info = &systemFontInfo;

        if (info->is_raster &&
            anchor == (TOP | LEFT) && 
            clipX1 <= 0 && clipY1 <= 0 && 
            clipX2 >= dest->width && clipY2 >= dest->height) {

            int H = info->ascent + info->descent;

            if (x >= 0 && y >= 0 && y + H < dest->height) {
                int destWidth = dest->width;
                gxj_pixel_type *d = &dest->pixelData[y * destWidth + x];
                compiled_glyph_drawer drawer;

                for (; n>0; n--, charArray++) {
                    jchar c = *charArray;
                    int W = info->widths[FASTIDX(c)];

                    if (!IS_FAST_GLYPH(c)) {
                        break;
                    }
                    if (x + W >= destWidth) {
                        break;
                    }
                    drawer = info->drawers[FASTIDX(c)];
                    if (drawer == NULL) {
                        break;
                    }
                    (drawer)(d, pixel, destWidth*2);
                    x += W;
                    d += W;
                }

                if (n == 0) {
                    return;
                }
            }
        }

        for (i=0; i<n; i++) {
            jchar c = charArray[i];
            if (!IS_FAST_GLYPH(c)) {
                break;
            }
        }

        if (i != n || !info->is_raster) {
            printf("gx_port_draw_chars() USE THE SLOW WAY...\n");
            // There is at least one glyph that we don't have cached bitmap
            // data. Let's call Windows to draw it (slow!)
            //
            // Or, if the system doesn't use raster font, we must use
            // ExtTextOut().
            hdc = getScreenBufferHDC(dest->pixelData, dest->width,
                                     dest->height);
            if (hdc != NULL) {
                RECT rect;
                HFONT old;
                rect.left   = clipX1;
                rect.top    = clipY1;
                rect.right  = clipX2;
                rect.bottom = clipY2;
                old = SelectObject(hdc,info->hfont);
                SetTextAlign(hdc, CvtRef(anchor));
                SetTextColor(hdc, RGB16TORGB24(pixel));
                SetBkMode(hdc, TRANSPARENT);
                ExtTextOut(hdc, x, y, ETO_CLIPPED, &rect, charArray, n, NULL);
                SelectObject(hdc, old);
                return;
            }
        }
    }
#endif

//    REPORT_CALL_TRACE(LC_LOWUI, "LCDUIdrawChars()\n");

//    if (n <= 0) {
        /* nothing to do */
//        return;
//    }

#if USE_NATIVE_FONT
    fontHeight = systemFontInfo.ascent + systemFontInfo.descent;
    fontDescent = systemFontInfo.descent;
#else
    fontHeight = TheFontBitmap[FONT_HEIGHT];
    fontDescent = TheFontBitmap[FONT_DESCENT];
#endif

    width = gx_port_get_charswidth(FONTPARAMS, charArray, n);

    xDest = x;
    if (anchor & RIGHT) {
        xDest -= width;
    }

    if (anchor & HCENTER) {
        xDest -= ((width) / 2);
    }

    yDest = y;  
    if (anchor & BOTTOM) {
        yDest -= fontHeight;
    }

    if (anchor & BASELINE) {
        yDest -= fontHeight - fontDescent;
    }

    yLimit = fontHeight;

    xStart = 0;
    xCharSource = 0;
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
        width -= diff;
        xDest += diff;

        while (diff > 0 && n > 0) {
            int w = gx_port_get_charswidth(FONTPARAMS, charArray, 1);
            if (diff < w) {
                xStart = diff;
                break;
            }
            diff -= w;
            charArray++;
            n--;
        }
        if (n <= 0) {
            return;
        }
    }

    diff = (xDest + width) - clipX2;
    if (diff > 0) {
        width -= diff;
        while (diff > 0 && n > 0) {
            int w = gx_port_get_charswidth(FONTPARAMS, charArray+n-1, 1);
            if (diff < w) {
                break;
            }
            diff -= w;
            n--;
        }
        if (n <= 0) {
            return;
        }
    }

    diff = clipY1 - yDest;
    if (diff > 0) {
        yCharSource += diff;
        yDest += diff;
    }

    diff = (yDest + fontHeight) - clipY2;
    if (diff > 0) {
        yLimit -= diff;
    }

    if (width <= 0 || yCharSource >= yLimit || nCharsToSkip >= n) {
        /* Nothing to do. */
        return;
    }

    widthRemaining = width;

    if (xStart != 0) {
        int startWidth;
        fontWidth = gx_port_get_charswidth(FONTPARAMS, charArray, 1);
        startWidth = fontWidth - xStart;

        /* Clipped, draw the right part of the first char. */
        drawCharImpl(dest,
            charArray[nCharsToSkip], (gxj_pixel_type)pixel, xDest, yDest,
            xStart, yCharSource, fontWidth, yLimit,
            TheFontBitmap, TheFontBitmapSize,
            fontWidth, fontHeight);
        nCharsToSkip++;
        xDest += startWidth;
        widthRemaining -= startWidth;
    }

    /* Draw all the fully wide chars. */
    for (i = nCharsToSkip; i < n; i++) {
        fontWidth = gx_port_get_charswidth(FONTPARAMS, charArray+i, 1);
        if (widthRemaining < fontWidth) {
            break;
        }
        // TODO: if the character is not clipped vertically, use
        // the compiled_glyph_drawer instead.
        drawCharImpl(dest,
            charArray[i], (gxj_pixel_type)pixel, xDest, yDest,
            0, yCharSource, fontWidth, yLimit,
            TheFontBitmap, TheFontBitmapSize,
            fontWidth, fontHeight);
        widthRemaining -= fontWidth;
        xDest += fontWidth;
    }

    if (i < n && widthRemaining > 0) {
        fontWidth = gx_port_get_charswidth(FONTPARAMS, charArray+i, 1);
        /* Clipped, draw the left part of the last char. */
        drawCharImpl(dest,
            charArray[i], (gxj_pixel_type)pixel, xDest, yDest,
            0, yCharSource, widthRemaining, yLimit,
            TheFontBitmap, TheFontBitmapSize,
            fontWidth, fontHeight);
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
gx_port_get_fontinfo(FONTPARAMS_PROTO, int *ascent,
                     int *descent, int *leading) {
    REPORT_CALL_TRACE(LC_LOWUI, "LCDUIgetFontInfo()\n");
    /* Surpress unused parameter warnings */
    (void)face;
    (void)size;
    (void)style;

#if USE_NATIVE_FONT
    {
        FastFontInfo *info = &systemFontInfo;

        *ascent  = info->ascent;
        *descent = info->descent;
        *leading = info->leading;
    }

#else

    *ascent  = TheFontBitmap[FONT_ASCENT];
    *descent = TheFontBitmap[FONT_DESCENT];
    *leading = TheFontBitmap[FONT_LEADING];

#endif
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
gx_port_get_charswidth(int face, int style, int size, 
                       const jchar *charArray, int n) {
#if USE_NATIVE_FONT
    {
        HWND hwnd;
        HDC hdc;
        HFONT old;
        SIZE size;
        int w = 0;
        int i;
        FastFontInfo *info = &systemFontInfo;

        for (i=0; i<n; i++) {
            jchar c = charArray[i];
            if (IS_FAST_GLYPH(c)) {
                w += (int)info->widths[FASTIDX(c)];
            } else {
                break;
            }
        }

        if (i == n) {
            return w;
        }

        hwnd = (HWND)winceapp_get_window_handle();
        hdc = GetDC(hwnd);

        if (hdc != NULL) {
            int retval;
            old = SelectObject(hdc, info->hfont);
            GetTextExtentPoint32W(hdc, charArray, n, &size);
            retval = size.cx;

            SelectObject(hdc, old);
            ReleaseDC(hwnd, hdc);

            return retval;
        }
    }
#endif

    /* Surpress unused parameter warnings */
    (void)face;
    (void)size;
    (void)style;
    (void)charArray;

    return n * TheFontBitmap[FONT_WIDTH];
}

int wince_init_fonts() {
#if USE_NATIVE_FONT

    HFONT font;
    LOGFONT logfont;
    int height;
    int is_raster;
    int screenWidth = winceapp_get_screen_width();
    int screenHeight = winceapp_get_screen_height();
    BOOLEAN fPocketPC = TRUE;
    TCHAR szPlatform[80];
    BOOLEAN bSmooth = FALSE;

    // Find out if this is a PocketPC or Smartphone platform
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(szPlatform)/sizeof(TCHAR), szPlatform, 0)) {
        if (0 == wcsicmp(szPlatform, TEXT("smartphone"))) {
            fPocketPC = FALSE;
        }
    }

    printf("wince_init_fonts() Platform=%s, screenWidth=%d, screenHeight=%d\n",
        fPocketPC ? "PocketPC" : "Smartphone", screenWidth, screenHeight);

    // FIXME: the font size and raster-ness should be stored in a
    // configuration file so that it's easy to control without
    // recompilation.
    if (screenWidth > 320) {
        is_raster = 0; // FIXME: do not hard code
        height = 24;
    } else if (fPocketPC) {
        is_raster = 1; // FIXME: do not hard code
        height = 16; //set it to the value suite for your device
    } else {
        is_raster = 1;
        height = 16;
    }

    // Note: according to MSDN, "Windows CE 2.0 and later support systems
    // that use either TrueType or raster fonts, but not both. The OEM
    // chooses the font type, raster or TrueType, at system design time,
    // and the application cannot be change the font type."
    //
    // So if we are running on a phone without raster font support, we cannot
    // use bitmap caching (unless we don't use native font but use
    // the default font from JWC).
#if 1
    font = (HFONT)GetStockObject(SYSTEM_FONT);
    if (NULL == font) {
        printf("wince_init_fonts() ERROR! 0x%X calling GetStockObject()\n",GetLastError());
    }

    SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &bSmooth, FALSE);
    if (bSmooth) {
        SystemParametersInfo(SPI_SETFONTSMOOTHING, 0, NULL, SPIF_UPDATEINIFILE);
    }

    GetObject(font, sizeof(LOGFONT), &logfont);
    logfont.lfHeight         = height;
    logfont.lfOutPrecision   = OUT_RASTER_PRECIS;
    logfont.lfQuality        = DRAFT_QUALITY;
    font = CreateFontIndirect(&logfont);
#else
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight         = height;
    logfont.lfWidth          = 0;
    logfont.lfEscapement     = 0;
    logfont.lfOrientation    = 0; 
    logfont.lfWeight         = FW_NORMAL;
    logfont.lfItalic         = 0;
    logfont.lfUnderline      = 0;
    logfont.lfStrikeOut      = 0;
    logfont.lfCharSet        = DEFAULT_CHARSET;
    logfont.lfOutPrecision   = OUT_RASTER_PRECIS;
    logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality        = DRAFT_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    logfont.lfFaceName[0]    = 0;

    font = CreateFontIndirect(&logfont);
#endif

    if (font == NULL) {
        printf("wince_init_fonts(): ERROR! font is NULL!\n");
        return 0;
    }

    // FIXME: support fonts of other styles as well.
    init_font(&systemFontInfo, font, is_raster);
    if (bSmooth) {
        // restore to old value
        SystemParametersInfo(SPI_SETFONTSMOOTHING,
                             bSmooth, NULL, SPIF_UPDATEINIFILE);
    }
    return 1;
#else
    return 0;
#endif
}

/*
 * Free the font resource used when the JVM app exits
 * IMPL_NOTE: call it when exiting from MIDP.
 */
/*
void javaImpl_font_finalize() {
    if (systemFontInfo.hfont!= NULL) {
        DeleteObject(systemFontInfo.hfont);
        systemFontInfo.hfont = NULL;
    }
    if (systemFontInfo.bitmaps != NULL) {
        free(systemFontInfo.bitmaps);
        systemFontInfo.bitmaps = NULL;
    }
}
*/

#ifdef __cplusplus
} // extern "C"
#endif
