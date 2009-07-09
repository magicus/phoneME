/*
 * @(#)PPCGraphicsDevice.java	1.6 06/10/10
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
 */

package sun.awt.pocketpc;

import java.awt.GraphicsDevice;
import java.awt.GraphicsConfiguration;
import java.awt.Window;
import java.awt.Rectangle;
import sun.awt.AppContext;

class PPCGraphicsDevice extends GraphicsDevice {
    private Window fullScreenWindow=null;
    private AppContext fullScreenAppContext=null;
    private Rectangle windowedModeBounds=null;

    public int getType() {
        return TYPE_RASTER_SCREEN;
    }
	
    public String getIDstring() {
        return "PPC Screen Device";
    }

    public Window getFullScreenWindow() {
        Window returnWindow = null;
        synchronized (this) {
            // Only return a handle to the current fs window if we are in the
            // same AppContext that set the fs window
            if (fullScreenAppContext == AppContext.getAppContext()) {
                returnWindow = fullScreenWindow;
            }
        }
        return returnWindow;
    }

    public void setFullScreenWindow(Window w) {
        if (fullScreenWindow != null && windowedModeBounds != null) {
            fullScreenWindow.setBounds(windowedModeBounds);
        }
        synchronized(this) {
            fullScreenWindow = w;
            if (fullScreenWindow == null)
                fullScreenAppContext = null;
            else
                fullScreenAppContext = AppContext.getAppContext();
        }
        if (fullScreenWindow != null) {
            windowedModeBounds = fullScreenWindow.getBounds();
            fullScreenWindow.setBounds(getDefaultConfiguration().getBounds());
            fullScreenWindow.setVisible(true);
            fullScreenWindow.toFront();
        }
    }
	
    public int getAvailableAcceleratedMemory() {
        return 0;
    }
	
    public boolean isFullScreenSupported() {
        return false;
    }

    public GraphicsConfiguration getDefaultConfiguration() {
        return graphicsConfig;
    }
	
    public GraphicsConfiguration[]getConfigurations() {
        return new GraphicsConfiguration[] {graphicsConfig};
    }
    private PPCGraphicsConfiguration graphicsConfig = new PPCGraphicsConfiguration(this);
}
