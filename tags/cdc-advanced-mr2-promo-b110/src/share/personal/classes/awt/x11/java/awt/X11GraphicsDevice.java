/*
 * @(#)X11GraphicsDevice.java	1.10 06/10/10
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

import java.util.Vector;

/**
 * This is an implementation of a GraphicsDevice object for a single
 * X11 screen.
 *
 * @see GraphicsEnvironment
 * @see GraphicsConfiguration
 * @version 10 Feb 1997
 */
class X11GraphicsDevice extends GraphicsDevice {
    private int screen;
    X11GraphicsConfig[] configs;
    X11GraphicsConfig defaultConfig;
    public X11GraphicsDevice(int screennum) {
        this.screen = screennum;
    }

    /**
     * Returns the X11 screen of the device.
     */
    public int getScreen() {
        return screen;
    }

    /**
     * Returns the X11 Display of this device.
     */
    public native int getDisplay();

    /**
     * Returns the type of the graphics device.
     * @see #TYPE_RASTER_SCREEN
     * @see #TYPE_PRINTER
     * @see #TYPE_IMAGE_BUFFER
     */
    public int getType() {
        return TYPE_RASTER_SCREEN;
    }

    /**
     * Returns the identification string associated with this graphics
     * device.
     */
    public native String getIDstring();

    private native int[] getDepths(int screen);

    private native int[] getVisualIDs(int screen, int depth);

    private native int getDefaultVisual();

    /**
     * Returns all of the graphics
     * configurations associated with this graphics device.
     */
    public GraphicsConfiguration[] getConfigurations() {
        X11GraphicsConfig[] ret = configs;
        if (ret == null) {
            int[] Xdepths = getDepths(screen);
            Vector Xconfigs = new Vector();
            if (defaultConfig != null)
                Xconfigs.add(defaultConfig);
            for (int i = 0; i < Xdepths.length; i++) {
                int[] Xvisuals = getVisualIDs(screen, Xdepths[i]);
                if (Xvisuals != null)
                    for (int j = 0; j < Xvisuals.length; j++)
                        Xconfigs.add(new X11GraphicsConfig(this, Xvisuals[j]));
            }
            ret = configs = new X11GraphicsConfig[Xconfigs.size()];
            System.arraycopy(Xconfigs.toArray(), 0, configs, 0, Xconfigs.size());
        }
        return ret;
    }

    /**
     * Returns the default graphics configuration
     * associated with this graphics device.
     */
    public GraphicsConfiguration getDefaultConfiguration() {
        if (defaultConfig == null) {
            /* FIXME: I should get my defaults here */
            //	   defaultConfig = X11GraphicsConfig(this, /* visual number */);

            getConfigurations();
            int defaultVis = getDefaultVisual();
            for (int i = 0; i < configs.length; i++)
                if (configs[i].visual == defaultVis) {
                    defaultConfig = configs[i];
                    break;
                }
            System.out.println("GCLen=" + configs.length + "  GC=" + configs);
            defaultConfig.getColorModel();
        }
        return defaultConfig;
    }

    public String toString() {
        return ("X11GraphicsDevice[screen=" + screen + "]");
    }
}
