/*
 * @(#)X11Graphics.java	1.27 06/10/10
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
package java.awt;

import java.io.*;
import java.util.*;
import java.awt.image.ImageObserver;

/**
 * X11Graphics is an object that encapsulates a graphics context for a
 * particular canvas.
 *
 * @version 1.22, 08/19/02
 */

class X11Graphics extends Graphics {
    private static final Font DEFAULT_FONT = new Font ("Dialog", Font.PLAIN, 12);
    private boolean disposed = false;
    Color	   foreground;
    Color 	   background;
    Font  	   font;
    int		   originX;	// TODO: change to float
    int		   originY;	// TODO: change to float
    int            gc;
    private int            drawable;
    private Rectangle      clip;
    private X11FontMetrics fontmetrics;
    //    private native void imageCreate(ImageRepresentation ir);


    private native int createGC(int drawable);

    private native int copyGC(int drawable, int gc);

    private native void destroyGC(int gc);

    //    private native void pSetFont(int gc, byte[] f);
    private native void pSetForeground(int gc, Color c);

    private native void pSetPaintMode(int gc, Color c);

    private native void pSetXORMode(int gc, Color cxor);

    private native void pDrawBytes(int drawable, int gc, int xfontset, byte[] data, int dataLen, int x, int y);

    private native void pClearRect(int drawable, int x, int y, int w, int h);

    private native void pFillRect(int drawable, int gc, int x, int y, int w, int h);

    private native void pDrawRect(int drawable, int gc, int x, int y, int w, int h);

    private native void pCopyArea(int drawable, int gc, int x, int y, int w, int h, int dx, int dy);

    private native void pDrawLine(int drawable, int gc, int x1, int y1, int x2, int y2);

    private native void pDrawPolygon(int drawable, int gc, int xPoints[], int yPoints[], int nPoints);

    private native void pFillPolygon(int drawable, int gc, int XPoints[], int yPoints[], int nPoints);

    private native void pDrawArc(int drawable, int gc, int x, int y, int w, int h, int start, int end);

    private native void pFillArc(int drawable, int gc, int x, int y, int w, int h, int start, int end);

    private native void pDrawRoundRect(int drawable, int gc, int x, int y, int w, int h, int arcWidth, int arcHeight);

    private native void pFillRoundRect(int drawable, int gc, int x, int y, int w, int h, int arcWidth, int arcHeight);

    private native void pDrawImage(int drawableSrc, int drawableDest, int clipmask, int x, int y, int w, int h, int sx, int sy);

    private native void pDrawImageBG(int drawableSrc, int drawableDest, int clipmask, int x, int y, int w, int h, int sx, int sy, Color c);

    private native void pDrawImageScale(int src, int drawableDest, int clipmask, int gc, int x, int y, int w, int h, int newW, int newH, int sx, int sy, int clipW, int clipH);

    private native void pDrawImageScaleBG(int src, int drawableDest, int clipmask, int gc, int x, int y, int w, int h, int newW, int newH, int sx, int sy, int clipW, int clipH, Color c);

    private native void pDrawImageSub(int drawableSrc, int drawableDest, int clipmask, int gc, int x, int y, int w, int h, int newX, int newY, int newW, int newH, int originx, int originy);

    private native void pDrawImageSubBG(int drawableSrc, int drawableDest, int clipmask, int gc, int x, int y, int w, int h, int newX, int newY, int newW, int newH, int originx, int originy, Color c);

    private native void pChangeClipRect(int gc, int x, int y, int w, int h);

    private native void pRemoveClip(int gc);

    X11Graphics(HeavyweightComponentXWindow xwindow) {
        drawable = xwindow.windowID;
        gc = createGC(drawable);
        foreground = xwindow.component.foreground;
        background = xwindow.component.background;
        if (xwindow instanceof WindowXWindow) {
            originX = -((WindowXWindow) xwindow).leftBorder;
            originY = -((WindowXWindow) xwindow).topBorder;
        }
        setFont(DEFAULT_FONT);
    }

    X11Graphics(X11Graphics g) {
        drawable = g.drawable;
        gc = copyGC(drawable, g.gc);
        foreground = g.foreground;
        background = g.background;
        font = g.font;
        fontmetrics = g.fontmetrics;
        originX = g.originX;
        originY = g.originY;
        clip = g.clip;
    }

    X11Graphics(X11Image img) {
        drawable = img.Xpixmap;
        gc = createGC(drawable);
        setFont(DEFAULT_FONT);
    }

    /**
     * Create a new X11Graphics Object based on this one.
     */
    public Graphics create() {
        return new X11Graphics(this);
    }

    /**
     * Translate
     */
    public void translate(int x, int y) {
        originX += x;
        originY += y;
    }

    /*
     * Subclasses should override disposeImpl() instead of dispose(). Client
     * code should always invoke dispose(), never disposeImpl().
     *
     * An X11Graphics cannot be used after being disposed.
     */
    protected native void disposeImpl();

    public final void dispose() {
        if (!disposed) {
            disposed = true;
            disposeImpl();
            destroyGC(gc);
        }
    }

    public void setFont(Font font) {
        if (font == null)
            font = DEFAULT_FONT;
        this.font = font;
        fontmetrics = font.getX11FontMetrics();
    }

    public Font getFont() {
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
        return fontmetrics;
    }

    /**
     * Gets font metrics for the given font.
     */
    public FontMetrics getFontMetrics(Font font) {
        return font.getX11FontMetrics();
    }

    /**
     * Sets the foreground color.
     */
    public void setColor(Color c) {
        if ((c != null) && (c != foreground)) {
            pSetForeground(gc, c);
            foreground = c;
        }
    }

    public Color getColor() {
        return foreground;
    }

    /**
     * Sets the paint mode to overwrite the destination with the
     * current color. This is the default paint mode.
     */
    public void setPaintMode() {
        pSetPaintMode(gc, foreground);
    }

    /**
     * Sets the paint mode to alternate between the current color
     * and the given color.
     */
    public void setXORMode(Color c1) {
        pSetXORMode(gc, c1);
    }

    /** Clears the rectangle indicated by x,y,w,h. */
    public void clearRect(int x, int y, int w, int h) {
        if (clip != null) {
            Rectangle dim = new Rectangle(x + originX, y + originY, w, h);
            if ((dim = clip.intersection(dim)) != null)
                pClearRect(drawable, dim.x, dim.y, dim.width, dim.height);
        } else


            pClearRect(drawable, x + originX, y + originY, w, h);
    }

    /** Fills the given rectangle with the foreground color. */
    public void fillRect(int X, int Y, int W, int H) {
        pFillRect(drawable, gc, X + originX, Y + originY, W, H);
    }

    /** Draws the given rectangle. */
    public void drawRect(int X, int Y, int W, int H) {
        pDrawRect(drawable, gc, X + originX, Y + originY, W, H);
    }

    /** Draws the given line. */
    public void drawLine(int x1, int y1, int x2, int y2) {
        pDrawLine(drawable, gc, x1 + originX, y1 + originY, x2 + originX, y2 + originY);
    }

    /**
     * Copies an area of the canvas that this graphics context paints to.
     * @param X the x-coordinate of the source.
     * @param Y the y-coordinate of the source.
     * @param W the width.
     * @param H the height.
     * @param dx the x-coordinate of the destination.
     * @param dy the y-coordinate of the destination.
     */
    public void copyArea(int X, int Y, int W, int H, int dx, int dy) {
        if (drawable != 0)
            pCopyArea(drawable, gc, X + originX, Y + originY, W, H, X + dx + originX, Y + dy + originY);
    }

    /** Draws a bunch of lines defined by an array of x points and y points */
    public void drawPolyline(int xPoints[], int yPoints[], int nPoints) {
        int i;
        for (i = 0; i < nPoints; i++) {
            xPoints[i] += originX;
            yPoints[i] += originY;
        }
        pDrawPolygon(drawable, gc, xPoints, yPoints, nPoints);
    }

    /** Draws a polygon defined by an array of x points and y points */
    public void drawPolygon(int xPoints[], int yPoints[], int nPoints) {
        int nxPoints[] = new int[nPoints + 1];
        int nyPoints[] = new int[nPoints + 1];
        int i;
        for (i = 0; i < nPoints; i++) {
            nxPoints[i] += originX;
            nyPoints[i] += originY;
        }
        nxPoints[i] = nxPoints[0] + originX;
        nyPoints[i] = nyPoints[0] + originY;
        pDrawPolygon(drawable, gc, xPoints, yPoints, nPoints + 1);
    }

    /** Fills a polygon with the current fill mask */
    public void fillPolygon(int xPoints[], int yPoints[], int nPoints) {
        int i;
        for (i = 0; i < nPoints; i++) {
            xPoints[i] += originX;
            yPoints[i] += originY;
        }
        pFillPolygon(drawable, gc, xPoints, yPoints, nPoints);
    }

    /** Draws an oval to fit in the given rectangle */
    public void drawOval(int x, int y, int w, int h) {
        pDrawArc(drawable, gc, x + originX, y + originY, w, h, 0, 360 * 64);
    }

    /** Fills an oval to fit in the given rectangle */
    public void fillOval(int x, int y, int w, int h) {
        pFillArc(drawable, gc, x + originX, y + originY, w, h, 0, 360 * 64);
    }

    /**
     * Draws an arc bounded by the given rectangle from startAngle to
     * endAngle. 0 degrees is a vertical line straight up from the
     * center of the rectangle. Positive angles indicate clockwise
     * rotations, negative angle are counter-clockwise.
     */
    public void drawArc(int x, int y, int w, int h, int startAngle, int endAngle) {
        pDrawArc(drawable, gc, x + originX, y + originY, w, h, startAngle * 64, endAngle * 64);
    }
	
    /** fills an arc. arguments are the same as drawArc. */
    public void fillArc(int x, int y, int w, int h, int startAngle, int endAngle) {
        pFillArc(drawable, gc, x + originX, y + originY, w, h, startAngle * 64, endAngle * 64);
    }

    /** Draws a rounded rectangle. */
    public void drawRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight) {
        pDrawRoundRect(drawable, gc, x + originX, y + originY, w, h, arcWidth, arcHeight);
    }

    /** Draws a filled rounded rectangle. */
    public void fillRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight) {
        pFillRoundRect(drawable, gc, x + originX, y + originY, w, h, arcWidth, arcHeight);
    }

    /** Draws the given string. */
    public void drawString(String string, int x, int y) {
        if (string == null) return;
        byte[] newstr = string.getBytes();
        pDrawBytes(drawable, gc, fontmetrics.XFontSet, newstr, newstr.length, x + originX, y + originY);
    }

    /** Draws the given character array. */
    public void drawChars(char chars[], int offset, int length, int x, int y) {
        String tmpstr = new String(chars);
        String substr = tmpstr.substring(offset, offset + length);
        byte[] newstr = substr.getBytes();
        pDrawBytes(drawable, gc, fontmetrics.XFontSet, newstr, newstr.length, x + originX, y + originY);
    }

    /**
     * Gets the current clipping area
     */
    public Rectangle getClipBounds() {
        if (clip != null)
            return new Rectangle (clip.x - originX, clip.y - originY, clip.width, clip.height);
        return null;
    }

    /** Returns a Shape object representing the X11Graphics clip. */
    public Shape getClip() {
        return getClipBounds();
    }

    /** Crops the clipping rectangle for this X11Graphics context. */
    public void clipRect(int x, int y, int w, int h) {
        Rectangle rect = new Rectangle(x + originX, y + originY, w, h);
        if (clip != null)
            rect = clip.intersection(rect);
        clip = rect;
        pChangeClipRect(gc, clip.x, clip.y, clip.width, clip.height);
    }

    /** Sets the clipping rectangle for this X11Graphics context. */
    public void setClip(int x, int y, int w, int h) {
        clip = new Rectangle (x + originX, y + originY, w, h);
        pChangeClipRect(gc, clip.x, clip.y, w, h);
    }

    /** Sets the X11Graphics clip to a Shape (only Rectangle allowed). */
    public void setClip(Shape clip) {
        if (clip == null) {
            pRemoveClip(gc);
        } else if (clip instanceof Rectangle) {
            Rectangle rect = (Rectangle) clip;
            setClip(rect.x, rect.y, rect.width, rect.height);
        } else {
            throw new IllegalArgumentException("setClip(Shape) only supports Rectangle objects");
        }
    }

    /**
     * Draws an image at x,y in nonblocking mode with a callback object.
     */
    public boolean drawImage(Image img, int x, int y, ImageObserver observer) {
        X11Image ximg = (X11Image) img;
        if ((drawable != 0) && (ximg.Xpixmap != 0)) {
            x += originX;
            y += originY;
            Rectangle dim = new Rectangle(x, y, ximg.width, ximg.height);
            int sx = 0, sy = 0;
            if (clip != null) {
                dim = clip.intersection(dim);
                int diff;
                diff = clip.x - x;
                if (diff > 0) sx = diff;
                diff = clip.y - y;
                if (diff > 0) sy = diff;
            }
            if ((dim.width > 0) && (dim.height > 0))
                pDrawImage(ximg.Xpixmap, drawable, ximg.Xclipmask, dim.x, dim.y, dim.width, dim.height, sx, sy);
        }
        ximg.addObserver(observer);
        return true;
    }

    /**
     * Draws an image at x,y in nonblocking mode with a solid background
     * color and a callback object.
     */
    public boolean drawImage(Image img, int x, int y, Color bg,
        ImageObserver observer) {
        X11Image ximg = (X11Image) img;
        if ((drawable != 0) && (ximg.Xpixmap != 0)) {
            x += originX;
            y += originY;
            Rectangle dim = new Rectangle(x, y, ximg.width, ximg.height);
            int sx = 0, sy = 0;
            if (clip != null) {
                dim = clip.intersection(dim);
                int diff;
                diff = clip.x - x;
                if (diff > 0) sx = diff;
                diff = clip.y - y;
                if (diff > 0) sy = diff;
            }
            if ((dim.width > 0) && (dim.height > 0))

                pDrawImageBG(ximg.Xpixmap, drawable, ximg.Xclipmask, dim.x, dim.y, dim.width, dim.height, sx, sy, bg);
        }
        ximg.addObserver(observer);
        return true;
    }

    /**
     * Draws an image scaled to x,y,w,h in nonblocking mode with a
     * callback object.
     */
    public boolean drawImage(Image img, int x, int y, int width, int height,
        ImageObserver observer) {
        X11Image ximg = (X11Image) img;
        if ((drawable != 0) && (ximg.Xpixmap != 0)) {
            x += originX;
            y += originY;
            Rectangle dim = new Rectangle(x, y, width, height);
            int sx = 0, sy = 0;
            if (clip != null) {
                dim = clip.intersection(dim);
                int diff;
                diff = clip.x - x;
                if (diff > 0) sx = (diff * ximg.width) / width;
                diff = clip.y - y;
                if (diff > 0) sy = (diff * ximg.height) / height;
            }
            if ((dim.width > 0) && (dim.height > 0))
                pDrawImageScale(ximg.Xpixmap, drawable, ximg.Xclipmask, gc, dim.x, dim.y, ximg.width, ximg.height, width, height, sx, sy, dim.width, dim.height);
        }
        ximg.addObserver(observer);
        return true;
    }

    /**
     * Draws an image scaled to x,y,w,h in nonblocking mode with a
     * solid background color and a callback object.
     */
    public boolean drawImage(Image img, int x, int y, int width, int height,
        Color bg, ImageObserver observer) {
        X11Image ximg = (X11Image) img;
        if ((drawable != 0) && (ximg.Xpixmap != 0)) {
            x += originX;
            y += originY;
            Rectangle dim = new Rectangle(x, y, width, height);
            int sx = 0, sy = 0;
            if (clip != null) {
                dim = clip.intersection(dim);
                int diff;
                diff = clip.x - x;
                if (diff > 0) sx = (diff * ximg.width) / width;
                diff = clip.y - y;
                if (diff > 0) sy = (diff * ximg.height) / height;
            }
            if ((dim.width > 0) && (dim.height > 0))

                pDrawImageScaleBG(ximg.Xpixmap, drawable, ximg.Xclipmask, gc, dim.x, dim.y, ximg.width, ximg.height, width, height, sx, sy, dim.width, dim.height, bg);
        }
        ximg.addObserver(observer);
        return true;
    }

    /**
     * Draws a subrectangle of an image scaled to a destination rectangle
     * in nonblocking mode with a callback object.
     */
    public boolean drawImage(Image img,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        ImageObserver observer) {
        X11Image ximg = (X11Image) img;
        if ((drawable != 0) && (ximg.Xpixmap != 0)) {
            Rectangle srcRect = new Rectangle(Math.min(sx1, sx2), Math.min(sy1, sy2), Math.abs(sx2 - sx1), Math.abs(sy2 - sy1));
            Rectangle newRect = srcRect.intersection(new Rectangle(0, 0, ximg.width, ximg.height));
            int nx1, nx2;
            nx1 = nx2 = newRect.x;
            if (sx1 < sx2)
                nx2 += newRect.width - 1;
            else
                nx1 += newRect.width - 1;
            int ny1, ny2;
            ny1 = ny2 = newRect.y;
            if (sy1 < sy2)
                ny2 += newRect.height - 1;
            else
                ny1 += newRect.height - 1;
            pDrawImageSub(ximg.Xpixmap, drawable, ximg.Xclipmask, gc, nx1, ny1, nx2, ny2, dx1, dy1, dx2, dy2, originX, originY);
        }
        ximg.addObserver(observer);
        return true;
    }

    /**
     * Draws a subrectangle of an image scaled to a destination rectangle in
     * nonblocking mode with a solid background color and a callback object.
     */
    public boolean drawImage(Image img,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color bg, ImageObserver observer) {
        X11Image ximg = (X11Image) img;
        if ((drawable != 0) && (ximg.Xpixmap != 0)) {
            Rectangle srcRect = new Rectangle(Math.min(sx1, sx2), Math.min(sy1, sy2), Math.abs(sx2 - sx1), Math.abs(sy2 - sy1));
            Rectangle newRect = srcRect.intersection(new Rectangle(0, 0, ximg.width, ximg.height));
            int nx1, nx2;
            nx1 = nx2 = newRect.x;
            if (sx1 < sx2)
                nx2 += newRect.width - 1;
            else
                nx1 += newRect.width - 1;
            int ny1, ny2;
            ny1 = ny2 = newRect.y;
            if (sy1 < sy2)
                ny2 += newRect.height - 1;
            else
                ny1 += newRect.height - 1;
            pDrawImageSubBG(ximg.Xpixmap, drawable, ximg.Xclipmask, gc, nx1, ny1, nx2, ny2, dx1, dy1, dx2, dy2, originX, originY, bg);
        }
        ximg.addObserver(observer);
        return true;
    }

    public String toString() {
        return getClass().getName() + "[" + originX + "," + originY + "]";
    }
    /* Outline the given region. */
    //public native void drawRegion(Region r);

    /* Fill the given region. */
    //public native void fillRegion(Region r);
}
