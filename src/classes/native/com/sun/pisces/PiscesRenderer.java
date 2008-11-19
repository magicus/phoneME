/*
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved. 
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
    protected AbstractSurface surface;

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
//        System.out.println("not implemented");
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
    
    /**
     * Creates a renderer that will write into a given surface.
     *
     * @param surface destination surface
     */
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
    
    /**
     * Turns antialiasing on/off. 
     * @param antialiasingOn <code>true</code> switches antialiasing ON. <code>false</code> OFF
     */
    public native void setAntialiasing(boolean antialiasingOn);

    /**
     * Returns <code>true</true> if antialiasing support is currently switched on. <code>false</code> otherwise.
     * @return state of antialising support
     */
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
     * Sets the current paint color.  An alpha value of 255 is used. Calling <code>setColor</code> also switches 
     * painting mode - i.e. if we have specified gradient, or texture previously, this will be overcome with <code>setColor</code>
     * call. Also note, that 3-param <code>setColor</code> sets fully opaque RGB color. To draw with semi-transparent color
     * use 4-param convenience method. 
     *
     * @param red a value between 0 and 255.
     * @param green a value between 0 and 255.
     * @param blue a value between 0 and 255.
     */
    public void setColor(int red, int green, int blue) {
        setColor(red, green, blue, 255);
    }
    
    /**
     * Sets current Compositing Rule (Porter-Duff) to be used in following rendering operation. Note that <code>compositeAlpha</code>
     * is not changed. 
     * @param compositeRule one of <code>RendererBase.COMPOSITE_*</code> constants.
     */
    public native void setCompositeRule(int compositeRule);

    /**
     * Function sets Renderer's compositing rule to compositeRule and composite alpha 
     * to alpha. Note, that setting alpha has effect for all compositing rules. This means that destination pixel is first calculated
     * due to current compositeRule and result is yet multiplied by composite alpha. This way we can add semitransparency to textures and so on.  
     * @param compositeRule compositing rule
     * @param alpha composite alpha. Value must be from 0.0 to 1.0.  
     * @see For supported values of compositeRule see CompositingRules, setComposite(int, float) 
     */
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

    /**
     * This method sets linear color-gradient data to be used as paint data in following rendering operation.
     * Imagine, we want to draw simple gradient from blue to red color. Each pixel on line perpendicular to line L = [[x0,y0], [x1, y1]] will have same constant color.
     * Pixels on perpendicular-line which passes [x0, y0] will be blue. Those on line passing [x1, y1] will be red. Colors on lines in between will be interpolated by <code>fractions</code>.
     * @param x0 x-coordinate of the starting point of the linear gradient
     * @param y0 y-coordinate of the starting point of the linear gradient
     * @param x1 x-coordinate of the end point of the linear gradient
     * @param y0 y-coordinate of the end point of the linear gradient
     * @param fractions this array defines normalized distances in which color (rgba[i]) starts to fade into next color (rgba[i+1]). This distance from the point [x0,y0] is given as fraction[i]*l, where l is length of line [[x0,y0], [x1,y1]]. fraction[i+1] says, in what distance fraction[i+1]*l from [x0,y0] should color already have firm value of rgba[i+1]. Values passed in fractions should be from interval <0.0, 1.0>, in 15.16 format.  
     * @param rgba colors which the linear gradient passes through. Generally should be fulfilled this formula <code>rgba.length == fractions.length</code>
     * @param cycleMethod some value from <code>GradientColorMap.CYCLE_*</code>. @see GradienColorMap
     * @param gradientTransform transformation applied to gradient paint data. This way we can either transform gradient fill together with filled object or leave it as if transformed gradient-filled object was a window through which we observe gradient area.
     * @see GradienColorMap
     */
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
     * defined in GradientColorMap (CYCLE_*). This is convenience method only. Same as if setLinearGradient method with 8 parameters was called with
     * fractions = {0x0000, 0x10000}, rgba = {color0, color1} and identity transformation matrix.           
     *
     * @param x0 x coordinate of point P0
     * @param y0 y coordinate of point P0     
     * @param color0 color of P0
     * @param x1 x coordinate of point P1
     * @param y1 y coordinate of point P1     
     * @param color1 color of P1
     * @param cycleMethod type of cycling of the gradient (NONE, REFLECT, REPEAT)
     * 
     * As Pisces Gradient support was added to support features introduced in SVG, see e.g. http://www.w3.org/TR/SVG11/pservers.html for more information and examples.         
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

    /**
     * This method sets radial gradient paint data to be used in subsequent rendering. Radial gradient data generated will be used to fill the touched pixels of the path we draw.
     * 
     * @param cx cx, cy and radius triplet defines the largest circle for the gradient. 100% gradient stop is mapped to perimeter of this circle. 
     * @param cy 
     * @param fx fx,fy defines focal point of the gradient. ie. 0% gradient stop is mapped to fx,fy point. If cx == fx and cy == fy, then gradient consists of homocentric circles. If these relations are not met, gradient field is deformed and eccentric ovals can be observed. 
     * @param fy
     * @param radius @see cx
     * @param fractions @see setLinearGradient
     * @param rgba @see setLinearGradient
     * @param cycleMethod @see setLinearGradient
     * @param gradientTransform @see setLinearGradient
     * 
     * As Pisces Gradient support was added to support features introduced in SVG, see e.g. http://www.w3.org/TR/SVG11/pservers.html for more information and examples. 
     */
    
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

    /**
     * Sets opacity (ie. 1.0f - alpha) factor to be used while painting texture. NOT IMPLEMENTED.  
     * @param opacity
     */
    public void setTextureOpacity(float opacity) {
    	notImplemented();
    }

    /**
     * Sets texture to be used in following render operations. For example to draw image as it is
     * we set identity transformation matrix on the PiscesRenderer, call setTexture with imageData 
     * @param imageType tells PiscesRenderer what is the pixel format of passed @param imageData. It can be one of <code>RendererBase.TYPE_*</code> values.
     * @param imageData image data. either int[] or byte[]. Real type recognition is based on imageType.
     * @param width texture image width
     * @param height texture image height
     * @param offset offset in imageData array, ie. position from which the data are interesting 
     * @param stride scanline length. Add stride to pixel index in order to get to pixel with the same x in the next row  
     * @param textureTransform transformation matrix 
     * @param repeat texture repetition  
     */
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
    
    
    /**
     * Flavour of setTexture method which works directly with ImageData (java.lcdui.Image), rather then with ARGB[] got as image.getRGB(). 
     * This method reduces number of buffers used while working with textures. Use thi method only when you are sure what you are doing.
     * @param image 
     * @param width
     * @param height
     * @param offset
     * @param stride
     * @param textureTransform
     */
    public void setTexture(Object image, int width, int height, int offset, int stride, Transform6 textureTransform) {
        if (image != null) {
            setTextureFromImageImpl(image, width, height, offset, stride, textureTransform);
        }
    }
    
    private native void setTextureFromImageImpl(Object image, int width, int height, int offset, int stride, Transform6 textureTransform);

    /**
     * NOT IMPLEMENTED
     * @return null
     */
    public PathSink getStroker() {
        notImplemented();
        return null;
    }

    /**
     * NOT IMPLEMENTED
     * @return nulll
     */
    public PathSink getFiller() {
        notImplemented();
        return null;
    }

    /**
     * NOT IMPLEMENTED
     * @return null
     */
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
    public void setStroke(int lineWidth, int capStyle, int joinStyle,
            int miterLimit, int[] dashArray, int dashPhase) {
        setStrokeImpl(lineWidth, capStyle, joinStyle, miterLimit, dashArray, dashPhase);
    }
    
    private native void setStrokeImpl(int lineWidth, int capStyle, int joinStyle,
            int miterLimit, int[] dashArray, int dashPhase);

    /**
     * Sets the current transform from user to window coordinates.
     *
     * @param transform an <code>Transform6</code> object.
     */
    public native void setTransform(Transform6 transform);
    
    /**
     * Returns current transformation in user cooordinates.
     * @return <code>Transform6</code>
     */
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

    /**
     * @defgroup WindingRules Winding rules - shape interior  
     * Winding rule determines what part of shape is determined as interior. This is 
     * important to determine what part of shape to fill. 
     * @def WIND_EVEN_ODD
     * @ingroup WindingRules
     * This define represents the even-odd winding rule. To see how this winding 
     * rule works, draw any closed shape and draw a line through the entire shape. 
     * Each time the line crosses the shape's border, increment a counter. When the 
     * counter is even, the line is outside the shape. When the counter is odd,
     * the line is in the interior of the shape.
     * @def WIND_NON_ZERO
     * @ingroup WindingRules
     * This define represents the non-zero winding rule. Similar to even-odd. 
     * We draw line through the entire shape. If intersecting edge is drawn from
     * left-to-right, we add 1 to counter. If it goes from right to left we add -1.
     * Everytime the counter is not zero, we assume it's interior part of shape.      
     */
    
    /**
     * Starts rendering session using windingRule when filling.
     * @param windingRule
     * @see WindingRules
     */
    public void beginRendering(int windingRule) {
        beginRenderingI(windingRule);
    }
    
    private native void beginRenderingI(int windingRule);

    /**
     * Begins the rendering of path data.  The supplied clipping
     * bounds are intersected against the current clip rectangle and
     * the destination image bounds; only pixels within the resulting
     * rectangle may be written to.
     * @see WindingRules
     */
    public void beginRendering(int minX, int minY, 
            int width, int height, int windingRule) {
        beginRenderingIIIII(minX, minY, width, height, windingRule);
    }
    
    private native void beginRenderingIIIII(int minX, int minY, 
            int width, int height, int windingRule);

    /**
     * Completes the rendering of path data.  Destination pixels will
     * be written at this time. Basicly all renderin between pair on beginRendering and endRendering method calls can be understood as an atomic transaction.
     */
    public native void endRendering();

    /**
     * Returns a bounding box containing all pixels drawn during the
     * rendering of the most recent primitive
     * (beginRendering/endRendering pair).  The bounding box is
     * returned in the form (x, y, width, height).
     */
    public native void getBoundingBox(int[] bbox);
    
    /**
     * Ensures stroker is used. It only sets it active and reuses stroker's previous settings (lineWidth, capStyle, ...).  
     */
    public void setStroke() {
        setStrokeImplNoParam();
    }
    private native void setStrokeImplNoParam();
    
    /**
     * Ensures filler is used. Uses settings applied to filler previously.
     */
    public native void setFill();

    /**
     * NOT IMPLEMENTED
     */
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
     * Draws filled rectangle using current filler settings: compositing rule, gradient, solid or texture fill ...
     * 
     * @param x the X coordinate in S15.16 format.
     * @param y the Y coordinate in S15.16 format.
     * @param w the width in S15.16 format.
     * @param h the height in S15.16 format.
     */
    public native void fillRect(int x, int y, int w, int h);

    /**
     * Draws nonfilled rectangle using current filler/stroker settings: compositing rule, gradient, solid or texture fill ...
     * 
     * @param x the X coordinate in S15.16 format.
     * @param y the Y coordinate in S15.16 format.
     * @param w the width in S15.16 format.
     * @param h the height in S15.16 format.
     */
    public native void drawRect(int x, int y, int w, int h);

    /**
     * Draws oval border.
     * @param x top left corner of bounding rectangle
     * @param y top left corner of bounding rectangle
     * @param w width of bounding rectangle
     * @param h height of bounding rectangle
     */
    public native void drawOval(int x, int y, int w, int h);
    
    /**
     * Draws filled oval.
     * @param x top left corner of bounding rectangle
     * @param y top left corner of bounding rectangle
     * @param w width of bounding rectangle
     * @param h height of bounding rectangle
     */
    public native void fillOval(int x, int y, int w, int h);

    /**
     * Draws filled rectangle using current filler/stroker settings: compositing rule, gradient, solid or texture fill ...
     * Corners are being drawn using ROUND join.
     * @param x the X coordinate in S15.16 format.
     * @param y the Y coordinate in S15.16 format.
     * @param w the width in S15.16 format.
     * @param h the height in S15.16 format.
     * @param aw width of corner arc
     * @param aw height of corner arc
     */
    public native void fillRoundRect(int x, int y, int w, int h, 
            int aw, int ah);

    /**
     * @see fillRoundRect
     * @param x
     * @param y
     * @param w
     * @param h
     * @param aw
     * @param ah
     */
    public native void drawRoundRect(int x, int y, int w, int h, 
            int aw, int ah);

    /**
     * 
     * @param x
     * @param y
     * @param width
     * @param height
     * @param startAngle
     * @param arcAngle
     * @param arcType
     */
    public native void drawArc(int x, int y, int width, int height,
            int startAngle, int arcAngle, int arcType);

    /**
     * 
     * @param x
     * @param y
     * @param width
     * @param height
     * @param startAngle
     * @param arcAngle
     * @param arcType
     */
    public native void fillArc(int x, int y, int width, int height,
            int startAngle, int arcAngle, int arcType);

    public void getImageData() {
        notImplemented();
    }
    
    /**
     * Clears rectangle (x, y, x + w, y + h). Clear sets all pixels to transparent black (0x00000000 ARGB).
     */
    public native void clearRect(int x, int y, int w, int h);

    /**
     * Parses commands and data into sequence of moveTo, LineTo, quadTo and other path calls.
     * @param data
     * @param commands
     * @param nCommands
     */
    public native void setPathData(float[] data, byte[] commands, 
            int nCommands);

    /**
     * Native finalizer. Releases native memory used by PiscesRenderer at lifetime.
     */
    public native void nativeFinalize();
}
