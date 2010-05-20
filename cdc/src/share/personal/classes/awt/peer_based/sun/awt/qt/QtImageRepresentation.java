/*
 * @(#)QtImageRepresentation.java	1.12 06/10/10
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


import sun.awt.image.ImageRepresentation;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.image.ColorModel;
import java.awt.Dimension;
import java.util.*;


/** Defines the representation used to store images in Qt. The image is stored as a Qt pixmap
 (server side image). */

class QtImageRepresentation extends ImageRepresentation {

    /*
     * Used to cache scaled qt pixmaps.
     */
    protected Map scaledImages = null;

    private static native void initIDs();
	
    static {
        initIDs();
    }
	
    QtImageRepresentation (QtImage image) {
        super(image, 0);
    }
	
    protected native void offscreenInit(Color bg);

    protected native boolean setBytePixels(int x, int y, int w, int h,
        ColorModel model,
        byte pix[], int off, int scansize);

    protected native boolean setIntPixels(int x, int y, int w, int h,
        ColorModel model,
        int pix[], int off, int scansize);

    protected native boolean finish(boolean force);

    /**
     * Sets the native image representation (QImage). The implementation 
     * must make a copy (shallow or deep) of the image passed.
     *
     * @param qimage contains a pointer to QImage.
     */
    protected native void setNativeImage(int qimage) ;

    protected native synchronized void imageDraw(Graphics g, int x, int y, Color c);

    protected native synchronized void imageStretch(Graphics g,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color c);

    protected synchronized void imageScale(Graphics g,
                                           int x, int y, int w, int h, Color c,
                                           boolean done)
    {
        if (scaledImages == null) {
            scaledImages = new Hashtable();
        }
        if (done) {
            Dimension d = new Dimension(w, h);
            Integer i = (Integer) scaledImages.get(d);
            int pm;
            if (i != null) {
                pm = i.intValue();
            } else {
                pm = createScaledQPixmap(w, h);
                scaledImages.put(d, new Integer(pm));
            }
            drawQPixmap(g, pm, x, y, c);
        }
    }

    protected native int createScaledQPixmap(int w, int h);

    protected native void drawQPixmap(Graphics g, int pm, int x, int y, Color c);

    protected native void disposeImageNative();

    /* Parameter pm is a pointer to the cached QPixmap entry. */
    protected native void disposePixmapEntry(int pm);

    protected void disposeImage() {
        if (scaledImages != null) {
            int pm;
            Integer i;
            for (Iterator it = scaledImages.entrySet().iterator();
                 it.hasNext(); )
            {
                i = (Integer) ((Map.Entry)it.next()).getValue();
                disposePixmapEntry(i.intValue());
            }
            scaledImages.clear();
        }
        disposeImageNative();
    }

    native int getRGB(ColorModel cm, int x, int y);

    native void getRGBs(ColorModel cm, int startX, int startY, int w, int h,
			int[] rgbArray, int offset, int scansize);

    /* to determine the type for the BufferedImage */
    native int getType();

    /* NOTE: x and y have to be values that are adjusted with the
       Graphics origin, i.e. origX + g.originX and origY + g.origin Y.
       This is so that we can avoid getting g.originX and g.originY
       through JNI.
       Returns true when it succeeds to draw, false otherwise.
    */
    native synchronized boolean imageDrawDirect(int gpointer, int x, int y,
					     Color c);

    boolean finishCalled = false;
    boolean drawSucceeded = false;
}
