/*
 * @(#)X11GraphicsEnvironment.java	1.11 06/10/10
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

/**
 * This is an implementation of a GraphicsEnvironment object for the
 * default local GraphicsEnvironment used by the JavaSoft JDK in X11
 * environments.
 *
 * @see GraphicsDevice
 * @see GraphicsConfiguration
 * @version 1.6, 08/19/02
 */

class X11GraphicsEnvironment extends GraphicsEnvironment implements Runnable {
    //    Map fontNameMap;
    //    Map xlfdMap;

    GraphicsDevice[] screens;
    static {
        java.security.AccessController.doPrivileged(
            new sun.security.action.LoadLibraryAction("x11awt"));
        initDisplay();
        /* Disable swing focus manager as we are mixing Swing and AWT components. */
        javax.swing.FocusManager.disableSwingFocusManager();
    }
    private static native void initDisplay();

    public X11GraphicsEnvironment() {
        new Thread (this, "X-Event-Thread").start();
    }
	
    public native void run();

    protected native int getNumScreens();

    protected GraphicsDevice makeScreenDevice(int screennum) {
        return new X11GraphicsDevice(screennum);
    }

    protected native int getDefaultScreenNum();

    public synchronized GraphicsDevice[] getScreenDevices() {
        GraphicsDevice[] ret = screens;
        if (ret == null) {
            int num = getNumScreens();
            ret = new GraphicsDevice[num];
            for (int i = 0; i < num; i++)
                ret[i] = makeScreenDevice(i);
            screens = ret;
        }
        return ret;
    }

    /**
     * Returns the default screen graphics device.
     */

    public GraphicsDevice getDefaultScreenDevice() {
        GraphicsDevice[] gd = getScreenDevices();
        System.out.println("NumSD=" + gd.length + "  defDS=" + getDefaultScreenNum());
        return gd[getDefaultScreenNum()];
    }

    public String[] getAvailableFontFamilyNames() {
        return X11FontMetrics.getFontList();
    }
	
    public String[] getAvailableFontFamilyNames(Locale l) {
        return X11FontMetrics.getFontList();
    }
}
