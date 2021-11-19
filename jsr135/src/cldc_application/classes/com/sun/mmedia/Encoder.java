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

import javax.microedition.lcdui.Image;

class Encoder {
    public static final String widthString = "width";
    public static final String heightString = "height";

    static String getParamForFormat(String encodingParams, String format, String paramName) {
        String formatString = "encoding="+format;
        // looking for beginning of encoding string
        int i = encodingParams.indexOf(formatString);
        if (-1 == i) {
            return null;
        }
        // looking for end of encoding params
        int j = encodingParams.indexOf(' ', i);
        if (-1 == j) {
            j = encodingParams.length();
        }
        // looking for first "paramName" value
        String paramNameString = paramName + "=";
        int k = encodingParams.indexOf(paramNameString);
        if (k > j || -1 == k) {
            // parameter is absent or belongs to different format
            // e.g. if user provide string like "encoding=jpg encoding=png&width=16&height=16"
            // "width" parameter for jpeg encoder has to be  null
            return null;
        }
        // looking for last "paramName" value in sequence
        // e.g. for "encoding=png&width=16&width=50" it has to be 50
        int lastParamIndex;
        do {
            lastParamIndex = k;
            k = encodingParams.indexOf(paramNameString, k+1);
        } while (k > lastParamIndex && k < j);

        // looking for end of "paramName" value string
        i = encodingParams.indexOf('&', lastParamIndex);
        if (-1 == i) {
            // ther is no more paramters
            i = j;
        }

        return encodingParams.substring(lastParamIndex + paramNameString.length(), i);
    }

    static byte[] scale(byte[] rgbData, int oldWidth, int oldHeight, int newWidth, int newHeight) {
        int width = newWidth;
        if (oldWidth < newWidth) {
            width = oldWidth;
        }
        int height = newHeight;
        if (oldHeight < newHeight) {
            height = oldHeight;
        }
        
        // Just stub. Need to implement scaling algorithm
        byte[] rgbRes = new byte[newWidth * newHeight];
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                rgbRes[j * height + i] = rgbData[j * width + i];                    
            }
        }
        return rgbRes;
    }
}
