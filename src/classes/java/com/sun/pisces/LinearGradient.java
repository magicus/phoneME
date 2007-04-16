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

public class LinearGradient extends Gradient {

    int x0, y0, x1, y1;
    long mx, my, b;

    // Construct a transform (mx, my, b) such that for each point (x,
    // y) the value mx*x + my*y + b will be the (unpadded) gradient
    // value.  The gradient value is defined by taking the projection
    // of (x, y) transformed by 'originalTransform' onto the vector
    // from (x0, y0) to (x1, y1); the gradient value is the length of
    // the projected vector divided by the length of the vector onto
    // which it is projected.
    private void computeTransform() {
        float fx0 = x0/65536.0f;
        float fx1 = x1/65536.0f;
        float fy0 = y0/65536.0f;
        float fy1 = y1/65536.0f;
        float fdx = fx1 - fx0;
        float fdy = fy1 - fy0;
        float flensq = fdx*fdx + fdy*fdy;

        float a00 = inverse.m00/65536.0f;
        float a01 = inverse.m01/65536.0f;
        float a02 = inverse.m02/65536.0f;
        float a10 = inverse.m10/65536.0f;
        float a11 = inverse.m11/65536.0f;
        float a12 = inverse.m12/65536.0f;

        this.mx = (int)(65536.0f*(a00*fdx + a10*fdy)/flensq);
        this.my = (int)(65536.0f*(a01*fdx + a11*fdy)/flensq);
        float t = fdx*fx0 + fdy*fy0;
        this.b = (int)(65536.0f*(a02*fdx + a12*fdy - t)/flensq);
    }

    public LinearGradient(int x0, int y0, int x1, int y1,
                          Transform6 gradientTransform,
                          GradientColorMap colorMap) {
        super(gradientTransform, colorMap);
        this.x0 = x0;
        this.y0 = y0;
        this.x1 = x1;
        this.y1 = y1;
        computeTransform();
    }

    public void setTransform(Transform6 transform) {
        super.setTransform(transform);
        computeTransform();
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
            
            long frac = x*mx + y*my + b;
            for (int i = 0; i < w; i++, didx++, x++) {
                dst[didx] = colorMap.getColor((int)frac);
                frac += mx;
            }
            
            dstOffset += dstScanlineStride;
        }
    }
}
