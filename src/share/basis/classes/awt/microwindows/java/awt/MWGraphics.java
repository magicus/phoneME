/*
 * @(#)MWGraphics.java	1.46 06/10/10
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
 *
 */
package         java.awt;

import          java.io.*;
import          java.util.*;
import      java.awt.image.BufferedImage;
import          java.awt.image.ImageObserver;
import sun.awt.ConstrainableGraphics;

/**
 * MWGraphics is an object that encapsulates a graphics context for drawing with Microwindows.
 *
 * @version 1.12, 11/27/01
 */

class MWGraphics extends Graphics2D implements ConstrainableGraphics {
    /** The default font to use when no font has been set or null is used. */

    private static final Font DEFAULT_FONT = new Font("Dialog", Font.PLAIN, 12);
    private boolean disposed = false;
    private AlphaComposite composite;
    /** The current foreground color. */

    private Color   foreground;
    /** The current background color. This is used by clearRect. */

    private Color   background;
    /** The current xor color. If null then we are in paint mode. */

    private Color   xorColor;
    /** The currently selected font for text operations. */

    private Font    font;
    /** The font metrics for the font. Cached for performance. */

    private MWFontMetrics fontMetrics;
    /** Translated X offset from native offset. */

    private int     originX;
    /** Translated Y offset from native offset. */

    private int     originY;
    /** The MicoWindows PSD used to represent this Graphics object. */

    int             psd;
    /** The current user clip rectangle or null if no clip has been set. This is stored in the
     native coordinate system and not the possibly translated Java coordinate system. */

    private Rectangle      clip;
    /** The rectangle this graphics object has been constrained too. This is stored in the
     native coordinate system and not the (possibly) translated Java coordinate system.
     If it is null then this graphics has not been constrained. The constrained rectangle
     is another layer of clipping independant of the user clip. The actaul clip used is the
     intersection */

    private Rectangle constrainedRect;
    /** The GraphicsConfiguration that this graphics uses. */

    private GraphicsConfiguration graphicsConfiguration;
    static native int pClonePSD(int psd);

    static native void pDisposePSD(int psd);

    private static native void pSetColor(int psd, int rgb);

    private static native void pSetComposite(int psd, int rule, float extraAlpha);

    private static native void pSetPaintMode(int psd);

    private static native void pSetXORMode(int psd, int rgb);

    private static native void pDrawString(int psd, int font, String string, int x, int y);

    private static native void pDrawChars(int psd, int font, char[] chars, int offset, int length, int x, int y);

    private static native void pFillRect(int psd, int x, int y, int width, int height);

    private static native void pClearRect(int psd, int x, int y, int width, int height, int rgb);

    private static native void pDrawRect(int psd, int x, int y, int width, int height);

    private static native void pCopyArea(int psd, int x, int y, int width, int height, int dx, int dy);

    private static native void pDrawLine(int psd, int x1, int y1, int x2, int y2);

    private static native void pDrawPolygon(int psd, int orinX, int originY, int xPoints[], int yPoints[], int nPoints, boolean close);

    private static native void pFillPolygon(int psd, int orinX, int originY, int xPoints[], int yPoints[], int nPoints);

    private static native void pDrawArc(int psd, int x, int y, int width, int height, int start, int end);

    private static native void pFillArc(int psd, int x, int y, int width, int height, int start, int end);

    private static native void pDrawOval(int psd, int x, int y, int width, int height);

    private static native void pFillOval(int psd, int x, int y, int width, int height);

    private static native void pDrawRoundRect(int psd, int x, int y, int width, int height, int arcWidth, int arcHeight);

    private static native void pFillRoundRect(int psd, int x, int y, int width, int height, int arcWidth, int arcHeight);

    private static native void pChangeClipRect(int psd, int x, int y, int width, int height);

    private static native void pRemoveClip(int psd);

    private static native MWImage getBufferedImagePeer(BufferedImage bi);

    MWGraphics(Window window) {
        psd = pClonePSD(((MWGraphicsDevice) window.getGraphicsConfiguration().getDevice()).psd);
        foreground = window.foreground;
        if (foreground == null)
            foreground = Color.black;
        pSetColor(psd, foreground.value);
        background = window.background;
        if (background == null)
            background = Color.black;
        composite = AlphaComposite.SrcOver;
        graphicsConfiguration = window.getGraphicsConfiguration();
        setFont(DEFAULT_FONT);
    }

    MWGraphics(MWGraphics g) {
        psd = pClonePSD(g.psd);
        foreground = g.foreground;
        background = g.background;
        composite = g.composite;
        graphicsConfiguration = g.graphicsConfiguration;
        font = g.font;
        fontMetrics = g.fontMetrics;
        originX = g.originX;
        originY = g.originY;
        clip = g.clip;
        constrainedRect = g.constrainedRect;
    }

    MWGraphics(MWOffscreenImage image) {
        psd = pClonePSD(image.psd);
        foreground = image.foreground;
        if (foreground == null)
            foreground = Color.black;
        background = image.background;
        if (background == null)
            background = Color.black;
        composite = AlphaComposite.SrcOver;
        graphicsConfiguration = image.gc;
        font = image.font;
        if (font == null)
            font = DEFAULT_FONT;
        fontMetrics = MWFontMetrics.getFontMetrics(font);
    }

    MWGraphics(MWSubimage image, int x, int y) {
        psd = pClonePSD(image.psd);
        composite = AlphaComposite.SrcOver;
        originX = x;
        originY = y;
    }

    /**
     * Create a new MWGraphics Object based on this one.
     */
    public Graphics create() {
        return new MWGraphics(this);
    }

    /**
     * Translate
     */
    public void     translate(int x, int y) {
        originX += x;
        originY += y;
    }

    public void finalize() {
        dispose();
    }

    public final void dispose() {
        if (!disposed) {
            disposed = true;
            pDisposePSD(psd);
        }
    }

    public void     setFont(Font font) {
        if (font != null && !font.equals(this.font)) {
            this.font = font;
            fontMetrics = MWFontMetrics.getFontMetrics(font);
        }
    }

    public Font     getFont() {
        return font;
    }

    /**
     * Gets the font metrics of the current font.
     * @return    the font metrics of this graphics
     *                    context's current font.
     * @see       java.awt.Graphics#getFont
     * @see       java.awt.FontMetrics
     * @see       java.awt.Graphics#getFontMetrics(Font)
     * @since     JDK1.0
     */
    public FontMetrics getFontMetrics() {
        return fontMetrics;
    }

    /**
     * Gets font metrics for the given font.
     */
    public FontMetrics getFontMetrics(Font font) {
        return MWFontMetrics.getFontMetrics(font);
    }

    /**
     * Sets the foreground color.
     */
    public void     setColor(Color c) {
        if ((c != null) && (c != foreground)) {
            foreground = c;
            pSetColor(psd, foreground.value);
        }
    }

    public Color    getColor() {
        return foreground;
    }

    public Composite getComposite() {
        return composite;
    }

    public GraphicsConfiguration getDeviceConfiguration() {
        return graphicsConfiguration;
    }

    public void setComposite(Composite comp) {
        if ((comp != null) && (comp != composite)) {
            if (!(comp instanceof AlphaComposite))
                throw new IllegalArgumentException("Only AlphaComposite is supported");
            composite = (AlphaComposite) comp;
            pSetComposite(psd, composite.rule, composite.extraAlpha);
        }
    }

    /**
     * Sets the paint mode to overwrite the destination with the
     * current color. This is the default paint mode.
     */
    public void     setPaintMode() {
        xorColor = null;
        pSetPaintMode(psd);
    }

    /**
     * Sets the paint mode to alternate between the current color
     * and the given color.
     */
    public void     setXORMode(Color color) {
        xorColor = color;
        pSetXORMode(psd, color.value);
    }

    /** Clears the rectangle indicated by x,y,w,h. */

    public void     clearRect(int x, int y, int w, int h) {
        pClearRect(psd, x + originX, y + originY, w, h, background.value);
    }

    /** Fills the given rectangle with the foreground color. */

    public void     fillRect(int x, int y, int w, int h) {
        pFillRect(psd, x + originX, y + originY, w, h);
    }

    /** Draws the given rectangle. */
    public void     drawRect(int x, int y, int w, int h) {
        pDrawRect(psd, x + originX, y + originY, w, h);
    }

    /** Draws the given line. */
    public void     drawLine(int x1, int y1, int x2, int y2) {
        pDrawLine(psd, x1 + originX, y1 + originY, x2 + originX, y2 + originY);
    }

    /**
     * Copies an area of the canvas that this graphics context paints to.
     * @param X the x-coordinate of the source.
     * @param Y the y-coordinate of the source.
     * @param W the width.
     * @param H the height.
     * @param dx the horizontal distance to copy the pixels.
     * @param dy the vertical distance to copy the pixels.
     */
    public void     copyArea(int X, int Y, int W, int H, int dx, int dy) {
        X += originX;
        Y += originY;
        pCopyArea(psd, X, Y, W, H, dx, dy);
    }

    /** Draws lines defined by an array of x points and y points */

    public void     drawPolyline(int xPoints[], int yPoints[], int nPoints) {
        pDrawPolygon(psd, originX, originY, xPoints, yPoints, nPoints, false);
    }

    /** Draws a polygon defined by an array of x points and y points */

    public void     drawPolygon(int xPoints[], int yPoints[], int nPoints) {
        pDrawPolygon(psd, originX, originY, xPoints, yPoints, nPoints, true);
    }

    /** Fills a polygon with the current fill mask */

    public void     fillPolygon(int xPoints[], int yPoints[], int nPoints) {
        pFillPolygon(psd, originX, originY, xPoints, yPoints, nPoints);
    }

    /** Draws an oval to fit in the given rectangle */
    public void     drawOval(int x, int y, int w, int h) {
        pDrawOval(psd, x + originX, y + originY, w, h);
    }

    /** Fills an oval to fit in the given rectangle */
    public void     fillOval(int x, int y, int w, int h) {
        pFillOval(psd, x + originX, y + originY, w, h);
    }

    /**
     * Draws an arc bounded by the given rectangle from startAngle to
     * endAngle. 0 degrees is a vertical line straight up from the
     * center of the rectangle. Positive start angle indicate clockwise
     * rotations, negative angle are counter-clockwise.
     */
    public void     drawArc(int x, int y, int w, int h, int startAngle, int endAngle) {
        pDrawArc(psd, x + originX, y + originY, w, h, startAngle, endAngle);
    }

    /** fills an arc. arguments are the same as drawArc. */
    public void     fillArc(int x, int y, int w, int h, int startAngle, int endAngle) {
        pFillArc(psd, x + originX, y + originY, w, h, startAngle, endAngle);
    }

    /** Draws a rounded rectangle. */
    public void     drawRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight) {
        pDrawRoundRect(psd, x + originX, y + originY, w, h, arcWidth, arcHeight);
    }

    /** Draws a filled rounded rectangle. */
    public void     fillRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight) {
        pFillRoundRect(psd, x + originX, y + originY, w, h, arcWidth, arcHeight);
    }

    /** Draws the given string. */
    public void     drawString(String string, int x, int y) {
        if (string == null)
            return;
        pDrawString(psd, fontMetrics.nativeFont, string, x + originX, y + originY);
    }

    /** Draws the given character array. */
    public void     drawChars(char chars[], int offset, int length, int x, int y) {
        pDrawChars(psd, fontMetrics.nativeFont, chars, offset, length, x + originX, y + originY);
    }

    /**
     * Gets the current clipping area
     */
    public Rectangle getClipBounds() {
        if (clip != null)
            return new Rectangle (clip.x - originX, clip.y - originY, clip.width, clip.height);
        return null;
    }

    /** Returns a Shape object representing the MicroWindows clip. */

    public Shape    getClip() {
        return getClipBounds();
    }

    public void constrain(int x, int y, int w, int h) {
        originX += x;
        originY += y;
        Rectangle rect = new Rectangle(originX, originY, w, h);
        if (constrainedRect != null)
            constrainedRect = constrainedRect.intersection(rect);
        else constrainedRect = rect;
        setupClip();
    }

    /** Crops the clipping rectangle for this MicroWindows context. */

    public void     clipRect(int x, int y, int w, int h) {
        Rectangle rect = new Rectangle(x + originX, y + originY, w, h);
        if (clip != null)
            clip = clip.intersection(rect);
        else
            clip = rect;
        setupClip();
    }

    /** Sets up the clip for this Graphics. The clip is the result of combining the user clip
     with the constrainedRect. */

    private void setupClip() {
        if (constrainedRect != null) {
            if (clip != null) {
                Rectangle intersection = constrainedRect.intersection(clip);
                pChangeClipRect(psd, intersection.x, intersection.y, intersection.width, intersection.height);
            } else {
                pChangeClipRect(psd, constrainedRect.x, constrainedRect.y, constrainedRect.width, constrainedRect.height);
            }
        } else {
            if (clip != null) {
                pChangeClipRect(psd, clip.x, clip.y, clip.width, clip.height);
            } else {
                pRemoveClip(psd);
            }
        }
    }

    /** Sets the clipping rectangle for this X11Graphics context. */
    public void     setClip(int x, int y, int w, int h) {
        clip = new Rectangle (x + originX, y + originY, w, h);
        setupClip();
    }

    /** Sets the clip to a Shape (only Rectangle allowed). */

    public void     setClip(Shape clip) {
        if (clip == null) {
            this.clip = null;
            setupClip();
        } else if (clip instanceof Rectangle) {
            Rectangle       rect = (Rectangle) clip;
            setClip(rect.x, rect.y, rect.width, rect.height);
        } else
            throw new IllegalArgumentException("setClip(Shape) only supports Rectangle objects");
    }

    /**
     * Draws an image at x,y in nonblocking mode with a callback object.
     */
    public boolean  drawImage(Image img, int x, int y, ImageObserver observer) {
        return drawImage(img, x, y, null, observer);
    }

    /**
     * Draws an image at x,y in nonblocking mode with a solid background
     * color and a callback object.
     */
    public boolean  drawImage(Image img, int x, int y, Color bg,
        ImageObserver observer) {
        MWImage mwimg;
        if (img == null)
            throw new NullPointerException("Image can't be null");
        if (img instanceof BufferedImage)
            mwimg = getBufferedImagePeer((BufferedImage) img);
        else
            mwimg = (MWImage) img;
        boolean isComplete = mwimg.isComplete();
        if (!isComplete) {
            mwimg.addObserver(observer);                
            mwimg.startProduction();
            isComplete = mwimg.isComplete();                    
        }
        int width = mwimg.getWidth();
        int height = mwimg.getHeight();
        if (width > 0 && height > 0) {		
            mwimg.drawImage(psd, x + originX, y + originY, bg);
        }
        return isComplete;
    }

    /**
     * Draws an image scaled to x,y,w,h in nonblocking mode with a
     * callback object.
     */
    public boolean  drawImage(Image img, int x, int y, int width, int height,
        ImageObserver observer) {
        return drawImage(img, x, y, width, height, null, observer);
    }

    /**
     * Draws an image scaled to x,y,w,h in nonblocking mode with a
     * solid background color and a callback object.
     */
    public boolean  drawImage(Image img, int x, int y, int width, int height,
        Color bg, ImageObserver observer) {
        MWImage mwimg;
        if (img == null)
            throw new NullPointerException("Image can't be null");
        if (img instanceof BufferedImage)
            mwimg = getBufferedImagePeer((BufferedImage) img);
        else
            mwimg = (MWImage) img;
        boolean isComplete = mwimg.isComplete();
        if (!isComplete) {
            mwimg.addObserver(observer);                
            mwimg.startProduction();
            isComplete = mwimg.isComplete();                    
        }
        int imgWidth = mwimg.getWidth();
        int imgHeight = mwimg.getHeight();
        if (imgWidth > 0 && imgHeight > 0) {		
            x += originX;
            y += originY;
            mwimg.drawImage(psd, x, y, x + width - 1, y + height - 1, 0, 0, imgWidth - 1, imgHeight - 1, bg);
        }
        return isComplete;
    }

    /**
     * Draws a subrectangle of an image scaled to a destination rectangle
     * in nonblocking mode with a callback object.
     */
    public boolean  drawImage(Image img,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        ImageObserver observer) {
        return drawImage(img, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, null, observer);
    }

    /**
     * Draws a subrectangle of an image scaled to a destination rectangle in
     * nonblocking mode with a solid background color and a callback object.
     */
    public boolean  drawImage(Image img,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color bg, ImageObserver observer) {
        MWImage mwimg;
        if (img == null)
            throw new NullPointerException("Image can't be null");
        if (img instanceof BufferedImage)
            mwimg = getBufferedImagePeer((BufferedImage) img);
        else
            mwimg = (MWImage) img;
        boolean isComplete = mwimg.isComplete();
        if (!isComplete) {
            mwimg.addObserver(observer);                
            mwimg.startProduction();
            isComplete = mwimg.isComplete();                    
        }
        int imgWidth = mwimg.getWidth();
        int imgHeight = mwimg.getHeight();
        if (imgWidth > 0 || imgHeight > 0) {			
            mwimg.drawImage(psd, dx1 + originX, dy1 + originY, dx2 + originX, dy2 + originY, sx1, sy1, sx2, sy2, bg);
        }
        return isComplete;
    }

    public String   toString() {
        return getClass().getName() + "[" + originX + "," + originY + "]";
    }
}
