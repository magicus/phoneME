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
 
#ifndef __JAVACALL_IMAGE_H
#define __JAVACALL_IMAGE_H

/**
 * @file javacall_image.h
 * @ingroup optionalImage
 * @brief Javacall interfaces for external image decoder
 */

#include "javacall_defs.h"
#include "javacall_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup optionalImage Image Decoder API 
 * @ingroup JTWI
 * 
 * NOTE: The following functions are optional.
 *
 * This porting layer gives a freedom of choice to customer whether 
 * they use their own image decoder library or use Sun¡¯s default implementation.
 * Sun¡¯s default image decoder library support PNG type. It generally shows good performance. 
 * But, If customer has more optimized solutions like H/W supported image processor or highly 
 * optimized S/W library they can choose to use this solution rather than Sun¡¯s default implementation.
 *
 * @{
 */

/**
 * @brief Image information return type
 */
typedef struct {
    /** Width of image */
    long width;
    /** Height of image */
    long height;
    /** Is this image has alpha value? */
    javacall_bool hasAlpha;
} javacall_image_info;

/**
 * Get image information
 * 
 * @param source        Pointer to image data source
 * @param sourceSize    Size of source in bytes
 * @param imageInfo     Pointer to javacall_image_info type buffer
 * 
 * @retval JAVACALL_OK      
 *          imageData is acceptable format and image information returned by imageInfo argument     
 * @retval JAVACALL_FAIL    
 *          imageData is not acceptable
 */
javacall_result javacall_image_get_info(const void* source, 
                                        long sourceSize,
                                        /*OUT*/ javacall_image_info* imageInfo);

/**
 * Start a decoding transaction of the source image to array of javacall_pixel type
 *
 * For Synchronous decoding:
 *   -# Copy the address of the source buffer to a static variable (also the handle can be used for that).
 *   -# Return JAVACALL_OK
 *
 * For Asynchronous decoding:
 *   -# Allocate a memory block of size sourceSize
 *   -# Copy the source buffer to the new allocated buffer.
 *   -# Send the buffer to the decoder which is in another task.
 *   -# Mark the decoding operation with a unique handle.
 *   -# Return JAVACALL_WOULD_BLOCK
 *
 * @param source        Pointer to image data source
 * @param sourceSize    Byte size of source
 * @param width         Width of source data
 * @param height        Height of source data
 * @param handle        Handle of this image decoding. This could be used from async decoding mode
 *                      It could return a NULL value, if this API return JAVACALL_OK
 *
 * @retval JAVACALL_OK          Decoding OK! - Synchronous decoding
 * @retval JAVACALL_FAIL        Fail
 * @retval JAVACALL_WOULD_BLOCK Image decoding performed asynchronously
 */
javacall_result javacall_image_decode_start(const void* source, 
                                            long sourceSize,
                                            long width, 
                                            long height,
                                            /*OUT*/ javacall_handle* handle);

/**
 * Finalize the image decoding transaction
 *
 * For Synchronous decoding:
 *   1.This function will be called right after javacall_image_decode_start() returns with JAVACALL_OK. 
 *   2.Decode the source buffer that was stored previously into decodeBuf , alphaBuf.
 * For Asynchronous decoding:
 *   1.This function will be called after javanotify_on_image_decode_end is invoked.
 *   2.The platform should only copy the decoded image to decodeBuf , alphaBuf.
 *
 * @param handle        The handle get from javacall_image_decode_start
 * @param decodeBuf     Pointer to decoding target buffer
 * @param decodeBufSize Size of decodeBuf in bytes
 * @param alphaBuf      Pointer to alpha data buffer
 *                      It could be a NULL if there is no need to get alpha data
 * @param alphaBufSize  Size of alphaBuf in bytes
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_image_decode_finish(javacall_handle handle,
                                             /*OUT*/ javacall_pixel* decodeBuf, 
                                             long decodeBufSize,
                                             /*OUT*/ char* alphaBuf, 
                                             long alphaBufSize);

/**
 * The implementation call this callback notify function when image decode done
 *
 * @param handle Handle that is returned from javacall_image_decode_start
 * @param result the decoding operation result
 */
void javanotify_on_image_decode_end(javacall_handle handle, 
                                    javacall_result result);


/** @} */
        

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_IMAGE_H */

    
