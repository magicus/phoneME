/*
 * @(#)RobotHelper.java	1.2 02/10/24
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


/* abstract class for platform dependent robot helper */

package sun.awt;

import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;
import sun.awt.qt.QtRobotHelper;

public abstract class RobotHelper {
  
    public abstract BufferedImage getScreenImage(Rectangle screenRect);
    public abstract Color getPixelColor(int x, int y);
    public abstract void doMouseAction(int x, int y, int buttons, boolean pressed);
    public abstract void doKeyAction(int keycode,  boolean pressed);

    static RobotHelper getRobotHelper(GraphicsDevice graphicsDevice) {
	/* Construct an appropriate RobotHelper for this graphics device */
        try {
	   return new QtRobotHelper(graphicsDevice);
        } catch (Exception ex) {
           System.out.println("Error in creating a helper: " + ex);
           return null;
        }
    }
  
    /* utility function to convert keycode + modifier to char */
    char getKeyChar(int keycode, int modifiers) {
	char c = KeyEvent.CHAR_UNDEFINED;
		
	if (keycode >= KeyEvent.VK_A && keycode <= KeyEvent.VK_Z) {
	    if ((modifiers & InputEvent.SHIFT_MASK) != 0) {
		c = (char)('A' + (keycode - KeyEvent.VK_A));
	    } else {
		c = (char)('a' + (keycode - KeyEvent.VK_A));
	    }
	} else if (keycode >= KeyEvent.VK_0 && keycode <= KeyEvent.VK_9) {
	    c = (char)('0' + (keycode - KeyEvent.VK_0));
	}
	return c;
    }
}
