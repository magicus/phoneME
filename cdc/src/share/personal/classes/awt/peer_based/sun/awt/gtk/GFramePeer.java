/*
 * @(#)GFramePeer.java	1.16 06/10/10
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

package sun.awt.gtk;

import sun.awt.peer.FramePeer;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import sun.awt.image.ImageRepresentation;

/*
 * Warning :
 * Two versions of this file exist in this workspace.
 * One for Personal Basis, and one for Personal Profile.
 * Don't edit the wrong one !!!
 */

/**
 * GFramePeer.java
 *
 * @author Nicholas Allen
 */

class GFramePeer extends GWindowPeer implements FramePeer, ImageObserver {
    private static native void initIDs();
    private static int defaultLeftBorder = 6;
    private static int defaultRightBorder = 6;
    private static int defaultTopBorder = 19;
    private static int defaultBottomBorder = 6;
    static {
        initIDs();
    }
    GFramePeer (GToolkit toolkit, Frame target) {
        super(toolkit, target);
        setTitle(target.getTitle());
        setResizable(target.isResizable());
        state = target.getState();
        topBorder = defaultTopBorder;
        leftBorder = defaultLeftBorder;
        bottomBorder = defaultBottomBorder;
        rightBorder = defaultRightBorder;
        setIconImage(target.getIconImage());
    }
	
    protected native void create();
	
    protected native void setTitleNative(byte[] title);

    public void setTitle(String title) {
        if (title != null)
            setTitleNative(stringToNulMultiByte(title));
    }
	
    public void setState(int state) {
        if (target.isVisible()) {
            setStateNative(state);
        }
        this.state = state;
    }
	
    private native void setStateNative(int state);

    public int getState() {
        return state;
    }

    private native void setIconImageNative(GdkImageRepresentation gdkImageRep);
    
    public void setIconImage(Image img) {
        if (img != null && img instanceof GdkImage) {
            if (((GdkImage) img).isComplete())
                imageUpdate(img, ALLBITS, 0, 0, 0, 0);
            else
                toolkit.prepareImage(img, 20, 20, this);
        }
    }
	
    public void setMenuBar(MenuBar mb) {
        GMenuBarPeer peer = (GMenuBarPeer) toolkit.getMenuComponentPeer(mb);
        setMenuBarNative(peer);
    }
	
    private native void setMenuBarNative(GMenuBarPeer peer);
	
    public void show() {
        // If we are making this frame visible in the iconic state then we need to make sure
        // the WM_HINTS property is set to IconicState or NormalState prior to making the frame visible.
        // Unfortunately, this is very much dependant on X implementation and as Gtk does not
        // provide us with a well designed or high level API, and misses out this functionality completely,
        // this is the only way of getting around it.
		
        setWMStateHints(state == Frame.ICONIFIED);
        super.show();
    }
	
    /** Sets the WM_STATE property on the window to IconicState. This is clalled prior to showing
     the window in order to make sure it is shown as a visible icon and not in the normal state
     as Gtk would normally do. */
	
    private native void setWMStateHints(boolean iconic);
	
    private void postWindowIconified() {
        state = Frame.ICONIFIED;
        GToolkit.postEvent(new WindowEvent ((Window) target, WindowEvent.WINDOW_ICONIFIED));
    }
	
    private void postWindowDeiconified() {
        state = Frame.NORMAL;
        GToolkit.postEvent(new WindowEvent ((Window) target, WindowEvent.WINDOW_DEICONIFIED));
    }
	
    /** Calculates the insets using any values appropriate (such as borders). */
	
    Insets calculateInsets() {
        return new Insets (topBorder + menuBarHeight, leftBorder, bottomBorder, rightBorder);
    }
	
    /** Sets up border values to some sensible values for this window. */
	
    void initBorders() {
        topBorder = defaultTopBorder;
        leftBorder = defaultLeftBorder;
        bottomBorder = defaultBottomBorder;
        rightBorder = defaultRightBorder;
    }
	
    /** Sets the default border values to be used for future windows. */
	
    void setDefaultBorders(int top, int left, int bottom, int right) {
        defaultTopBorder = top;
        defaultLeftBorder = left;
        defaultBottomBorder = bottom;
        defaultRightBorder = right;
    }
	
    public boolean imageUpdate(Image img, int infoflags, int x, int y, int width, int height) {
        if ((infoflags & (ALLBITS | FRAMEBITS)) > 0) {
            if (img instanceof GdkImage) {
                ImageRepresentation ir = ((GdkImage) img).getImageRep();
                if (ir instanceof GdkImageRepresentation)
                    setIconImageNative((GdkImageRepresentation) ir);
            }
            return false;
        }
        if ((infoflags & (ABORT | ERROR)) > 0)
            return false;
        return true;
    }    
    private int menuBarHeight;
    /* Stores the state of this frame (whether it is iconified or not). */
	
    private int state;
}
