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

#include <kni.h>

/**
 * @file
 *
 * platform dependent character drawing
 */
 
int gxjport_draw_chars(int pixel, const jshort *clip, void *dst, int dotted,
                       int face, int style, int size,
                       int x, int y, int anchor,
                       const jchar *chararray, int n) {
    (void)pixel;
    (void)clip;
    (void)dst;
    (void)dotted;
    (void)face;
    (void)style;
    (void)size;
    (void)x;
    (void)y;
    (void)anchor;
    (void)chararray;
    (void)n;
    
    return KNI_FALSE;
}                           

int gxjport_get_font_info(int face, int style, int size,
                          int *ascent, int *descent, int *leading) {
    (void)face;
    (void)style;
    (void)size;
    (void)ascent;
    (void)descent;
    (void)leading;
                          
    return KNI_FALSE;
}                            

int gxjport_get_chars_width(int face, int style, int size,
                            const jchar *charArray, int n) {
    (void)face;
    (void)style;
    (void)size;
    (void)charArray;
    (void)n;

    return -1;
}
