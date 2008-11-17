/*
 * @(#)QtRobotHelper.java	1.5 04/08/19
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

package sun.awt.qt;

import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;

import sun.awt.RobotHelper;

public class QtRobotHelper extends RobotHelper {
    GraphicsDevice qtDevice;
    
    public static native void init();

    public native void doMouseActionNative(int x, int y,
                                     int buttons, boolean pressed);

    public void doMouseAction(int x, int y,
                                     int buttons, boolean pressed) {
       doMouseActionNative(x,y,buttons,pressed);
    }

    native int getPixel(int x, int y);

    public native void getPixelsNative(int pixelArray[], int x, int y,
                                 int width, int height);
    public void getPixels(int pixelArray[], int x, int y,
                                 int width, int height) {
       getPixelsNative(pixelArray, x, y, width, height);
    }

    public native void doKeyActionNative(int keySym, boolean pressed);
    public void doKeyAction(int keySym, boolean pressed) {
       doKeyActionNative(keySym, pressed);
    }

    public native void doKeyActionOnWidget(int KeySym, int widgetType,
                                           boolean pressed);
    static {
        //java.security.AccessController.doPrivileged
        //    (new sun.security.action.LoadLibraryAction("qtrobot"));
        //System.out.println("Successfully loaded qtrobot lib");
	init();
    }
    
    public QtRobotHelper(GraphicsDevice graphicsDevice) throws AWTException {        
//	if (!(graphicsDevice instanceof QtGraphicsDevice)) {
//	    throw new 
//		IllegalArgumentException("screen device is not on QT");
//	}
	qtDevice = graphicsDevice;        
    }

    public BufferedImage getScreenImage(Rectangle screenRect) {
        
	GraphicsConfiguration graphicsConfiguration = 
	    qtDevice.getDefaultConfiguration();
  
	BufferedImage image = 
	    graphicsConfiguration.createCompatibleImage(screenRect.width,
							screenRect.height);
	int rgbArray[] = new int[screenRect.width * screenRect.height];
	/* get the pixels */

	getPixels(rgbArray, screenRect.x, screenRect.y,
		  screenRect.width, screenRect.height);
	/* and copy them to the image */
        
	image.setRGB(0, 0, screenRect.width, screenRect.height, 
		     rgbArray, 0, screenRect.width);                     
	return image;
    }

    public Color getPixelColor(int x, int y) {
	int pixelValue = getPixel(x,y);        
	ColorModel cm = qtDevice.getDefaultConfiguration().getColorModel(); 
        Color color = new Color(cm.getRGB(pixelValue));        
        //return new Color(cm.getRGB(pixelValue));        
        return color;
    }

}
