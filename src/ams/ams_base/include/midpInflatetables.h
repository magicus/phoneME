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
 * @file
 *
 * Static definition tables used to inflate file data.
 */

#ifndef _KJARTABLES_H_
#define _KJARTABLES_H_

static const unsigned char ll_extra_bits[] = {
    /* 257 */   0,   /* 3       */
    /* 258 */   0,   /* 4       */
    /* 259 */   0,   /* 5       */
    /* 260 */   0,   /* 6       */
    /* 261 */   0,   /* 7       */
    /* 262 */   0,   /* 8       */
    /* 263 */   0,   /* 9       */
    /* 264 */   0,   /* 10      */
    /* 265 */   1,   /* 11,12   */
    /* 266 */   1,   /* 13,14   */

    /* 267 */   1,   /* 15,16   */
    /* 268 */   1,   /* 17,18   */
    /* 269 */   2,   /* 19-22   */
    /* 270 */   2,   /* 23-26   */
    /* 271 */   2,   /* 27-30   */
    /* 272 */   2,   /* 31-34   */
    /* 273 */   3,   /* 35-42   */
    /* 274 */   3,   /* 43-50   */
    /* 275 */   3,   /* 51-58   */
    /* 276 */   3,   /* 59-66   */

    /* 277 */   4,   /* 67-82   */
    /* 278 */   4,   /* 83-98   */
    /* 279 */   4,   /* 99-114  */
    /* 280 */   4,   /* 115-130 */
    /* 281 */   5,   /* 131-162 */
    /* 282 */   5,   /* 163-194 */
    /* 283 */   5,   /* 195-226 */
    /* 284 */   5,   /* 227-257 */
    /* 285 */   0,   /* 258     */
};

#define MAX_ZIP_EXTRA_LENGTH_BITS 5

static const unsigned short ll_length_base[] = {
    /* 257   0 */   3,
    /* 258   0 */   4,
    /* 259   0 */   5,
    /* 260   0 */   6,
    /* 261   0 */   7,
    /* 262   0 */   8,
    /* 263   0 */   9,
    /* 264   0 */   10,
    /* 265   1 */   11,  /* - 12 */ 
    /* 266   1 */   13,  /* - 14 */ 

    /* 267   1 */   15,  /* - 16 */ 
    /* 268   1 */   17,  /* - 18 */ 
    /* 269   2 */   19,  /* - 22 */ 
    /* 270   2 */   23,  /* - 26 */ 
    /* 271   2 */   27,  /* - 30 */ 
    /* 272   2 */   31,  /* - 34 */ 
    /* 273   3 */   35,  /* - 42 */ 
    /* 274   3 */   43,  /* - 50 */ 
    /* 275   3 */   51,  /* - 58 */ 
    /* 276   3 */   59,  /* - 66 */ 

    /* 277   4 */   67,  /* - 82 */ 
    /* 278   4 */   83,  /* - 98 */ 
    /* 279   4 */   99,  /* - 114 */ 
    /* 280   4 */   115, /* - 130 */ 
    /* 281   5 */   131, /* - 162 */ 
    /* 282   5 */   163, /* - 194 */ 
    /* 283   5 */   195, /* - 226 */ 
    /* 284   5 */   227, /* - 257 */ 
    /* 285   0 */   258
};


static const unsigned char dist_extra_bits[] = {
    /*  0 */   0,    /* 1            */
    /*  1 */   0,    /* 2            */
    /*  2 */   0,    /* 3            */
    /*  3 */   0,    /* 4            */
    /*  4 */   1,    /* 5,6          */
    /*  5 */   1,    /* 7,8          */
    /*  6 */   2,    /* 9-12         */
    /*  7 */   2,    /* 13-16        */
    /*  8 */   3,    /* 17-24        */
    /*  9 */   3,    /* 25-32        */

    /* 10 */   4,    /* 33-48        */
    /* 11 */   4,    /* 49-64        */
    /* 12 */   5,    /* 65-96        */
    /* 13 */   5,    /* 97-128       */
    /* 14 */   6,    /* 129-192      */
    /* 15 */   6,    /* 193-256      */
    /* 16 */   7,    /* 257-384      */
    /* 17 */   7,    /* 385-512      */
    /* 18 */   8,    /* 513-768      */
    /* 19 */   8,    /* 769-1024     */

    /* 20 */   9,    /* 1025-1536    */
    /* 21 */   9,    /* 1537-2048    */
    /* 22 */   10,   /* 2049-3072    */
    /* 23 */   10,   /* 3073-4096    */
    /* 24 */   11,   /* 4097-6144    */
    /* 25 */   11,   /* 6145-8192    */
    /* 26 */   12,   /* 8193-12288   */
    /* 27 */   12,   /* 12289-16384  */
    /* 28 */   13,   /* 16385-24576  */
    /* 29 */   13,   /* 24577-32768  */
};

#define MAX_ZIP_EXTRA_DISTANCE_BITS 13
#define MAX_ZIP_DISTANCE_CODE 29

static const unsigned int dist_base[] = {
    /*  0   0  */   1,
    /*  1   0  */   2,
    /*  2   0  */   3,
    /*  3   0  */   4,
    /*  4   1  */   5,      /* -6     */
    /*  5   1  */   7,      /* -8     */
    /*  6   2  */   9,      /* -12    */
    /*  7   2  */   13,     /* -16    */
    /*  8   3  */   17,     /* -24    */
    /*  9   3  */   25,     /* -32    */

    /* 10   4  */   33,     /* -48    */
    /* 11   4  */   49,     /* -64    */
    /* 12   5  */   65,     /* -96    */
    /* 13   5  */   97,     /* -128   */
    /* 14   6  */   129,    /* -192   */
    /* 15   6  */   193,    /* -256   */
    /* 16   7  */   257,    /* -384   */
    /* 17   7  */   385,    /* -512   */
    /* 18   8  */   513,    /* -768   */
    /* 19   8  */   769,    /* -1024  */
    /* 20   9  */   1025,   /* -1536  */
    /* 21   9  */   1537,   /* -2048  */
    /* 22   10 */   2049,   /* -3072  */
    /* 23   10 */   3073,   /* -4096  */
    /* 24   11 */   4097,   /* -6144  */
    /* 25   11 */   6145,   /* -8192  */
    /* 26   12 */   8193,   /* -12288 */
    /* 27   12 */   12289,  /* -16384 */
    /* 28   13 */   16385,  /* -24576 */
    /* 29   13 */   24577,  /* -32768 */
};


/*
 * The order in which the code lengths of the Code Length Alphabet is
 * given. See section 3.2.7 of RFC 1951.
 */

static const char ccode_idx[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
};

/* A table for reversing 5 bits in a binary number */

static const unsigned char reverse5[] = {
    0x0, 0x10, 0x8, 0x18, 0x4, 0x14, 0xc, 0x1c,
    0x2, 0x12, 0xa, 0x1a, 0x6, 0x16, 0xe, 0x1e,
    0x1, 0x11, 0x9, 0x19, 0x5, 0x15, 0xd, 0x1d,
    0x3, 0x13, 0xb, 0x1b, 0x7, 0x17, 0xf, 0x1f
};

#define REVERSE_5BITS(code) reverse5[code]
#define REVERSE_9BITS(code) \
      ((reverse5[((code) & 0x1F)] << 4) | reverse5[(code) >> 4])
#define REVERSE_15BITS(code) \
      (  (reverse5[code & 0x1F] << 10) | (reverse5[((code) >> 5) & 0x1F] << 5) \
       | (reverse5[code >> 10]) )


#define HUFFMAN_ENTRY(char, bits) ((char << 4) + bits)


#ifdef USE_FIXED_HUFFMAN_CODE_TABLES

/* The following tables can be removed from releases
 * that care for size rather that speed. 
 *
 * "Fixed Huffman" can use the following tables, rather than doing the
 * complicated calculations that it does.
 */

static const struct HuffmanCodeTable fixedHuffmanCodeTable = { 
    { 
        7,            /* min code length */
        9,            /* max code length */
    }, 
    { 
        HUFFMAN_ENTRY(0x100, 7), 
        HUFFMAN_ENTRY('P',   8), 
        HUFFMAN_ENTRY(0x010, 8), 
        HUFFMAN_ENTRY(0x118, 8), 
        HUFFMAN_ENTRY(0x110, 7), 
        HUFFMAN_ENTRY('p',   8), 
        HUFFMAN_ENTRY('0',   8), 
        HUFFMAN_ENTRY(0x0c0, 9), 
        HUFFMAN_ENTRY(0x108, 7), 
        HUFFMAN_ENTRY('`',   8), 
        HUFFMAN_ENTRY(' ',   8), 
        HUFFMAN_ENTRY(0x0a0, 9), 
        HUFFMAN_ENTRY(0x000, 8), 
        HUFFMAN_ENTRY(0x080, 8), 
        HUFFMAN_ENTRY('@',   8), 
        HUFFMAN_ENTRY(0x0e0, 9), 
        HUFFMAN_ENTRY(0x104, 7), 
        HUFFMAN_ENTRY('X',   8), 
        HUFFMAN_ENTRY(0x018, 8), 
        HUFFMAN_ENTRY(0x090, 9), 
        HUFFMAN_ENTRY(0x114, 7), 
        HUFFMAN_ENTRY('x',   8), 
        HUFFMAN_ENTRY('8',   8), 
        HUFFMAN_ENTRY(0x0d0, 9), 
        HUFFMAN_ENTRY(0x10c, 7), 
        HUFFMAN_ENTRY('h',   8), 
        HUFFMAN_ENTRY('(',   8), 
        HUFFMAN_ENTRY(0x0b0, 9), 
        HUFFMAN_ENTRY(0x008, 8), 
        HUFFMAN_ENTRY(0x088, 8), 
        HUFFMAN_ENTRY('H',   8), 
        HUFFMAN_ENTRY(0x0f0, 9), 
        HUFFMAN_ENTRY(0x102, 7), 
        HUFFMAN_ENTRY('T',   8), 
        HUFFMAN_ENTRY(0x014, 8), 
        HUFFMAN_ENTRY(0x11c, 8), 
        HUFFMAN_ENTRY(0x112, 7), 
        HUFFMAN_ENTRY('t',   8), 
        HUFFMAN_ENTRY('4',   8), 
        HUFFMAN_ENTRY(0x0c8, 9), 
        HUFFMAN_ENTRY(0x10a, 7), 
        HUFFMAN_ENTRY('d',   8), 
        HUFFMAN_ENTRY('$',   8), 
        HUFFMAN_ENTRY(0x0a8, 9), 
        HUFFMAN_ENTRY(0x004, 8), 
        HUFFMAN_ENTRY(0x084, 8), 
        HUFFMAN_ENTRY('D',   8), 
        HUFFMAN_ENTRY(0x0e8, 9), 
        HUFFMAN_ENTRY(0x106, 7), 
        HUFFMAN_ENTRY('\\',  8), 
        HUFFMAN_ENTRY(0x01c, 8), 
        HUFFMAN_ENTRY(0x098, 9), 
        HUFFMAN_ENTRY(0x116, 7), 
        HUFFMAN_ENTRY('|',   8), 
        HUFFMAN_ENTRY('<',   8), 
        HUFFMAN_ENTRY(0x0d8, 9), 
        HUFFMAN_ENTRY(0x10e, 7), 
        HUFFMAN_ENTRY('l',   8), 
        HUFFMAN_ENTRY(',',   8), 
        HUFFMAN_ENTRY(0x0b8, 9), 
        HUFFMAN_ENTRY(0x00c, 8), 
        HUFFMAN_ENTRY(0x08c, 8), 
        HUFFMAN_ENTRY('L',   8), 
        HUFFMAN_ENTRY(0x0f8, 9), 
        HUFFMAN_ENTRY(0x101, 7), 
        HUFFMAN_ENTRY('R',   8), 
        HUFFMAN_ENTRY(0x012, 8), 
        HUFFMAN_ENTRY(0x11a, 8), 
        HUFFMAN_ENTRY(0x111, 7), 
        HUFFMAN_ENTRY('r',   8), 
        HUFFMAN_ENTRY('2',   8), 
        HUFFMAN_ENTRY(0x0c4, 9), 
        HUFFMAN_ENTRY(0x109, 7), 
        HUFFMAN_ENTRY('b',   8), 
        HUFFMAN_ENTRY('"',   8), 
        HUFFMAN_ENTRY(0x0a4, 9), 
        HUFFMAN_ENTRY(0x002, 8), 
        HUFFMAN_ENTRY(0x082, 8), 
        HUFFMAN_ENTRY('B',   8), 
        HUFFMAN_ENTRY(0x0e4, 9), 
        HUFFMAN_ENTRY(0x105, 7), 
        HUFFMAN_ENTRY('Z',   8), 
        HUFFMAN_ENTRY(0x01a, 8), 
        HUFFMAN_ENTRY(0x094, 9), 
        HUFFMAN_ENTRY(0x115, 7), 
        HUFFMAN_ENTRY('z',   8), 
        HUFFMAN_ENTRY(':',   8), 
        HUFFMAN_ENTRY(0x0d4, 9), 
        HUFFMAN_ENTRY(0x10d, 7), 
        HUFFMAN_ENTRY('j',   8), 
        HUFFMAN_ENTRY('*',   8), 
        HUFFMAN_ENTRY(0x0b4, 9), 
        HUFFMAN_ENTRY(0x00a, 8), 
        HUFFMAN_ENTRY(0x08a, 8), 
        HUFFMAN_ENTRY('J',   8), 
        HUFFMAN_ENTRY(0x0f4, 9), 
        HUFFMAN_ENTRY(0x103, 7), 
        HUFFMAN_ENTRY('V',   8), 
        HUFFMAN_ENTRY(0x016, 8), 
        HUFFMAN_ENTRY(0x11e, 8), 
        HUFFMAN_ENTRY(0x113, 7), 
        HUFFMAN_ENTRY('v',   8), 
        HUFFMAN_ENTRY('6',   8), 
        HUFFMAN_ENTRY(0x0cc, 9), 
        HUFFMAN_ENTRY(0x10b, 7), 
        HUFFMAN_ENTRY('f',   8), 
        HUFFMAN_ENTRY('&',   8), 
        HUFFMAN_ENTRY(0x0ac, 9), 
        HUFFMAN_ENTRY(0x006, 8), 
        HUFFMAN_ENTRY(0x086, 8), 
        HUFFMAN_ENTRY('F',   8), 
        HUFFMAN_ENTRY(0x0ec, 9), 
        HUFFMAN_ENTRY(0x107, 7), 
        HUFFMAN_ENTRY('^',   8), 
        HUFFMAN_ENTRY(0x01e, 8), 
        HUFFMAN_ENTRY(0x09c, 9), 
        HUFFMAN_ENTRY(0x117, 7), 
        HUFFMAN_ENTRY('~',   8), 
        HUFFMAN_ENTRY('>',   8), 
        HUFFMAN_ENTRY(0x0dc, 9), 
        HUFFMAN_ENTRY(0x10f, 7), 
        HUFFMAN_ENTRY('n',   8), 
        HUFFMAN_ENTRY('.',   8), 
        HUFFMAN_ENTRY(0x0bc, 9), 
        HUFFMAN_ENTRY(0x00e, 8), 
        HUFFMAN_ENTRY(0x08e, 8), 
        HUFFMAN_ENTRY('N',   8), 
        HUFFMAN_ENTRY(0x0fc, 9), 
        HUFFMAN_ENTRY(0x100, 7), 
        HUFFMAN_ENTRY('Q',   8), 
        HUFFMAN_ENTRY(0x011, 8), 
        HUFFMAN_ENTRY(0x119, 8), 
        HUFFMAN_ENTRY(0x110, 7), 
        HUFFMAN_ENTRY('q',   8), 
        HUFFMAN_ENTRY('1',   8), 
        HUFFMAN_ENTRY(0x0c2, 9), 
        HUFFMAN_ENTRY(0x108, 7), 
        HUFFMAN_ENTRY('a',   8), 
        HUFFMAN_ENTRY('!',   8), 
        HUFFMAN_ENTRY(0x0a2, 9), 
        HUFFMAN_ENTRY(0x001, 8), 
        HUFFMAN_ENTRY(0x081, 8), 
        HUFFMAN_ENTRY('A',   8), 
        HUFFMAN_ENTRY(0x0e2, 9), 
        HUFFMAN_ENTRY(0x104, 7), 
        HUFFMAN_ENTRY('Y',   8), 
        HUFFMAN_ENTRY(0x019, 8), 
        HUFFMAN_ENTRY(0x092, 9), 
        HUFFMAN_ENTRY(0x114, 7), 
        HUFFMAN_ENTRY('y',   8), 
        HUFFMAN_ENTRY('9',   8), 
        HUFFMAN_ENTRY(0x0d2, 9), 
        HUFFMAN_ENTRY(0x10c, 7), 
        HUFFMAN_ENTRY('i',   8), 
        HUFFMAN_ENTRY(')',   8), 
        HUFFMAN_ENTRY(0x0b2, 9), 
        HUFFMAN_ENTRY(0x009, 8), 
        HUFFMAN_ENTRY(0x089, 8), 
        HUFFMAN_ENTRY('I',   8), 
        HUFFMAN_ENTRY(0x0f2, 9), 
        HUFFMAN_ENTRY(0x102, 7), 
        HUFFMAN_ENTRY('U',   8), 
        HUFFMAN_ENTRY(0x015, 8), 
        HUFFMAN_ENTRY(0x11d, 8), 
        HUFFMAN_ENTRY(0x112, 7), 
        HUFFMAN_ENTRY('u',   8), 
        HUFFMAN_ENTRY('5',   8), 
        HUFFMAN_ENTRY(0x0ca, 9), 
        HUFFMAN_ENTRY(0x10a, 7), 
        HUFFMAN_ENTRY('e',   8), 
        HUFFMAN_ENTRY('%',   8), 
        HUFFMAN_ENTRY(0x0aa, 9), 
        HUFFMAN_ENTRY(0x005, 8), 
        HUFFMAN_ENTRY(0x085, 8), 
        HUFFMAN_ENTRY('E',   8), 
        HUFFMAN_ENTRY(0x0ea, 9), 
        HUFFMAN_ENTRY(0x106, 7), 
        HUFFMAN_ENTRY(']',   8), 
        HUFFMAN_ENTRY(0x01d, 8), 
        HUFFMAN_ENTRY(0x09a, 9), 
        HUFFMAN_ENTRY(0x116, 7), 
        HUFFMAN_ENTRY('}',   8), 
        HUFFMAN_ENTRY('=',   8), 
        HUFFMAN_ENTRY(0x0da, 9), 
        HUFFMAN_ENTRY(0x10e, 7), 
        HUFFMAN_ENTRY('m',   8), 
        HUFFMAN_ENTRY('-',   8), 
        HUFFMAN_ENTRY(0x0ba, 9), 
        HUFFMAN_ENTRY(0x00d, 8), 
        HUFFMAN_ENTRY(0x08d, 8), 
        HUFFMAN_ENTRY('M',   8), 
        HUFFMAN_ENTRY(0x0fa, 9), 
        HUFFMAN_ENTRY(0x101, 7), 
        HUFFMAN_ENTRY('S',   8), 
        HUFFMAN_ENTRY(0x013, 8), 
        HUFFMAN_ENTRY(0x11b, 8), 
        HUFFMAN_ENTRY(0x111, 7), 
        HUFFMAN_ENTRY('s',   8), 
        HUFFMAN_ENTRY('3',   8), 
        HUFFMAN_ENTRY(0x0c6, 9), 
        HUFFMAN_ENTRY(0x109, 7), 
        HUFFMAN_ENTRY('c',   8), 
        HUFFMAN_ENTRY('#',   8), 
        HUFFMAN_ENTRY(0x0a6, 9), 
        HUFFMAN_ENTRY(0x003, 8), 
        HUFFMAN_ENTRY(0x083, 8), 
        HUFFMAN_ENTRY('C',   8), 
        HUFFMAN_ENTRY(0x0e6, 9), 
        HUFFMAN_ENTRY(0x105, 7), 
        HUFFMAN_ENTRY('[',   8), 
        HUFFMAN_ENTRY(0x01b, 8), 
        HUFFMAN_ENTRY(0x096, 9), 
        HUFFMAN_ENTRY(0x115, 7), 
        HUFFMAN_ENTRY('{',   8), 
        HUFFMAN_ENTRY(';',   8), 
        HUFFMAN_ENTRY(0x0d6, 9), 
        HUFFMAN_ENTRY(0x10d, 7), 
        HUFFMAN_ENTRY('k',   8), 
        HUFFMAN_ENTRY('+',   8), 
        HUFFMAN_ENTRY(0x0b6, 9), 
        HUFFMAN_ENTRY(0x00b, 8), 
        HUFFMAN_ENTRY(0x08b, 8), 
        HUFFMAN_ENTRY('K',   8), 
        HUFFMAN_ENTRY(0x0f6, 9), 
        HUFFMAN_ENTRY(0x103, 7), 
        HUFFMAN_ENTRY('W',   8), 
        HUFFMAN_ENTRY(0x017, 8), 
        HUFFMAN_ENTRY(0x11f, 8), 
        HUFFMAN_ENTRY(0x113, 7), 
        HUFFMAN_ENTRY('w',   8), 
        HUFFMAN_ENTRY('7',   8), 
        HUFFMAN_ENTRY(0x0ce, 9), 
        HUFFMAN_ENTRY(0x10b, 7), 
        HUFFMAN_ENTRY('g',   8), 
        HUFFMAN_ENTRY('\'',  8), 
        HUFFMAN_ENTRY(0x0ae, 9), 
        HUFFMAN_ENTRY(0x007, 8), 
        HUFFMAN_ENTRY(0x087, 8), 
        HUFFMAN_ENTRY('G',   8), 
        HUFFMAN_ENTRY(0x0ee, 9), 
        HUFFMAN_ENTRY(0x107, 7), 
        HUFFMAN_ENTRY('_',   8), 
        HUFFMAN_ENTRY(0x01f, 8), 
        HUFFMAN_ENTRY(0x09e, 9), 
        HUFFMAN_ENTRY(0x117, 7), 
        HUFFMAN_ENTRY(0x07f, 8), 
        HUFFMAN_ENTRY('?',   8), 
        HUFFMAN_ENTRY(0x0de, 9), 
        HUFFMAN_ENTRY(0x10f, 7), 
        HUFFMAN_ENTRY('o',   8), 
        HUFFMAN_ENTRY('/',   8), 
        HUFFMAN_ENTRY(0x0be, 9), 
        HUFFMAN_ENTRY(0x00f, 8), 
        HUFFMAN_ENTRY(0x08f, 8), 
        HUFFMAN_ENTRY('O',   8), 
        HUFFMAN_ENTRY(0x0fe, 9), 
        HUFFMAN_ENTRY(0x100, 7), 
        HUFFMAN_ENTRY('P',   8), 
        HUFFMAN_ENTRY(0x010, 8), 
        HUFFMAN_ENTRY(0x118, 8), 
        HUFFMAN_ENTRY(0x110, 7), 
        HUFFMAN_ENTRY('p',   8), 
        HUFFMAN_ENTRY('0',   8), 
        HUFFMAN_ENTRY(0x0c1, 9), 
        HUFFMAN_ENTRY(0x108, 7), 
        HUFFMAN_ENTRY('`',   8), 
        HUFFMAN_ENTRY(' ',   8), 
        HUFFMAN_ENTRY(0x0a1, 9), 
        HUFFMAN_ENTRY(0x000, 8), 
        HUFFMAN_ENTRY(0x080, 8), 
        HUFFMAN_ENTRY('@',   8), 
        HUFFMAN_ENTRY(0x0e1, 9), 
        HUFFMAN_ENTRY(0x104, 7), 
        HUFFMAN_ENTRY('X',   8), 
        HUFFMAN_ENTRY(0x018, 8), 
        HUFFMAN_ENTRY(0x091, 9), 
        HUFFMAN_ENTRY(0x114, 7), 
        HUFFMAN_ENTRY('x',   8), 
        HUFFMAN_ENTRY('8',   8), 
        HUFFMAN_ENTRY(0x0d1, 9), 
        HUFFMAN_ENTRY(0x10c, 7), 
        HUFFMAN_ENTRY('h',   8), 
        HUFFMAN_ENTRY('(',   8), 
        HUFFMAN_ENTRY(0x0b1, 9), 
        HUFFMAN_ENTRY(0x008, 8), 
        HUFFMAN_ENTRY(0x088, 8), 
        HUFFMAN_ENTRY('H',   8), 
        HUFFMAN_ENTRY(0x0f1, 9), 
        HUFFMAN_ENTRY(0x102, 7), 
        HUFFMAN_ENTRY('T',   8), 
        HUFFMAN_ENTRY(0x014, 8), 
        HUFFMAN_ENTRY(0x11c, 8), 
        HUFFMAN_ENTRY(0x112, 7), 
        HUFFMAN_ENTRY('t',   8), 
        HUFFMAN_ENTRY('4',   8), 
        HUFFMAN_ENTRY(0x0c9, 9), 
        HUFFMAN_ENTRY(0x10a, 7), 
        HUFFMAN_ENTRY('d',   8), 
        HUFFMAN_ENTRY('$',   8), 
        HUFFMAN_ENTRY(0x0a9, 9), 
        HUFFMAN_ENTRY(0x004, 8), 
        HUFFMAN_ENTRY(0x084, 8), 
        HUFFMAN_ENTRY('D',   8), 
        HUFFMAN_ENTRY(0x0e9, 9), 
        HUFFMAN_ENTRY(0x106, 7), 
        HUFFMAN_ENTRY('\\',  8), 
        HUFFMAN_ENTRY(0x01c, 8), 
        HUFFMAN_ENTRY(0x099, 9), 
        HUFFMAN_ENTRY(0x116, 7), 
        HUFFMAN_ENTRY('|',   8), 
        HUFFMAN_ENTRY('<',   8), 
        HUFFMAN_ENTRY(0x0d9, 9), 
        HUFFMAN_ENTRY(0x10e, 7), 
        HUFFMAN_ENTRY('l',   8), 
        HUFFMAN_ENTRY(',',   8), 
        HUFFMAN_ENTRY(0x0b9, 9), 
        HUFFMAN_ENTRY(0x00c, 8), 
        HUFFMAN_ENTRY(0x08c, 8), 
        HUFFMAN_ENTRY('L',   8), 
        HUFFMAN_ENTRY(0x0f9, 9), 
        HUFFMAN_ENTRY(0x101, 7), 
        HUFFMAN_ENTRY('R',   8), 
        HUFFMAN_ENTRY(0x012, 8), 
        HUFFMAN_ENTRY(0x11a, 8), 
        HUFFMAN_ENTRY(0x111, 7), 
        HUFFMAN_ENTRY('r',   8), 
        HUFFMAN_ENTRY('2',   8), 
        HUFFMAN_ENTRY(0x0c5, 9), 
        HUFFMAN_ENTRY(0x109, 7), 
        HUFFMAN_ENTRY('b',   8), 
        HUFFMAN_ENTRY('"',   8), 
        HUFFMAN_ENTRY(0x0a5, 9), 
        HUFFMAN_ENTRY(0x002, 8), 
        HUFFMAN_ENTRY(0x082, 8), 
        HUFFMAN_ENTRY('B',   8), 
        HUFFMAN_ENTRY(0x0e5, 9), 
        HUFFMAN_ENTRY(0x105, 7), 
        HUFFMAN_ENTRY('Z',   8), 
        HUFFMAN_ENTRY(0x01a, 8), 
        HUFFMAN_ENTRY(0x095, 9), 
        HUFFMAN_ENTRY(0x115, 7), 
        HUFFMAN_ENTRY('z',   8), 
        HUFFMAN_ENTRY(':',   8), 
        HUFFMAN_ENTRY(0x0d5, 9), 
        HUFFMAN_ENTRY(0x10d, 7), 
        HUFFMAN_ENTRY('j',   8), 
        HUFFMAN_ENTRY('*',   8), 
        HUFFMAN_ENTRY(0x0b5, 9), 
        HUFFMAN_ENTRY(0x00a, 8), 
        HUFFMAN_ENTRY(0x08a, 8), 
        HUFFMAN_ENTRY('J',   8), 
        HUFFMAN_ENTRY(0x0f5, 9), 
        HUFFMAN_ENTRY(0x103, 7), 
        HUFFMAN_ENTRY('V',   8), 
        HUFFMAN_ENTRY(0x016, 8), 
        HUFFMAN_ENTRY(0x11e, 8), 
        HUFFMAN_ENTRY(0x113, 7), 
        HUFFMAN_ENTRY('v',   8), 
        HUFFMAN_ENTRY('6',   8), 
        HUFFMAN_ENTRY(0x0cd, 9), 
        HUFFMAN_ENTRY(0x10b, 7), 
        HUFFMAN_ENTRY('f',   8), 
        HUFFMAN_ENTRY('&',   8), 
        HUFFMAN_ENTRY(0x0ad, 9), 
        HUFFMAN_ENTRY(0x006, 8), 
        HUFFMAN_ENTRY(0x086, 8), 
        HUFFMAN_ENTRY('F',   8), 
        HUFFMAN_ENTRY(0x0ed, 9), 
        HUFFMAN_ENTRY(0x107, 7), 
        HUFFMAN_ENTRY('^',   8), 
        HUFFMAN_ENTRY(0x01e, 8), 
        HUFFMAN_ENTRY(0x09d, 9), 
        HUFFMAN_ENTRY(0x117, 7), 
        HUFFMAN_ENTRY('~',   8), 
        HUFFMAN_ENTRY('>',   8), 
        HUFFMAN_ENTRY(0x0dd, 9), 
        HUFFMAN_ENTRY(0x10f, 7), 
        HUFFMAN_ENTRY('n',   8), 
        HUFFMAN_ENTRY('.',   8), 
        HUFFMAN_ENTRY(0x0bd, 9), 
        HUFFMAN_ENTRY(0x00e, 8), 
        HUFFMAN_ENTRY(0x08e, 8), 
        HUFFMAN_ENTRY('N',   8), 
        HUFFMAN_ENTRY(0x0fd, 9), 
        HUFFMAN_ENTRY(0x100, 7), 
        HUFFMAN_ENTRY('Q',   8), 
        HUFFMAN_ENTRY(0x011, 8), 
        HUFFMAN_ENTRY(0x119, 8), 
        HUFFMAN_ENTRY(0x110, 7), 
        HUFFMAN_ENTRY('q',   8), 
        HUFFMAN_ENTRY('1',   8), 
        HUFFMAN_ENTRY(0x0c3, 9), 
        HUFFMAN_ENTRY(0x108, 7), 
        HUFFMAN_ENTRY('a',   8), 
        HUFFMAN_ENTRY('!',   8), 
        HUFFMAN_ENTRY(0x0a3, 9), 
        HUFFMAN_ENTRY(0x001, 8), 
        HUFFMAN_ENTRY(0x081, 8), 
        HUFFMAN_ENTRY('A',   8), 
        HUFFMAN_ENTRY(0x0e3, 9), 
        HUFFMAN_ENTRY(0x104, 7), 
        HUFFMAN_ENTRY('Y',   8), 
        HUFFMAN_ENTRY(0x019, 8), 
        HUFFMAN_ENTRY(0x093, 9), 
        HUFFMAN_ENTRY(0x114, 7), 
        HUFFMAN_ENTRY('y',   8), 
        HUFFMAN_ENTRY('9',   8), 
        HUFFMAN_ENTRY(0x0d3, 9), 
        HUFFMAN_ENTRY(0x10c, 7), 
        HUFFMAN_ENTRY('i',   8), 
        HUFFMAN_ENTRY(')',   8), 
        HUFFMAN_ENTRY(0x0b3, 9), 
        HUFFMAN_ENTRY(0x009, 8), 
        HUFFMAN_ENTRY(0x089, 8), 
        HUFFMAN_ENTRY('I',   8), 
        HUFFMAN_ENTRY(0x0f3, 9), 
        HUFFMAN_ENTRY(0x102, 7), 
        HUFFMAN_ENTRY('U',   8), 
        HUFFMAN_ENTRY(0x015, 8), 
        HUFFMAN_ENTRY(0x11d, 8), 
        HUFFMAN_ENTRY(0x112, 7), 
        HUFFMAN_ENTRY('u',   8), 
        HUFFMAN_ENTRY('5',   8), 
        HUFFMAN_ENTRY(0x0cb, 9), 
        HUFFMAN_ENTRY(0x10a, 7), 
        HUFFMAN_ENTRY('e',   8), 
        HUFFMAN_ENTRY('%',   8), 
        HUFFMAN_ENTRY(0x0ab, 9), 
        HUFFMAN_ENTRY(0x005, 8), 
        HUFFMAN_ENTRY(0x085, 8), 
        HUFFMAN_ENTRY('E',   8), 
        HUFFMAN_ENTRY(0x0eb, 9), 
        HUFFMAN_ENTRY(0x106, 7), 
        HUFFMAN_ENTRY(']',   8), 
        HUFFMAN_ENTRY(0x01d, 8), 
        HUFFMAN_ENTRY(0x09b, 9), 
        HUFFMAN_ENTRY(0x116, 7), 
        HUFFMAN_ENTRY('}',   8), 
        HUFFMAN_ENTRY('=',   8), 
        HUFFMAN_ENTRY(0x0db, 9), 
        HUFFMAN_ENTRY(0x10e, 7), 
        HUFFMAN_ENTRY('m',   8), 
        HUFFMAN_ENTRY('-',   8), 
        HUFFMAN_ENTRY(0x0bb, 9), 
        HUFFMAN_ENTRY(0x00d, 8), 
        HUFFMAN_ENTRY(0x08d, 8), 
        HUFFMAN_ENTRY('M',   8), 
        HUFFMAN_ENTRY(0x0fb, 9), 
        HUFFMAN_ENTRY(0x101, 7), 
        HUFFMAN_ENTRY('S',   8), 
        HUFFMAN_ENTRY(0x013, 8), 
        HUFFMAN_ENTRY(0x11b, 8), 
        HUFFMAN_ENTRY(0x111, 7), 
        HUFFMAN_ENTRY('s',   8), 
        HUFFMAN_ENTRY('3',   8), 
        HUFFMAN_ENTRY(0x0c7, 9), 
        HUFFMAN_ENTRY(0x109, 7), 
        HUFFMAN_ENTRY('c',   8), 
        HUFFMAN_ENTRY('#',   8), 
        HUFFMAN_ENTRY(0x0a7, 9), 
        HUFFMAN_ENTRY(0x003, 8), 
        HUFFMAN_ENTRY(0x083, 8), 
        HUFFMAN_ENTRY('C',   8), 
        HUFFMAN_ENTRY(0x0e7, 9), 
        HUFFMAN_ENTRY(0x105, 7), 
        HUFFMAN_ENTRY('[',   8), 
        HUFFMAN_ENTRY(0x01b, 8), 
        HUFFMAN_ENTRY(0x097, 9), 
        HUFFMAN_ENTRY(0x115, 7), 
        HUFFMAN_ENTRY('{',   8), 
        HUFFMAN_ENTRY(';',   8), 
        HUFFMAN_ENTRY(0x0d7, 9), 
        HUFFMAN_ENTRY(0x10d, 7), 
        HUFFMAN_ENTRY('k',   8), 
        HUFFMAN_ENTRY('+',   8), 
        HUFFMAN_ENTRY(0x0b7, 9), 
        HUFFMAN_ENTRY(0x00b, 8), 
        HUFFMAN_ENTRY(0x08b, 8), 
        HUFFMAN_ENTRY('K',   8), 
        HUFFMAN_ENTRY(0x0f7, 9), 
        HUFFMAN_ENTRY(0x103, 7), 
        HUFFMAN_ENTRY('W',   8), 
        HUFFMAN_ENTRY(0x017, 8), 
        HUFFMAN_ENTRY(0x11f, 8), 
        HUFFMAN_ENTRY(0x113, 7), 
        HUFFMAN_ENTRY('w',   8), 
        HUFFMAN_ENTRY('7',   8), 
        HUFFMAN_ENTRY(0x0cf, 9), 
        HUFFMAN_ENTRY(0x10b, 7), 
        HUFFMAN_ENTRY('g',   8), 
        HUFFMAN_ENTRY('\'',  8), 
        HUFFMAN_ENTRY(0x0af, 9), 
        HUFFMAN_ENTRY(0x007, 8), 
        HUFFMAN_ENTRY(0x087, 8), 
        HUFFMAN_ENTRY('G',   8), 
        HUFFMAN_ENTRY(0x0ef, 9), 
        HUFFMAN_ENTRY(0x107, 7), 
        HUFFMAN_ENTRY('_',   8), 
        HUFFMAN_ENTRY(0x01f, 8), 
        HUFFMAN_ENTRY(0x09f, 9), 
        HUFFMAN_ENTRY(0x117, 7), 
        HUFFMAN_ENTRY(0x07f, 8), 
        HUFFMAN_ENTRY('?',   8), 
        HUFFMAN_ENTRY(0x0df, 9), 
        HUFFMAN_ENTRY(0x10f, 7), 
        HUFFMAN_ENTRY('o',   8), 
        HUFFMAN_ENTRY('/',   8), 
        HUFFMAN_ENTRY(0x0bf, 9), 
        HUFFMAN_ENTRY(0x00f, 8), 
        HUFFMAN_ENTRY(0x08f, 8), 
        HUFFMAN_ENTRY('O',   8), 
        HUFFMAN_ENTRY(0x0ff, 9), 
    }
};

static const struct shortHuffmanCodeTable fixedHuffmanDistanceTable = { 
    { 
        5,            /* quick bits */
        5,            /* max code length */
    }, 
    {
        HUFFMAN_ENTRY(0x00, 5), 
        HUFFMAN_ENTRY(0x10, 5), 
        HUFFMAN_ENTRY(0x08, 5), 
        HUFFMAN_ENTRY(0x18, 5), 
        HUFFMAN_ENTRY(0x04, 5), 
        HUFFMAN_ENTRY(0x14, 5), 
        HUFFMAN_ENTRY(0x0c, 5), 
        HUFFMAN_ENTRY(0x1c, 5), 
        HUFFMAN_ENTRY(0x02, 5), 
        HUFFMAN_ENTRY(0x12, 5), 
        HUFFMAN_ENTRY(0x0a, 5), 
        HUFFMAN_ENTRY(0x1a, 5), 
        HUFFMAN_ENTRY(0x06, 5), 
        HUFFMAN_ENTRY(0x16, 5), 
        HUFFMAN_ENTRY(0x0e, 5), 
        HUFFMAN_ENTRY(0x1e, 5), 
        HUFFMAN_ENTRY(0x01, 5), 
        HUFFMAN_ENTRY(0x11, 5), 
        HUFFMAN_ENTRY(0x09, 5), 
        HUFFMAN_ENTRY(0x19, 5), 
        HUFFMAN_ENTRY(0x05, 5), 
        HUFFMAN_ENTRY(0x15, 5), 
        HUFFMAN_ENTRY(0x0d, 5), 
        HUFFMAN_ENTRY(0x1d, 5), 
        HUFFMAN_ENTRY(0x03, 5), 
        HUFFMAN_ENTRY(0x13, 5), 
        HUFFMAN_ENTRY(0x0b, 5), 
        HUFFMAN_ENTRY(0x1b, 5), 
        HUFFMAN_ENTRY(0x07, 5), 
        HUFFMAN_ENTRY(0x17, 5), 
        HUFFMAN_ENTRY(0x0f, 5), 
        HUFFMAN_ENTRY(0x1f, 5), 
    }
};

#endif /* USE_FIXED_HUFFMAN_CODE_TABLES */

#endif /* _KJARTABLES_H_ */


