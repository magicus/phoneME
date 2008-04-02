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
#include <malloc.h>
#include "gxj_intern_font_bitmap.h"

#define BUFSIZE 256
#define FONTBITMAPSIZE 1024*1024
#define ECHO 0

// TEST means that we want compatibility with the previous version
// (to be able to compare output with the original code)
#define TEST 0
#define V1_1 1

unsigned char BitMask[8] = {0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1};

char buf[BUFSIZE];
int lineno = 0;
int lastchar = -1;

unsigned char *fontbitmap = NULL;
int mapLen=0;

int fontWidth, fontHeight, ascent, descent, leading;
#if V1_1
int range_hi, first_lo, last_lo;
#endif

int define_file()
{
    sscanf(buf+1, "%i%i%i%i%i", &fontWidth, &fontHeight, &ascent, &descent, &leading);
    fontbitmap[FONT_WIDTH] = fontWidth;
    fontbitmap[FONT_HEIGHT] = fontHeight;
    fontbitmap[FONT_ASCENT] = ascent;
    fontbitmap[FONT_DESCENT] = descent;
    fontbitmap[FONT_LEADING] = leading;
}
#define FONT_CODE_RANGE_HIGH 5
#define FONT_CODE_FIRST_LOW 6
/* inclusive */
#define FONT_CODE_LAST_LOW 7

#if V1_1
int define_range()
{
    sscanf(buf+1, "%x%x%x", &range_hi, &first_lo, &last_lo);
    fontbitmap[FONT_CODE_RANGE_HIGH] = range_hi;
    fontbitmap[FONT_CODE_FIRST_LOW] = first_lo;
    fontbitmap[FONT_CODE_LAST_LOW] = last_lo;
    lastchar = range_hi*0x100 + first_lo -1;
}
#endif


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
	fprintf(stderr, "warning at line %i defining character %i(0x%x) after character %i(0x%x)\n", lineno, newchar, newchar, lastchar, lastchar);
    }
    if( newchar < range_hi*0x100 + first_lo || newchar > range_hi*0x100 + last_lo ) {
        fprintf(stderr, "error at line %i character %i(0x%x) out of range 0x%x..0x%x\n", lineno, newchar, newchar, range_hi*0x100 + first_lo, range_hi*0x100 + last_lo);
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

            if (byteIndex >= mapLen) {
                mapLen = byteIndex+1;
            }

// fprintf(stderr,"firstPixelIndex=%i pixelIndex=%i byteIndex=%i\n",firstPixelIndex,pixelIndex,byteIndex);

	    if (byteIndex >= FONTBITMAPSIZE) {
		fprintf(stderr,"error at line %i: FONTBITMAPSIZE too small: %i vs required %i\n",lineno, FONTBITMAPSIZE, byteIndex);
		return;
	    }

            //const int bitmapByte = fontbitmap[byteIndex];
            const int bitOffset = pixelIndex % 8;

            /* we don't draw "background" pixels, only foreground */
            if (buf[j] == '*') {
                fontbitmap[byteIndex] |= BitMask[bitOffset];
            }
	 }
        } else {
	    fprintf(stderr,"error at line %i: missing data (wrong char '%c' in \"%s\" at col %i)\n", lineno, buf[fontWidth], buf, fontWidth);
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
#if V1_1
	case '%':
	    define_range();
	    break;
#endif
	default:
	    fprintf(stderr,"error at line %i: bad first char in [%s]\n",lineno,buf);
	    break;
	}
    }
}

void print_bitmap()
{
    int i;
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
#if TEST
    printf("// starts off with width, height, ascent, descent, leading, "
#if V!_1
	   "range_high_byte, first_code_low_byte, last_code_low_byte, "
#endif
	   "then data\n"
	   "unsigned char TheFontBitmap[%i] = {",mapLen);
#else
    printf("// starts off with width, height, ascent, descent, leading, "
#if V1_1
	   "range_high_byte, first_code_low_byte, last_code_low_byte, "
#endif
           "then data\n"
	   "unsigned char TheFontBitmap[%i] = {\n",mapLen);
#endif    
    for(i=0;i<mapLen;i++) {
	if(i==FONT_DATA) {
#if TEST
	    printf("/*data starts here */ ");
#else
	    printf("/* data starts here */\n");
#endif
#if !TEST
	} else if((i-FONT_DATA)%16==0) {
	    printf("\n");
#else
#endif
	}
#if TEST
	printf("0x%x,",fontbitmap[i]);
#else
	printf("0x%02x,",fontbitmap[i]);
#endif    
    }
#if TEST
    printf("};\n");
#else
    printf("\n};\n");
#endif    
}
void check_size()
{
    int calculated_size = ((last_lo-first_lo+1)*fontWidth*fontHeight+7)/8+FONT_DATA;
    fprintf(stderr, "info predicted table size is %i bytes\n", calculated_size);
    if( calculated_size != mapLen) {
	fprintf(stderr, "error calculated size mismatch (bug in our code): %i vs %i\n", calculated_size, mapLen);
    }
}
main()
{
    fontbitmap = (unsigned char*)malloc(FONTBITMAPSIZE);
    memset(fontbitmap,0,FONTBITMAPSIZE);
    process_file();
    print_bitmap();
    check_size();
    free(fontbitmap);
}
