/*
 * @(#)X11GraphicsConfig.java	1.10 06/10/10
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

import java.awt.image.ColorModel;
import java.awt.image.BufferedImage;

/**
 * This is an implementation of a GraphicsConfiguration object for a
 * single X11 visual.
 *
 * @see GraphicsEnvironment
 * @see GraphicsDevice
 * @version 1.40, 02/02/00
 */
class X11GraphicsConfig extends GraphicsConfiguration {
    X11GraphicsDevice screen;
    int visual;
    ColorModel colorModel;
    private Rectangle bounds;
    private native void init(int visualNum, int screen);

    //    private native ColorModel makeColorModel ();
    
    X11GraphicsConfig(X11GraphicsDevice device, int visualnum) {
        this.screen = device;
        this.visual = visualnum;
        init(visualnum, screen.getScreen());
        bounds = new Rectangle(0, 0, getXResolution(screen.getScreen()), getYResolution(screen.getScreen()));
    }    

    /**
     * Return the graphics device associated with this configuration.
     */
    public GraphicsDevice getDevice() {
        return screen;
    }

    /**
     * Returns a BufferedImage with channel layout and color model
     * compatible with this graphics configuration.  This method
     * has nothing to do with memory-mapping
     * a device.  This BufferedImage has
     * a layout and color model
     * that is closest to this native device configuration and thus
     * can be optimally blitted to this device.
     */
    public BufferedImage createCompatibleImage(int width, int height) {
        return new X11Image(width, height, null);
    }

    private native ColorModel getNativeColorModel(int visual);

    public synchronized ColorModel getColorModel() {
        if (colorModel == null)
            colorModel = getNativeColorModel(visual);
        return colorModel;
    }

    private native int getXResolution(int screen);

    private native int getYResolution(int screen);

    public String toString() {
        return ("X11GraphicsConfig[dev=" + screen + ",vis=0x" + Integer.toHexString(visual) + "]");
    }

    public Rectangle getBounds() {
        return bounds;
    }
}
