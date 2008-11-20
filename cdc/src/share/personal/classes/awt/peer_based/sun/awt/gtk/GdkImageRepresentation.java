/*
 * @(#)GdkImageRepresentation.java	1.10 06/10/10
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

import sun.awt.image.ImageRepresentation;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.image.ColorModel;

/** Defines the representation used to store images in Gdk. The image is stored as a Gdk pixmap
 (server side image). */

class GdkImageRepresentation extends ImageRepresentation {
    private static native void initIDs();
    static {
        initIDs();
    }
    GdkImageRepresentation (GdkImage image) {
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

    protected native synchronized void imageDraw(Graphics g, int x, int y, Color c);

    protected native synchronized void imageStretch(Graphics g,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color c);

    protected native void disposeImage();
	
    native int getRGB(ColorModel cm, int x, int y);

    native void getRGBs(ColorModel cm, int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize);
}
