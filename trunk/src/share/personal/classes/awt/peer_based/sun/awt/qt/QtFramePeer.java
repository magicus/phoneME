/*
 * @(#)QtFramePeer.java	1.26 06/10/10
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

import sun.awt.peer.FramePeer;
import java.awt.*;
import java.awt.event.*;
import java.awt.Image.*;

/**
 * QtFramePeer.java
 *
 * @author Nicholas Allen
 */

class QtFramePeer extends QtWindowPeer implements FramePeer
{
    private static native void initIDs();

    public native void setState(int state);

    public native int getState();

    static
    {
	initIDs();
    }
	
    QtFramePeer (QtToolkit toolkit, Frame target)
    {
	super (toolkit, target);
	setTitle (target.getTitle());
        setResizable(target.isResizable());
    }
	
    protected native void create(QtComponentPeer parentPeer, 
                                 boolean isUndecorated);

    protected void create(QtComponentPeer parentPeer) {
        create(parentPeer, ((Frame)target).isUndecorated());
    }

    protected native void setTitleNative(String title);
    public void setTitle(String title)
    {
	if(title!=null)
	    setTitleNative(title);
    }


    public void setIconImage(Image im) { }
	
    public void setMenuBar(MenuBar mb)
    {
	QtMenuBarPeer peer = (QtMenuBarPeer)toolkit.getMenuComponentPeer(mb);
	setMenuBarNative(peer);
    }
	
    private native void setMenuBarNative (QtMenuBarPeer peer);
	
    /**
     * Called to inform the Frame that it has moved.
     */
    private void handleMoved()
    {
        toolkit.postEvent(new ComponentEvent(target, ComponentEvent.COMPONENT_MOVED));
    }
	
    /** Calculates the insets using any values appropriate (such as borders). */
    Insets calculateInsets ()
    {
	return new Insets (topBorder + menuBarHeight, leftBorder, bottomBorder, rightBorder);
    }
	
    /**
     * Allows callback from the Qt native layer to update the menu bar height
     * as well as the insets.
     */
    private void updateMenuBarHeight(int h) {
        menuBarHeightDelta = h - menuBarHeight;
        menuBarHeight = h;
        setInsets(calculateInsets(), true);
    }

    // 6182409: Window.pack does not work correctly.
    // FramePeer has this notion of menu bar.
    protected int getHeightDelta() {
        return super.getHeightDelta()+ menuBarHeightDelta;
    }

    // 6182409: Window.pack does not work correctly.
    // FramePeer has this notion of menu bar.
    protected void resetInsetsDeltas() {
        super.resetInsetsDeltas();
        menuBarHeightDelta = 0;
    }

    private int menuBarHeight = 0;
    private int menuBarHeightDelta = 0;
}

