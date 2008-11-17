/*
 * @(#)MWGraphicsEnvironment.java	1.18 06/10/10
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

import java.util.Locale;
import java.awt.image.BufferedImage;

/**
 * This is an implementation of a GraphicsEnvironment object for the
 * default local GraphicsEnvironment used by the JavaSoft JDK in MW
 * environments.
 *
 * @see GraphicsDevice
 * @see GraphicsConfiguration
 * @version 10 Feb 1997
 */

class MWGraphicsEnvironment extends GraphicsEnvironment implements Runnable {
    MWGraphicsDevice defaultScreenDevice;
    private static native void init();

    private native void destroy();
    private Thread eventThread;
    MWGraphicsEnvironment() {
        java.security.AccessController.doPrivileged(
            new sun.security.action.LoadLibraryAction("microwindowsawt"));
        init();
        Runtime.getRuntime().addShutdownHook(
            new Thread() {
                public void run() {
                    eventThread.interrupt();
                    MWGraphicsEnvironment.this.destroy();
                }
            }
        );
        defaultScreenDevice = new MWGraphicsDevice(this);
        eventThread = new Thread(this, "MicroWindows");
        eventThread.start();
    }

    public native void run();

    public synchronized GraphicsDevice[] getScreenDevices() {
        return new GraphicsDevice[] {defaultScreenDevice};
    }

    /**
     * Returns the default screen graphics device.
     */

    public GraphicsDevice getDefaultScreenDevice() {
        return defaultScreenDevice;
    }

    public String[] getAvailableFontFamilyNames() {
        return MWFontMetrics.getFontList();
    }

    public String[] getAvailableFontFamilyNames(Locale l) {
        return MWFontMetrics.getFontList();
    }

    void setWindow(MWGraphicsDevice device, Window window) {
        pSetWindow(device.psd, window);
    }

    private native void pSetWindow(int psd, Window window);

    /**
     * Returns a <code>Graphics2D</code> object for rendering into the
     * specified {@link BufferedImage}.
     * @param img the specified <code>BufferedImage</code>
     * @return a <code>Graphics2D</code> to be used for rendering into
     * the specified <code>BufferedImage</code>.
     */
    public Graphics2D createGraphics(BufferedImage img) {
        return img.createGraphics();
    }
    /*
     protected void finalize() throws Throwable
     {
     destroy();
     super.finalize();
     }
     */
}
