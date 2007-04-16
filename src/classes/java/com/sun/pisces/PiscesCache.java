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
 
 
package com.sun.pisces;

/**
 * An object used to cache pre-rendered complex paths.
 *
 * @see PiscesRenderer#render
 */
public final class PiscesCache {

    boolean isValid = false;

    int bboxX0, bboxY0, bboxX1, bboxY1;

    byte[] rowAARLE = null;
    int alphaRLELength = 0;
    int[] rowOffsetsRLE = null;

    int alphaWidth = 0;
    int alphaHeight = 0;

    int[] minTouched = null;
    
    private PiscesCache() {}

    public static PiscesCache createInstance() {
        return new PiscesCache();
    }

    private static final float ROWAA_RLE_FACTOR = 1.2f;
    private static final float TOUCHED_FACTOR = 1.2f;
    private static final int MIN_TOUCHED_LEN = 32;

    private void reallocRowAARLE(int newLength) {
        if (rowAARLE == null) {
            rowAARLE = new byte[newLength];
        } else if (rowAARLE.length < newLength) {
            int len = Math.max(newLength,
                               (int)(rowAARLE.length*ROWAA_RLE_FACTOR));
            byte[] newRowAARLE = new byte[len];
            System.arraycopy(rowAARLE, 0, newRowAARLE, 0, rowAARLE.length);
            rowAARLE = newRowAARLE;
        }
    }

    private void reallocRowInfo(int newHeight) {
        if (minTouched == null) {
            int len = Math.max(newHeight, MIN_TOUCHED_LEN);
            minTouched = new int[len];
            rowOffsetsRLE = new int[len];
        } else if (minTouched.length < newHeight) {
            int len = Math.max(newHeight,
                               (int)(minTouched.length*TOUCHED_FACTOR));
            int[] newMinTouched = new int[len];
            int[] newRowOffsetsRLE = new int[len];
            System.arraycopy(minTouched, 0, newMinTouched, 0,
                             minTouched.length);
            System.arraycopy(rowOffsetsRLE, 0, newRowOffsetsRLE, 0,
                             rowOffsetsRLE.length);
            minTouched = newMinTouched;
            rowOffsetsRLE = newRowOffsetsRLE;
        }
    }

    void addRLERun(byte val, int runLen) {
        reallocRowAARLE(alphaRLELength + 2);
        rowAARLE[alphaRLELength++] = val;
        rowAARLE[alphaRLELength++] = (byte)runLen;
    }

    void addRow(int minX, int offset) {
        reallocRowInfo(alphaHeight + 1);
        minTouched[alphaHeight] = minX;
        rowOffsetsRLE[alphaHeight] = offset;
        ++alphaHeight;
    }

    public synchronized boolean isValid() {
        return isValid;
    }

    public synchronized void dispose() {
        alphaWidth = alphaHeight = 0;

        rowAARLE = null;
        alphaRLELength = 0;

        minTouched = null;
        rowOffsetsRLE = null;

        isValid = false;
    }
}
