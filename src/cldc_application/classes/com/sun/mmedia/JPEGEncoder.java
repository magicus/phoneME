/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 * Wrapper class for native JPEG encoder.
 * 
 */
class JPEGEncoder  extends Encoder {
    public static final String formatString = "jpeg";
    public static final String qualityString = "quality";
        
    public static byte[] encode(int[] rgbData, int width, int height, String encodeParams) {
        String qualityParam = getParamForFormat(encodeParams, formatString, qualityString);
        String widthParam = getParamForFormat(encodeParams, formatString, widthString);
        String heightParam = getParamForFormat(encodeParams, formatString, heightString);

        /* REVISE: where to place defualt value? */
        int quality = 80;
        try {
            quality = Integer.parseInt(qualityParam);        
        } catch (NumberFormatException ex) {
            // Intentionally ignored
        }
        
        int targetWidth = width;
        try {
            targetWidth = Integer.parseInt(widthParam);        
        } catch (NumberFormatException ex) {
            // Intentionally ignored
        }

        int targetHeight = height;
        try {
            targetHeight = Integer.parseInt(heightParam);        
        } catch (NumberFormatException ex) {
            // Intentionally ignored
        }

        byte [] arr = new byte[3 * rgbData.length];
        int idx = 0;
        for (int i = 0; i < rgbData.length; i++) {
            arr[idx++] = (byte)((rgbData[i] >> 16) & 0xFF);
            arr[idx++] = (byte)((rgbData[i] >> 8) & 0xFF);
            arr[idx++] = (byte)(rgbData[i] & 0xFF);
        }
        
        if (targetHeight != height || targetWidth != width) {
            arr = scale(arr, width, height, targetWidth, targetHeight);
        }

        return encode0(arr, targetWidth, targetHeight, quality);
    }

    private static native byte[] encode0(byte[] rgbData, int w, int h, int qaulity);
}
