/*
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
package com.sun.mmedia;

/**
 * Utility class that provides I/F to native 
 * image/audio/video format conversion code:
 *
 * currently implemented image RGB -to-JPEG & -to-PNG conversion
 */
public final class ImageEncoder {

    public static final int CONVERT_RGB_TO_JPEG = 1;
    public static final int CONVERT_RGB_TO_PNG = 2;
    /**
     * converts xrgb image to jpeg or png format.
     * this function is platform independent, since
     * byte ordder doesn't depend on big-/little-endian platform.
     *
     * @param xrgb raw RGB data in form of byte array (4 bytes per pixel)
     * @param w width of frame
     * @param h height of frame
     * @param qual quality - for JPEG
     * @param compressed data to be output
     * @param encType 1 for JPEG, 2 for PNG
     * @return the size of the compressed data
     */
    public static native int RGBByteCompress(byte[] xrgb, int w, int h, 
        int qual, byte[] compressed, int encType);
    /**
     * converts xrgb image to jpeg or png format
     * this function is platform DEPENDENT, since
     * byte order in int depends on big-/little-endian platform !
     * Current implementation works for little-endian only (x86) !
     *
     * @param xrgb raw RGB data in form of int array (32-bit per pixel)
     * @param w width of frame
     * @param h height of frame
     * @param qual quality - for JPEG
     * @param compressed data to be output
     * @param encType 1 for JPEG, 2 for PNG
     * @return the size of the compressed data
     */
    public static native int RGBIntCompress(int [] xrgb, int w, int h, 
        int qual, byte[] compressed, int encType);
}
