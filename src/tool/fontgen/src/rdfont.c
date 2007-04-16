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

#include <string.h>
#include <stdio.h>
#include "gxj_font_bitmap.c.0"

#define BUFY 20
#define BUFX 20


char buf[BUFY][BUFX];

unsigned char BitMask[8] = {0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};

int width;
int height;

static void drawChar(unsigned char c, unsigned char *fontbitmap, unsigned long mapLen,
                     int fontWidth, int fontHeight) {

    int i;
    int j;
    int xDest;
    int yDest;
    unsigned long byteIndex;
    int bitOffset;
    unsigned long pixelIndex;
    unsigned char bitmapByte;
    unsigned long firstPixelIndex =
        (FONT_DATA * 8) + (c * fontHeight * fontWidth);

    memset(buf,' ',sizeof(buf));

    for (i = 0; i < fontHeight; i++) {
        for (j = 0; j < fontWidth; j++) {
            pixelIndex = firstPixelIndex + (i * fontWidth) + j;
            byteIndex = pixelIndex / 8;

            if (byteIndex >= mapLen) {
                break;
            }

            bitmapByte = fontbitmap[byteIndex];
            bitOffset = pixelIndex % 8;

            /* we don't draw "background" pixels, only foreground */
            if ((bitmapByte & BitMask[bitOffset]) != 0) {
		buf[j][i]='*';
            }
        }
    }
    
    printf("#----------\n");
    printf(": %x\n",c);
    printf("#----------\n");
    for(i=0;i<fontHeight;i++) {
	for(j=0;j<fontWidth;j++) {
	    printf("%c",buf[j][i]);
	}
	printf(".\n");
    }
    printf("#----------\n");
}

int main()
{
  width = TheFontBitmap[FONT_WIDTH];
  height = TheFontBitmap[FONT_HEIGHT];
  printf("# Font parameters:\n");
  printf("# width height ascent descent leading\n");
  printf("@ %i %i %i %i %i\n",width, height,TheFontBitmap[FONT_ASCENT],TheFontBitmap[FONT_DESCENT],TheFontBitmap[FONT_LEADING]);
  int lastchar = (sizeof(TheFontBitmap)-FONT_DATA)*8 / (width*height);
  int i;
  for(i=0;i<lastchar;i++) {
      drawChar(i,TheFontBitmap,sizeof(TheFontBitmap),width,height);
  }
}

