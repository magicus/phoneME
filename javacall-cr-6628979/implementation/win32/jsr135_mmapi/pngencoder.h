/*
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
 
#ifndef __JAVAUTIL_MEDIA_PNG_ENCODER_H
#define __JAVAUTIL_MEDIA_PNG_ENCODER_H

/**
 * Get PNG buffer size for image that has width and height
 * 
 * @param width     Width of image
 * @param height    Height of image
 * 
 * @return PNG buffer size need by javautil_media_rgb_to_png API
 */
int javautil_media_get_png_size(int width, int height);

/**
 * Encode rgb888 format data to PNG data format (there is no compression)
 * 
 * @param input     Pointer to rgb888 data
 * @param output    Pointer to PNG encode buffer
 * @param width     Width of image
 * @param height    Height of image
 * 
 * @return Byte size of encoded PNG data
 */
int javautil_media_rgb_to_png(unsigned char *input, 
                              unsigned char *output,
                              int width, 
                              int height);

/**
 * Encode rgbX888 format data to PNG data format (there is no compression)
 * 
 * @param input     Pointer to rgbX888 data
 * @param output    Pointer to PNG encode buffer
 * @param width     Width of image
 * @param height    Height of image
 * 
 * @return Byte size of encoded PNG data
 */
int javautil_media_rgbX888_to_png(unsigned char *input, 
                                  unsigned char *output,
                                  int width, 
                                  int height);

#endif  /* __JAVAUTIL_MEDIA_PNG_ENCODER_H */
