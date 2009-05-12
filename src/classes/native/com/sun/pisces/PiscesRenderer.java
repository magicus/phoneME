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

public interface PiscesRenderer extends NativeFinalization {

    public static final int ARC_OPEN = 0;
    public static final int ARC_CHORD = 1;
    public static final int ARC_PIE = 2;
    

    public void setAntialiasing(boolean antialiasingOn);

    public boolean getAntialiasing();

    /**
     * Sets the current paint color.
     *
     * @param red a value between 0 and 255.
     * @param green a value between 0 and 255.
     * @param blue a value between 0 and 255.
     * @param alpha a value between 0 and 255.
     */
    public void setColor(int red, int green, int blue, int alpha);

    /**
     * Sets the current paint color.  An alpha value of 255 is used.
     *
     * @param red a value between 0 and 255.
     * @param green a value between 0 and 255.
     * @param blue a value between 0 and 255.
     */
    public void setColor(int red, int green, int blue);
    
    public void setCompositeRule(int compositeRule);

    public void setComposite(int compositeRule, float alpha);


    public void setLinearGradient(int x0, int y0, int x1, int y1,
                                  int[] fractions, int[] rgba,
                                  int cycleMethod,
                                  Transform6 gradientTransform);

    /**
     * Java2D-style linear gradient creation. The color changes proportionally
     * between point P0 (color0) nad P1 (color1). Cycle method constants are
     * defined in GradientColorMap (CYCLE_*).          
     *
     * @param x0 x coordinate of point P0
     * @param y0 y coordinate of point P0     
     * @param color0 color of P0
     * @param x1 x coordinate of point P1
     * @param y1 y coordinate of point P1     
     * @param color1 color of P1
     * @param cycleMethod type of cycling of the gradient (NONE, REFLECT, REPEAT)
     *          
     */
    public void setLinearGradient(int x0, int y0, int color0, 
                                  int x1, int y1, int color1,
                                  int cycleMethod);

    public void setRadialGradient(int cx, int cy, int fx, int fy,
                                  int radius,
                                  int[] fractions, int[] rgba,
                                  int cycleMethod,
                                  Transform6 gradientTransform);

    public void setTextureOpacity(float opacity);

    public void setTexture(int imageType,
                           Object imageData, 
                           int width, int height,
                           int offset, int stride,
                           Transform6 textureTransform,
                           boolean repeat);

    
    public PathSink getStroker();

    public PathSink getFiller();

    public PathSink getTextFiller();

    /**
     * Sets the current stroke parameters.
     *
     * @param lineWidth the sroke width, in S15.16 format.
     * @param capStyle the line cap style, one of
     * <code>Stroker.CAP_*</code>.
     * @param joinStyle the line cap style, one of
     * <code>Stroker.JOIN_*</code>.
     * @param miterLimit the stroke miter limit, in S15.16 format.
     * @param dashArray an <code>int</code> array containing the dash
     * segment lengths in S15.16 format, or <code>null</code>.
     * @param dashPhase the starting dash offset, in S15.16 format.
     */
    public void setStroke(int lineWidth, int capStyle, int joinStyle,
            int miterLimit, int[] dashArray, int dashPhase);
    
    /**
     * Sets the current transform from user to window coordinates.
     *
     * @param transform an <code>Transform6</code> object.
     */
    public void setTransform(Transform6 transform);
    
    public Transform6 getTransform();
    

    /**
     * Sets a clip rectangle for all primitives.  Each primitive will be
     * clipped to the intersection of this rectangle and the destination
     * image bounds.
     */
    public void setClip(int minX, int minY, int width, int height);

    /**
     * Resets the clip rectangle.  Each primitive will be clipped only
     * to the destination image bounds.
     */
    public void resetClip();

    public void beginRendering(int windingRule);
    

    /**
     * Begins the rendering of path data.  The supplied clipping
     * bounds are intersected against the current clip rectangle and
     * the destination image bounds; only pixels within the resulting
     * rectangle may be written to.
     */
    public void beginRendering(int minX, int minY, 
            int width, int height, int windingRule);
    

    /**
     * Completes the rendering of path data.  Destination pixels will
     * be written at this time.
     */
    public void endRendering();

    /**
     * Returns a bounding box containing all pixels drawn during the
     * rendering of the most recent primitive
     * (beginRendering/endRendering pair).  The bounding box is
     * returned in the form (x, y, width, height).
     */
    public void getBoundingBox(int[] bbox);
    
    public void setStroke();
    
    public void setFill();

    public void setTextFill();

    public void moveTo(int x0, int y0);

    public void lineTo(int x1, int y1);

    public void lineJoin();

    public void quadTo(int x1, int y1, int x2, int y2);

    public void cubicTo(int x1, int y1, int x2, int y2, int x3, int y3);

    public void close();

    public void end();

    public void drawLine(int x0, int y0, int x1, int y1);

    /**
     * 
     * @param x the X coordinate in S15.16 format.
     * @param y the Y coordinate in S15.16 format.
     * @param w the width in S15.16 format.
     * @param h the height in S15.16 format.
     */
    public void fillRect(int x, int y, int w, int h);

    public void drawRect(int x, int y, int w, int h);

    public void drawOval(int x, int y, int w, int h);

    public void fillOval(int x, int y, int w, int h);

    public void fillRoundRect(int x, int y, int w, int h, 
            int aw, int ah);

    public void drawRoundRect(int x, int y, int w, int h, 
            int aw, int ah);

    public void drawArc(int x, int y, int width, int height,
            int startAngle, int arcAngle, int arcType);

    public void fillArc(int x, int y, int width, int height,
            int startAngle, int arcAngle, int arcType);

    public void getImageData();
    
    public void clearRect(int x, int y, int w, int h);

    public void setPathData(float[] data, byte[] commands, 
            int nCommands);

    public void nativeFinalize();
}
