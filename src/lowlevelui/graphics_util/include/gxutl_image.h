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
 

#ifndef _GXUTL_IMAGE_H_ 
#define _GXUTL_IMAGE_H_ 

/**
 * @file
 * @ingroup lowui_gxutl
 *
 * @brief Porting api for graphics_util library
 */

#include <midpError.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Type of image */
typedef enum {
    GXUTL_IMAGE_FORMAT_UNSUPPORTED,
    GXUTL_IMAGE_FORMAT_PNG,
    GXUTL_IMAGE_FORMAT_JPEG,
    GXUTL_IMAGE_FORMAT_RAW,
} gxutl_image_format;

/** PNG file header */
extern const unsigned char gxutl_png_header[16];

/** JPEG file header */
extern const unsigned char gxutl_jpeg_header[4];

/** RAW platform dependent image file header */
extern const unsigned char gxutl_raw_header[4];

/** PNG image buffer layout */
typedef struct {
    unsigned char header[16]; /**< Must equal gxutl_png_header */
    unsigned char width[4];   /**< Big-endian integer */
    unsigned char height[4];  /**< Big-endian integer */
    unsigned char data[1];    /**< Variable length byte array */
} gxutl_image_buffer_png;

/** JPEG image buffer layout */
typedef struct {
    unsigned char header[4];  /**< Must equal gxutl_jpeg_header */
    /** Length of data (including these 2 bytes), big-endian. */
    unsigned char length[2];
    unsigned char id[5];      /**< 4A, 46, 49, 46, 00 ("JFIF" string) */
    /**
     * JFIF format revision (often 01, 02).
     * The most significant byte is used for major revisions;
     * the least significant byte for minor revisions.
     */
    unsigned char version[2];
    /**
     * Units used for density:
     *   0 - no units, densityX and densityY specify the pixel aspect ratio;
     *   1 - densityX and densityY are dots per inch;
     *   2 - densityX and densityY are dots per cm.
     */
    unsigned char units;
    unsigned char densityX[2];     /**< Horizontal density (big-endian) */
    unsigned char densityY[2];     /**< Vertical density (big-endian) */
    unsigned char thumbnailWidth;  /**< 0 - no thumbnail */
    unsigned char thumbnailHeight; /**< 0 - no thumbnail */
    /*
     * 3 * thumbnailWidth * thumbnailHeight bytes:
     * 24-bit RGB values for the thumbnail pixels.
     */
} gxutl_image_buffer_jpeg;

/** RAW image buffer layout */
typedef struct {
    unsigned char header[4];  /**< Must equal gxutl_raw_header */
    unsigned int width;       /**< Default endian intergar */
    unsigned int height;      /**< Default endian intergar */
    unsigned int hasAlpha;    /**< Default endian intergar */
    unsigned char data[1];    /**< Variable length byte array */
} gxutl_image_buffer_raw;

/**
 * Identify image format from a given image buffer.
 *
 * @param imgBuffer buffer contains the image data
 * @param length    number of bytes of the buffer
 * @param format    return image format
 * @param width     return image width
 * @param height    return image height
 *
 * @return one of error codes:
 *              MIDP_ERROR_NONE,
 *              MIDP_ERROR_UNSUPPORTED
 *              MIDP_ERROR_IMAGE_CORRUPTED
 */
MIDP_ERROR gxutl_image_get_info(unsigned char *imgBuffer,
		 	        unsigned int length,
			        gxutl_image_format *format,
			        unsigned int *width,
			        unsigned int *height);

#ifdef __cplusplus
}
#endif
#endif /* _GXUTL_IMAGE_H_ */
