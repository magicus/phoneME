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

public abstract class AbstractSurface implements Surface, NativeFinalization {
    static {
        PiscesLibrary.load();
    }
    
    private final class AbstractSurfaceDestination 
            implements SurfaceDestination {
        public void drawSurface(Surface ps, int srcX, int srcY, 
                int dstX, int dstY, int width, int height, float opacity) {
            if (ps instanceof NativeSurface) {
                drawSurfaceImpl((NativeSurface)ps, srcX, srcY, dstX, dstY, 
                        width, height, opacity);
                return;
            }

            int srcW = ps.getWidth();
            int srcH = ps.getHeight();
            int dstW = getWidth();
            int dstH = getHeight();
            
            if (srcX < 0) {
                dstX -= srcX;
                width += srcX;
                srcX = 0;
            }
            
            if (srcY < 0) {
                dstY -= srcY;
                height += srcY;
                srcY = 0;
            }
            
            if (dstX < 0) {
                srcX -= dstX;
                width += dstX;
                dstX = 0;
            }
            
            if (dstY < 0) {
                srcY -= dstY;
                height += dstY;
                dstY = 0;
            }
            
            if ((srcX + width) > srcW) {
                width = srcW - srcX;
            }
            
            if ((srcY + height) > srcH) {
                height = srcH - srcY;
            }
            
            if ((dstX + width) > dstW) {
                width = dstW - dstX;
            }
            
            if ((dstY + height) > dstH) {
                height = dstH - dstY;
            }
            
            if ((width > 0) && (height > 0) && (opacity > 0)) {
                int size = width * height;
                int[] srcRGB = new int[size];

                ps.getRGB(srcRGB, 0, width, srcX, srcY, width, height);
                drawRGBImpl(srcRGB, 0, width, dstX, dstY, width, height, 
                        opacity);
            }
        }
    
        public void drawRGB(int[] argb, int offset, int scanLength, 
                int x, int y, int width, int height, float opacity) {
            drawRGBImpl(argb, offset, scanLength, x, y, width, height, opacity);
        }
    }
    
    protected long nativePtr = 0L;
    private final NativeFinalizer finalizer;

    protected AbstractSurface() {
        this.finalizer = NativeFinalizer.createInstance(this);
    }
        
    public SurfaceDestination createSurfaceDestination() {
        return new AbstractSurfaceDestination();
    }

    public native int getWidth();
    
    public native int getHeight();
    
    public native void getRGB(int[] argb, int offset, int scanLength, 
            int x, int y, int width, int height);
    
    public native void setRGB(int[] argb, int offset, int scanLength, 
            int x, int y, int width, int height);

    private native void drawSurfaceImpl(NativeSurface ps, int srcX, int srcY, 
            int dstX, int dstY, int width, int height, float opacity);

    private native void drawRGBImpl(int[] argb, int offset, int scanLength, 
            int x, int y, int width, int height, float opacity);

    public native void nativeFinalize();
}
