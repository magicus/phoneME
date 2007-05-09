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

public abstract class RendererBase extends LineSink {

    // Enumerated constants

    public static final int DEFAULT_SUBPIXEL_LG_POSITIONS_X = 3;
    public static final int DEFAULT_SUBPIXEL_LG_POSITIONS_Y = 3;

    public static final int WIND_EVEN_ODD = 0;
    public static final int WIND_NON_ZERO = 1;
    
    public static final int COMPOSITE_CLEAR    = 0;
    public static final int COMPOSITE_SRC      = 1;
    public static final int COMPOSITE_SRC_OVER = 2;
    
    /**
     * Constant indicating 8/8/8 RGB pixel data stored in an
     * <code>int</code> array.
     */
    public static final int TYPE_INT_RGB = 1;

    /**
     * Constant indicating 8/8/8/8 ARGB pixel data stored in an
     * <code>int</code> array.
     */
    public static final int TYPE_INT_ARGB = 2;

    /**
     * Constant indicating 8/8/8/8 ARGB alpha-premultiplied pixel data stored 
     * in a <code>int</code> array.
     */
    public static final int TYPE_INT_ARGB_PRE = 3;

    /**
     * Constant indicating 5/6/5 RGB pixel data stored in an
     * <code>short</code> array.
     */
    public static final int TYPE_USHORT_565_RGB = 8;

    /**
     * Constant indicating 8 bit grayscale pixel data stored in a
     * <code>byte</code> array.
     */
    public static final int TYPE_BYTE_GRAY = 10;

    protected int imageType;

    public RendererBase(int imageType) {
        this.imageType = imageType;
    }

    public int getImageType() {
        return imageType;
    }

    public abstract void setAntialiasing(int subpixelLgPositionsX,
                                         int subpixelLgPositionsY);

    public abstract int getSubpixelLgPositionsX();

    public abstract int getSubpixelLgPositionsY();

    public abstract void setColor(int red, int green, int blue, int alpha);

    public abstract void setPaint(Paint paint);

    public abstract void beginRendering(int boundsX, int boundsY,
                                        int boundsWidth, int boundsHeight,
                                        int windingRule);

    public abstract void moveTo(int x0, int y0);

    public void lineJoin() {
        // Do nothing
    }

    public abstract void lineTo(int x1, int y1);

    public abstract void close();

    public void end() {
        // Do nothing
    }

    public abstract void endRendering();

    public abstract void getBoundingBox(int[] bbox);

    //    public abstract void setCache(PiscesCache cache);

    //    public abstract void renderFromCache(PiscesCache cache);

//     public abstract void drawImage(int imageType, Object data,
//                                    int width, int height,
//                                    int offset, int stride,
//                                    int x, int y);

    public void getImageData(Object data, int offset, int scanlineStride) {
        // do nothing
    }
    
    public abstract void clearRect(int x, int y, int w, int h);
}
