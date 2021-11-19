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


#ifdef __cplusplus
extern "C" {
#endif
    
    
#include "javacall_font.h"

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
                                        javacall_font_size size) {
    return JAVACALL_FAIL;                                        
}
    
    
    
/**
 * Draws the first n characters to Offscreen memory image specified using the current font,
 * color.
 * 
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
                        int                         textLen){
    return JAVACALL_FAIL;
}
    
    
    
/**
 * Query for the font info structure for a given font specs
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
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 *
 */
javacall_result javacall_font_get_info( javacall_font_face  face, 
                                        javacall_font_style style, 
                                        javacall_font_size  size, 
                                        /*out*/ int* ascent,
                                        /*out*/ int* descent,
                                        /*out*/ int* leading) 
{
    *ascent =0;
    *descent=0;
    *leading=0;
    return JAVACALL_FAIL;
}
        
/**
 * return the char width for the first n characters in charArray if
 * they were to be drawn in the font indicated by the parameters.
 *
 *
 * @param face The font face to be used
 * @param style The font style to be used
 * @param size The font size to be used
 * @param charArray The string to be measured
 * @param charArraySize The number of character to be measured
 * @return total advance width in pixels (a non-negative value)
 */
int javacall_font_get_width(javacall_font_face     face, 
                            javacall_font_style    style, 
                            javacall_font_size     size,
                            const javacall_utf16* charArray, 
                            int                    charArraySize) {
    return 0;
}
    
    
#ifdef __cplusplus
} //extern "C"
#endif




