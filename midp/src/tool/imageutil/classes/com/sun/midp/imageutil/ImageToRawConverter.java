
/**
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
 *
 *
 * This file should be compiled and run using standard j2se.
 */

package com.sun.midp.imageutil;

import java.io.*;

/**
 * This class generates raw data from image according to 
 * specified raw format and pixel format.
 * Supported formats are listed in static formatList array.
 * To add new supported format one should define new raw or/and
 * color format constants if needed, update static formatList array,
 * implement new conversion function byte[] _function_name_(BufferedImage image)
 * and update imageToByteArray function to support new format.
 */

public class ImageToRawConverter {

    /** value for invalid format indication */
    static public final int FORMAT_INVALID     = -1;
    /** Put Pixel raw format */
    static public final int RAW_FORMAT_PP      = 0;
    /** RGBA raw format */
    static public final int RAW_FORMAT_ARGB    = 1;

    /** pixel format - 24bit color */
    static public final int COLOR_FORMAT_888   = 0;
    /** pixel format - 16bit color */
    static public final int COLOR_FORMAT_565   = 1;

    /** int byte order - little-endian */
    static public final int INT_FORMAT_LITTLE_ENDIAN   = 0;
    /** int byte order - big-endian */
    static public final int INT_FORMAT_BIG_ENDIAN   = 1;

    /** list of supported pairs raw format - color format */
    static final int formatList [][]  = {
        {RAW_FORMAT_PP, COLOR_FORMAT_565}, 
        {RAW_FORMAT_ARGB, COLOR_FORMAT_888}
    };

    /** byte sequence that indentifies raw format */
    static final short[] rawMagic = { 0x89, 'S', 'U', 'N'};

    /** current raw, color and int formats */
    protected int  rawFormat, colorFormat, intFormat;

    /**
     * Defines if format is currently supported.
     *
     * @param rawFormat raw format
     * @param colorFormat color format
     * @return true if format is supported false otherwise
     */    
    static public boolean isFormatSupported(int rawFormat, int colorFormat)
    {
        for (int i = 0; i < formatList.length; ++i) {
            if ((formatList[i][0] == rawFormat) &&
                (formatList[i][1] == colorFormat)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Constructs instance of ImageToRawConverter which will
     * convert images to raw data in specified here format.
     * Raw format and color format should be in list of
     * supported formarts, otherwise IllegalArgumentException
     * will be thrown.
     *
     * @param rawFormat  required raw format
     * @param colorFormat  required color format
     * @param endian required int format
     * @exception IllegalArgumentException if unsupported format
     */
    public ImageToRawConverter(int rawFormat, int colorFormat, int endian)
        throws IllegalArgumentException
    {
        if (!isFormatSupported(rawFormat, colorFormat)) {
            throw new IllegalArgumentException("invalid format");
        }
        this.rawFormat = rawFormat;
        this.colorFormat = colorFormat;
        this.intFormat = endian;
    }    

    /**
     * Converts image to raw data in byte array.
     *
     * @param imageData image pixels in 32bit ARGB format
     * @param width image width
     * @param height image height
     * @param hasAlpha true if image has alpha channel
     * @return byte[] raw data
     */
    public byte[] convertToRaw(int[] imageData, int width, int height, 
            boolean hasAlpha)
    {
        if (imageData == null) {
            throw new IllegalArgumentException("Source image data is null");
        }

        byte [] ret = null;
        // build raw data
        if ((rawFormat == RAW_FORMAT_PP) && 
            (colorFormat == COLOR_FORMAT_565)) {
            ret = imageToPutpixel565(imageData, width, height, hasAlpha);
        } else if ((rawFormat == RAW_FORMAT_ARGB) && 
            (colorFormat == COLOR_FORMAT_888)) {
            ret = imageToARGB888(imageData, width, height, hasAlpha);
        }
        return ret;
    }


    /** 
     * Convert 24bit color to 16bit color 
     * @param x the color in format 8bit-R 8bit-G 8bit-B to convert
     * @return int 16bit color in format 5bit-R 6bit-G 5bit-B
     */
    private short RGB888TORGB565(int x) 
    {
        return ((short)(((x & 0x00F80000) >> 8) + 
                ((x & 0x0000FC00) >> 5) + 
                ((x & 0x000000F8) >> 3)));
    }

    /**
     * Converts image to PutPixel raw format, 16bit color format, big-endian.
     * Output byte array represents the following c-struct:
     * typedef struct {
     *     byte header[4];  // Must equal RAW_HEADER 
     *     int32 width;
     *     int32 height;
     *     int32 hasAlpha;
     *     byte data[1];    // variable length byte array 
     * } MIDP_IMAGE_BUFFER_RAW;
     * where RAW image file header
     * const byte RAW_HEADER[4] = {0x89, 'S', 'U', 'N'};
     * and data array consists of image pixel array - 16bit per pixel, 
     * RGB(5, 6, 5) - and following alpha channel array if any - 8bit per pixel
     *
     * @param imageData image pixels in 32 bit ARGB format
     * @param width image width
     * @param height image height
     * @param hasAlpha true if the image has alpha channel
     * @return byte[] raw data in PutPixel raw format and 16bit color format
     */
    private byte[] imageToPutpixel565(int[] imageData, int width, int height, 
            boolean hasAlpha)
    {
        hasAlpha = reallyHasAlpha(imageData);

        // sizeof resulting raw buffer = 
        // sizeof(RAW_HEADER) + 
        // sizeof(MIDP_IMAGE_BUFFER_RAW.width) + 
        // sizeof(MIDP_IMAGE_BUFFER_RAW.height) + 
        // sizeof(MIDP_IMAGE_BUFFER_RAW.hasAlpha) + 
        // sizeof(pixe_l565) * image_pixel_count + 
        // (hasAlpha ? alpha_size * image_pixel_count : 0)

        int rawsz = 4 + 4 + 4 + 4 + 2 * imageData.length;
        int dataOffset = 4 + 4 + 4 + 4;
        int alphaOffset = rawsz;
        if (hasAlpha) rawsz += 1 * imageData.length;

        byte[] rawData = new byte[rawsz];
        
        fillRawHeader(rawData, width, height, hasAlpha);
        
        for (int i = 0; i < imageData.length; ++i) {
            short val = RGB888TORGB565(imageData[i]);
            storeValue(rawData, dataOffset + i * 2, val, intFormat);
            if (hasAlpha) {
                rawData[alphaOffset + i] = (byte)((imageData[i] >> 24) & 0xFF);
            }
        }

        return rawData;
    }

    /**
     * Converts image to ARGB with 24bits per pixel in big-endian.
     * Output byte array represents the following c-struct:
     * typedef struct {
     *     byte header[4];  // Must equal RAW_HEADER 
     *     int32 width;
     *     int32 height;
     *     int32 hasAlpha;
     *     byte data[1];    // variable length byte array 
     * } MIDP_IMAGE_BUFFER_RAW;
     * where RAW image file header
     * const byte RAW_HEADER[4] = {0x89, 'S', 'U', 'N'};
     * and data array consists of image pixel array - 24bit per pixel, 
     * RGBA(8, 8, 8, 8)
     *
     * @param imageData image pixels in 32 bit ARGB format
     * @param width image width
     * @param height image height
     * @param hasAlpha true if the image has alpha channel
     * @return byte[] raw data in RGBA format
     */
    private byte[] imageToARGB888(int[] imageData, int width, int height, 
            boolean hasAlpha)
    {
        hasAlpha = reallyHasAlpha(imageData);

        // sizeof resulting raw buffer = 
        // sizeof(RAW_HEADER) + 
        // sizeof(MIDP_IMAGE_BUFFER_RAW.width) + 
        // sizeof(MIDP_IMAGE_BUFFER_RAW.height) + 
        // sizeof(MIDP_IMAGE_BUFFER_RAW.hasAlpha) + 
        // sizeof(pixe_l565) * image_pixel_count + 
        // (hasAlpha ? alpha_size * image_pixel_count : 0)

        int rawsz = 4 + 4 + 4 + 4 + 4 * imageData.length;
        int dataOffset = 4 + 4 + 4 + 4;

        byte[] rawData = new byte[rawsz];

        fillRawHeader(rawData, width, height, hasAlpha);

        for (int i = 0; i < imageData.length; ++i) {
            // write ARGB
            storeValue(rawData, dataOffset + i*4, imageData[i], intFormat);
        }

        return rawData;

    }

    /**
     * Fills byte array with raw header 
     *     byte RAW_HEADER[4] = {0x89, 'S', 'U', 'N'};
     *     int32 width;
     *     int32 height;
     *     int32 hasAlpha;
     *
     * @param rawData byte array to fill header for, length of that
     *           array must be greater than 16, otherwise function fails 
     *           (returns false)
     * @param width width of the image
     * @param height height of the image
     * @param hasAlpha if image has alpha channel
     */
    void fillRawHeader(byte [] rawData, int width, int height, 
                              boolean hasAlpha)
    {
        int magicLength = rawMagic.length;
        for (int i = 0; i < magicLength; ++i) {
            rawData[i] = (byte)(rawMagic[i] & 0xFF);
        }
            
        storeValue(rawData, magicLength, width, intFormat);
        storeValue(rawData, magicLength + 4, height, intFormat);
        storeValue(rawData, magicLength + 8, 
                (int)(hasAlpha ? 1 : 0), intFormat);
    }
    
    /**
     * writes int to byte array at specified position 
     * in big- or little- endian.
     *
     * @param data target byte array 
     * @param offset offset in byte array
     * @param value source integer
     * @param endian endian type
     */
    private static void storeValue(byte [] data, int offset, 
                                   int value, int endian)
    {
        if (endian == INT_FORMAT_BIG_ENDIAN) {
            data[offset + 0] = (byte)((value >> 24) & 0xFF);
            data[offset + 1] = (byte)((value >> 16) & 0xFF);
            data[offset + 2] = (byte)((value >> 8) & 0xFF);
            data[offset + 3] = (byte)(value & 0xFF);
        } else { 
            data[offset + 0] = (byte)(value & 0xFF);
            data[offset + 1] = (byte)((value >> 8) & 0xFF);
            data[offset + 2] = (byte)((value >> 16) & 0xFF);
            data[offset + 3] = (byte)((value >> 24) & 0xFF);
        }
    }

    /**
     * writes short to byte array at specified position 
     * in big- or little- endian.
     *
     * @param data target byte array 
     * @param offset offset in byte array
     * @param value source integer
     * @param endian endian type
     */
    private static void storeValue(byte [] data, int offset, 
                                   short value, int endian)
    {
        if (endian == INT_FORMAT_BIG_ENDIAN) {
            data[offset + 0] = (byte)((value >> 8) & 0xFF);
            data[offset + 1] = (byte)(value & 0xFF);
        } else { 
            data[offset + 0] = (byte)(value & 0xFF);
            data[offset + 1] = (byte)((value >> 8) & 0xFF);
        }
    }

    /**
     * Checks if the image that supposely has alpha channel really
     * has it, i.e there is at least one non opaque pixel.
     * 
     * @param imageData image data in 32 bits ARGB format
     * @return true if the image really has alpha channel
     */
    private static boolean reallyHasAlpha(int[] imageData) {
        boolean hasAlpha = false;
        for (int i = 0; i < imageData.length; ++i) {
            if ((imageData[i] & 0xFF000000) != 0xFF000000) {
                hasAlpha = true;
                break;
            }
        }

        return hasAlpha;
    }
}
