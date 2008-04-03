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
 * Decodes single images after parsing headers. Helper for GIFPlayer.
 * Performs LZW decoding, palette mapping, interlacing, tranparency follow-up
 */
class GIFImageDecoder {

    final static int UNDRAW_LEAVE = 1;
    final static int UNDRAW_RESTORE_BACKGROUND = 2;
    final static int UNDRAW_RESTORE_PREVIOUS = 3;

    private int width, height, colorDepth, globalColorDepth, curColorDepth;
    private byte[] globalPalette;
    private byte[] curPalette;
    private int[] argb;
    private int[] curArgb;
    private int backgroundIndex;

    private int transparentColorIndex = -1;
    private int undrawFlag = UNDRAW_LEAVE;

    private int framePosX, framePosY, frameWidth, frameHeight;
    private boolean interlace;

    /// Initialize with Global Descriptor data
    GIFImageDecoder(int width, int height, int colorDepth) {
        this.width = width;
        this.height = height;
        this.colorDepth = colorDepth;

        globalColorDepth = colorDepth;
        globalPalette = new byte [3 * (1 << globalColorDepth)];

        // default global color map - increasing grays
        globalPalette[0] = 0;
        globalPalette[1] = 0;
        globalPalette[2] = 0;
        int shift = 8 - globalColorDepth;
        int add = (1 << shift) - 1;
        for (int i = 1; i < (1 << globalColorDepth); ++i) {
            byte gray = (byte)((i << shift) | add);
            globalPalette[3 * i + 0] = gray;
            globalPalette[3 * i + 1] = gray;
            globalPalette[3 * i + 2] = gray;
        }

        backgroundIndex = -1;

        curColorDepth = globalColorDepth;
        curPalette = globalPalette;
        
        argb = new int [width * height];
        for (int i = 0; i < width * height; ++i)
            argb[i] = 0x00badbad; // transparent
    }

    /// Set Global Descriptor's palette data (optional)
    void setGlobalPalette(int colorDepth, byte[] palette, int backgroundIndex) {
        if (curPalette == globalPalette) {
            curPalette = palette;
            curColorDepth = colorDepth;
        }
        globalPalette = palette;
        globalColorDepth = colorDepth;

        this.backgroundIndex = backgroundIndex;
    }

    /// Clear current image to background color
    void clearImage() {
        if (globalPalette != null && backgroundIndex >= 0) {
            int bkg = getColor(backgroundIndex, globalPalette);
            for (int i = 0; i < width * height; ++i)
                argb[i] = bkg;
        }
    }

    /// Get current image (after undraw)
    //int [] getCurrentARGB() {
    //    return argb;
    //}

    /// Get color from palette
    private int getColor(int index, byte[] palette) {
        index *= 3;
        return 0xff000000 | ((palette[index + 0] & 0xff) << 16) |
                            ((palette[index + 1] & 0xff) << 8 ) |
                             (palette[index + 2] & 0xff);
    }
    
    /// Prepare for next frame decoding (set Local Descriptor data)
    void newFrame(int relx, int rely, int width, int height, boolean interlaceFlag) {
        if (relx + width > this.width)
            framePosX = this.width - width;
        else
            framePosX = relx;

        if (rely + height > this.height)
            framePosY = this.height - height;
        else
            framePosY = rely;

        // Relying that frameWidth < this.width && frameHeight << this.height
        frameWidth = width;
        frameHeight = height;
        interlace = interlaceFlag;
    }

    /// Set Local Descriptor's palette data (optional)
    void setLocalPalette(int colorDepth, byte[] palette) {
        curColorDepth = colorDepth;
        curPalette = palette;
    }

    /**
     * Set Local Description Extension data (optional)
     *
     * udrawFlag: see UNDRAW_* constants
     * tranparentColor:  -1 - no transparent color, otherwise - index from color table
     */
    void setGraphicsControl(int undrawFlag, int transparentColorIndex) {
        this.undrawFlag = undrawFlag;
        this.transparentColorIndex = transparentColorIndex;
    }

    /// Decode frame data from dataBlocks to out
    boolean decodeImage(int lzwCodeSize, int dataSize, byte data[], int out[]) {
        curArgb = out;                                            

        // Get saved image
        System.arraycopy(argb, 0, curArgb, 0, width * height);

        // LZW decode
        boolean result = lzwImage(lzwCodeSize, dataSize, data);

        // Undraw
        switch (undrawFlag) {
        case UNDRAW_LEAVE:
        {
            int pixel = framePosX + framePosY * width;
            for (int i = 0; i < frameHeight; ++i, pixel += width)
                System.arraycopy(curArgb, pixel, argb, pixel, frameWidth);
            break;
        }
        case UNDRAW_RESTORE_BACKGROUND:
        if (globalPalette != null && backgroundIndex >= 0) {
            int pixel = framePosX + framePosY * width;
            int wdif = width - frameWidth;
            int bkg = getColor(backgroundIndex, globalPalette);

            for (int i = 0; i < frameHeight; ++i, pixel += wdif)
                for (int j = 0; j < frameWidth; ++j, ++pixel)
                    argb[pixel] = bkg;
            break;
        }
        case UNDRAW_RESTORE_PREVIOUS:
            // Do nothing
            break;
        }
        resetLocalData();

        return result;
    }

    // Should be called at the end of the image parsing
    private void resetLocalData() {
        undrawFlag = UNDRAW_LEAVE;
        transparentColorIndex = -1;
        curColorDepth = globalColorDepth;
        curPalette = globalPalette; 
    }

    private void drawLine(byte[] line, int y/*, int multiply*/) {
        if (y > frameHeight)
            return;
        int pixel = (framePosY + y) * width + framePosX;

        for (int i = 0; i < frameWidth; ++i, ++pixel) {
            int idx = line[i] & 0xff;
            if (idx != transparentColorIndex)
                curArgb[pixel] = getColor(idx, curPalette);
        }
    }

    // ------------------------- LZW decoder ------------------------
    // This is converted from the native version. The java version is 
    // much faster when we have JIT.

    private short prefix[]  = new short[4096];
    private byte  suffix[]  = new byte[4096];
    private byte  outCode[] = new byte[4097];

    /**
     * Parses image, using current palette and graphics control.
     * Returns true if the image has been wholly parsed,
     * false if it was truncated.
     */
    private boolean lzwImage(int initCodeSize, int dataSize, byte block[]) {

        byte [] rasline = new byte[frameWidth];
        
        int OUTCODELENGTH = 4097;

        int clearCode = (1 << initCodeSize);
        int eofCode = clearCode + 1;
        int bitMask;
        int curCode;
        int outCount = OUTCODELENGTH;

        /* Variables used to walk read data */
        int remain = dataSize;
        int byteoff = 0;
        int accumbits = 0;
        int accumdata = 0;

        /* Variables used to decompress the data */
        int codeSize = initCodeSize + 1;
        int maxCode = 1 << codeSize;
        int codeMask = maxCode - 1;
        int freeCode = clearCode + 2;
        int code = 0;
        int oldCode = 0;
        char prevChar = 0;

        /* Variables used for writing pixels */
        int x = frameWidth;
        int y = 0;
        int off = 0;
        int passinc = interlace ? 8 : 1;
        int passht = passinc;

        bitMask = (1 << curColorDepth) - 1;

        /* Read codes until the eofCode is encountered */
        for (;;) {
            if (accumbits < codeSize) {
                while (remain < 2) {
                    /* Sometimes we have one last byte to process... */
                    if (remain == 1 && accumbits + 8 >= codeSize)
                        break;

                    if (off > 0)
                        drawLine(rasline, y/*, passht*/);
                        
                    /* quietly accept truncated GIF images */
                    return false;
                }

                /* 2 bytes at a time saves checking for accumbits < codeSize.
                 * We know we'll get enough and also that we can't overflow
                 * since codeSize <= 12.
                 */
                if (remain >= 2) {
                    remain -= 2;
                    accumdata += (block[byteoff++] & 0xff) << accumbits;
                    accumbits += 8;
                    accumdata += (block[byteoff++] & 0xff) << accumbits;
                    accumbits += 8;
                } else {
                    remain--;
                    accumdata += (block[byteoff++] & 0xff) << accumbits;
                    accumbits += 8;
                }
            }

            /* Compute the code */
            code = accumdata & codeMask;
            accumdata >>= codeSize;
            accumbits -= codeSize;

            /*
             * Interpret the code
             */
            if (code == clearCode) {
                /* Clear code sets everything back to its initial value, then
                 * reads the immediately subsequent code as uncompressed data.
                 */

                /* Note that freeCode is one less than it is supposed to be,
                 * this is because it will be incremented next time round the 
                 * loop
                 */
                freeCode = clearCode + 1;
                codeSize = initCodeSize + 1;
                maxCode = 1 << codeSize;
                codeMask = maxCode - 1;

                /* Continue if we've NOT reached the end, some Gif images
                 * contain bogus codes after the last clear code.
                 */
                if (y < height)
                    continue;

                return true;
            }

            if (code == eofCode)
                return true;

            /* It must be data: save code in CurCode */
            curCode = code;
            /* Whenever it gets here outCount is always equal to 
               OUTCODELENGTH, so no need to reset outCount. */
            //outCount = OUTCODELENGTH;

            /* If greater or equal to freeCode, not in the hash table
             * yet; repeat the last character decoded
             */
            if (curCode >= freeCode) {
                if (curCode > freeCode)
                    /*
                     * if we get a code too far outside our range, it
                     * could case the parser to start traversing parts
                     * of our data structure that are out of range...
                     */
                    return true;

                curCode = oldCode;
                outCode[--outCount] = (byte)prevChar;
            }

            /* Unless this code is raw data, pursue the chain pointed
             * to by curCode through the hash table to its end; each
             * code in the chain puts its associated output code on
             * the output queue.
             */
             while (curCode > bitMask) {
                 outCode[--outCount] = suffix[curCode];
                 if (outCount == 0)
                     /*
                      * In theory this should never happen since our
                      * prefix and suffix arrays are monotonically
                      * decreasing and so outCode will only be filled
                      * as much as those arrays, but I don't want to
                      * take that chance and the test is probably
                      * cheap compared to the read and write operations.
                      * If we ever do overflow the array, we will just
                      * flush the rest of the data and quietly accept
                      * the GIF as truncated here.
                      */
                    return true;

                 curCode = prefix[curCode];
             }

            /* The last code in the chain is treated as raw data. */
            prevChar = (char)curCode;
            outCode[--outCount] = (byte)prevChar;

            /* Now we put the data out to the Output routine. It's
             * been stacked LIFO, so deal with it that way...
             */
            int len = OUTCODELENGTH - outCount; /* This is why I commented out
                                               the code that resets outCount. */
            while (--len >= 0) {
                rasline[off++] = outCode[outCount++];

                /* Update the X-coordinate, and if it overflows, update the
                 * Y-coordinate
                 */
                if (--x == 0) {
                    /* If a non-interlaced picture, just increment y to the next
                     * scan line.  If it's interlaced, deal with the interlace as
                     * described in the GIF spec.  Put the decoded scan line out
                     * to the screen if we haven't gone past the bottom of it
                     */
                    drawLine(rasline, y/*, passht*/);

                    x = frameWidth;
                    off = 0;
                    /*  pass        inc     ht      ystart */
                    /*   0           8      8          0   */
                    /*   1           8      4          4   */
                    /*   2           4      2          2   */
                    /*   3           2      1          1   */
                    y += passinc;
                    while (y >= height) {
                        passinc = passht;
                        passht >>= 1;
                        y = passht;
                        if (passht == 0)
                            return true;
                    }
                }
            }

            /* Build the hash table on-the-fly. No table is stored in the file. */
            prefix[freeCode] = (short)oldCode;
            suffix[freeCode] = (byte)prevChar;
            oldCode = code;
            /* Point to the next slot in the table.  If we exceed the
             * maxCode, increment the code size unless
             * it's already 12.  If it is, do nothing: the next code
             * decompressed better be CLEAR
             */
            if (++freeCode >= maxCode) {
                if (codeSize < 12) {
                    codeSize++;
                    maxCode <<= 1;
                    codeMask = maxCode - 1;
                } else
                    /* Just in case */
                    freeCode = maxCode - 1;
            }
        }
    }
}
