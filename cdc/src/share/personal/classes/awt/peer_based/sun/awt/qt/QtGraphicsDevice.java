/*
 * @(#)QtGraphicsDevice.java	1.11 06/10/10
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


import java.awt.GraphicsDevice;
import java.awt.GraphicsConfiguration;
import java.awt.Window;
import java.awt.Rectangle;
import sun.awt.AppContext;

class QtGraphicsDevice extends GraphicsDevice {
    private Window fullScreenWindow=null;
    //6208413
    // tracks which AppContext created the window
    private AppContext fullScreenAppContext = null ;

    private Rectangle windowedModeBounds=null;

    public QtGraphicsDevice() {
    }
  
    /**
     * Returns <code>true</code> if this <code>GraphicsDevice</code>
     * supports full-screen exclusive mode.
     * @return whether full-screen exclusive mode is available for
     * this graphics device
     * @since 1.4
     */
    public boolean isFullScreenSupported() {
        return false;
    }

    public int getAvailableAcceleratedMemory() {
        return 0;
    }

    /**
     * Enter full-screen mode, or return to windowed mode.
     * <p>
     * If <code>isFullScreenSupported</code> returns <code>true</code>, full
     * screen mode is considered to be <i>exclusive</i>, which implies:
     * <ul>
     * <li>Windows cannot overlap the full-screen window.  All other application
     * windows will always appear beneath the full-screen window in the Z-order.
     * <li>Input method windows are disabled.  It is advisable to call
     * <code>Component.enableInputMethods(false)</code> to make a component
     * a non-client of the input method framework.
     * </ul>
     * <p>
     * If <code>isFullScreenSupported</code> returns
     * <code>false</code>, full-screen exclusive mode is simulated by resizing
     * the window to the size of the screen and positioning it at (0,0).
     * <p>
     * When returning to windowed mode from an exclusive full-screen window, any
     * display changes made by calling <code>setDisplayMode</code> are
     * automatically restored to their original state.
     * @param w a window to use as the full-screen window; <code>null</code>
     * if returning to windowed mode.
     * @see #isFullScreenSupported
     * @see #getFullScreenWindow
     * @see #setDisplayMode
     * @see Component#enableInputMethods
     * @since 1.4
     */
    public void setFullScreenWindow(Window w) {
        if (fullScreenWindow != null && windowedModeBounds != null) {
            fullScreenWindow.setBounds(windowedModeBounds);
        }
        // 6208413
        synchronized(this) {
            // Set the full screen window
            fullScreenWindow = w;
            if (fullScreenWindow == null)
                fullScreenAppContext = null;
            else
                fullScreenAppContext = AppContext.getAppContext();
        } 
        // 6208413
        if (fullScreenWindow != null) {
            windowedModeBounds = fullScreenWindow.getBounds();
            fullScreenWindow.setBounds(getDefaultConfiguration().getBounds());
            fullScreenWindow.setVisible(true);
            fullScreenWindow.toFront();
        }
    }

    /**
     * Returns the <code>Window</code> object representing the
     * full-screen window if the device is in full-screen mode.
     * @return the full-screen window, <code>null</code> if the device is
     * not in full-screen mode. 
     * @see #setFullScreenWindow(Window)
     * @since 1.4
     */
    public Window getFullScreenWindow() {
        Window returnWindow = null;
        synchronized (this) {
            // Only return a handle to the current fs window if we are in the
            // same AppContext that set the fs window
            if (fullScreenAppContext == AppContext.getAppContext()) {
                returnWindow = fullScreenWindow;
            }
        }
        return returnWindow;   //6208413
    }

    public int getType() {
        return TYPE_RASTER_SCREEN;
    }
	
    public String getIDstring() {
        return "Qt Screen Device";
    }
	
    public GraphicsConfiguration getDefaultConfiguration() {
        return graphicsConfig;
    }
	
    public GraphicsConfiguration[]getConfigurations() {
        return new GraphicsConfiguration[] {graphicsConfig};
    }
    
    private QtGraphicsConfiguration graphicsConfig = new QtGraphicsConfiguration(this);
}
