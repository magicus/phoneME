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

#ifndef _GXJ_PORT_H
#define _GXJ_PORT_H

/**
 * @file
 * @ingroup lowui_port
 *
 * @brief Porting Interface of putpixel based graphics system
 */


#include <gx_image.h>
#include <imgapi_image.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable verbose clip checks of screen buffer vs clip[XY][12] and
 * X,Y coordinates (or pointers) against screen buffer and clip[XY][12].
 * This is useful for debugging clipping bugs.
 */
#ifndef ENABLE_BOUNDS_CHECKS
#define ENABLE_BOUNDS_CHECKS 0
#endif

#if ENABLE_DYNAMIC_PIXEL_FORMAT
typedef unsigned short gxj_pixel16_type;
typedef unsigned int gxj_pixel32_type;
typedef gxj_pixel32_type gxj_pixel_type;

extern int pp_enable_32bit_mode;
#elif ENABLE_32BITS_PIXEL_FORMAT
/**
 * 32-bit pixel.
 */
typedef unsigned int gxj_pixel_type;
typedef gxj_pixel_type gxj_pixel32_type;
#else
/**
 * 16-bit pixel.
 * The color encoding used in pixels is 565, that is,
 * 5+6+5=16 bits for red, green, blue.
 */
typedef unsigned short gxj_pixel_type;
typedef gxj_pixel_type gxj_pixel16_type;
#endif

/** 8-bit alpha */
typedef unsigned char gxj_alpha_type;

/** Screen buffer definition */
typedef struct _gxj_screen_buffer {
    int width;	/**< width in pixel */
    int height;	/**< height in pixel */
    gxj_pixel_type *pixelData; /**< pointer to array of pixel data */
    gxj_alpha_type *alphaData; /**< pointer to array of alpha data */
#if ENABLE_BOUNDS_CHECKS
    java_graphics *g; /**< Associated Graphics object */
#endif
} gxj_screen_buffer;

/**
 * Each port must define one system screen buffer
 * from where pixels are copied to physical screen.
 */
extern gxj_screen_buffer gxj_system_screen_buffer;

#if ENABLE_DYNAMIC_PIXEL_FORMAT

#define GXJ_PIXELTOMIDP_32(x) GXJ_MIDPTOPIXEL_32(x)
#define GXJ_MIDPTOPIXEL_32(x) ( ((x) & 0xFF00FF00) | (((x) << 16) & 0xFF0000) | (((x) >> 16) & 0xFF))

#define GXJ_MIDPTOOPAQUEPIXEL_32(x) GXJ_MIDPTOPIXEL_32( (x) | 0xFF000000 ) 
#define GXJ_PIXELTOOPAQUEMIDP_32(x) ( GXJ_PIXELTOMIDP_32(x) | 0xFF000000 )

#define GXJ_MIDPTOOPAQUEPIXEL_16(x) ((((x) & 0x00F80000) >> 8) | \
                                  (((x) & 0x0000FC00) >> 5) | \
			          (((x) & 0x000000F8) >> 3) )

#define GXJ_PIXELTOOPAQUEMIDP_16(x) ( (((x) & 0x001F) << 3) | (((x) & 0x001C) >> 2) | \
                                   (((x) & 0x07E0) << 5) | (((x) & 0x0600) >> 1) | \
                                   (((x) & 0xF800) << 8) | (((x) & 0xE000) << 3) | 0xFF000000)

#define GXJ_PIXELTOMIDP_16(x, a) ( GXJ_PIXELTOOPAQUEMIDP_16(x) | ((((int)(a)) << 24) & 0xFF000000) )
#define GXJ_MIDPTOPIXEL_16(x) GXJ_MIDPTOOPAQUEPIXEL_16(x) 

#elif ENABLE_RGBA8888_PIXEL_FORMAT

#define GXJ_MIDPTOPIXEL(x) ( (((x) << 8) & 0xFFFFFF00) | (((x) >> 24) & 0xFF) )
#define GXJ_PIXELTOMIDP(x) ( (((x) >> 8) & 0x00FFFFFF) | (((x) << 24) & 0xFF000000) )

#define GXJ_MIDPTOOPAQUEPIXEL(x) ( ((x) << 8) | 0xFF )
#define GXJ_PIXELTOOPAQUEMIDP(x) ( ((x) >> 8) | 0xFF000000 )

#elif ENABLE_ABGR8888_PIXEL_FORMAT

#define GXJ_PIXELTOMIDP(x) GXJ_MIDPTOPIXEL(x)
#define GXJ_MIDPTOPIXEL(x) ( ((x) & 0xFF00FF00) | (((x) << 16) & 0xFF0000) | (((x) >> 16) & 0xFF))

#define GXJ_MIDPTOOPAQUEPIXEL(x) GXJ_MIDPTOPIXEL( (x) | 0xFF000000 ) 
#define GXJ_PIXELTOOPAQUEMIDP(x) ( GXJ_PIXELTOMIDP(x) | 0xFF000000 )

#else

/**
 * @name Accessing pixel colors
 * These macros return separate colors packed as 5- and 6-bit fields
 * in a pixel.
 * The pixel color encoding is 565, that is, 5+6+5=16 bits for red, green,
 * blue.
 * The returned separate colors are 8 bits as in Java RGB.
 * @{
 */
#define GXJ_GET_RED_FROM_PIXEL(P)   (((P) >> 8) & 0xF8)
#define GXJ_GET_GREEN_FROM_PIXEL(P) (((P) >> 3) & 0xFC)
#define GXJ_GET_BLUE_FROM_PIXEL(P)  (((P) << 3) & 0xF8)
/** @} */

/** Convert 24-bit RGB color to 16bit (565) color */
#define GXJ_MIDPTOOPAQUEPIXEL(x) ((((x) & 0x00F80000) >> 8) | \
                                  (((x) & 0x0000FC00) >> 5) | \
			          (((x) & 0x000000F8) >> 3) )

/** Convert 16-bit (565) color to 24-bit RGB color */
#define GXJ_PIXELTOOPAQUEMIDP(x) ( (((x) & 0x001F) << 3) | (((x) & 0x001C) >> 2) | \
                                   (((x) & 0x07E0) << 5) | (((x) & 0x0600) >> 1) | \
                                   (((x) & 0xF800) << 8) | (((x) & 0xE000) << 3) | 0xFF000000)

#define GXJ_PIXELTOMIDP(x, a) ( GXJ_PIXELTOOPAQUEMIDP(x) | ((((int)(a)) << 24) & 0xFF000000) )
#define GXJ_MIDPTOPIXEL(x) GXJ_MIDPTOOPAQUEPIXEL(x) 

#endif

/**
 * Convert a Java platform image object to its native representation.
 *
 * @param jimg Java platform Image object to convert from
 * @param sbuf pointer to Screen buffer structure to populate
 * @param g optional Graphics object for debugging purposes only.
 *          Provide NULL if this parameter is irrelevant.
 *
 * @return the given 'sbuf' pointer for convenient usage,
 *         or NULL if the image is null.
 */
gxj_screen_buffer* gxj_get_image_screen_buffer_impl(const java_imagedata *img,
						    gxj_screen_buffer *sbuf,
						    jobject graphics);


#ifdef __cplusplus
}
#endif

#endif /* _GXJ_PORT_H */
