/*
 * @(#)QtRobotHelper.java	1.8 06/10/10
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

import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;

import sun.awt.RobotHelper;

class QtRobotHelper extends RobotHelper {
    QtGraphicsDevice qtDevice;
    Rectangle bounds;
    
    public static native void init();

    public native void doMouseActionNative(int x, int y, 
				     int buttons, boolean pressed);

    public void doMouseAction(int x, int y, 
				     int buttons, boolean pressed) {
       doMouseActionNative(x,y,buttons,pressed);
    }

    private static native void pCreateScreenCapture(int screenPSD, 
						    int imagePSD,
						    int x, int y,
						    int width, int height);

    private static native int pGetScreenPixel(int psd, int x, int y);

    //public native void doKeyAction(int keySym, boolean pressed);
    public native void doKeyActionNative(int keySym, boolean pressed);
    public void doKeyAction(int keySym, boolean pressed) {
        doKeyActionNative(keySym, pressed);
    }
 
    public native void doKeyActionOnWidget(int KeySym, int widgetType,
                                           boolean pressed);
    static {
	init();
    }
    
    public QtRobotHelper(GraphicsDevice graphicsDevice) throws AWTException {        
	if (graphicsDevice.getClass().getName().indexOf("QtGraphicsDevice") == -1) {
	    throw new 
		IllegalArgumentException("screen device is not on QT");
	}
	qtDevice = (QtGraphicsDevice) graphicsDevice;        
    }

    public BufferedImage getScreenImage(Rectangle screenRect) {
    
        /* clip the region based on PBP's frame bounds */
        if (bounds == null) 
            bounds = qtDevice.getDefaultConfiguration().getBounds();
        screenRect = screenRect.intersection(bounds);    

	QtImage image =
	    new QtImage(screenRect.width, 
		        screenRect.height,
		    (QtGraphicsConfiguration)qtDevice.getDefaultConfiguration());

			pCreateScreenCapture(qtDevice.psd, image.psd, screenRect.x,
			     screenRect.y, screenRect.width,
			     screenRect.height);
	/* return subImage, to get a BufferedImage object */	
	return image.getSubimage(0, 0, screenRect.width, screenRect.height);
    }

    public Color getPixelColor(int x, int y) {
		ColorModel cm = qtDevice.getDefaultConfiguration().getColorModel(); 
		int rgb;
		rgb = pGetScreenPixel(qtDevice.psd, x, y);
		Color color = new Color(cm.getRGB(rgb));
		
		return color;
    }
}
