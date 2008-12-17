/*
 * @(#)GdkSubImage.java	1.10 06/10/10
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

import sun.awt.image.BufferedImagePeer;
import java.awt.Color;
import java.awt.Image;
import java.awt.Graphics;
import java.awt.image.ColorModel;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.FilteredImageSource;
import java.awt.image.CropImageFilter;
import java.awt.image.BufferedImage;
import java.awt.image.RasterFormatException;

/** An image which represents an area of another image. It shares the same pixel storage
 as the source. */

class GdkSubImage extends Image implements BufferedImagePeer, GdkDrawableImage {
    /** The source image for which this is a sub image of. */
	
    private GdkImage sourceImage;
    /** The area of the source image this is a sub image of. */
	
    private int x, y, width, height;
    /** The image producer. This is just a CropImageFilter on the origional image. */
	
    private ImageProducer producer;
    GdkSubImage (GdkImage sourceImage, int x, int y, int width, int height) {
        this.sourceImage = sourceImage;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }
	
    public int getWidth(ImageObserver observer) {
        return width;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight(ImageObserver observer) {
        return height;
    }

    public int getHeight() {
        return height;
    }
	
    public synchronized ImageProducer getSource() {
        if (producer == null)
            producer = new FilteredImageSource (sourceImage.getSource(), new CropImageFilter(x, y, width, height));
        return producer;
    }
	
    public Graphics getGraphics() {
        Graphics g = sourceImage.getGraphics();
        g.translate(x, y);
        g.clipRect(0, 0, width, height);
        return g;
    }
	
    public Object getProperty(String name, ImageObserver observer) {
        return sourceImage.getProperty(name, observer);
    }

    public boolean isComplete() {
        return sourceImage.isComplete();
    }
	
    public void flush() {}
	
    // BufferedImagePeer implementation
	
    public int getType() {
        return sourceImage.getType();
    }

    public ColorModel getColorModel() {
        return sourceImage.getColorModel();
    }

    public int getRGB(int x, int y) {
        return sourceImage.getRGB(x + this.x, y + this.y);
    }
	
    public int[] getRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        return sourceImage.getRGB(startX + this.x, startY + this.y, w, h, rgbArray, offset, scansize);
    }
	
    public void setRGB(int x, int y, int rgb) {
        sourceImage.setRGB(x + this.x, y + this.y, rgb);
    }
	
    public void setRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        sourceImage.setRGB(startX + this.x, startY + this.y, w, h, rgbArray, offset, scansize);
    }
	
    public Object getProperty(String name) {
        return sourceImage.getProperty(name);
    }

    public BufferedImage getSubimage(int x, int y, int w, int h) {
        if ((x + w < this.x) || (x + w > this.x + this.width)) {
            throw new RasterFormatException("(x + width) is outside raster");
        }
        if ((y + h < this.y) || (y + h > this.y + this.height)) {
            throw new RasterFormatException("(y + height) is outside raster");
        }
        return GToolkit.createBufferedImage(new GdkSubImage (sourceImage, x + this.x, y + this.y, width, height));
    }

    public String[] getPropertyNames() {
        return sourceImage.getPropertyNames();
    }
	
    public boolean draw(GdkGraphics g, int x, int y, ImageObserver observer) {
        return sourceImage.draw(g, x, y, x + width, y + height, this.x, this.y, this.x + width, this.y + height, observer);
    }
	
    public boolean draw(GdkGraphics g, int x, int y, int width, int height,
        ImageObserver observer) {
        return sourceImage.draw(g, x, y, x + width, y + height, this.x, this.y, this.x + this.width, this.y + this.height, observer);
    }
	
    public boolean draw(GdkGraphics g, int x, int y, Color bg,
        ImageObserver observer) {
        return sourceImage.draw(g, x, y, x + width, y + height, this.x, this.y, this.x + width, this.y + height, bg, observer);
    }
	
    public boolean draw(GdkGraphics g, int x, int y, int width, int height,
        Color bg, ImageObserver observer) {
        return sourceImage.draw(g, x, y, x + width, y + height, this.x, this.y, this.x + this.width, this.y + this.height, bg, observer);
    }
	
    public boolean draw(GdkGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        ImageObserver observer) {
        return sourceImage.draw(g, dx1, dy1, dx2, dy2, sx1 + this.x, sy1 + this.y, sx2 + this.x, sy2 + this.y, observer);
    }
	
    public boolean draw(GdkGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color bgcolor, ImageObserver observer) {
        return sourceImage.draw(g, dx1, dy1, dx2, dy2, sx1 + this.x, sy1 + this.y, sx2 + this.x, sy2 + this.y, bgcolor, observer);
    }
}
