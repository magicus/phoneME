/*
 * @(#)XletFrame.java	1.9 05/03/21
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

package sun.mtask.xlet;

import java.awt.Container;
import java.awt.Frame;
import java.awt.BorderLayout;
import java.awt.Insets;

import sun.awt.SunToolkit;
import java.awt.Toolkit;

import sun.mtask.xlet.PXletManager;

import java.awt.event.WindowEvent;
import java.awt.event.WindowAdapter;

class XletFrame extends Frame
{
    Container theContainer;
    // Dimensions of an xlet frame
    private static int x;
    private static int y;
    private static int w;
    private static int h;
    private static int wOffset;
    private static int hOffset;
    private static boolean initialized = false;

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));
    
    private boolean doDecorations;
    
    XletFrame(String name)
    {
	super(name);
	if (!XletFrame.initialized) {
	    throw new Error("FATAL ERROR: Did not initialize "+
			    "xlet frame dimensions");
	}
	
        String decorations = System.getProperty("cdcams.decorations");
	if (decorations == null) {
	    doDecorations = true;
	} else {
	    doDecorations = decorations.equals("true");
	}
	
        if (doDecorations) {
	    addWindowListener(new WindowAdapter() {
		    public void windowClosing(WindowEvent windowevent) {
			System.exit(0);
		    }
		});
	} else {
	    XletFrame.w += XletFrame.wOffset;
	    XletFrame.h += XletFrame.hOffset;
	    setUndecorated(true);
	}
    }

    XletFrame(String name, String laf, String theme)
    {
	this(name);
    }

    XletFrame(String name, PXletManager manager,
	      String laf, String theme)
    {
	this(name);
    }

    class XletContainer extends Container 
    {
	Frame frame;
	
	XletContainer(Frame xletFrame) 
	{
	    frame = xletFrame;
	}
	
	public void setVisible(boolean visible) 
	{
	    if (visible) {
		frame.setBounds(x, y, w, h);
	    }
	    
	    super.setVisible(visible);
	    frame.setVisible(visible);
	}
    }
    
    Container getContainer()
    {
	if (theContainer != null) {
	    return theContainer;
	}
	Container c = new XletContainer(this);
	c.setLayout(new BorderLayout());
	add(c);
        validate();
	// Keep container invisible, per the xlet spec
	c.setVisible(false);
	// This is the one container
	theContainer = c;
	return theContainer;
    }

    // No look and feel changes in vanilla basis or personal
    void changeLookAndFeel(String lafName)
    {
        if (verbose) {
	    System.err.println("LOOK AND FEEL CHANGE NOT SUPPORTED");
        }
    }
    
    void changeLookAndFeelTheme(String themeName)
    {
        if (verbose) {
	    System.err.println("LOOK AND FEEL CHANGE NOT SUPPORTED");
        }
    }

    //    native void synthesizeFocusIn();

    public void activate()
    {
        SunToolkit tk = (SunToolkit) Toolkit.getDefaultToolkit();
        tk.activate(this);
    }
    
    public void deactivate()
    {
        SunToolkit tk = (SunToolkit) Toolkit.getDefaultToolkit();
        tk.deactivate(this);
    }

    public static void setFrameDimensions(int dimX, int dimY, 
					  int dimW, int dimH, 
					  int dimWOffset,
					  int dimHOffset)
    {
        if (verbose) {
	    System.err.println("SETTING XLET FRAME DIMENSIONS: "+
			   dimX + ", " +
			   dimY + ", " +
			   dimW + ", " +
			   dimH + ", " +
			   dimWOffset + ", " +
			   dimHOffset);
	}
	x = dimX;
	y = dimY;
	w = dimW;
	h = dimH;
	wOffset = dimWOffset;
	hOffset = dimHOffset;
    }
    
    //
    // Source XletFrame with x,y,w,h,hOffset arguments
    //
    public static void main(String[] args) 
    {
	String xStr = args[0];
	String yStr = args[1];
	String wStr = args[2];
	String hStr = args[3];
	String wOffsetStr = args[4];
	String hOffsetStr = args[5];
	try {
	    setFrameDimensions(Integer.parseInt(xStr),
	                       Integer.parseInt(yStr),
	                       Integer.parseInt(wStr),
	                       Integer.parseInt(hStr),
	                       Integer.parseInt(wOffsetStr),
	                       Integer.parseInt(hOffsetStr));
	    initialized = true;
	} catch (Exception e) {
	    e.printStackTrace();
	}
    }

}
