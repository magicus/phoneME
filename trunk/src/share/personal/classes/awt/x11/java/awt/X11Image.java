/*
 * @(#)X11Image.java	1.25 06/10/10
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

import java.util.Hashtable;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Vector;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ImageConsumer;
import java.awt.image.ColorModel;
import java.awt.image.IndexColorModel;
import java.awt.image.DirectColorModel;

/** @version 1.16, 12/05/01
 */


class X11Image extends java.awt.image.BufferedImage implements ImageConsumer {
    static final int REDRAW_COUNT = 20;
    int Xpixmap;        /* X Drawable pixmap, so you can use the XDraw funcs with image */
    int Xclipmask;      /* XImage, clientside XBitmap of clipmask, no need for draw func functionality */
    int width, height;
    int status;
    private Hashtable myProperties;
    private HashSet observers = new HashSet();
    private ColorModel colorModel;
    private int scanlineCount;
    private ImageProducer myProducer;
    private Vector XpixmapList;    /* Was going to be used with multiple frame images */
    /* create the offscreen image source */
    private native int pCreatePixmap(int width, int height, Color bg);

    private native int pCreatePixmapImg(int Ximage, int width, int height); /* This will destroy the image */

    private native int pCreateClipmask(int width, int height);  /* needed for transparency */

    private native int pGetSubPixmapImage(int Ximage, int x, int y, int width, int height);

    private native int pGetSubImage(int Ximage, int x, int y, int width, int height);

    static private native void initIDs();
    static {
        Toolkit.getDefaultToolkit();
        initIDs();
    }
    X11Image(int w, int h, Color background) {
        width = w;
        height = h;
        if ((width > 0) && (height > 0)) {
            Xpixmap = pCreatePixmap(w, h, background);
            Xclipmask = pCreateClipmask(w, h);
            /*	status |= ImageObserver.WIDTH|ImageObserver.HEIGHT; */
        }
        colorModel = Toolkit.getDefaultToolkit().getColorModel();
    }

    X11Image(ImageProducer imageProd) {
        width = -1;
        height = -1;
        myProducer = imageProd;
        myProducer.startProduction(this);
    }

    X11Image(int Ximage, int Xclipmask, int w, int h) {
        Xpixmap = pCreatePixmapImg(Ximage, w, h);
        this.Xclipmask = Xclipmask;
        width = w;
        height = h;
        colorModel = Toolkit.getDefaultToolkit().getColorModel();
    }

    public Graphics getGraphics() {
        return new X11Graphics(this);
    }

    public int getWidth() {
        return width;
    }

    public int getWidth(ImageObserver observer) {
        if (width == -1) addObserver(observer);
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int getHeight(ImageObserver observer) {
        if (height == -1) addObserver(observer);
        return height;
    }

    public Object getProperty(String name, ImageObserver observer) {
        if (myProperties == null) {
            addObserver(observer);
            return UndefinedProperty;
        }
        return myProperties.get(name);
    }

    public String[] getPropertyNames() {
        Object[] names = myProperties.keySet().toArray();
        String[] newNames = new String[names.length];
        System.arraycopy(names, 0, newNames, 0, newNames.length);
        return newNames;
    }

    int getStatus(ImageObserver observer) {
        addObserver(observer);
        return status;
    }

    public java.awt.image.BufferedImage getSubImage(int x, int y, int w, int h) {
        int Ximage = pGetSubPixmapImage(Xpixmap, x, y, w, h);
        int newXclipmask = pGetSubImage(Xclipmask, x, y, w, h);
        return new X11Image(Ximage, newXclipmask, w, h);
    }

    boolean prepareImage(int width, int height, ImageObserver observer) {
        addObserver(observer);
        if ((width > 0) && (height > 0)) {
            setDimensions(width, height);
            return true;
        }
        return false;
    }

    void addObserver(ImageObserver observer) {
        if (observer != null)
            observers.add(observer);
    }

    public void flush() {}

    public ImageProducer getSource() {
        return myProducer;
    }

    /*****  --- Consumer Stuff --- *****/


    public void imageComplete(int stat) {
        int obsStat = 0;
        if (stat == STATICIMAGEDONE)
            obsStat = ImageObserver.ALLBITS;
        else if (stat == SINGLEFRAMEDONE)
            obsStat = ImageObserver.FRAMEBITS;
        else if (stat == IMAGEERROR)
            obsStat = ImageObserver.ERROR;
        else if (stat == IMAGEABORTED)
            obsStat = ImageObserver.ABORT;
        status |= obsStat;
        if ((obsStat != 0) && (observers != null)) {
            Iterator obs = observers.iterator();
            synchronized (observers) {
                while (obs.hasNext()) {
                    ImageObserver iobs = (ImageObserver) obs.next();
                    iobs.imageUpdate(this, obsStat, 0, 0, width, height);
                }
            }
        }
        if ((obsStat & (ImageObserver.ALLBITS | ImageObserver.ERROR | ImageObserver.ABORT)) != 0)
            myProducer.removeConsumer(this);
    }

    public void setColorModel(ColorModel cm) {
        colorModel = cm;
    }

    public void setDimensions(int width, int height) {
        if (Xpixmap != 0) Thread.dumpStack();
        this.width = width;
        this.height = height;
        if ((this.width > 0) && (this.height > 0)) {
            Xpixmap = pCreatePixmap(this.width, this.height, null);
            Xclipmask = pCreateClipmask(this.width, this.height);
        }
        /*
         if(this.width>0) status|=ImageObserver.WIDTH;
         if(this.height>0) status|=ImageObserver.HEIGHT;
         */

        Iterator obs = observers.iterator();
        synchronized (observers) {
            while (obs.hasNext()) {
                ImageObserver iobs = (ImageObserver) obs.next();
                iobs.imageUpdate(this, ImageObserver.WIDTH, 0, 0, width, 0);
                iobs.imageUpdate(this, ImageObserver.HEIGHT, 0, 0, 0, height);
            }
        }
    }

    public void setProperties(Hashtable props) {
        myProperties = props;
    }

    public void setHints(int hints) {/*
     System.out.println("ImageHints:");

     if((hints&RANDOMPIXELORDER) != 0)
     System.out.println("Hints: random order");

     if((hints&TOPDOWNLEFTRIGHT) != 0)
     System.out.println("Hints: top down");

     if((hints&COMPLETESCANLINES) != 0)
     System.out.println("Hints: complete scan lines");

     if((hints&SINGLEPASS) != 0)
     System.out.println("Hints: single pass");

     if((hints&SINGLEFRAME) != 0)
     System.out.println("Hints: single frame");
     */}

    private native void pSetPixels(int src, int clipmask, int[] pixels, int x, int y, int w, int h, int scansize);

    private native int[] pGetPixels(int src, int clipmask, int[] pixels, int x, int y, int w, int h, int scansize, int offset);

    private native int[] XIndexColorCvt(ColorModel cm, byte[] pixels, int off);

    private native int[] XIndexColorCvt(ColorModel cm, int[] pixels, int off);

    private native int[] XDirectColorCvt(ColorModel cm, int[] pixels, int off);

    private int[] XDefaultColorCvt(ColorModel cm, byte[] pixels, int off) {
        int[] p = new int[pixels.length - off];
        for (int i = off; i < pixels.length; i++)
            p[i - off] = cm.getRGB(pixels[i] & 0xFF);
        return p;
    }

    private int[] XDefaultColorCvt(ColorModel cm, int[] pixels, int off) {
        int[] p = new int[pixels.length - off];
        for (int i = off; i < pixels.length; i++)
            p[i - off] = cm.getRGB(pixels[i]);
        return p;
    }

    public void setPixels(int x, int y, int w, int h, ColorModel cm, byte[] pixels, int off, int scansize) {
        int[] p;
        if (cm instanceof IndexColorModel)
            p = XIndexColorCvt(cm, pixels, off);
        else p = XDefaultColorCvt(cm, pixels, off);
        if (scansize == 0) scansize = p.length;
        pSetPixels(Xpixmap, Xclipmask, p, x, y, w, h, scansize);
        scanlineCount++;
        if (scanlineCount % REDRAW_COUNT == 0) {
            Iterator obs = observers.iterator();
            synchronized (observers) {
                while (obs.hasNext()) {
                    ImageObserver iobs = (ImageObserver) obs.next();
                    iobs.imageUpdate(this, ImageObserver.SOMEBITS, x, y, w, h);
                }
            }
        }
    }

    public void setPixels(int x, int y, int w, int h, ColorModel cm, int[] pixels, int off, int scansize) {
        int[] p;
        if (cm instanceof DirectColorModel)
            p = XDirectColorCvt(cm, pixels, off);
        else if (cm instanceof IndexColorModel)
            p = XIndexColorCvt(cm, pixels, off);
        else p = XDefaultColorCvt(cm, pixels, off);
        if (scansize == 0) scansize = p.length;
        pSetPixels(Xpixmap, Xclipmask, p, x, y, w, h, scansize);
        scanlineCount++;
        status |= ImageObserver.SOMEBITS;
        if (scanlineCount % REDRAW_COUNT == 0) {
            Iterator obs = observers.iterator();
            synchronized (observers) {
                while (obs.hasNext()) {
                    ImageObserver iobs = (ImageObserver) obs.next();
                    iobs.imageUpdate(this, ImageObserver.SOMEBITS, x, y, w, h);
                }
            }
        }
    }

    public void setRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        System.out.println("setRGB...." + colorModel);
        int[] p = XDirectColorCvt(colorModel, rgbArray, offset);
        if (scansize == 0) scansize = p.length;
        pSetPixels(Xpixmap, Xclipmask, p, startX, startY, w, h, scansize);
    }

    public int[] getRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        return null;
    }
}
