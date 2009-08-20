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

#ifndef __PORTING_FONT_H
#define __PORTING_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_font.h
 * @ingroup MandatoryFont
 * @brief Javacall interfaces for font drawing
 */
    
#include "javacall_lcd.h"
    

/**
 * @defgroup MandatoryFont Font API
 * @ingroup JTWI
 *
 * The following APIs are required for Font support, both for drawing directly to
 * the video memory (VRAM) and for drawing to offscreen memory images
 *
 * @{
 */

/* flags for font descriptors */
    
/**
 * @int javacall_font_size
 * @brief Font size
 * 
 * Made not enum to support free integeral sizes on special devices  
 */

typedef int javacall_font_size;

/** 
 * Define the following constants in javacall_platform_defs.h:
 * JAVACALL_FONT_SIZE_SMALL
 * JAVACALL_FONT_SIZE_MEDIUM
 * JAVACALL_FONT_SIZE_LARGE
 */


/**
 * @enum javacall_font_style
 * @brief Font style
 */
typedef enum {
    /** The plain style constant. */
    JAVACALL_FONT_STYLE_PLAIN         =0x00,
    /** The bold style constant. */
    JAVACALL_FONT_STYLE_BOLD          =0x01,
    /** The italicized style constant. */
    JAVACALL_FONT_STYLE_ITALIC        =0x02,
    /** The underlined style constant. */
    JAVACALL_FONT_STYLE_UNDERLINE     =0x04
} javacall_font_style;    

/**
 * @enum javacall_font_face
 * @brief Font face
 */
typedef enum {
    /** The "system" font face. */
    JAVACALL_FONT_FACE_SYSTEM         =0x00,
    /** The "monospace" font face. */
    JAVACALL_FONT_FACE_MONOSPACE      =0x20,
    /** The "proportional" font face. */
    JAVACALL_FONT_FACE_PROPORTIONAL   =0x40
} javacall_font_face;    
    

/**
 * Set font appearance params 
 * 
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @return <tt>JAVACALL_OK</tt> if font set successfully, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_font_set_font( javacall_font_face face, 
                                        javacall_font_style style, 
                                        javacall_font_size size);
    
    
    
/**
 * Draws the first n characters to Offscreen memory image specified using the current font,
 * color.
 *
 * Example 1 :
 * clipX1 = 30, clipY1 = 30, clipX2 = 450, clipY2 = 250, 
 * destBufferHoriz = 400, destBufferVet = 300, x = 40, y = 100
 * @image HTML font1.png
 *
 * Example 2 :
 * clipX1 = 30, clipY1 = 30, clipX2 = 100, clipY2 = 250, 
 * destBufferHoriz = 400, destBufferVet = 300, x = 40, y = 100
 * @image HTML font2.png
 *
 * Example 3 :
 * clipX1 = 30, clipY1 = 30, clipX2 = 100, clipY2 = 250, 
 * destBufferHoriz = 400, destBufferVet = 300, x = -20, y = 100
 * @image HTML font3.png
 *
 * @param color color of font
 * @param clipX1 top left X coordinate of the clipping area where the pixel 
 *               (clipX1,clipY1) is the first top-left pixel in the clip area.
 *               clipX1 is guaranteeded to be larger or equal 0 and smaller or equal 
 *               than clipX2.
 * @param clipY1 top left Y coordinate of the clipping area where the pixel 
 *               (clipX1,clipY1) is the first top-left pixel in the clip area.
 *               clipY1 is guaranteeded to be larger or equal 0 and smaller or equal 
 *               than clipY2
 * @param clipX2 bottom right X coordinate of the clipping area where the pixel 
 *               (clipX2,clipY2) is the last bottom right pixel in the clip area.
 *               clipX2 is guaranteeded to be larger or equal than clipX1 and 
 *               smaller or equal than destBufferHoriz.
 * @param clipY2 bottom right Y coordinate of the clipping area where the pixel 
 *               (clipX2,clipY2) is the last bottom right pixel in the clip area
 *               clipY2 is guaranteeded to be larger or equal than clipY1 and 
 *               smaller or equal than destBufferVert.
 * @param destBuffer  where to draw the chars
 * @param destBufferHoriz horizontal size of destination buffer
 * @param destBufferVert  vertical size of destination buffer
 * @param x The x coordinate of the top left font coordinate 
 * @param y The y coordinate of the top left font coordinate 
 * @param text Pointer to the characters to be drawn
 * @param textLen The number of characters to be drawn
 * @return <tt>JAVACALL_OK</tt> if font rendered successfully, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error or not supported
 */
javacall_result javacall_font_draw(javacall_pixel   color, 
                        int                         clipX1, 
                        int                         clipY1, 
                        int                         clipX2, 
                        int                         clipY2,
                        javacall_pixel*             destBuffer, 
                        int                         destBufferHoriz, 
                        int                         destBufferVert,
                        int                         x, 
                        int                         y, 
                        const javacall_utf16*     text, 
                        int                         textLen);
    
    
    
/**
 * Query for the font info structure for a given font specs
 *
 * @image HTML font4.png
 *
 * @param face The font face to be used (Defined in <B>Font.java</B>)
 * @param style The font style to be used (Defined in
 * <B>Font.java</B>)
 * @param size The font size to be used. (Defined in <B>Font.java</B>)
 *
 * @param ascent return value of font's ascent
 * @param descent return value of font's descent
 * @param leading return value of font's leading 
 * 
 * @return <tt>JAVACALL_OK</tt> if successful, <tt>JAVACALL_FAIL</tt> or 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error or not supported
 *
 */
javacall_result javacall_font_get_info( javacall_font_face  face, 
                                        javacall_font_style style, 
                                        javacall_font_size  size, 
                                        /*out*/ int* ascent,
                                        /*out*/ int* descent,
                                        /*out*/ int* leading);
        
/**
 * return the char width for the first n characters in charArray if
 * they were to be drawn in the font indicated by the parameters.
 *
 *
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @param text The string to be measured
 * @param textLen The number of character to be measured
 * @return total advance width in pixels (a non-negative value)
 *         or 0 if not supported
 */
int javacall_font_get_width(javacall_font_face      face, 
                            javacall_font_style     style, 
                            javacall_font_size      size,
                            const javacall_utf16* text, 
                            int                     textLen);
/** @} */
    
    
#ifdef __cplusplus
} //extern "C"
#endif

#endif

