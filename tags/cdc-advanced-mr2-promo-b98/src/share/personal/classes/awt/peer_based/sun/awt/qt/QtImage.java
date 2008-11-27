/*
 * @(#)QtImage.java	1.18 06/10/10
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

package sun.awt.qt;


import java.awt.Component;
import java.awt.Graphics;
import java.awt.GraphicsEnvironment;
import java.awt.GraphicsConfiguration;
import java.awt.Color;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.BufferedImage;
import java.awt.image.RasterFormatException;
import sun.awt.image.ImageRepresentation;
import sun.awt.image.BufferedImagePeer;

class QtImage extends sun.awt.image.Image implements										    BufferedImagePeer, QtDrawableImage 
{
    QtGraphicsConfiguration gc;
    Component c;

    /* Tunnel the Offscreen image constructor to the super class. */
    protected QtImage(Component c, int w, int h) {
        super(c, w, h);
        if (gc == null)
            gc = (QtGraphicsConfiguration)GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration();
        this.c = c;

    }

    public QtImage(Component c, int w, int h, QtGraphicsConfiguration gc) {
        super(c, w, h);
        this.gc = gc;
        this.c = c;
    }
    /**
     * Construct an image from an ImageProducer object.
     */
    public QtImage(ImageProducer producer) {
	super(producer);
        gc = (QtGraphicsConfiguration)
	    GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration();
    }

    public Graphics getGraphics() {
        // 6258458
        // Per J2SE 1.5 spec, we should throw UOE if the method is called
        // on a non-offscreen image
        if ( this.c == null ) {
            throw new UnsupportedOperationException("Not a offscreen image");
        }
        // 6258458

        Graphics g = new QtGraphics (this);
        if (c != null)
            initGraphics(g);
        return g;
    }

    public boolean isComplete() {
        return ((getImageRep().check(null) & ImageObserver.ALLBITS) != 0);
    }    

    protected ImageRepresentation makeImageRep() {
        return new QtImageRepresentation(this);
    }
	
    protected ImageRepresentation getImageRep() {
	return super.getImageRep();
    }

    // BufferedImagePeer implementation
	
    public int getType() {
	ImageRepresentation imgRep = this.getImageRep();
	if (imgRep instanceof sun.awt.qt.QtImageRepresentation) {
	    return ((QtImageRepresentation) imgRep).getType();	    
	} else {
	    return BufferedImage.TYPE_CUSTOM;
	}
    }

    public ColorModel getColorModel() {
        return gc.getColorModel();
    }

    public Object getProperty(String name) {
	return super.getProperty(name, null);
    }

    public BufferedImage getSubimage(int x, int y, int w, int h) {

        if ((x+w < x) || (x+w > getWidth())) {
            throw new RasterFormatException("(x + width) is outside raster");
        }

        if ((y+h < y) || (y+h > getHeight())) {
            throw new RasterFormatException("(y + height) is outside raster");
        }
 
        return QtToolkit.createBufferedImage(new QtSubImage(this, x, y, w, h));
    }

    public String[] getPropertyNames() {
        return null;
    }
	
    public int getRGB(int x, int y) {
        return ((QtImageRepresentation)
		getImageRep()).getRGB(getColorModel(), x, y);
    }

    public int[] getRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) 
    {
	if (null == rgbArray)
	    rgbArray = new int[offset + h * scansize];

	else if (rgbArray.length < offset + h * scansize)
	    throw new IllegalArgumentException("rgbArray is not large enough to store all the values");

        ((QtImageRepresentation) getImageRep()).getRGBs(getColorModel(), 
							startX,
							startY, 
							w, 
							h, 
							rgbArray, 
							offset, 
							scansize);
	return rgbArray;
    }
    
    private static final ColorModel COLOR_MODEL_FOR_SET_RGB = new DirectColorModel(32, 0xff0000, 0xff00, 0xff, 0);
    public void setRGB(int x, int y, int rgb) {
        getImageRep().setPixels(x, y, 1, 1, COLOR_MODEL_FOR_SET_RGB, new int[] {rgb}, 0, 0);
    }
	
    public void setRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        getImageRep().setPixels(startX, startY, w, h,
				COLOR_MODEL_FOR_SET_RGB, rgbArray,
				offset, scansize); 
    }
	
    // QtDrawableImage implementation
	
    public boolean draw(QtGraphics g, int x, int y, ImageObserver observer) {
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }

	QtImageRepresentation imgRep = (QtImageRepresentation)
	    getImageRep();
	
	if (imgRep.finishCalled && imgRep.drawSucceeded && g != null
	    && g.data != 0 && 
	    imgRep.imageDrawDirect(g.data, x + g.originX, y +
				   g.originY, null)) {
	    return true;
	} else {
	    return getImageRep().drawImage(g, x, y, null, observer);
	}
    }
	
    public boolean draw(QtGraphics g, int x, int y, int width, int height,
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
	
    public boolean draw(QtGraphics g, int x, int y, Color bg,
        ImageObserver observer) {
        if (hasError()) {
            if (observer != null) {
                observer.imageUpdate(this,
                    ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }

	QtImageRepresentation imgRep = (QtImageRepresentation)
	    getImageRep();

	if (imgRep.finishCalled && imgRep.drawSucceeded && g != null
	    && g.data != 0 &&
	    imgRep.imageDrawDirect(g.data, x + g.originX, y +
				   g.originY, bg)) {
	    return true;
	} else {
	    return getImageRep().drawImage(g, x, y, bg, observer);
	}
    }
	
    public boolean draw(QtGraphics g, int x, int y, int width, int height,
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
	
    public boolean draw(QtGraphics g, int dx1, int dy1, int dx2, int dy2,
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
	
    public boolean draw(QtGraphics g, int dx1, int dy1, int dx2, int dy2,
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
