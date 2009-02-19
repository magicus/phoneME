/*
 * @(#)QtGraphicsDevice.java	1.8 06/10/10
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

package java.awt;

import sun.awt.AppContext;

/**
 @version 1.1, 11/13/01 */

class QtGraphicsDevice extends GraphicsDevice {
    /** Contains the fullscreen window, or null if not full screen. */
    private Window fullScreenWindow = null;   //6208413
    // tracks which AppContext created the window
    private AppContext fullScreenAppContext = null ; 

	QtGraphicsDevice(QtGraphicsEnvironment environment) {
		this.environment = environment;
		// psd = openScreen();
		configuration = new QtDefaultGraphicsConfiguration(this);
	}

	QtGraphicsDevice(QtGraphicsEnvironment environment, int psd) {
		this.environment = environment;
		this.psd = psd;
		configuration = new QtDefaultGraphicsConfiguration(this);
	}

	public int getType() {
		return TYPE_RASTER_SCREEN;
	}

	public String getIDstring() {
		return "Qt Screen";
	}

	public GraphicsConfiguration getDefaultConfiguration() {
		return configuration;
	}

	public GraphicsConfiguration[]getConfigurations() {
		return new GraphicsConfiguration[]{configuration};
	}

	/** Gets the bounds for this screen. */

	Rectangle getBounds() {
		return new Rectangle(getScreenX(psd), getScreenY(psd), getScreenWidth(psd), getScreenHeight(psd));
	}

	void setWindow(Window window) {
		super.setWindow(window);

		// Notify the graphics environment so it can send events to this graphics device to the
		// associated window.

		environment.setWindow(this, window);
	}

	public boolean isFullScreenSupported()
	{
		return false;   //6229147
	}


	public void setFullScreenWindow(Window w)
	{
        synchronized (this) {
            // Associate fullscreen window with current AppContext
            if (w == null) {
                fullScreenAppContext = null;
            } else {
                fullScreenAppContext = AppContext.getAppContext();
            }
            fullScreenWindow = w;   //6208413
        }
	}

	public Window getFullScreenWindow()
	{
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


	public int getAvailableAcceleratedMemory()
	{
		return -1;
	}



	/* Implementation dependent code...Here is Qt stuff */

	private native int getScreenWidth(int psd);

	private native int getScreenHeight(int psd);

	private native int getScreenX(int psd);

	private native int getScreenY(int psd);

	private QtGraphicsConfiguration configuration;

	private QtGraphicsEnvironment environment;

	int psd;
}



