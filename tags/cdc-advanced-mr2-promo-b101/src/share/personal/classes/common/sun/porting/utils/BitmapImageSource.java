/*
 * @(#)BitmapImageSource.java	1.13 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 */
package sun.porting.utils;

import java.awt.Color;
import java.awt.image.*;

/**
 * A memory image source whose memory representation consists of
 * a packed bitmap (32 bits packed into each int) and an IndexColorModel
 * describing the colors represented by the bits (i.e. the background color
 * maps to pixel value 0 and the foreground color to pixel value 1).
 * @version 1.8
 */
public class BitmapImageSource extends MemoryImageSource {
    /**
     * Create an image source for an image of the given width and height,
     * with packed bitmap data stored in pix[] and using the given 
     * IndexColorModel to translate the bits into colors.
     * @param w The width of the image
     * @param h The height of the image
     * @param pix The bitmap data, packed 32 pixels per int.  The data must
     * be padded so that each scanline starts on an int boundary.
     * @param cm The color model describing the meanings of 0 (background)
     * and 1 (foreground).
     */
    public BitmapImageSource(int w, int h, int pix[], IndexColorModel cm) {
        super(w, h, cm, translatePixels(pix, w, h), 0, w);
    }

    /**
     * Create an IndexColorModel of size 2 that translates color 0 to the given
     * background color and color 1 to the given foreground color.
     * @param fg The desired foreground color
     * @param bg The desired background color
     * @return An IndexColorModel which maps 1/0 to the given colors.
     */
    protected static IndexColorModel makeColorModel(Color fg, Color bg) {
        byte r[] = new byte[2];
        byte g[] = new byte[2];
        byte b[] = new byte[2];
        r[0] = (byte) bg.getRed();
        g[0] = (byte) bg.getGreen();
        b[0] = (byte) bg.getBlue();
        r[1] = (byte) fg.getRed();
        g[1] = (byte) fg.getGreen();
        b[1] = (byte) fg.getBlue();
        return new IndexColorModel(8, 2, r, g, b, 0);
    }

    /**
     * Translate bitmap image data into byte image data.
     * Takes a bitmap packed 32 pixels to an int (i.e. 1 bit of data per pixel,
     * plus padding), and returns one byte of data per pixel, no padding.
     * @param pix The input pixel data (bitmap data)
     * @param width The width of the input bitmap
     * @param height The height of the input bitmap
     * @return A byte array containing the translated bitmap.
     */ 
    protected static byte[] translatePixels(int pix[], int width, int height) {
        byte data[] = new byte[height * width];
        for (int y = 0; y < height; ++y) {
            int ix = y * ((width + 31) >> 5);
            int d = pix[ix++];
            int mask = (int) 0x80000000;
            for (int x = 0; x < width; ++x) {
                if (mask == 0) {
                    d = pix[ix++];
                    mask = (int) 0x80000000;
                }
                if ((mask & d) != 0) {
                    data[y * width + x] = 1;
                }
                mask >>>= 1;
            }
        }
        return data;
    }
}
