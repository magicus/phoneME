/*
 * @(#)MWGraphicsDevice.java	1.15 06/10/10
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

/**
 @author Nicholas Allen
 @version 1.1, 11/13/01 */

class MWGraphicsDevice extends GraphicsDevice {
    MWGraphicsDevice(MWGraphicsEnvironment environment) {
        this.environment = environment;
        psd = openScreen();
        configuration = new MWDefaultGraphicsConfiguration(this);
    }

    public int getType() {
        return TYPE_RASTER_SCREEN;
    }

    public String getIDstring() {
        return "Micro Windows Screen";
    }

    public GraphicsConfiguration getDefaultConfiguration() {
        return configuration;
    }

    public GraphicsConfiguration[]getConfigurations() {
        return new GraphicsConfiguration[] {configuration};
    }

    /** Gets the bounds for this screen. */

    Rectangle getBounds() {
        return new Rectangle(getScreenWidth(psd), getScreenHeight(psd));
    }

    void setWindow(Window window) {
        super.setWindow(window);
        // Notify the graphics environment so it can send events to this graphics device to the
        // associated window.

        environment.setWindow(this, window);
    }

    /** Opens the micro windows screen for this graphics device. In Micro Windows there can be
     only one which is created using GdOpenScreen().
     @return the PSD for this graphics device. */

    private native int openScreen();

    private native int getScreenWidth(int psd);

    private native int getScreenHeight(int psd);
    private MWGraphicsConfiguration configuration;
    private MWGraphicsEnvironment environment;
    /** The Micro Windows PSD used for drawing operations on this graphics device. */

    int psd;
}
