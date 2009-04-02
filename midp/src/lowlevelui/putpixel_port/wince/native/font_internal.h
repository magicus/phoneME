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

#ifndef _FONT_INTERNAL_H
#define _FONT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define FONTPARAMS face, style, size
#define FONTPARAMS_PROTO int face, int style, int size

extern int gfFontInit;

extern int wince_init_fonts();
extern void gx_port_draw_chars(jint pixel, const jshort *clip, 
                               //const java_imagedata *dst,
                               gxj_screen_buffer *sbuf,
                               int dotted,
                               int face, int style, int size,
                               int x, int y, int anchor, 
                               const jchar *charArray, int n);
extern void gx_port_get_fontinfo(int face, int style, int size,
                                 int *ascent, int *descent, int *leading);
extern int gx_port_get_charswidth(int face, int style, int size, 
                                  const jchar *charArray, int n);

#ifdef __cplusplus
}
#endif

#endif /* _FONT_INTERNAL_H */
