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

package com.sun.midp.automation;
import java.io.*;


/**
 * Encodes image data in RGB888 format into uncompressed BMP
 */
class BMPEncoder {
    /** Magic number that identifies BMP format */
    private final static byte[] MAGIC = { 0x42, 0x4D };

    /** The size of BMP file header */
    private final static int FILE_HEADER_SIZE = 14;

    /** The size of DIB header */
    private final static int DIB_HEADER_SIZE = 40;

    /** Image data to encode */
    private byte[] rgb888;

    /** Image width */
    private int width;

    /** Image height */
    private int height;

    /** BMP raw padding */
    private int padding;

    /** BMP file size */
    private int fileSize;

    /** Output stream to write BMP into */
    private DataOutputStream os;

    /** 
     * Constructor.
     *
     * @param rgb888 image data
     * @param width image width
     * @param height image height
     */
    BMPEncoder(byte[] rgb888, int width, int height) {
        this.rgb888 = rgb888;
        this.width = width;
        this.height = height;

        padding = (4 - (width % 4)) % 4;
        int dataSize = (width * 3 + padding) * height;
        fileSize = FILE_HEADER_SIZE + DIB_HEADER_SIZE + dataSize;

    }

    /**
     * Encodes image data into uncompressed BMP.
     *
     * @return byte array representing BMP file
     */
    byte[] encode() {
        ByteArrayOutputStream baos = new ByteArrayOutputStream(fileSize);
        os = new DataOutputStream(baos);
        byte[] bmp = null;
       
        try {
            writeFileHeader();
            writeDIBHeader();
            writeData();

            bmp = baos.toByteArray();
        } catch (Exception e) {
        } finally {
            try {
                baos.close();
            } catch (Exception ee) {
            }
        }

        return bmp;
    }

    /**
     * Writes BMP file header
     */
    private void writeFileHeader() 
        throws java.io.IOException {

        // the magic number
        for (int i = 0; i < MAGIC.length; ++i) {
            os.writeByte(MAGIC[i]);
        }

        // the size of the BMP file in bytes
        os.writeInt(toLittleEndian(fileSize));

        // reserved
        os.writeShort(0);
        os.writeShort(0);

        // the offset, i.e. starting address, of the byte where 
        // the bitmap data can be found.
        os.writeInt(toLittleEndian(FILE_HEADER_SIZE + DIB_HEADER_SIZE));
    }

    /**
     * Writes DIB header
     */
    private void writeDIBHeader() 
        throws java.io.IOException {

        // the size of this header
        os.writeInt(toLittleEndian(DIB_HEADER_SIZE));

        // the bitmap width in pixels (signed integer)
        os.writeInt(toLittleEndian(width));

        // the bitmap height in pixels (signed integer)
        os.writeInt(toLittleEndian(height));

        // the number of color planes being used. must be set to 1
        os.writeShort(toLittleEndian((short)1));

        // the number of bits per pixel, which is the color depth of the image
        os.writeShort(toLittleEndian((short)24));

        // the compression method being used
        os.writeInt(0);

        // image size. 0 is valid for uncompressed
        os.writeInt(0);

        // the horizontal resolution of the image
        os.writeInt(0);

        // the vertical resolution of the image
        os.writeInt(0);

        // the number of colors in the color palette, or 0 to default to 2^n
        os.writeInt(0);

        // the number of important colors used, generally ignored
        os.writeInt(0);
    }

    /**
     * Writes BMP pixel data
     */
    private void writeData() 
        throws java.io.IOException {

        for (int j = height - 1; j >= 0; j--) {
            for (int i = 0; i < width; i++) {
                int val = rgb888[i + width * j];
                os.writeByte(val & 0x000000FF);
                os.writeByte((val >>> 8) & 0x000000FF);
                os.writeByte((val >>> 16) & 0x000000FF);
            }

            // number of bytes in each row must be padded to multiple of 4
            for (int i = 0; i < padding; i++) {
                os.writeByte(0);
            }
        }        
    }

    /**
     * Converts integer value into little-endian format
     *
     * @param val value to convert
     * @return integer value in little-endian format
     */
    private static int toLittleEndian(int val) {
        int b1 = val & 0xff;
        int b2 = (val >> 8) & 0xff;
        int b3 = (val >> 16) & 0xff;
        int b4 = (val >> 24) & 0xff;

        int rv = (b1 << 24 | b2 << 16 | b3 << 8 | b4 << 0);

        return rv;
    }

    /**
     * Converts short value into little-endian format
     *
     * @param val value to convert
     * @return short value in little-endian format
     */
    private static int toLittleEndian(short val) {
        int b1 = val & 0xff;
        int b2 = (val >> 8) & 0xff;

        short rv = (short)(b1 << 8 | b2 << 0);

        return rv;
    }
}
