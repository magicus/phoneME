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

public class RadialGradient extends Gradient {

    float cx, cy, rsq;
    float fx, fy;

    public RadialGradient(int cx, int cy, int fx, int fy, int radius,
                          Transform6 gradientTransform,
                          GradientColorMap colorMap) {
        super(gradientTransform, colorMap);

        this.cx = cx/65536.0f;
        this.cy = cy/65536.0f;
        float r = radius/65536.0f;
        this.rsq = r*r;
        this.fx = fx/65536.0f;
        this.fy = fy/65536.0f;

        float fcx = this.fx - this.cx;
        float fcy = this.fy - this.cy;
        float d = (float)Math.sqrt(fcx*fcx + fcy*fcy);
        if (d > r*0.97f) {
            float f = (r*0.97f)/d;
            this.fx = this.cx + f*fcx;
            this.fy = this.cy + f*fcy;
        }
    }

    public void paint(int x, int y, int width, int height,
                      int[] minTouched, int[] maxTouched,
                      int[] dst,
                      int dstOffset, int dstScanlineStride) {
        float a00 = inverse.m00/65536.0f;
        float a01 = inverse.m01/65536.0f;
        float a02 = inverse.m02/65536.0f;
        float a10 = inverse.m10/65536.0f;
        float a11 = inverse.m11/65536.0f;
        float a12 = inverse.m12/65536.0f;

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
            
            double txx = x*a00 + y*a01 + a02;
            double tyy = x*a10 + y*a11 + a12;
            
            double fxx = fx - txx;
            double fyy = fy - tyy;
            double A = fxx*fxx + fyy*fyy;
            double cfx = cx - fx;
            double cfy = cy - fy;
            double B = 2.0f*(cfx*fxx + cfy*fyy);
            double C = cfx*cfx + cfy*cfy - rsq;
            double Csq = C*C;
            double U = -B/(2.0f*C);
            double dU = (a00*cfx + a10*cfy)/C;
            double V = (B*B - 4.0f*A*C)/(4.0f*Csq);
            double dV = (2.0f*a00*a10*cfx*cfy +
                         a00*(a00*(cfx*cfx - C) - B*cfx + 2.0f*C*fxx) +
                         a10*(a10*(cfy*cfy - C) - B*cfy + 2.0f*C*fyy))/Csq;
            double tmp = a10*cfx - a00*cfy;
            double ddV = 2.0f*((a00*a00 + a10*a10)*rsq - tmp*tmp)/Csq;
            
            for (int i = 0; i < w; i++, didx++, x++) {
                double g = U + Math.sqrt(V);
                U += dU;
                V += dV;
                dV += ddV;
                
                dst[didx] = colorMap.getColor((int)(g*65536.0));
            }
            
            dstOffset += dstScanlineStride;
        }
    }
}

