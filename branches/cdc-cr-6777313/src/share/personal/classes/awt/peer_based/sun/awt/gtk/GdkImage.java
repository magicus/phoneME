/*
 * @(#)GdkImage.java	1.17 06/10/10
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

package sun.awt.gtk;

import java.awt.Component;
import java.awt.Graphics;
import java.awt.GraphicsEnvironment;
import java.awt.Color;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.BufferedImage;
import java.awt.image.RasterFormatException;
import sun.awt.image.ImageRepresentation;
import sun.awt.image.BufferedImagePeer;

class GdkImage extends sun.awt.image.Image implements BufferedImagePeer, GdkDrawableImage {
    GdkGraphicsConfiguration gc;
    /* Tunnel the Offscreen image constructor to the super class. */
    protected GdkImage(Component c, int w, int h) {
        super(c, w, h);
        gc = (GdkGraphicsConfiguration) c.getGraphicsConfiguration();
        if (gc == null) {
            gc = (GdkGraphicsConfiguration) GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration();
        }
    }

    /**
     * Construct an image from an ImageProducer object.
     */
    public GdkImage(ImageProducer producer) {
        super(producer);
        gc = (GdkGraphicsConfiguration) GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration();
    }

    public Graphics getGraphics() {
        Graphics g = new GdkGraphics (this);
        initGraphics(g);
        return g;
    }

    public boolean isComplete() {
        return ((getImageRep().check(null) & ImageObserver.ALLBITS) != 0);
    }

    protected ImageRepresentation makeImageRep() {
        return new GdkImageRepresentation(this);
    }

    protected ImageRepresentation getImageRep() {
        return super.getImageRep();
    }

    // BufferedImagePeer implementation

    public int getType() {
        return BufferedImage.TYPE_CUSTOM;
    }

    public ColorModel getColorModel() {
        return gc.getColorModel();
    }

    public Object getProperty(String name) {
        return super.getProperty(name, null);
    }

    public BufferedImage getSubimage(int x, int y, int w, int h) {
        if ((x + w < x) || (x + w > getWidth())) {
            throw new RasterFormatException("(x + width) is outside raster");
        }
        if ((y + h < y) || (y + h > getHeight())) {
            throw new RasterFormatException("(y + height) is outside raster");
        }
        return GToolkit.createBufferedImage(new GdkSubImage(this, x, y, w, h));
    }

    public String[] getPropertyNames() {
        return null;
    }

    public int getRGB(int x, int y) {
        return ((GdkImageRepresentation) getImageRep()).getRGB(ColorModel.getRGBdefault(),
                x, y);
    }

    public int[] getRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        if (null == rgbArray)
            rgbArray = new int[offset + h * scansize];
        else if (rgbArray.length < offset + h * scansize)
            throw new IllegalArgumentException("rgbArray is not large enough to store all the values");
        ((GdkImageRepresentation) getImageRep()).getRGBs(ColorModel.getRGBdefault(),
            startX, startY, w, h,
            rgbArray, offset, scansize);
        return rgbArray;
    }
    /** The color model used for setRGB calls. We don't use the default RGB color model because this has
     alpha and BufferedImages don't have an alpha channel. This may be changed in the future if we
     add alpha channel rendering to the Graphics object for BufferedImages.
     see bug 4732134 */

    private static final ColorModel COLOR_MODEL_FOR_SET_RGB = new DirectColorModel(24, 0xff0000, 0xff00, 0xff);
    public void setRGB(int x, int y, int rgb) {
        getImageRep().setPixels(x, y, 1, 1, COLOR_MODEL_FOR_SET_RGB, new int[] {rgb}, 0, 0);
    }

    public void setRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        getImageRep().setPixels(startX, startY, w, h, COLOR_MODEL_FOR_SET_RGB, rgbArray, offset, scansize);
    }

    // GdkDrawableImage implementation

    public boolean draw(GdkGraphics g, int x, int y, ImageObserver observer) {
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        return getImageRep().drawImage(g, x, y, null, observer);
    }

    public boolean draw(GdkGraphics g, int x, int y, int width, int height,
        ImageObserver observer) {
        if (width == 0 || height == 0) {
            return true;
        }
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        return getImageRep().drawScaledImage(g, x, y, width, height, null, observer);
    }

    public boolean draw(GdkGraphics g, int x, int y, Color bg,
        ImageObserver observer) {
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        return getImageRep().drawImage(g, x, y, bg, observer);
    }

    public boolean draw(GdkGraphics g, int x, int y, int width, int height,
        Color bg, ImageObserver observer) {
        if (width == 0 || height == 0) {
            return true;
        }
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        return getImageRep().drawScaledImage(g, x, y, width, height, bg, observer);
    }

    public boolean draw(GdkGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        ImageObserver observer) {
        if (dx1 == dx2 || dy1 == dy2) {
            return true;
        }
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        return getImageRep().drawStretchImage(g,
                dx1, dy1, dx2, dy2,
                sx1, sy1, sx2, sy2,
                null, observer);
    }

    public boolean draw(GdkGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color bgcolor, ImageObserver observer) {
        if (dx1 == dx2 || dy1 == dy2) {
            return true;
        }
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        return getImageRep().drawStretchImage(g,
                dx1, dy1, dx2, dy2,
                sx1, sy1, sx2, sy2,
                bgcolor, observer);
    }
}
