/* 
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
 *
 */

package com.sun.pisces;

import com.sun.me.gci.renderer.GCIAdvancedShapeRenderer;
import com.sun.me.gci.renderer.GCIRenderContext;
import com.sun.me.gci.renderer.GCIRenderer;
import com.sun.me.gci.renderer.GCIShapeRenderer;
import com.sun.me.gci.surface.GCIDrawingSurface;
import com.sun.me.gci.surface.GCISurfaceInfo;


/**
 * <code>PiscesShapeRenderer</code>
 */
public class PiscesShapeRenderer implements GCIShapeRenderer {
    protected PiscesRenderer piscesRenderer;

	// 1.0 in Fixed point 16.16 notation.
    static final float FIXED_1_0 = 65536.0f;
    
    GCIRenderContext context;
    int[] clip = new int[4];
    
    private static final int[] GCI_TO_PISCES_COMPOSITE = {
        RendererBase.COMPOSITE_SRC_OVER,
        RendererBase.COMPOSITE_CLEAR,
        RendererBase.COMPOSITE_SRC,
        RendererBase.COMPOSITE_SRC_OVER        
    };
    
    /**
     * Factory method as mandated by GCI.
     */
    public static GCIShapeRenderer newInstance(GCIDrawingSurface surface,
        boolean lcduiGraphics,
        boolean useContextTransform) {
        if ( surface.isSurfaceInfoDynamic() ) {
            return new PiscesShapeRenderer.DynamicSurface(surface);
        } else {
            return new PiscesShapeRenderer(surface);
        }
    }
    
    /**
     * Factory method as mandated by GCI.
     */
    public static GCIAdvancedShapeRenderer newAdvancedInstance(
        GCIDrawingSurface surface) {
        if ( surface.isSurfaceInfoDynamic() ) {
            return new PiscesAdvancedShapeRenderer.DynamicSurface(surface);
        } else {
            return new PiscesAdvancedShapeRenderer(surface);
        }
    }
    
    public GCIRenderer create() {
        return new PiscesShapeRenderer(this.context.getDrawingSurface());
    }
    
    protected static int getByteShiftForPixelArrayCount(int arrayType){
        int denom = 1;
         switch (arrayType){
            case GCISurfaceInfo.TYPE_JAVA_ARRAY_BYTE:
                denom = 0;
            break;
            case GCISurfaceInfo.TYPE_JAVA_ARRAY_SHORT:
                denom = 1;
            break;
            default:
                denom = 2;
            break;    
         }
         return denom;
    }
    
    /** Creates a new instance of PiscesShapeRenderer */
    protected PiscesShapeRenderer(GCIDrawingSurface surface) {
        piscesRenderer = new PiscesRenderer(new PiscesGCISurface(surface));    
        
        this.clip[0] = 0;
        this.clip[1] = 0;
        this.clip[2] = surface.getWidth();
        this.clip[3] = surface.getHeight();
    }
    
    /**
     * Notifies the renderer that a render context is created
     * for this renderer. The context must be initialized with all the
     * attributes when this method is invoked.
     */
    public void contextCreated(GCIRenderContext context) {
        GCIDrawingSurface surface = context.getDrawingSurface();
        this.context = context;
        clipModified();
        paintModified();
        strokeModified();
        compositeModified();
        transformModified();
    }
    
    /**
     * Notifies the renderer that the render context is disposed. The
     * renderer implementation can free any resources allocated for
     * servicing the context.
     */
    public void contextDisposed() {
        releaseSurface();
        this.context = null;
    }
    
    /**
     * Notifies the renderer that some of the attribute[s] in the
     * render context has been modified. 
     *
     * @param attribute one of the constant specified in the <b>see also</b>
     *        section should be passed.
     *
     * @see com.sun.me.gci.renderer.GCIRenderContext#ATTRIBUTE_CLIP
     * 
     */
    public void attributesModified(int attribute) {
        if ( (attribute & GCIRenderContext.ATTRIBUTE_CLIP) != 0 ){
            this.clipModified();
        }
        if ( (attribute & GCIRenderContext.ATTRIBUTE_PAINT) != 0 ){
            this.paintModified();
        }
        if ( (attribute & GCIRenderContext.ATTRIBUTE_COMPOSITE) != 0 ){
            this.compositeModified();
        }
        if ( (attribute & GCIRenderContext.ATTRIBUTE_STROKE) != 0 ){
            this.strokeModified();
        }
    }
    
    void acquireSurface() {
        ((PiscesGCISurface)piscesRenderer.surface).acquireSurface();       
    }
    
    void releaseSurface() {
       ((PiscesGCISurface)piscesRenderer.surface).releaseSurface();
    }
    
    public void drawPolyline(int xPoints[], int yPoints[],
                         int nPoints,
                         boolean close) {
        fillOrDrawPolyline(xPoints, yPoints, nPoints, close,
            true /* stroke the polyline */
            );
    }
    
    public void fillPolygon(int xPoints[], int yPoints[],int nPoints) {
        fillOrDrawPolyline(xPoints, yPoints, nPoints,
            true, /* close the path */
            false /* fill the polyline */
            );
    }
    
    private void fillOrDrawPolyline(int xPoints[], int yPoints[],
        int nPoints,
        boolean close,
        boolean stroke) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.beginRendering(RendererBase.WIND_NON_ZERO);

        // path begin
        if ( !stroke )
            piscesRenderer.setFill();
        piscesRenderer.moveTo((xPoints[0]<<16), (yPoints[0]<<16));
        for (int i = 1; i < nPoints; i++) {
            piscesRenderer.lineTo((xPoints[i]<<16), (yPoints[i]<<16));
        }
        if (close) {
            piscesRenderer.close();
        }
        piscesRenderer.end(); // path end
        piscesRenderer.endRendering();
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawArc(int x, int y, int width, int height,
                        int startAngle, int arcAngle) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.drawArc(x<<16,y<<16,width<<16,height<<16,
            startAngle<<16,arcAngle<<16,
            PiscesRenderer.ARC_OPEN );
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillArc(int x, int y, int width, int height,
                        int startAngle, int arcAngle) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.fillArc(x<<16,y<<16,width<<16,height<<16,
            startAngle<<16,arcAngle<<16,
            PiscesRenderer.ARC_PIE);
        this.context.getDrawingSurface().renderingEnd(this.clip);
        
    }
    
    public void fillOval(int x, int y, int w, int h) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.fillOval(x<<16, y<<16, w<<16, h<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillRoundRect(int x, int y, int w, int h, int aw, int ah) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.fillRoundRect(x<<16, y<<16, w<<16, h<<16, aw<<16, ah<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillRect(int x, int y, int w, int h) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.fillRect(x<<16, y<<16, w<<16, h<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }
    public void fillRect2(int x, int y, int w, int h) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.fillRect(x, y, w, h);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawRoundRect(int x, int y, int w, int h, int aw, int ah) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.drawRoundRect(x<<16, y<<16, w<<16, h<<16, aw<<16, ah<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawRect(int x, int y, int w, int h) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.drawRect(x<<16, y<<16, w<<16, h<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawOval(int x, int y, int w, int h) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.drawOval(x<<16, y<<16, w<<16, h<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawLine(int x0, int y0, int x1, int y1) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.drawLine(x0<<16, y0<<16, x1<<16, y1<<16);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void paintModified() {
        int paintMode = context.getPaintMode();
        if ( paintMode == GCIRenderContext.PAINT_MODE_COLOR ){
            int color = context.getPaintColor();
            piscesRenderer.setColor((color>>16) & 0xFF, // red
                (color>>8) & 0xFF,             // green
                (color) & 0xFF,                // blue
                (color>>24) & 0xFF );          // alpha
        }
        else
        if ( paintMode == GCIRenderContext.PAINT_MODE_XOR ){

        }
        else
        if ( paintMode == GCIRenderContext.PAINT_MODE_LINEAR_GRADIENT ){
             // for agui
        }
    }

    public void clipModified() {
        context.getClip(this.clip);
        piscesRenderer.setClip(this.clip[0], this.clip[1],this.clip[2], this.clip[3]);
    }
    
    public void strokeModified() {
        float[] floatDash = this.context.getDashPattern();
        int dashPhase = 0;
        int[] fpDash = null;
        
        if ( floatDash != null ) {
            fpDash = new int[floatDash.length];
            for ( int i = 0 ; i < fpDash.length ; i++ ){
                fpDash[i] = (int)(floatDash[i]*FIXED_1_0);
            }
            dashPhase = (int)(this.context.getDashPhase()*FIXED_1_0);
        }
        piscesRenderer.setStroke((int)(this.context.getLineWidth()*FIXED_1_0),
            this.context.getCapStyle(),
            this.context.getJoinStyle(),
            (int)(this.context.getMiterLimit()*FIXED_1_0),
            fpDash,
            dashPhase);
    }

    public void transformModified() {
    }

    public void compositeModified() {
        piscesRenderer.setComposite(
                GCI_TO_PISCES_COMPOSITE[this.context.getCompositeRule()],
                this.context.getCompositeAlpha());
    }

    public final void fontModified() {
    }
    
    public void hintsModified() {
        piscesRenderer.setAntialiasing(this.context.isAntiAliasingOn());
    }

    
    static class DynamicSurface extends PiscesShapeRenderer {
        DynamicSurface(GCIDrawingSurface surface) {
            super(surface);
        }
        public GCIRenderer create() {
            return new DynamicSurface(this.context.getDrawingSurface());
        }

        public void fillPolygon(int[] xPoints, int[] yPoints, int nPoints) {
            this.acquireSurface();
            super.fillPolygon(xPoints, yPoints, nPoints);
            this.releaseSurface();
        }

        public void drawPolyline(int[] xPoints, int[] yPoints, int nPoints, boolean close) {
            this.acquireSurface();
            super.drawPolyline(xPoints, yPoints, nPoints, close);
            this.releaseSurface();
        }

        public void fillRoundRect(int x, int y, int w, int h, int aw, int ah) {
            this.acquireSurface();
            super.fillRoundRect(x, y, w, h, aw, ah);
            this.releaseSurface();
        }

        public void fillRect(int x, int y, int w, int h) {
            this.acquireSurface();
            super.fillRect(x, y, w, h);
            this.releaseSurface();
        }

        public void fillOval(int x, int y, int w, int h) {
            this.acquireSurface();
            super.fillOval(x, y, w, h);
            this.releaseSurface();
        }

        public void fillArc(int x, int y, int width, int height, 
            int startAngle, int arcAngle) {
            this.acquireSurface();
            super.fillArc(x, y, width, height, startAngle, arcAngle);
            this.releaseSurface();
        }

        public void drawRoundRect(int x, int y, int w, int h, int aw, int ah) {
            this.acquireSurface();
            super.drawRoundRect(x, y, w, h, aw, ah);
            this.releaseSurface();
        }

        public void drawRect(int x, int y, int w, int h) {
            this.acquireSurface();
            super.drawRect(x, y, w, h);
            this.releaseSurface();
        }

        public void drawOval(int x, int y, int w, int h) {
            this.acquireSurface();
            super.drawOval(x, y, w, h);
            this.releaseSurface();
        }

        public void drawLine(int x0, int y0, int x1, int y1) {
            this.acquireSurface();
            super.drawLine(x0, y0, x1, y1);
            this.releaseSurface();
        }

        public void drawArc(int x, int y, int width, int height, 
            int startAngle, int arcAngle) {
            this.acquireSurface();
            super.drawArc(x, y, width, height, startAngle, arcAngle);
            this.releaseSurface();
        }
    }
}


class PiscesAdvancedShapeRenderer extends PiscesShapeRenderer 
    implements GCIAdvancedShapeRenderer {
    PiscesAdvancedShapeRenderer(GCIDrawingSurface surface) {
        super(surface);
    }
    
    public GCIRenderer create() {
        return new PiscesAdvancedShapeRenderer(this.context.getDrawingSurface());
    }
    
    public void transformModified() {
        float[] tx = new float[6];
        this.context.getTransformMatrix(tx);
        piscesRenderer.setTransform(new Transform6(
            (int)(tx[0]*FIXED_1_0),
            (int)(tx[1]*FIXED_1_0),
            (int)(tx[2]*FIXED_1_0),
            (int)(tx[3]*FIXED_1_0), 
            (int)(tx[4]*FIXED_1_0),
            (int)(tx[5]*FIXED_1_0)));
    }
    
    public void drawLine(float x0, float y0, float x1, float y1) {
        //this.context.getDrawingSurface().renderingBegin();
        super.drawLine((int)(x0), 
            (int)(y0), 
            (int)(x1),
            (int)(y1));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillRect(float x, float y, float width, float height) {
        //this.context.getDrawingSurface().renderingBegin();
        super.fillRect((int)(x), 
            (int)(y), 
            (int)(width),
            (int)(height));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawRect(float x, float y, float width, float height) {
        //this.context.getDrawingSurface().renderingBegin();
        super.drawRect((int)(x), 
            (int)(y), 
            (int)(width),
            (int)(height));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawRoundRect(float x, float y, float width, 
        float height, float arcWidth, float arcHeight) {
        //this.context.getDrawingSurface().renderingBegin();
        super.drawRoundRect((int)(x), 
            (int)(y), 
            (int)(width),
            (int)(height),
            (int)(arcWidth),
            (int)(arcHeight));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillRoundRect(float x, float y, float width, float height, 
        float arcWidth, float arcHeight) {
        //this.context.getDrawingSurface().renderingBegin();
        super.fillRoundRect((int)(x), 
            (int)(y), 
            (int)(width),
            (int)(height),
            (int)(arcWidth),
            (int)(arcHeight));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawOval(float x, float y, float width, float height) {
        //this.context.getDrawingSurface().renderingBegin();
        super.drawOval((int)(x), 
            (int)(y), 
            (int)(width),
            (int)(height));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillOval(float x, float y, float width, float height) {
        //this.context.getDrawingSurface().renderingBegin();
        super.fillOval((int)(x), 
            (int)(y), 
            (int)(width),
            (int)(height));
        //this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void drawArc(float x, float y, float width, 
        float height, float startAngle, float arcAngle, int arcType) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.drawArc((int)(x*FIXED_1_0), 
            (int)(y*FIXED_1_0), 
            (int)(width*FIXED_1_0),
            (int)(height*FIXED_1_0),
            (int)(startAngle*FIXED_1_0),
            (int)(arcAngle*FIXED_1_0),
            arcType);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void fillArc(float x, float y, float width, 
        float height, float startAngle, float arcAngle, int arcType) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.fillArc((int)(x*FIXED_1_0), 
            (int)(y*FIXED_1_0), 
            (int)(width*FIXED_1_0),
            (int)(height*FIXED_1_0),
            (int)(startAngle*FIXED_1_0),
            (int)(arcAngle*FIXED_1_0),
            arcType);
        this.context.getDrawingSurface().renderingEnd(this.clip);
    }

    public void beginRendering(int[] outputRegion, int windingRule) {
        this.context.getDrawingSurface().renderingBegin();
        piscesRenderer.beginRendering(outputRegion[0],outputRegion[1],
            outputRegion[2],outputRegion[3],windingRule);
    }

    public void beginRendering(int windingRule) {
        piscesRenderer.beginRendering(windingRule);
    }

    public void endRendering(int[] affectedRegion) {
        piscesRenderer.endRendering();
        piscesRenderer.getBoundingBox(affectedRegion);
        this.context.getDrawingSurface().renderingEnd(affectedRegion);
    }

    public void pathBegin(float pathMinX, float pathMinY, 
        float pathMaxX, float pathMaxY, boolean strokePath) {
        pathBegin(strokePath);
    }

    public void pathBegin(boolean strokePath) {
         if (strokePath) {
            piscesRenderer.setStroke();
        } else {
            piscesRenderer.setFill();
        }
    }

    public void pathMoveTo(float x0, float y0) {
        piscesRenderer.moveTo((int)(x0*FIXED_1_0), (int)(y0*FIXED_1_0));
    }

    public void pathLineTo(float x1, float y1) {
         piscesRenderer.lineTo((int)(x1*FIXED_1_0), (int)(y1*FIXED_1_0));
    }

    public void pathQuadTo(float x1, float y1, float x2, float y2) {
         piscesRenderer.quadTo((int)(x1*FIXED_1_0), (int)(y1*FIXED_1_0),
             (int)(x2*FIXED_1_0), (int)(y2*FIXED_1_0));
    }

    public void pathCubicTo(float x1, float y1, float x2, float y2, 
        float x3, float y3) {
         piscesRenderer.cubicTo((int)(x1*FIXED_1_0), (int)(y1*FIXED_1_0),
             (int)(x2*FIXED_1_0), (int)(y2*FIXED_1_0),
             (int)(x3*FIXED_1_0), (int)(y3*FIXED_1_0));
    }

    public void pathClose() {
        piscesRenderer.close();
    }

    public void pathEnd() {
        piscesRenderer.end();
    }
    
     static class DynamicSurface extends PiscesAdvancedShapeRenderer {
        DynamicSurface(GCIDrawingSurface surface) {
            super(surface);
        }
        
        public GCIRenderer create() {
            return new DynamicSurface(this.context.getDrawingSurface());
        }

        public void fillPolygon(int[] xPoints, int[] yPoints, int nPoints) {
            this.acquireSurface();
            super.fillPolygon(xPoints, yPoints, nPoints);
            this.releaseSurface();
        }

        public void drawPolyline(int[] xPoints, int[] yPoints, int nPoints, boolean close) {
            this.acquireSurface();
            super.drawPolyline(xPoints, yPoints, nPoints, close);
            this.releaseSurface();
        }

        public void fillRoundRect(int x, int y, int w, int h, int aw, int ah) {
            this.acquireSurface();
            super.fillRoundRect(x, y, w, h, aw, ah);
            this.releaseSurface();
        }

        public void fillRect(int x, int y, int w, int h) {
            this.acquireSurface();
            super.fillRect(x, y, w, h);
            this.releaseSurface();
        }

        public void fillOval(int x, int y, int w, int h) {
            this.acquireSurface();
            super.fillOval(x, y, w, h);
            this.releaseSurface();
        }

        public void fillArc(int x, int y, int width, int height, 
            int startAngle, int arcAngle) {
            this.acquireSurface();
            super.fillArc(x, y, width, height, startAngle, arcAngle);
            this.releaseSurface();
        }

        public void drawRoundRect(int x, int y, int w, int h, int aw, int ah) {
            this.acquireSurface();
            super.drawRoundRect(x, y, w, h, aw, ah);
            this.releaseSurface();
        }

        public void drawRect(int x, int y, int w, int h) {
            this.acquireSurface();
            super.drawRect(x, y, w, h);
            this.releaseSurface();
        }

        public void drawOval(int x, int y, int w, int h) {
            this.acquireSurface();
            super.drawOval(x, y, w, h);
            this.releaseSurface();
        }

        public void drawLine(int x0, int y0, int x1, int y1) {
            this.acquireSurface();
            super.drawLine(x0, y0, x1, y1);
            this.releaseSurface();
        }

        public void drawArc(int x, int y, int width, int height, 
            int startAngle, int arcAngle) {
            this.acquireSurface();
            super.drawArc(x, y, width, height, startAngle, arcAngle);
            this.releaseSurface();
        }
        
        public void beginRendering(int[] outputRegion, int windingRule) {
            this.acquireSurface();
            super.beginRendering(outputRegion, windingRule);
        }

        public void endRendering(int[] affectedRegion) {
            super.endRendering(affectedRegion);
            this.releaseSurface();
        }

        public void fillArc(float x, float y, float width, 
            float height, float startAngle, float arcAngle, int arcType) {
            this.acquireSurface();
            super.fillArc(x, y, width, height, startAngle, arcAngle, arcType);
            this.releaseSurface();
        }

        public void drawArc(float x, float y, float width, 
            float height, float startAngle, float arcAngle, int arcType) {
            this.acquireSurface();
            super.drawArc(x, y, width, height, startAngle, arcAngle, arcType);
            this.releaseSurface();
        }

        public void fillRoundRect(float x, float y, float width, 
            float height, float arcWidth, float arcHeight) {
            this.acquireSurface();
            super.fillRoundRect(x, y, width, height, arcWidth, arcHeight);
            this.releaseSurface();
        }

        public void fillRect(float x, float y, float width, float height) {
            this.acquireSurface();
            super.fillRect(x, y, width, height);
            this.releaseSurface();
        }

        public void fillOval(float x, float y, float width, float height) {
            this.acquireSurface();
            super.fillOval(x, y, width, height);
            this.releaseSurface();
        }

        public void drawRoundRect(float x, float y, float width, 
            float height, float arcWidth, float arcHeight) {
            this.acquireSurface();
            super.drawRoundRect(x, y, width, height, arcWidth, arcHeight);
            this.releaseSurface();
        }

        public void drawRect(float x, float y, float width, float height) {
            this.acquireSurface();
            super.drawRect(x, y, width, height);
            this.releaseSurface();
        }

        public void drawOval(float x, float y, float width, float height) {
            this.acquireSurface();
            super.drawOval(x, y, width, height);
            this.releaseSurface();
        }

        public void drawLine(float x0, float y0, float x1, float y1) {
            this.acquireSurface();
            super.drawLine(x0, y0, x1, y1);
            this.releaseSurface();
        }

    }
}

