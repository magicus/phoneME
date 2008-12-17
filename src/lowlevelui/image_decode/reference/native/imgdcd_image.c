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
 * @brief Common internal functions of image.
 */
#include <midpError.h>
#include <string.h>
#include <imgdcd_image_util.h>

/* PNG file header */
const unsigned char imgdcd_png_header[16] = {0x89, 'P', 'N', 'G',
				      0x0d, 0x0a, 0x1a, 0x0a,
				      0x00, 0x00, 0x00, 0x0d,
				      0x49, 0x48, 0x44, 0x52};

/* JPEG file header */
const unsigned char imgdcd_jpeg_header[4] = {0xff, 0xd8, 0xff, 0xe0};

/* RAW platform dependent image file header */
const unsigned char imgdcd_raw_header[4] = {0x89, 'S', 'U', 'N'};

/**
 * Identify image format from a given image buffer.
 *
 * @param imgBuf buffer contains the image data
 * @param length    number of bytes of the buffer
 * @param format    return image format
 * @param width	    return image width
 * @param height    return image height
 *
 * @return one of error codes:
 *		MIDP_ERROR_NONE,
 *		MIDP_ERROR_UNSUPPORTED
 *		MIDP_ERROR_IMAGE_CORRUPTED
 */
MIDP_ERROR imgdcd_image_get_info(unsigned char *imgBuf,
			        unsigned int length,
			        imgdcd_image_format *format,
			        unsigned int *width,
			        unsigned int *height) {

    /* check for a PNG header signature */
    if ((length >= 4 + 8) &&
        (memcmp(imgBuf, &imgdcd_raw_header, 4) == 0)) {

	*format = IMGDCD_IMAGE_FORMAT_RAW;

	/* Assume default endian */
	*width  = ((imgdcd_image_buffer_raw *)imgBuf)->width;
	*height = ((imgdcd_image_buffer_raw *)imgBuf)->height;

	return MIDP_ERROR_NONE;

    } else if ((length >= 16 + 8) &&
	       (memcmp(imgBuf, &imgdcd_png_header, 16) == 0)) {

        *format = IMGDCD_IMAGE_FORMAT_PNG;

	/* Big endian */
	*width  = (imgBuf[16] << 24) |
		  (imgBuf[17] << 16) |
		  (imgBuf[18] <<  8) |
		   imgBuf[19];

	*height = (imgBuf[20] << 24) |
		  (imgBuf[21] << 16) |
		  (imgBuf[22] <<  8) |
		   imgBuf[23];

	return MIDP_ERROR_NONE;

    } else if ((length >= 4 + 8) &&
               (memcmp(imgBuf, &imgdcd_jpeg_header, 4) == 0)) {
        unsigned int i, field_len;

	*format = IMGDCD_IMAGE_FORMAT_JPEG;

	/*
         * Find SOF (Start Of Frame) marker:
         * format of SOF
         *   2 bytes    Marker Identity  (0xff 0xc<N>)
         *   2 bytes    Length of block
         *   1 byte     bits/sample
         *   2 bytes    Image Height
         *   2 bytes    Image Width
         *   1 bytes    Number of components
         *   n bytes    the components
         *
         * Searching for the byte pair representing SOF is unsafe
         * because a prior marker might contain the SOFn pattern
         * so we must skip over the preceding markers.
         *
         * When editing this code, don't forget to make the appropriate changes
         * in src/lowlevelui/graphics/putpixel/classes/javax/microedition/
         * lcdui/ImageDataFactory.java.
         */

        i = 2;

        while (i + 8 < length) {
            if (imgBuf[i] != 0xff) {
                break;
            }

            if ((imgBuf[i+1] & 0xf0) == 0xc0) {
                unsigned char code = imgBuf[i+1];

                if (code != 0xc4 && code != 0xc8 && code != 0xcc) {
                    /* Big endian */
                    *height = (imgBuf[i + 5] << 8) | imgBuf[i + 6];
                    *width  = (imgBuf[i + 7] << 8) | imgBuf[i + 8];
                    return MIDP_ERROR_NONE;
                }
            }

            /* Go to the next marker */
            field_len = (imgBuf[i + 2] << 8) | imgBuf[i + 3];
            i += field_len + 2;
        }

        /* Reach here means we didn't find Marker */

        return MIDP_ERROR_IMAGE_CORRUPTED;
    } else {
        *format = IMGDCD_IMAGE_FORMAT_UNSUPPORTED;
        return MIDP_ERROR_UNSUPPORTED;
    }
}
