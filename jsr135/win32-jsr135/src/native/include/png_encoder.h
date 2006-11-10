/*
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
#ifndef __PNG_ENCODER_H__
#define __PNG_ENCODER_H__

/*
 * Used as indicator of input format instead of simple pixel size.
 * When pixel array is int[] then it shall be represented either XRGB or in BGRX
 * depending on platform endianess !
 */
typedef enum {
    PNG_ENCODER_COLOR_XRGB = 0x08,
    PNG_ENCODER_COLOR_BGRX = 0x10
} PNG_ENCODER_INPUT_COLOR_FORMAT;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is the actual RGB to PNG converter top-level function.
 * @param input Pointer to the RGB data
 * @param output Pointer to where the compressed data is to be written
 * @param width Width of the frame
 * @param height Height of the frame
 * @param colorFormat 0 if big-endian input data (XRGB), 
                       else little-endian (BGRX)
 * @return ? size of converted image in bytes ?
 */
int RGBToPNG(unsigned char *input, unsigned char *output,
	     int width, int height,
             PNG_ENCODER_INPUT_COLOR_FORMAT colorFormat);
#ifdef __cplusplus
}
#endif

#endif /* __PNG_ENCODER_H__ */
