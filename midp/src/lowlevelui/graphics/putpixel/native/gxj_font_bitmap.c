/*
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

#include "gxj_intern_font_bitmap.h"

// starts off with width, height, ascent, descent, leading, then data
unsigned char TheFontBitmap[2021] = {0x9,0xE,0xB,0x3,0x0,/*data starts here */ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0xc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0x8,0x4,0x2,0x1,0x0,0x80,0x40,0x20,0x0,0x8,0x0,0x0,0x0,0x0,0x0,0x90,0x48,0x24,0x12,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x20,0x90,0x48,0xfe,0x24,0x12,0x3f,0x89,0x4,0x82,0x40,0x0,0x0,0x0,0x0,0x4,0xf,0x88,0x24,0x2,0x0,0xf8,0x2,0x1,0x20,0x8f,0x81,0x0,0x80,0x0,0x0,0x60,0x48,0x98,0x80,0x80,0x80,0x80,0x80,0x8c,0x89,0x3,0x0,0x0,0x0,0x0,0x0,0xe0,0x88,0x40,0x20,0x8,0xa,0x8,0x94,0x32,0x18,0xf2,0x0,0x0,0x0,0x0,0x1,0x0,0x80,0x40,0x20,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x80,0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x0,0x80,0x40,0x0,0x0,0x20,0x8,0x4,0x1,0x0,0x80,0x40,0x20,0x10,0x8,0x4,0x4,0x2,0x0,0x0,0x0,0x0,0x0,0x6c,0x1c,0x3f,0x87,0x6,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x40,0x20,0x10,0x7f,0x4,0x2,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7,0x3,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0xf0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x0,0x0,0x0,0x0,0x0,0x0,0xf,0x8,0x44,0x22,0x11,0x8,0x84,0x42,0x21,0x10,0x87,0x80,0x0,0x0,0x0,0x0,0x8,0x1c,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x8,0x1f,0x0,0x0,0x0,0x0,0x0,0xf0,0x84,0x2,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0xfc,0x0,0x0,0x0,0x0,0x3,0xc2,0x10,0x8,0x4,0x1c,0x1,0x0,0x80,0x44,0x21,0xe0,0x0,0x0,0x0,0x0,0x1,0x1,0x81,0x40,0xa0,0x90,0x88,0x7e,0x2,0x1,0x1,0xc0,0x0,0x0,0x0,0x0,0x7e,0x20,0x10,0x8,0x7,0xc0,0x10,0x8,0x4,0x42,0x1e,0x0,0x0,0x0,0x0,0x0,0x70,0x40,0x40,0x20,0x17,0xc,0x44,0x22,0x11,0x8,0x78,0x0,0x0,0x0,0x0,0x7,0xe2,0x10,0x10,0x8,0x8,0x4,0x4,0x2,0x2,0x1,0x0,0x0,0x0,0x0,0x0,0xf,0x8,0x44,0x22,0x10,0xf0,0x84,0x42,0x21,0x10,0x87,0x80,0x0,0x0,0x0,0x0,0x3c,0x21,0x10,0x88,0x44,0x61,0xd0,0x8,0x4,0x4,0x1c,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xc,0x6,0x0,0x0,0x0,0x0,0x60,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x30,0x18,0x0,0x0,0x0,0x1,0xc0,0xc0,0xc0,0x0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0x80,0x20,0x8,0x2,0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7,0xe0,0x1,0xf8,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x20,0x8,0x2,0x0,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0,0x0,0x3,0xc2,0x10,0x8,0x4,0x4,0x4,0x4,0x2,0x0,0x0,0x80,0x0,0x0,0x0,0x0,0xf,0x8,0x48,0x14,0xea,0x95,0x4a,0xa5,0x4d,0x20,0x8,0x3,0xe0,0x0,0x0,0x0,0x70,0x8,0xa,0x5,0x4,0x42,0x21,0xf1,0x4,0x82,0xe3,0x80,0x0,0x0,0x0,0x3,0xf0,0x84,0x42,0x21,0x1f,0x8,0x44,0x22,0x11,0x9,0xf8,0x0,0x0,0x0,0x0,0x3,0xa2,0x32,0x9,0x4,0x80,0x40,0x20,0x10,0x4,0x21,0xe0,0x0,0x0,0x0,0x0,0x3e,0x8,0x84,0x22,0x11,0x8,0x84,0x42,0x21,0x11,0x1f,0x0,0x0,0x0,0x0,0x0,0xfe,0x21,0x10,0x9,0x7,0x82,0x41,0x0,0x80,0x42,0x7f,0x0,0x0,0x0,0x0,0x3,0xf8,0x84,0x40,0x24,0x1e,0x9,0x4,0x2,0x1,0x1,0xe0,0x0,0x0,0x0,0x0,0x3,0xa2,0x32,0x9,0x4,0x80,0x47,0xa0,0x90,0x44,0x21,0xe0,0x0,0x0,0x0,0x0,0x71,0xd0,0x48,0x24,0x13,0xf9,0x4,0x82,0x41,0x20,0xb8,0xe0,0x0,0x0,0x0,0x0,0x7c,0x8,0x4,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x3e,0x0,0x0,0x0,0x0,0x0,0x7c,0x8,0x4,0x2,0x1,0x0,0x80,0x44,0x22,0x10,0xf0,0x0,0x0,0x0,0x0,0xe,0xe2,0x21,0x20,0x90,0x50,0x38,0x12,0x8,0x84,0x27,0x18,0x0,0x0,0x0,0x0,0x3c,0x8,0x4,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x9f,0xc0,0x0,0x0,0x0,0x1,0x83,0x41,0x31,0x98,0xca,0xa5,0x52,0x49,0x24,0x82,0xe3,0x80,0x0,0x0,0x0,0x6,0x3d,0x4,0xc2,0x51,0x28,0x92,0x48,0xa4,0x52,0x1b,0xc4,0x0,0x0,0x0,0x0,0x3,0x82,0x22,0x9,0x4,0x82,0x41,0x20,0x90,0x44,0x41,0xc0,0x0,0x0,0x0,0x0,0x3f,0x8,0x44,0x22,0x11,0x8,0xf8,0x40,0x20,0x10,0x1e,0x0,0x0,0x0,0x0,0x0,0x38,0x22,0x20,0x90,0x48,0x24,0x12,0x9,0x4,0x44,0x1c,0x6,0xc,0xc0,0x0,0x3,0xf0,0x84,0x42,0x21,0x10,0x8f,0x84,0x82,0x21,0x9,0xc6,0x0,0x0,0x0,0x0,0x7,0xa4,0x32,0x9,0x0,0x70,0x6,0x0,0x90,0x4c,0x25,0xe0,0x0,0x0,0x0,0x0,0x7f,0xe2,0x21,0x0,0x80,0x40,0x20,0x10,0x8,0x4,0xf,0x80,0x0,0x0,0x0,0x1,0xc7,0x41,0x20,0x90,0x48,0x24,0x12,0x9,0x4,0xc6,0x1c,0x0,0x0,0x0,0x0,0x7,0x1d,0x4,0x82,0x22,0x11,0x8,0x82,0x81,0x40,0x40,0x20,0x0,0x0,0x0,0x0,0x18,0x38,0xc,0x46,0x22,0xaa,0x55,0x2a,0x88,0x84,0x42,0x20,0x0,0x0,0x0,0x0,0x71,0xd0,0x44,0x41,0x40,0x40,0x20,0x28,0x22,0x20,0xb8,0xe0,0x0,0x0,0x0,0x1,0xc7,0x41,0x11,0x8,0x82,0x80,0x80,0x40,0x20,0x10,0x3e,0x0,0x0,0x0,0x0,0x3,0xf9,0x4,0x4,0x4,0x4,0x2,0x2,0x2,0x2,0x9,0xfc,0x0,0x0,0x0,0x0,0x1,0xc0,0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1,0x0,0x80,0x40,0x20,0x1c,0x0,0x0,0x10,0x4,0x1,0x0,0x40,0x10,0x4,0x1,0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x38,0x4,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1,0x3,0x80,0x0,0x40,0x50,0x44,0x41,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0xfe,0x0,0x0,0x8,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf,0x0,0x43,0xe2,0x11,0x8,0x84,0x3d,0x0,0x0,0x0,0x0,0x3,0x0,0x80,0x40,0x2c,0x19,0x88,0x44,0x22,0x11,0x99,0xb0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x74,0xc6,0x40,0x20,0x10,0xc,0x61,0xc0,0x0,0x0,0x0,0x0,0x3,0x0,0x80,0x41,0xa3,0x31,0x8,0x84,0x42,0x33,0x6,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7,0xc,0x64,0x13,0xf9,0x0,0xc6,0x1c,0x0,0x0,0x0,0x0,0x0,0x70,0x44,0x20,0x10,0x1e,0x4,0x2,0x1,0x0,0x80,0xf0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6c,0xcc,0x42,0x21,0x19,0x83,0x40,0x22,0x11,0x8,0x78,0x0,0x30,0x8,0x4,0x2,0xc1,0x90,0x88,0x44,0x22,0x11,0x1d,0xc0,0x0,0x0,0x0,0x0,0x10,0x0,0x0,0xe,0x1,0x0,0x80,0x40,0x20,0x10,0x3e,0x0,0x0,0x0,0x0,0x0,0x20,0x0,0x0,0x1c,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x8,0x44,0x1c,0x0,0xc,0x2,0x1,0x0,0x9c,0x48,0x28,0x1c,0x9,0x4,0x47,0x70,0x0,0x0,0x0,0x0,0x1c,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x8,0x4,0xf,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x36,0xcd,0xa4,0x92,0x49,0x24,0x92,0xed,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6c,0x19,0x8,0x84,0x42,0x21,0x11,0xdc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x70,0xc6,0x41,0x20,0x90,0x4c,0x61,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0xc1,0x98,0x84,0x42,0x21,0x19,0x8b,0x4,0x2,0x3,0xc0,0x0,0x0,0x0,0x0,0x6,0xcc,0xc4,0x22,0x11,0x8,0xcc,0x1a,0x1,0x0,0x81,0xe0,0x0,0x0,0x0,0x0,0x77,0xc,0x44,0x2,0x1,0x0,0x81,0xf0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf8,0x82,0x40,0x1f,0x0,0x48,0x23,0xe0,0x0,0x0,0x0,0x0,0x0,0x4,0x2,0x7,0xe0,0x80,0x40,0x20,0x10,0x8,0x83,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x19,0x84,0x42,0x21,0x10,0x88,0x4c,0x1b,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xe3,0xa0,0x88,0x84,0x41,0x40,0xa0,0x20,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0xae,0x92,0x55,0x2a,0x95,0x44,0x42,0x20,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7,0x71,0x10,0x50,0x10,0x14,0x11,0x1d,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x38,0xe8,0x22,0x21,0x10,0x50,0x28,0x8,0x4,0x4,0xc,0x0,0x0,0x0,0x0,0x0,0x7f,0x21,0x1,0x1,0x1,0x1,0x9,0xfc,0x0,0x0,0x0,0x0,0x0,0xc0,0x80,0x40,0x20,0x10,0x8,0x18,0x2,0x1,0x0,0x80,0x40,0x20,0xc,0x0,0x4,0x2,0x1,0x0,0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1,0x0,0x80,0x40,0x0,0x30,0x4,0x2,0x1,0x0,0x80,0x40,0x18,0x10,0x8,0x4,0x2,0x1,0x3,0x0,0x1,0x89,0x24,0x8c,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x80,0xc0,0x0,0x0,0x0};
