/*
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
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
#ifndef __JPEG_ENCODER_H__
#define __JPEG_ENCODER_H__

/*
 * Used as indicator of input format instead of simple pixel size.
 * When pixel array is int[] then it shall be represented either XRGB or in BGRX
 * depending on platform endianess !
 */
typedef enum {
    JPEG_ENCODER_COLOR_GRAYSCALE = 0x01,
    JPEG_ENCODER_COLOR_RGB = 0x02,
    JPEG_ENCODER_COLOR_BGR = 0x04,
    JPEG_ENCODER_COLOR_XRGB = 0x08,
    JPEG_ENCODER_COLOR_BGRX = 0x10
    /*TBD: can add datatypes for 16-bit colors ... 
    JPEG_ENCODER_COLOR_RGB565 = 0x20,
    JPEG_ENCODER_COLOR_BGR565 = 0x40,
     */
} JPEG_ENCODER_INPUT_COLOR_FORMAT;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is the actual RGB to JPEG converter top-level function.
 * @param inData Pointer to the RGB data
 * @param width Width of the frame
 * @param height Height of the frame
 * @param quality Quality is a value between 1 and 100, 100 being highest
 * @param outData Pointer to where the compressed data is to be written
 * @param colorFormat format of pixel color
 * @return ? size of converted image in bytes ?
 */
int RGBToJPEG(char *inData, int width, int height, int quality,
            char *outData, 
            JPEG_ENCODER_INPUT_COLOR_FORMAT colorFormat);
#ifdef __cplusplus
}
#endif

#endif /* __JPEG_ENCODER_H__ */
