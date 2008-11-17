/*
 * @(#)MWSubimage.java	1.15 06/10/10
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

import java.awt.image.CropImageFilter;
import java.awt.image.FilteredImageSource;
import java.awt.image.ImageObserver;

/** @version 1.7, 03/13/02 */


/** An offscreen image created by BufferedImage.getSubimage(int width, int height). As it is not
 possible to create a graphics from an image produced
 through the image producer/consumer model this class exists to allow us to get the graphics
 for a subimage image. */

class MWSubimage extends MWImage {
    private int originX, originY;
    private int subWidth, subHeight;
    MWSubimage(MWImage img, int x, int y, int width, int height) {
        super(img);
        originX = x;
        originY = y;
        subWidth = width;
        subHeight = height;
        started = true;
        producer = new FilteredImageSource(img.getSource(), new CropImageFilter(x, y, width, height));
    }

    public Graphics getGraphics() {
        return new MWGraphics (this, originX, originY);
    }

    public int getRGB(int x, int y) {
        if (x < 0 || y < 0 || x >= subWidth || y >= subHeight)
            throw new java.lang.ArrayIndexOutOfBoundsException(x + y * subWidth);
        return super.getRGB(x + originX, y + originY);
    }

    public int[] getRGB(int startX, int startY, int w, int h, int[] rgbArray, int offset, int scansize) {
        if (startX < 0 || startY < 0 || startX + w > subWidth || startY + h > subHeight)
            throw new java.lang.ArrayIndexOutOfBoundsException(startX + startY * subWidth);
        return super.getRGB(startX + originX, startY + originY, w, h, rgbArray, offset, scansize);
    }

    public void setRGB(int x, int y, int rgb) {
        if (x < 0 || y < 0 || x >= subWidth || y >= subHeight)
            throw new java.lang.ArrayIndexOutOfBoundsException(x + y * subWidth);
        super.setRGB(x + originX, y + originY, rgb);
    }

    public void setRGB(int startX, int startY, int w, int h, int[] rgbArray, int offset, int scansize) {
        if (startX < 0 || startY < 0 || startX + w > subWidth || startY + h > subHeight)
            throw new java.lang.ArrayIndexOutOfBoundsException(startX + startY * subWidth);
        super.setRGB(startX + originX, startY + originY, w, h, rgbArray, offset, scansize);
    }

    public int getWidth() {
        return subWidth;
    }

    public int getWidth(ImageObserver observer) {
        if (width == -1)
            addObserver(observer);
        return subWidth;
    }

    public int getHeight() {
        return subHeight;
    }

    public int getHeight(ImageObserver observer) {
        if (height == -1)
            addObserver(observer);
        return subHeight;
    }

    void drawImage(int psd, int x, int y, Color bg) {
        super.drawImage(psd, x, y, x + subWidth - 1, y + subHeight - 1, originX, originY, originX + subWidth - 1, originY + subHeight - 1, bg);
    }

    void drawImage(int psd, int dx1, int dy1, int dx2, int dy2, int sx1, int sy1, int sx2, int sy2, Color bg) {
        super.drawImage(psd, dx1, dy1, dx2, dy2, sx1 + originX, sy1 + originY, sx2 + originX, sy2 + originY, bg);
    }
}
