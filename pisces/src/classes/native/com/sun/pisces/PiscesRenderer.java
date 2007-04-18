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

public final class PiscesRenderer extends PathSink 
        implements NativeFinalization {
    static {
        PiscesLibrary.load();
    }

    private static boolean messageShown = false;

    public static final int ARC_OPEN = 0;
    public static final int ARC_CHORD = 1;
    public static final int ARC_PIE = 2;
    
    long nativePtr = 0L;
    final AbstractSurface surface;

    private final NativeFinalizer finalizer;
        
    static {
        String strValue;
        int strokeXBias = 0; // default x bias
        int strokeYBias = 0; // default y bias

        strValue = Configuration.getProperty("pisces.stroke.xbias");
        if (strValue != null) {
            try {
                strokeXBias = Integer.parseInt(strValue);
            } catch (NumberFormatException e) {
            }
        }
        
        strValue = Configuration.getProperty("pisces.stroke.ybias");
        if (strValue != null) {
            try {
                strokeYBias = Integer.parseInt(strValue);
            } catch (NumberFormatException e) {
            }
        }

        staticInitialize(strokeXBias, strokeYBias);
    }
    
    private void notImplemented() {
        new RuntimeException().printStackTrace();
        System.out.println("not implemented");

//         throw new RuntimeException("PiscesRendererNativeImpl: " +
//                 "Not implemented!");
    }
    
    /**
     * Creates a renderer that will write into a given pixel array.
     *
     * @param data the destination surface
     * where pixel data should be written.
     * @param width the width of the pixel array.
     * @param height the height of the pixel array.
     * @param offset the starting offset of the pixel array.
     * @param scanlineStride the scanline stride of the pixel array, in array
     * entries.
     * @param pixelStride the pixel stride of the pixel array, in array
     * entries.
     * @param type the pixel format, one of the
     * <code>RendererBase.TYPE_*</code> constants.
     */
    public PiscesRenderer(Object data, int width, int height,
                          int offset, int scanlineStride, int pixelStride,
                          int type) {
        this((AbstractSurface)data);
    }

    public PiscesRenderer(AbstractSurface surface) {
        if (!messageShown) {
            System.out.println("Using Pisces Renderer (native version)");
        }

        this.finalizer = NativeFinalizer.createInstance(this);
        this.surface = surface;
        initialize();
        messageShown = true;
    }

    private static native void staticInitialize(int strokeXBias, 
            int strokeYBias);

    private native void initialize();
    
    public native void setAntialiasing(boolean antialiasingOn);

    public native boolean getAntialiasing();

    /**
     * Sets the current paint color.
     *
     * @param red a value between 0 and 255.
     * @param green a value between 0 and 255.
     * @param blue a value between 0 and 255.
     * @param alpha a value between 0 and 255.
     */
    public native void setColor(int red, int green, int blue, int alpha);

    /**
     * Sets the current paint color.  An alpha value of 255 is used.
     *
     * @param red a value between 0 and 255.
     * @param green a value between 0 and 255.
     * @param blue a value between 0 and 255.
     */
    public void setColor(int red, int green, int blue) {
        setColor(red, green, blue, 255);
    }
    
    public native void setCompositeRule(int compositeRule);

    public native void setComposite(int compositeRule, float alpha);

    int[] gcm_fractions = null;
    int[] gcm_rgba = null;
    int gcm_cycleMethod = -1;
    GradientColorMap gradientColorMap = null;
    
    private boolean arraysDiffer(int[] a, int[] b) {
        if (a == null) {
            return true;
        }
        int len = b.length;
        if (a.length != len) {
            return true;
        }
        for (int i = 0; i < len; i++) {
            if (a[i] != b[i]) {
                return true;
            }
        }
        
        return false;
    }
    
    private int[] cloneArray(int[] src) {
        int len = src.length;
        int[] dst = new int[len];
        System.arraycopy(src, 0, dst, 0, len);
        return dst;
    }
    
    private void setGradientColorMap(int[] fractions, int[] rgba,
                                     int cycleMethod) {
        if (fractions.length != rgba.length) {
            throw new IllegalArgumentException("fractions.length != rgba.length!");
        }
        
        if (gradientColorMap == null ||
            gcm_cycleMethod != cycleMethod ||
            arraysDiffer(gcm_fractions, fractions) ||
            arraysDiffer(gcm_rgba, rgba)) {
            this.gradientColorMap =
                new GradientColorMap(fractions, rgba, cycleMethod);
            this.gcm_cycleMethod = cycleMethod;
            this.gcm_fractions = cloneArray(fractions);
            this.gcm_rgba = cloneArray(rgba);
        }
    }
    
    private native void setLinearGradientImpl(int x0, int y0, int x1, int y1,
                                              int[] colors,
                                              int cycleMethod,
                                              Transform6 gradientTransform);

    public void setLinearGradient(int x0, int y0, int x1, int y1,
                                  int[] fractions, int[] rgba,
                                  int cycleMethod,
                                  Transform6 gradientTransform) {
        setGradientColorMap(fractions, rgba, cycleMethod);
        setLinearGradientImpl(x0, y0, x1, y1,
                              gradientColorMap.colors, cycleMethod,
                              gradientTransform);
    }

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
                                  int cycleMethod) {
      int[] fractions = {0x0000, 0x10000};
      int[] rgba = {color0, color1};
      Transform6 ident = new Transform6(1 << 16, 0, 0, 1 << 16, 0, 0);
      setLinearGradient(x0, y0, x1, y1, fractions, rgba, cycleMethod, ident);
    }

    private native void setRadialGradientImpl(int cx, int cy, int fx, int fy,
                                              int radius,
                                              int[] colors,
                                              int cycleMethod,
                                              Transform6 gradientTransform);

    public void setRadialGradient(int cx, int cy, int fx, int fy,
                                  int radius,
                                  int[] fractions, int[] rgba,
                                  int cycleMethod,
                                  Transform6 gradientTransform) {
        setGradientColorMap(fractions, rgba, cycleMethod);
        setRadialGradientImpl(cx, cy, fx, fy, radius,
                              gradientColorMap.colors, cycleMethod,
                              gradientTransform);
    }

    public void setTextureOpacity(float opacity) {
	notImplemented();
    }

    public void setTexture(int imageType,
                           Object imageData, 
                           int width, int height,
                           int offset, int stride,
                           Transform6 textureTransform,
                           boolean repeat) {
        if (imageData instanceof int[]) {
            setTextureImpl(imageType, (int[])imageData, width, height, offset, 
                    stride, textureTransform, repeat);
        }
    }

     private native void setTextureImpl(int imageType, int[] imageData,
            int width, int height, int offset, int stride,
            Transform6 textureTransform, boolean repeat);
    
    public PathSink getStroker() {
        notImplemented();
        return null;
    }

    public PathSink getFiller() {
        notImplemented();
        return null;
    }

    public PathSink getTextFiller() {
        notImplemented();
        return null;
    }

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
    public native void setStroke(int lineWidth, int capStyle, int joinStyle,
            int miterLimit, int[] dashArray, int dashPhase);

    /**
     * Sets the current transform from user to window coordinates.
     *
     * @param transform an <code>Transform6</code> object.
     */
    public native void setTransform(Transform6 transform);
    
    public Transform6 getTransform() {
        Transform6 transform = new Transform6();
        getTransformImpl(transform);
        return transform;
    }
    
    private native void getTransformImpl(Transform6 transform);

    /**
     * Sets a clip rectangle for all primitives.  Each primitive will be
     * clipped to the intersection of this rectangle and the destination
     * image bounds.
     */
    public native void setClip(int minX, int minY, int width, int height);

    /**
     * Resets the clip rectangle.  Each primitive will be clipped only
     * to the destination image bounds.
     */
    public native void resetClip();

    public native void beginRendering(int windingRule);

    /**
     * Begins the rendering of path data.  The supplied clipping
     * bounds are intersected against the current clip rectangle and
     * the destination image bounds; only pixels within the resulting
     * rectangle may be written to.
     */
    public native void beginRendering(int minX, int minY, 
            int width, int height, int windingRule);

    /**
     * Completes the rendering of path data.  Destination pixels will
     * be written at this time.
     */
    public native void endRendering();

    /**
     * Returns a bounding box containing all pixels drawn during the
     * rendering of the most recent primitive
     * (beginRendering/endRendering pair).  The bounding box is
     * returned in the form (x, y, width, height).
     */
    public native void getBoundingBox(int[] bbox);
    
    public native void setStroke();
    
    public native void setFill();

    public void setTextFill() {
        notImplemented();
    }

    public native void moveTo(int x0, int y0);

    public native void lineTo(int x1, int y1);

    public native void lineJoin();

    public native void quadTo(int x1, int y1, int x2, int y2);

    public native void cubicTo(int x1, int y1, int x2, int y2, int x3, int y3);

    public native void close();

    public native void end();

    public native void drawLine(int x0, int y0, int x1, int y1);

    /**
     * 
     * @param x the X coordinate in S15.16 format.
     * @param y the Y coordinate in S15.16 format.
     * @param w the width in S15.16 format.
     * @param h the height in S15.16 format.
     */
    public native void fillRect(int x, int y, int w, int h);

    public native void drawRect(int x, int y, int w, int h);

    public native void drawOval(int x, int y, int w, int h);

    public native void fillOval(int x, int y, int w, int h);

    public native void fillRoundRect(int x, int y, int w, int h, 
            int aw, int ah);

    public native void drawRoundRect(int x, int y, int w, int h, 
            int aw, int ah);

    public native void drawArc(int x, int y, int width, int height,
            int startAngle, int arcAngle, int arcType);

    public native void fillArc(int x, int y, int width, int height,
            int startAngle, int arcAngle, int arcType);

    public void getImageData() {
        notImplemented();
    }
    
    public native void clearRect(int x, int y, int w, int h);

    public native void setPathData(float[] data, byte[] commands, 
            int nCommands);

    public native void nativeFinalize();
}
