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

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "gxj_intern_font_bitmap.h"

#define BUFSIZE 256
#define MAXFONTBITMAPCOUNT 1024
#define ECHO 0

unsigned char BitMask[8] = {0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};

char buf[BUFSIZE];
int lineno = 0;
int lastchar = -1;

unsigned char fontbitmap_common_header[FONT_DATA];
unsigned char *fontbitmaps[MAXFONTBITMAPCOUNT];
unsigned char *fontbitmap_current = NULL;
int rangeIndex=-1;
int mapLen[MAXFONTBITMAPCOUNT];

int fontWidth, fontHeight, ascent, descent, leading;
int range_hi, first_lo, last_lo;

void check_size(int range);

int define_file()
{
    sscanf(buf+1, "%i%i%i%i%i", &fontWidth, &fontHeight, &ascent, &descent, &leading);
    fontbitmap_common_header[FONT_WIDTH] = fontWidth;
    fontbitmap_common_header[FONT_HEIGHT] = fontHeight;
    fontbitmap_common_header[FONT_ASCENT] = ascent;
    fontbitmap_common_header[FONT_DESCENT] = descent;
    fontbitmap_common_header[FONT_LEADING] = leading;
}

int define_range()
{
    if (rangeIndex >= 0)
        check_size(rangeIndex);
    sscanf(buf+1, "%x%x%x", &range_hi, &first_lo, &last_lo);
    if (range_hi > 0xff || first_lo > 0xff || last_lo > 0xff) {
        fprintf(stderr, "Incorrect character range. Note that surrogate characters are not supported");
        return -1;
    }
    if (range_hi >= 0xd8 && range_hi <= 0xdf) {
        fprintf(stderr, "Unexpected surrogate code point specification encountered. Note that surrogate characters are not supported");
        return -1;
    }
    fontbitmap_current = (unsigned char*)malloc(
        (last_lo-first_lo+1) * ((fontWidth * fontHeight + 7) / 8) + FONT_DATA);
    if (fontbitmap_current == NULL) {
        fprintf(stderr, "Memory exhausted");
        return -1;
    }
    memset(fontbitmap_current,0,(last_lo-first_lo+1) * ((fontWidth * fontHeight + 7) / 8) + FONT_DATA);
    memcpy(fontbitmap_current, fontbitmap_common_header, FONT_DATA);
    fontbitmap_current[FONT_CODE_RANGE_HIGH] = range_hi;
    fontbitmap_current[FONT_CODE_FIRST_LOW] = first_lo;
    fontbitmap_current[FONT_CODE_LAST_LOW] = last_lo;
    lastchar = range_hi*0x100 + first_lo -1;
    rangeIndex++;
    if (rangeIndex == MAXFONTBITMAPCOUNT) {
        fprintf(stderr, "Maximum supported number of ranges (%i) exceeded", MAXFONTBITMAPCOUNT);
        return -1;
    }
    fontbitmaps[rangeIndex] = fontbitmap_current;
    mapLen[rangeIndex] = 0;
    return 0;
}


int get_any_line()
{
    if( NULL != fgets(buf,sizeof(buf),stdin) ) {
        lineno++;
        return 1;
    } else {
        return 0;
    }
}

/* skip empty and comment lines */
int get_line()
{
    int rc;
    do {
        rc = get_any_line();
        int j = strlen(buf)-1;
        while ( j>=0 && buf[j]<=' ' ) buf[j--] = 0;
    } while ( rc && (buf[0] == '#' || buf[0] == 0) );
#if ECHO
    fprintf(stderr,"%i: [%s]\n",lineno,buf);
#endif
    return rc;
}

void define_char()
{
    int newchar;
    sscanf(buf+1,"%x",&newchar);
    if( newchar != lastchar+1 ) {
        fprintf(stderr, "warning at line %i defining character %i(0x%x) after character %i(0x%x)\n",
                lineno, newchar, newchar, lastchar, lastchar);
    }
    if( newchar < range_hi*0x100 + first_lo || newchar > range_hi*0x100 + last_lo ) {
        fprintf(stderr, "error at line %i character %i(0x%x) out of range 0x%x..0x%x\n",
                lineno, newchar, newchar, range_hi*0x100 + first_lo, range_hi*0x100 + last_lo);
        return;
    }
    lastchar = newchar;
    unsigned int c = newchar - (range_hi*0x100 + first_lo);
    unsigned long firstPixelIndex =
        (FONT_DATA * 8) + (c * fontHeight * fontWidth);

    int i,j;
    for (i = 0; i < fontHeight; i++) {
        if( get_line() && buf[fontWidth] == '.' )
        {
            for (j = 0; j < fontWidth; j++) {
                const int pixelIndex = firstPixelIndex + (i * fontWidth) + j;
                const int byteIndex = pixelIndex / 8;

                if (byteIndex >= mapLen[rangeIndex]) {
                    mapLen[rangeIndex] = byteIndex+1;
                }

                const int bitOffset = pixelIndex % 8;

                /* we don't draw "background" pixels, only foreground */
                if (buf[j] == '*') {
                    fontbitmap_current[byteIndex] |= BitMask[bitOffset];
                }
            }
        } else {
            fprintf(stderr,"error at line %i: missing data (wrong char '%c' in \"%s\" at col %i)\n",
                    lineno, buf[fontWidth], buf, fontWidth);
            return;
        }
    }
}


void process_file()
{
    while( get_line() )
    {
	switch( buf[0] )
	{
	// '#' for comments
	case '@':
	    define_file();
	    break;
	case ':':
	    define_char();
	    break;
	case 0:
	    break;
	case '%':
	    if (define_range() < 0) {
            fprintf(stderr, "cannot continue");
            return;
        }
	    break;
	default:
	    fprintf(stderr,"error at line %i: bad first char in [%s]\n",lineno,buf);
	    break;
	}
    }
    check_size(rangeIndex);
}

void print_bitmap()
{
    int i;
    int printRangeIndex;
    char heading[] =
        "/*\n"
        " *\n"
        " *\n"
        " * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.\n"
        " * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER\n"
        " * \n"
        " * This program is free software; you can redistribute it and/or\n"
        " * modify it under the terms of the GNU General Public License version\n"
        " * 2 only, as published by the Free Software Foundation.\n"
        " * \n"
        " * This program is distributed in the hope that it will be useful, but\n"
        " * WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n"
        " * General Public License version 2 for more details (a copy is\n"
        " * included at /legal/license.txt).\n"
        " * \n"
        " * You should have received a copy of the GNU General Public License\n"
        " * version 2 along with this work; if not, write to the Free Software\n"
        " * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA\n"
        " * 02110-1301 USA\n"
        " * \n"
        " * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa\n"
        " * Clara, CA 95054 or visit www.sun.com if you need additional\n"
        " * information or have any questions.\n"
        " */\n"
        "\n"
        "#include \"gxj_intern_font_bitmap.h\"\n";

    printf("%s", heading);
    for (printRangeIndex = 0; printRangeIndex <= rangeIndex; printRangeIndex++) {
        fontbitmap_current = fontbitmaps[printRangeIndex];
        printf("// starts off with width, height, ascent, descent, leading, "
               "range_high_byte, first_code_low_byte, last_code_low_byte, "
               "then data\n"
               "unsigned char TheFontBitmap%02x%02x[%i] = {\n",
               fontbitmap_current[FONT_CODE_RANGE_HIGH],
               fontbitmap_current[FONT_CODE_FIRST_LOW],
               mapLen[printRangeIndex]);
        for(i=0;i<mapLen[printRangeIndex];i++) {
            if(i==FONT_DATA) {
                printf("/* data starts here */\n");
            } else if((i-FONT_DATA)%16==0) {
                printf("\n");
            }
            printf("0x%02x,",fontbitmap_current[i]);
        }
        printf("\n};\n\n");
    }
    printf("pfontbitmap FontBitmaps[] =\n"
           "{ (pfontbitmap)%i",
           rangeIndex+1);
    for (printRangeIndex = 0; printRangeIndex <= rangeIndex; printRangeIndex++) {
        fontbitmap_current = fontbitmaps[printRangeIndex];
        printf(", TheFontBitmap%02x%02x",
               fontbitmap_current[FONT_CODE_RANGE_HIGH],
               fontbitmap_current[FONT_CODE_FIRST_LOW]);
        free(fontbitmap_current);
    }
    printf(" };\n");

}
void check_size(int range)
{
    int calculated_size = ((last_lo-first_lo+1)*fontWidth*fontHeight+7)/8+FONT_DATA;
    fprintf(stderr, "info predicted table size is %i bytes\n", calculated_size);
    if( calculated_size != mapLen[range]) {
	fprintf(stderr, "error calculated size mismatch (bug in our code): %i vs %i\n",
            calculated_size, mapLen[range]);
    }
}
main()
{
    process_file();
    print_bitmap();
}
