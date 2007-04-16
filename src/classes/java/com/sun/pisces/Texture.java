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

public class Texture extends Paint {

    int imageType;
    int[] intData;
    int imageWidth;
    int imageHeight;
    int stride;

    boolean repeat;

    long m00, m01, m02;
    long m10, m11, m12;

    int wmask, hmask;

    boolean interpolate = true;

    private void computeTransform() {
        this.m00 = (long)inverse.m00;
        this.m01 = (long)inverse.m01;
        this.m10 = (long)inverse.m10;
        this.m11 = (long)inverse.m11;
        this.m02 = (long)inverse.m02 + (m00 >> 1) + (m01 >> 1);
        this.m12 = (long)inverse.m12 + (m10 >> 1) + (m11 >> 1);

        if (interpolate) {
            this.m02 -= 32768;
            this.m12 -= 32768;
        }
    }

    public Texture(int imageType,
                   Object data,
                   int width, int height,
                   int offset, int stride,
                   Transform6 textureTransform,
                   boolean repeat) {
        super(textureTransform);

        this.imageType = imageType;

        int[] srcData = (int[])data;
        this.intData = new int[(width + 2)*(height + 2)];

        // prepare additional pixels for interpolation
        int copyToFirstCol;
        int copyToLastCol;
        int copyToFirstRow;
        int copyToLastRow;
        
        if (repeat) {
            copyToFirstCol = width - 1;
            copyToLastCol = 0;
            copyToFirstRow = height - 1;
            copyToLastRow = 0;            
        } else {
            copyToFirstCol = 0;
            copyToLastCol = width - 1;
            copyToFirstRow = 0;
            copyToLastRow = height - 1;
        }
        
        int sidx = offset;
        int didx = width + 2;
        for (int y = 0; y < height; y++) {
            System.arraycopy(srcData, sidx, intData, didx + 1, width);
            intData[didx] = intData[didx + copyToFirstCol + 1];
            intData[didx + width + 1] = intData[didx + copyToLastCol + 1];
            sidx += stride;
            didx += width + 2;
        }
        
        System.arraycopy(intData, (copyToFirstRow + 1) * (width + 2), 
                intData, 0, width + 2);
        System.arraycopy(intData, (copyToLastRow + 1) * (width + 2), 
                intData, (height + 1) * (width + 2), width + 2);
        
        this.imageWidth = width;
        this.imageHeight = height;
        this.stride = width + 2;
        this.repeat = repeat;
        computeTransform();
        
        int iw = imageWidth;
        int wshift = 0;
        while (iw > 0) {
            iw >>= 1;
            ++wshift;
        }
        this.wmask = 0xffffffff << (wshift - 1);

        int ih = imageHeight;
        int hshift = 0;
        while (ih > 0) {
            ih >>= 1;
            ++hshift;
        }
        this.hmask = 0xffffffff << (hshift - 1);
    }

    public void setTransform(Transform6 transform) {
        super.setTransform(transform);
        computeTransform();
    }

    public void setQuality(int quality) {
        // Interpolate if quality >= 1/2
        this.interpolate = (quality > 65536/2);        
        computeTransform();
    }

    private int mod(int x, int y) {
        x = x % y;
        if (x < 0) {
            x += y;
        }
        return x;
    }

    private long mod(long x, long y) {
        x = x % y;
        if (x < 0) {
            x += y;
        }
        return x;
    }

    private int interp(int x0, int x1, int frac) {
        return ((x0 << 16) + (x1 - x0)*frac + 0x8000) >> 16;
    }

    public void paint(int x, int y, int width, int height,
                      int[] minTouched, int[] maxTouched,
                      int[] dst,
                      int dstOffset, int dstScanlineStride) {
        int sx = x;

        for (int j = 0; j < height; j++, y++) {
            int minX = minTouched[j];
            int maxX = maxTouched[j];
            int w = (maxX >= minX) ? (maxX - minX + 1) : 0;
            if (w + minX > width) {
                w = width - minX;
            }
            
            int didx = dstOffset + minX;
            x = sx + minX;
        
            long ltx = x*m00 + y*m01 + m02;
            long lty = x*m10 + y*m11 + m12;

            for (int i = 0; i < w; i++, didx++) {
                int tx = (int)(ltx >> 16);
                int ty = (int)(lty >> 16);

                int hfrac = (int)(ltx & 0xffff);
                int vfrac = (int)(lty & 0xffff);

                // It appears to be cheaper to perform a bounds check
                // for every pixel and only perform 'mod' when needed
                // that to 'mod' every pixel

                // If tx is in bounds, tx & wmask must be 0. The
                // converse is not necessarily true; i.e., the test is
                // conservative

		boolean inBounds = true;

                if ((tx & wmask) != 0) {
                    if (tx < -1 || tx >= imageWidth) {
                        if (repeat) {
                            ltx = mod(ltx, imageWidth << 16);
                            tx = (int)(ltx >> 16);
                        } else {
			    inBounds = false;
                        }
                    }
                }
                if ((ty & hmask) != 0) {
                    if (ty < -1 || ty >= imageHeight) {
                        if (repeat) {
                            lty = mod(lty, imageHeight << 16);
                            ty = (int)(lty >> 16);
                        } else {
			    inBounds = false;
                        }
                    }
                }

		if (inBounds) {
		    int sidx = (ty + 1)*stride + tx + 1;
		    int p00 = intData[sidx];
		    if (interpolate) {
			int p01 = intData[sidx + 1];
			sidx += stride;
			int p10 = intData[sidx];
			int p11 = intData[sidx + 1];
			
			int a00 = (p00 >> 24) & 0xff;
			int r00 = (p00 >> 16) & 0xff;
			int g00 = (p00 >> 8)  & 0xff;
			int b00 =  p00        & 0xff;
			
			int a01 = (p01 >> 24) & 0xff;
			int r01 = (p01 >> 16) & 0xff;
			int g01 = (p01 >> 8)  & 0xff;
			int b01 =  p01        & 0xff;
			
			int a0 = interp(a00, a01, hfrac);
			int r0 = interp(r00, r01, hfrac);
			int g0 = interp(g00, g01, hfrac);
			int b0 = interp(b00, b01, hfrac);
			
			int a10 = (p10 >> 24) & 0xff;
			int r10 = (p10 >> 16) & 0xff;
			int g10 = (p10 >> 8)  & 0xff;
			int b10 =  p10        & 0xff;

			int a11 = (p11 >> 24) & 0xff;
			int r11 = (p11 >> 16) & 0xff;
			int g11 = (p11 >> 8)  & 0xff;
			int b11 =  p11        & 0xff;
			
			int a1 = interp(a10, a11, hfrac);
			int r1 = interp(r10, r11, hfrac);
			int g1 = interp(g10, g11, hfrac);
			int b1 = interp(b10, b11, hfrac);
			
			int a = interp(a0, a1, vfrac);
			int r = interp(r0, r1, vfrac);
			int g = interp(g0, g1, vfrac);
			int b = interp(b0, b1, vfrac);
                        
			dst[didx] = (a << 24) | (r << 16) | (g << 8) | b;
		    } else {
			dst[didx] = p00;
		    }
		} else {
                    dst[didx] = 0;
                }
		
                ltx += m00;
                lty += m10;
	    }
            
            dstOffset += dstScanlineStride;
        }
    }
}
