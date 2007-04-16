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

import com.sun.midp.main.Configuration;

/**
 *
 * @version 
 */
public final class PiscesRenderer extends PathSink {
    
    public static final int ARC_OPEN = 0;
    public static final int ARC_CHORD = 1;
    public static final int ARC_PIE = 2;

    // IMPL NOTE - use Perseus conventions
    public static final int COMMAND_MOVE_TO  = 0;
    public static final int COMMAND_LINE_TO  = 1;
    public static final int COMMAND_QUAD_TO  = 2;
    public static final int COMMAND_CUBIC_TO = 3;
    public static final int COMMAND_CLOSE    = 4;

    private static final boolean enableLogging = false;
    private static java.io.PrintStream logStream = null;

    private static final int STROKE_X_BIAS;
    private static final int STROKE_Y_BIAS;
    
    static {
        if (enableLogging) {
            //String s = System.getProperty("piscesLogFile");
            //if (s != null) {
                try {
                    logStream = System.out;
                    //new java.io.PrintStream(new java.io.FileOutputStream(s));
                } catch (Exception e) {
                    System.err.println("Error using System.out: " + e);
                    //System.err.println("Error opening log file: " + e);
                }
            //}
        }

        String strValue;
        int intValue;
        
        strValue = Configuration.getProperty("pisces.stroke.xbias");
        intValue = 0; // default x bias
        if (strValue != null) {
            try {
                intValue = Integer.parseInt(strValue);
            } catch (NumberFormatException e) {
            }
        }
        STROKE_X_BIAS = intValue;
        
        strValue = Configuration.getProperty("pisces.stroke.ybias");
        intValue = 0; // default y bias
        if (strValue != null) {
            try {
                intValue = Integer.parseInt(strValue);
            } catch (NumberFormatException e) {
            }
        }
        STROKE_Y_BIAS = intValue;
    }

    private static boolean messageShown = false;

    private static int DEFAULT_FILLER_FLATNESS = 1 << 15;

    Object data = null;
    int width, height;
    int offset, scanlineStride, pixelStride;
    int type;

    RendererBase rdr;
    public PathSink fillerP = null;
    public PathSink textFillerP = null;
    public PathSink strokerP = null;

    PathSink externalConsumer;
    boolean inSubpath = false;
    boolean isPathFilled = false;
    
    int lineWidth = 1 << 16;
    int capStyle = 0;
    int joinStyle = 0;
    int miterLimit = 10 << 16;
    int[] dashArray = null;
    int dashPhase = 0;

    Transform6 transform = new Transform6();

    Paint paint;
    Transform6 paintTransform;
    Transform6 paintCompoundTransform;

    int[] gcm_fractions = null;
    int[] gcm_rgba = null;
    int gcm_cycleMethod = -1;
    GradientColorMap gradientColorMap = null;
 
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 255;

    // Current bounding box for all primitives
    int bbMinX = Integer.MIN_VALUE;
    int bbMinY = Integer.MIN_VALUE;
    int bbMaxX = Integer.MAX_VALUE;
    int bbMaxY = Integer.MAX_VALUE;
    
    /**
     * Creates a renderer that will write into a given pixel array.
     *
     * @param data an <code>int</code> or <code>short</code> array
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
        if (!messageShown) {
            System.out.println("Using Pisces Renderer (java version)");
        }

        if (data instanceof NativeSurface) {
            NativeSurface ns = (NativeSurface)data;
            
            this.data = ns.getData();
            this.width = ns.getWidth();
            this.height = ns.getHeight();
            this.offset = 0;
            this.scanlineStride = ns.getWidth();
            this.pixelStride = 1;
        } else {
            this.data = data;
            this.width = width;
            this.height = height;
            this.offset = offset;
            this.scanlineStride = scanlineStride;
            this.pixelStride = pixelStride;
        }
        
        this.type = type;
        this.rdr = new Renderer(this.data, this.width, this.height,
                this.offset, this.scanlineStride, this.pixelStride,
                type);

        invalidate();
        setFill();

        messageShown = true;
    }

    private void invalidate() {
        fillerP = null;
        textFillerP = null;
        strokerP = null;
    }

    private boolean antialiasingOn = true;

    public void setAntialiasing(boolean antialiasingOn) {
        this.antialiasingOn = antialiasingOn;
        int samples = antialiasingOn ? 3 : 0;
        rdr.setAntialiasing(samples, samples);
        invalidate();
    }

    public boolean getAntialiasing() {
        return this.antialiasingOn;
    }

    /**
     * Sets the current paint color.
     *
     * @red a value between 0 and 255.
     * @green a value between 0 and 255.
     * @blue a value between 0 and 255.
     * @alpha a value between 0 and 255.
     */
    public void setColor(int red, int green, int blue, int alpha) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.setColor(" +
                                  red + ", " +
                                  green + ", " +
                                  blue + ", " +
                                  alpha + ");");
            }
        }
        rdr.setColor(red, green, blue, alpha);
        this.red = red;
        this.green = green;
        this.blue = blue;
        this.alpha = alpha;

        this.paint = null;
    }

    /**
     * Sets the current paint color.  An alpha value of 255 is used.
     *
     * @red a value between 0 and 255.
     * @green a value between 0 and 255.
     * @blue a value between 0 and 255.
     */
    public void setColor(int red, int green, int blue) {
        setColor(red, green, blue, 255);
    }

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

    private void setPaintTransform(Transform6 paintTransform) {
        this.paintTransform = new Transform6(paintTransform);
        this.paintCompoundTransform = new Transform6(paintTransform);
    }

    public void setLinearGradient(int x0, int y0, int x1, int y1,
                                  int[] fractions, int[] rgba,
                                  int cycleMethod,
                                  Transform6 gradientTransform) {
        setPaintTransform(gradientTransform);
        setGradientColorMap(fractions, rgba, cycleMethod);
        this.paint = new LinearGradient(x0, y0, x1, y1,
                                        paintCompoundTransform,
                                        gradientColorMap);
        rdr.setPaint(paint);
    }

    public void setRadialGradient(int cx, int cy, int fx, int fy,
                                  int radius,
                                  int[] fractions, int[] rgba,
                                  int cycleMethod,
                                  Transform6 gradientTransform) {
        setPaintTransform(gradientTransform);
        setGradientColorMap(fractions, rgba, cycleMethod);
        this.paint = new RadialGradient(cx, cy, fx, fy, radius,
                                        paintCompoundTransform,
                                        gradientColorMap);
        rdr.setPaint(paint);
    }

    public void setTexture(int imageType,
                           Object imageData, 
                           int width, int height,
                           int offset, int stride,
                           Transform6 textureTransform,
                           boolean repeat) {
        Transform6 textureCompoundTransform = new Transform6(this.transform);
        textureCompoundTransform.postMultiply(textureTransform);
        setPaintTransform(textureCompoundTransform);

        this.paint = new Texture(imageType, imageData,
                                 width, height, offset, stride,
                                 textureCompoundTransform, repeat);
        rdr.setPaint(paint);
    }

    public Flattener fillFlattener = new Flattener();
    Transformer fillTransformer = new Transformer();
    
    public Flattener textFlattener = new Flattener();
    Transformer textTransformer = new Transformer();

    Stroker strokeStroker = new Stroker();
    Dasher strokeDasher = new Dasher();
    public Flattener strokeFlattener = new Flattener();
    Transformer strokeTransformer = new Transformer();

    public PathSink getStroker() {
        if (this.strokerP == null) {
            strokeStroker.setOutput(rdr);
            strokeStroker.setParameters(lineWidth,
                                        capStyle, joinStyle,
                                        miterLimit, transform);
            if (dashArray == null) {
                strokeFlattener.setOutput(strokeStroker);
                strokeFlattener.setFlatness(1 << 16);
            } else {
                strokeDasher.setOutput(strokeStroker);
                strokeDasher.setParameters(dashArray, dashPhase, transform);
                strokeFlattener.setOutput(strokeDasher);
                strokeFlattener.setFlatness(1 << 16);
            }

            Transform6 t = transform;
            t = new Transform6(transform);
            t.m02 += STROKE_X_BIAS;
            t.m12 += STROKE_Y_BIAS;
            strokeTransformer.setTransform(t);
            strokeTransformer.setOutput(strokeFlattener);
            this.strokerP = strokeTransformer;
        }
        return strokerP;
    }

    public PathSink getFiller() {
        if (this.fillerP == null) {
            fillFlattener.setOutput(rdr);
            
            String flatness = System.getProperty("filler.flatness");
            if (flatness != null) {
                fillFlattener.setFlatness(Integer.parseInt(flatness));
            } else {
                fillFlattener.setFlatness(DEFAULT_FILLER_FLATNESS);
            }
            fillTransformer.setOutput(fillFlattener);
            fillTransformer.setTransform(transform);
            this.fillerP = fillTransformer;
        }
        return fillerP;
    }

    public PathSink getTextFiller() {
        if (textFillerP == null) {
            textFlattener.setOutput(rdr);
            textFlattener.setFlatness(1 << (16 -
                             Math.min(rdr.getSubpixelLgPositionsX(),
                                      rdr.getSubpixelLgPositionsY())));
            textTransformer.setOutput(textFlattener);
            textTransformer.setTransform(transform);
            this.textFillerP = textTransformer;
        }
        return textFillerP;
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
        if (enableLogging) {
            if (logStream != null) {
                logStream.print("dashArray = ");
                if (dashArray == null) {
                    logStream.println("null;");
                } else {
                    logStream.print("{");
                    for (int i = 0; i < dashArray.length; i++) {
                        logStream.print(" " + dashArray[i]);
                    }
                    logStream.print(" };");
                }

                logStream.println("pr.setStroke(" +
                                  lineWidth + ", " +
                                  capStyle + ", " +
                                  joinStyle + ", " +
                                  miterLimit + ", " +
                                  "dashArray, " +
                                  dashPhase + ");");
            }
        }
        this.lineWidth = lineWidth;
        this.capStyle = capStyle;
        this.joinStyle = joinStyle;
        this.miterLimit = miterLimit;
        this.dashArray = dashArray;
        this.dashPhase = dashPhase;
        this.strokerP = null;
        setStroke();
    }

    /**
     * Sets the current transform from user to window coordinates.
     *
     * @param transform an <code>Transform6</code> object.
     */
    public void setTransform(Transform6 transform) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("transform = new Transform6(" +
                                  transform.m00 + ", " +
                                  transform.m01 + ", " +
                                  transform.m10 + ", " +
                                  transform.m11 + ", " +
                                  transform.m02 + ", " +
                                  transform.m12 + ");");
                logStream.println("pr.setTransform(transform);");
            }
        }
        this.transform = transform;

        if (paint != null) {
            setPaintTransform(paintTransform);
            paint.setTransform(this.paintCompoundTransform);
            rdr.setPaint(paint);
        }

        invalidate();
    }

    public Transform6 getTransform() {
        return new Transform6(transform);
    }

    /**
     * Sets a clip rectangle for all primitives.  Each primitive will be
     * clipped to the intersection of this rectangle and the destination
     * image bounds.
     */
    public void setClip(int minX, int minY, int width, int height) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.setClip(" +
                                  minX + ", " +
                                  minY + ", " +
                                  width + ", " +
                                  height + ");");
            }
        }

        this.bbMinX = minX;
        this.bbMinY = minY;
        this.bbMaxX = minX + width;
        this.bbMaxY = minY + height;
    }

    /**
     * Resets the clip rectangle.  Each primitive will be clipped only
     * to the destination image bounds.
     */
    public void resetClip() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.resetClip()");
            }
        }
        this.bbMinX = Integer.MIN_VALUE;
        this.bbMinY = Integer.MIN_VALUE;
        this.bbMaxX = Integer.MAX_VALUE;
        this.bbMaxY = Integer.MAX_VALUE;
    }

    public void beginRendering(int windingRule) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.beginRendering(" + windingRule + ")");
            }
        }
        myBeginRendering(windingRule);
    }

    private void myBeginRendering(int windingRule) {
        int minX = Math.max(0, bbMinX);
        int minY = Math.max(0, bbMinY);
        int maxX = Math.min(width, bbMaxX);
        int maxY = Math.min(height, bbMaxY);
        myBeginRendering(minX, minY, maxX - minX, maxY - minY, windingRule);
    }

    /**
     * Begins the rendering of path data.  The supplied clipping
     * bounds are intersected against the current clip rectangle and
     * the destination image bounds; only pixels within the resulting
     * rectangle may be written to.
     */
    public void beginRendering(int minX, int minY, int width, int height,
                               int windingRule) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.beginRendering(" +
                                  minX + ", " +
                                  minY + ", " +
                                  width + ", " +
                                  height + ", " +
                                  windingRule + ");");
            }
        }
        myBeginRendering(minX, minY, width, height, windingRule);
    }

    private void myBeginRendering(int minX, int minY, int width, int height,
                                  int windingRule) {
        inSubpath = false;
        
        int maxX = minX + width;
        int maxY = minY + height;
        
        minX = Math.max(minX, 0);
        minX = Math.max(minX, bbMinX);

        minY = Math.max(minY, 0);
        minY = Math.max(minY, bbMinY);

        maxX = Math.min(maxX, this.width);
        maxX = Math.min(maxX, bbMaxX);

        maxY = Math.min(maxY, this.height);
        maxY = Math.min(maxY, bbMaxY);

        width = maxX - minX;
        height = maxY - minY;
        rdr.beginRendering(minX, minY, width, height, windingRule);
    }

    public void moveTo(int x0, int y0) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.moveTo(" + x0 + ", " + y0 + ");");
            }
        }
        if (inSubpath && isPathFilled) {
            externalConsumer.close();
        }
        inSubpath = false;
        externalConsumer.moveTo(x0, y0);
    }

    public void lineTo(int x1, int y1) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.lineTo(" + x1 + ", " + y1 + ");");
            }
        }
        inSubpath = true;
        externalConsumer.lineTo(x1, y1);
    }

    public void lineJoin() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.lineJoin();");
            }
        }
        externalConsumer.lineJoin();
    }

    public void quadTo(int x1, int y1, int x2, int y2) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.quadTo(" +
                                  x1 + ", " + y1 + ", " +
                                  x2 + ", " + y2 + ");");
            }
        }
        inSubpath = true;
        externalConsumer.quadTo(x1, y1, x2, y2);
    }

    public void cubicTo(int x1, int y1, int x2, int y2, int x3, int y3) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.cubicTo(" +
                                  x1 + ", " + y1 + ", " +
                                  x2 + ", " + y2 + ", " +
                                  x3 + ", " + y3 + ");");
            }
        }
        inSubpath = true;
        externalConsumer.cubicTo(x1, y1, x2, y2, x3, y3);
    }

    public void close() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.close();");
            }
        }
        inSubpath = false;
        externalConsumer.close();
    }

    public void end() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.end();");
            }
        }
        if (inSubpath && isPathFilled) {
            close();
        }
        inSubpath = false;
        externalConsumer.end();
    }

    /**
     * Completes the rendering of path data.  Destination pixels will
     * be written at this time.
     */
    public void endRendering() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.endRendering();");
            }
        }
        end();
        myEndRendering();
    }

    private void myEndRendering() {
        rdr.endRendering();
    }

    private void renderPath(int numCommands,
                            byte[] commands,
                            float[] coordsXY,
                            int windingRule) {
        beginRendering(windingRule);
        int clen = coordsXY.length;
	int offset = 0;
        for (int i = 0; i < numCommands; i++) {
            int command = commands[i] & 0xff;
            switch (command) {
            case COMMAND_MOVE_TO:
                if (offset >= 0 & offset < clen - 1) {
                    int x0 = (int)(coordsXY[offset++]*65536.0f);
                    int y0 = (int)(coordsXY[offset++]*65536.0f);
                    moveTo(x0, y0);
                }
                break;

            case COMMAND_LINE_TO:
                if (offset >= 0 & offset < clen - 1) {
                    int x1 = (int)(coordsXY[offset++]*65536.0f);
                    int y1 = (int)(coordsXY[offset++]*65536.0f);
                    lineTo(x1, y1);
                }
                break;

            case COMMAND_QUAD_TO:
                if (offset >= 0 & offset < clen - 3) {
                    int x1 = (int)(coordsXY[offset++]*65536.0f);
                    int y1 = (int)(coordsXY[offset++]*65536.0f);
                    int x2 = (int)(coordsXY[offset++]*65536.0f);
                    int y2 = (int)(coordsXY[offset++]*65536.0f);
                    quadTo(x1, y1, x2, y2);
                }
                break;

            case COMMAND_CUBIC_TO:
                if (offset >= 0 & offset < clen - 5) {
                    int x1 = (int)(coordsXY[offset++]*65536.0f);
                    int y1 = (int)(coordsXY[offset++]*65536.0f);
                    int x2 = (int)(coordsXY[offset++]*65536.0f);
                    int y2 = (int)(coordsXY[offset++]*65536.0f);
                    int x3 = (int)(coordsXY[offset++]*65536.0f);
                    int y3 = (int)(coordsXY[offset++]*65536.0f);
                    cubicTo(x1, y1, x2, y2, x3, y3);
                }
                break;

            case COMMAND_CLOSE:
                close();
                break;
            }
        }
        endRendering();
    }

    /**
     * Render a complex path, possibly caching the results in a form
     * that can be rendered more rapidly at a future time.  The cache
     * will be valid across changes in paint style, but not across
     * changes to the transform, stroke/fill mode setting, stroke
     * parameters, or winding rule.
     
     * <p> The implementation does not check the validity of the cache
     * relative to changes in the renderer state.  It is up to the
     * caller to manually invalidate the cache object as needed.  The
     * other parameters must contain a valid description of the path
     * even if a valid cache is passed in.  If <code>cache</code> is
     * <code>null</code>, no caching is performed.
     *
     * <p> This method is equivalent to:
     *
     * <pre>
     * beginRendering(windingRule);
     *
     * PiscesCache cache = getCache();
     * if (cache != null) {
     *   if (cache.isValid()) {
     *     // Render using the cached form of the path
     *     renderFromCache(cache);
     *   } else {
     *     // Perform rendering and optionally place a pre-renderered
     *     // representation of the results into the cache
     *     renderAndComputeCache(numCommands, commands, offsets, coordsXY,
     *                           windingRule, cache);
     *   }
     * } else {
     *   // Perform rendering without a cache
     *   renderNoCache(numCommands, commands, offsets, coordsXY, windingRule);
     * }
     *
     * endRendering();
     * </pre>
     *
     * <p> Any command for which the value of <code>offsets</code>
     * would lead to a reference outside of the bounds of
     * <code>coordsXY</code> will not be issued.
     *
     * <p> Retrieval of the bounding box using <code>getBoundingBox</code>
     * following a call to <code>render</code> is supported.
     */
    public void renderPath(int numCommands,
                           byte[] commands,
                           float[] coordsXY,
                           int windingRule,
                           PiscesCache cache) {
        if (cache != null) {
            if (cache.isValid()) {
                rdr.renderFromCache(cache);
            } else {
                rdr.setCache(cache);
                renderPath(numCommands, commands, coordsXY, windingRule);
                rdr.setCache(null);
            }
        } else {
            renderPath(numCommands, commands, coordsXY, windingRule);
        }
    }

    /**
     * Returns a bounding box containing all pixels drawn during the
     * rendering of the most recent primitive
     * (beginRendering/endRendering pair).  The bounding box is
     * returned in the form (x, y, width, height).
     */
    public void getBoundingBox(int[] bbox) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("bbox = new int[4];");
                logStream.println("pr.getBoundingBox(bbox);");
            }
        }
        rdr.getBoundingBox(bbox);
    }

    public void setStroke() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.setStroke();");
            }
        }
        isPathFilled = false;
        this.externalConsumer = getStroker();
    }
    
    public void setFill() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.setFill();");
            }
        }
        isPathFilled = true ;
        this.externalConsumer = getFiller();
    }

    public void setTextFill() {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.setTextFill();");
            }
        }
        isPathFilled = true;
        this.externalConsumer = getTextFiller();
    }

    public void drawLine(int x0, int y0, int x1, int y1) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.drawLine(" +
                                  x0 + ", " +
                                  y0 + ", " +
                                  x1 + ", " +
                                  y1 + ");"); 
            }
        }
        myBeginRendering(RendererBase.WIND_NON_ZERO);
        PathSink stroker = getStroker();
        stroker.moveTo(x0, y0);
        stroker.lineTo(x1, y1);
        stroker.end();
        myEndRendering();
    }

    private static int[] convert8To5;
    private static int[] convert8To6;

    static {
        convert8To5 = new int[256];
        convert8To6 = new int[256];

        for (int i = 0; i < 256; i++) {
            convert8To5[i] = (i*31 + 127)/255;
            convert8To6[i] = (i*63 + 127)/255;
        }
    }

    /**
     * 
     * @param x the X coordinate in S15.16 format.
     * @param y the Y coordinate in S15.16 format.
     * @param w the width in S15.16 format.
     * @param h the height in S15.16 format.
     */
    public void fillRect(int x, int y, int w, int h) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.fillRect(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ");"); 
            }
        }

        if (w <= 0 || h <= 0) {
            return;
        }

        // Renderer will detect aligned rectangles
        PathSink filler = getFiller();
        fillOrDrawRect(filler, x, y, w, h);
    }

    public void drawRect(int x, int y, int w, int h) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.drawRect(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ");"); 
            }
        }

        if (w <= 0 || h <= 0) {
            return;
        }

        // If dashing is disabled, and using mitered joins,
        // simply draw two opposing rect outlines separated
        // by linewidth
        if (dashArray == null) {
            if ((joinStyle == Stroker.JOIN_MITER)
                    && (miterLimit >= PiscesMath.SQRT_TWO)) {
                int x0 = x + STROKE_X_BIAS;
                int y0 = y + STROKE_Y_BIAS;
                int x1 = x0 + w;
                int y1 = y0 + h;
                
                int lw = lineWidth;
                int m = lineWidth/2;
                
                PathSink filler = getFiller();
                myBeginRendering(RendererBase.WIND_NON_ZERO);
                filler.moveTo(x0 - m, y0 - m);
                filler.lineTo(x1 + m, y0 - m);
                filler.lineTo(x1 + m, y1 + m);
                filler.lineTo(x0 - m, y1 + m);
                filler.close();
                
                // Hollow out interior if w and h are greater than linewidth
                if ((x1 - x0) > lw && (y1 - y0) > lw) {
                    filler.moveTo(x0 + m, y0 + m);
                    filler.lineTo(x0 + m, y1 - m);
                    filler.lineTo(x1 - m, y1 - m);
                    filler.lineTo(x1 - m, y0 + m);
                    filler.close();
                }
                
                filler.end();
                myEndRendering();
                return;
            } else if (joinStyle == Stroker.JOIN_ROUND) {
                // IMPL NOTE - accelerate hollow rects with round joins
            }
        }

        PathSink stroker = getStroker();
        fillOrDrawRect(stroker, x, y, w, h);
    }

    private void fillOrDrawRect(PathSink consumer,
                                int x, int y, int w, int h) {
        int x0 = x;
        int y0 = y;
        int x1 = x0 + w;
        int y1 = y0 + h;
        
        myBeginRendering(RendererBase.WIND_NON_ZERO);
        consumer.moveTo(x0, y0);
        consumer.lineTo(x1, y0);
        consumer.lineTo(x1, y1);
        consumer.lineTo(x0, y1);
        consumer.close();
        consumer.end();
        myEndRendering();
    }

    public void drawOval(int x, int y, int w, int h) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.drawOval(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ");"); 
            }
        }
        fillOrDrawOval(x, y, w, h, true);
    }

    public void fillOval(int x, int y, int w, int h) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.fillOval(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ");"); 
            }
        }
        fillOrDrawOval(x, y, w, h, false);
    }

    // Emit quarter-arc about a central point (cx, cy).
    // Each quadrant is suitably reflected
    private void emitQuadrants(PathSink consumer, int cx, int cy,
                               int[] points, int nPoints) {
        // Emit quarter-arc once for each quadrant, suitably
        // reflected
        for (int pass = 0; pass < 4; pass++) {
            int xsign = 1;
            int ysign = 1;
            if (pass == 1 || pass == 2) xsign = -1;
            if (pass == 2 || pass == 3) ysign = -1;
            int incr = 2*xsign*ysign;
            int idx = (incr > 0) ? 0 : 2*nPoints - 2;

            for (int j = 0; j < nPoints; j++) {
                consumer.lineTo(cx + xsign*points[idx],
                                cy + ysign*points[idx + 1]);
                idx += incr;
            }
        }
    }

    private void emitOval(PathSink consumer,
                          int cx, int cy, int rx, int ry,
                          int nPoints, boolean reverse) {
        int i = reverse ? nPoints - 1 : 1;
        int incr = reverse ? -1 : 1;

        consumer.moveTo(cx + rx, cy);

        nPoints /= 4;
        int[] points = new int[2*nPoints];
        int idx = 0;

        for (int j = 0; j < nPoints; j++) {
            int theta = i*PiscesMath.TWO_PI/(4*nPoints);
            int ox = PiscesMath.cos(theta);
            int oy = PiscesMath.sin(theta);
            points[idx++] = (int)((long)rx*ox >> 16);
            points[idx++] = (int)((long)ry*oy >> 16);

            i += incr;
        }
        
        emitQuadrants(consumer, cx, cy, points, nPoints);
        consumer.close();
    }

    // Emit the outline of an oval, offset by pen radius lw2
    // The interior path may self-intersect, but this is handled
    // by using a WIND_NON_ZERO wining rule
    private void emitOffsetOval(PathSink consumer,
                                int cx, int cy, int rx, int ry,
                                int lw2,
                                int nPoints, boolean inside) {
        int i = inside ? nPoints - 1 : 1;
        int incr = inside ? -1 : 1;

        double drx = rx/65536.0;
        double dry = ry/65536.0;
        double dlw2 = lw2/65536.0;

        consumer.moveTo(cx + rx + lw2*incr, cy);

        nPoints /= 4;
        int[] points = new int[2*nPoints];
        int idx = 0;

        for (int j = 0; j < nPoints; j++) {
            double dtheta = i*(Math.PI/2.0)/nPoints;
            double cosTheta = Math.cos(dtheta);
            double sinTheta = Math.sin(dtheta);
            double drxSinTheta = drx*sinTheta;
            double dryCosTheta = dry*cosTheta;
            double den = dlw2/Math.sqrt(drxSinTheta*drxSinTheta +
                                        dryCosTheta*dryCosTheta);
            double dpx = cosTheta*(drx + incr*dry*den);
            double dpy = sinTheta*(dry + incr*drx*den);
            
            int px = (int)(dpx*65536.0);
            int py = (int)(dpy*65536.0);

            points[idx++] = px;
            points[idx++] = py;

            i += incr;
        }

        emitQuadrants(consumer, cx, cy, points, nPoints);
        consumer.close();
    }

    private void fillOrDrawOval(int x, int y, int w, int h,
                                boolean hollow) {
        if (w <= 0 || h <= 0) {
            return;
        }

//         if (!antialiasingOn) {
//             lineWidth = (lineWidth + 0xffff) & 0xffff0000;
//         }

        int w2 = w >> 1;
        int h2 = h >> 1;
        int cx = x + w2;
        int cy = y + h2;
        int lineWidth2 = hollow ? lineWidth/2 : 0;
        
        int wl = w2 + lineWidth2;
        int hl = h2 + lineWidth2;
        int nPoints = Math.max(16, Math.max(wl, hl) >> 13);

        myBeginRendering(RendererBase.WIND_NON_ZERO);

        // Stroke the outline if dashing
        if (hollow && dashArray != null) {
            PathSink stroker = getStroker();
            emitOval(stroker, cx, cy, w2, h2, nPoints, false);
            stroker.end();
            myEndRendering();
            return;
        }

        if (!antialiasingOn) {
            cx += STROKE_X_BIAS;
        }

        // Draw exterior outline
        PathSink filler = getFiller();

        if (w == h) {
            emitOval(filler, cx, cy, wl, hl, nPoints, false);
        } else {
            emitOffsetOval(filler, cx, cy, w2, h2, lineWidth2,
                           nPoints, false);
        }

        // Draw interior in the reverse direction
        if (hollow) {
            wl = w2 - lineWidth2;
            hl = h2 - lineWidth2;
            
            if (wl > 0 && hl > 0) {
                if (w == h) {
                    emitOval(filler, cx, cy, wl, hl, nPoints, true);
                } else {
                    emitOffsetOval(filler, cx, cy, w2, h2, lineWidth2,
                                   nPoints, true);
                }
            }
        }
        filler.end();
        myEndRendering();
    }

    private static final long acv = (long)(65536.0*0.22385762508460333);

    public void fillRoundRect(int x, int y, int w, int h,
                              int aw, int ah) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.fillRoundRect(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ", " +
                                  aw + ", " +
                                  ah + ");"); 
            }
        }
        if (w <= 0 || h <= 0) {
            return;
        }
        fillOrDrawRoundRect(x, y, w, h, aw, ah, false);
    }

    public void drawRoundRect(int x, int y, int w, int h,
                              int aw, int ah) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.drawRoundRect(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ", " +
                                  aw + ", " +
                                  ah + ");"); 
            }
        }
        if (w < 0 || h < 0) {
            return;
        }
        fillOrDrawRoundRect(x, y, w, h, aw, ah, true);
    }

    // Args are S15.16
    private void emitRoundRect(PathSink consumer,
                               int x, int y, int w, int h, int aw, int ah,
                               boolean reverse) {
        int xw = x + w;
        int yh = y + h;

        int aw2 = aw >> 1;
        int ah2 = ah >> 1;
        int acvaw = (int)(acv*aw >> 16);
        int acvah = (int)(acv*ah >> 16);
        int xacvaw = x + acvaw;
        int xw_acvaw = xw - acvaw;
        int yacvah = y + acvah;
        int yh_acvah = yh - acvah;
        int xaw2 = x + aw2;
        int xw_aw2 = xw - aw2;
        int yah2 = y + ah2;
        int yh_ah2 = yh - ah2;
        
        consumer.moveTo(x, yah2);
        if (reverse) {
            consumer.cubicTo(x, yacvah, xacvaw, y, xaw2, y);
            consumer.lineTo(xw_aw2, y);
            consumer.cubicTo(xw_acvaw, y, xw, yacvah, xw, yah2);
            consumer.lineTo(xw, yh_ah2);
            consumer.cubicTo(xw, yh_acvah, xw_acvaw, yh, xw_aw2, yh);
            consumer.lineTo(xaw2, yh);
            consumer.cubicTo(xacvaw, yh, x, yh_acvah, x, yh_ah2);
        } else {
            consumer.lineTo(x, yh_ah2);
            consumer.cubicTo(x, yh_acvah, xacvaw, yh, xaw2, yh);
            consumer.lineTo(xw_aw2, yh);
            consumer.cubicTo(xw_acvaw, yh, xw, yh_acvah, xw, yh_ah2);
            consumer.lineTo(xw, yah2);
            consumer.cubicTo(xw, yacvah, xw_acvaw, y, xw_aw2, y);
            consumer.lineTo(xaw2, y);
            consumer.cubicTo(xacvaw, y, x, yacvah, x, yah2);
        }
        consumer.close();
        consumer.end();
    }

    private void fillOrDrawRoundRect(int x, int y, int w, int h,
                                     int aw, int ah,
                                     boolean stroke) {
        PathSink consumer = stroke ? getStroker() : getFiller();

        if (aw < 0) aw = -aw;
        if (aw > w) aw = w;
        if (ah < 0) ah = -ah;
        if (ah > h) ah = h;

        // If stroking but not dashing, draw the outer and inner
        // contours explicitly as round rects
        //
        // Note - this only works if aw == ah since the result of tracing
        // a circle with a circular pen is a larger circle, but the result
        // of tracing an ellipse with a circular pen is not (generally)
        // an ellipse...
        if (stroke && dashArray == null && aw == ah) {
            int lineWidth2 = lineWidth >> 1;

            myBeginRendering(RendererBase.WIND_NON_ZERO);
            PathSink filler = getFiller();

            x += STROKE_X_BIAS;
            y += STROKE_Y_BIAS;

            emitRoundRect(filler,
                          x - lineWidth2, y - lineWidth2,
                          w + lineWidth, h + lineWidth,
                          aw + lineWidth, ah + lineWidth, false);

            // Empty out inner rect
            w -= lineWidth;
            h -= lineWidth;
            if (w > 0 && h > 0) {
                emitRoundRect(filler,
                              x + lineWidth2, y + lineWidth2,
                              w, h,
                              aw - lineWidth, ah - lineWidth, true);
            }
            myEndRendering();
        } else {
            myBeginRendering(RendererBase.WIND_NON_ZERO);
            emitRoundRect(consumer, x, y, w, h, aw, ah, false);
            myEndRendering();
        }
    }

    public void drawArc(int x, int y, int width, int height,
                        int startAngle, int arcAngle, int arcType) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.drawArc(" +
                                  x + ", " +
                                  y + ", " +
                                  width + ", " +
                                  height + ", " +
                                  startAngle + ", " + 
                                  arcAngle + ", " + 
                                  arcType + ");"); 
           }
        }
        fillOrDrawArc(x, y, width, height, startAngle, arcAngle, arcType, true);
    }

    public void fillArc(int x, int y, int width, int height,
                        int startAngle, int arcAngle, int arcType) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.fillArc(" +
                                  x + ", " +
                                  y + ", " +
                                  width + ", " +
                                  height + ", " +
                                  startAngle + ", " +
                                  arcAngle + ", " + 
                                  arcType + ");"); 
            }
        }
        fillOrDrawArc(x, y, width, height, startAngle, arcAngle, arcType, false);
    }

    private void emitArc(PathSink consumer,
                         int cx, int cy, int rx, int ry,
                         int startAngle, int endAngle,
                         int nPoints) {
        boolean first = true;

        for (int i = 0; i < nPoints; i++) {
            int theta = startAngle + i*(endAngle - startAngle)/(nPoints - 1);
            int ox = PiscesMath.cos(theta);
            int oy = PiscesMath.sin(theta);

            int lx = cx + (int)((long)rx*ox >> 16);
            int ly = cy - (int)((long)ry*oy >> 16);
            if (first) {
                consumer.moveTo(lx, ly);
                first = false;
            } else {
                consumer.lineTo(lx, ly);
            }
        }
    }

    public void fillOrDrawArc(int x, int y, int width, int height,
                              int startAngle, int arcAngle, int arcType,
                              boolean stroke) {
        PathSink consumer = stroke ? getStroker() : getFiller();
        if (width <= 0 || height <= 0) {
            return;
        }

        int w2 = width >> 1;
        int h2 = height >> 1;
        int cx = x + w2;
        int cy = y + h2;

        startAngle = (int)(((long)startAngle*PiscesMath.TWO_PI)/(360*65536));
        arcAngle = (int)(((long)arcAngle*PiscesMath.TWO_PI)/(360*65536));

        int endAngle = startAngle + arcAngle;

        int nPoints = Math.max(16, Math.max(w2, h2) >> 16);

        myBeginRendering(RendererBase.WIND_NON_ZERO);
        emitArc(consumer, cx, cy, w2, h2, startAngle, endAngle, nPoints);
        if (arcType == ARC_PIE) {
            consumer.lineTo(cx, cy);
        }
        if (!stroke || (arcType == ARC_CHORD) || (arcType == ARC_PIE)) {
            consumer.close();
        }
        consumer.end();
        myEndRendering();
    }
    
    public void getImageData() {
        rdr.getImageData(data, offset, scanlineStride);
    }

    public void clearRect(int x, int y, int w, int h) {
        if (enableLogging) {
            if (logStream != null) {
                logStream.println("pr.clearRect(" +
                                  x + ", " +
                                  y + ", " +
                                  w + ", " +
                                  h + ");"); 
            }
        }

        int maxX = x + w;
        int maxY = y + h;
        
        x = Math.max(x, 0);
        x = Math.max(x, bbMinX);

        y = Math.max(y, 0);
        y = Math.max(y, bbMinY);

        maxX = Math.min(maxX, this.width);
        maxX = Math.min(maxX, bbMaxX);

        maxY = Math.min(maxY, this.height);
        maxY = Math.min(maxY, bbMaxY);

        rdr.clearRect(x, y, maxX - x, maxY - y);
    }

    public void setPathData(float[] data, byte[] commands, int nCommands) {
        throw new IllegalStateException("Not implemented");
    }
}
