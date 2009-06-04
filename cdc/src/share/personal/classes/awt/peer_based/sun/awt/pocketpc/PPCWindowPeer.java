/*
 * @(#)PPCWindowPeer.java	1.4 02/12/09
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


import java.util.Vector;
import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;
import sun.awt.PeerBasedToolkit;

class PPCWindowPeer extends PPCPanelPeer implements WindowPeer 
{
    private static native void initIDs();
    
    static 
    {
	initIDs();
    }

    // PPCComponentPeer overrides
  
    public void dispose() {
	allWindows.removeElement(this);
	super.dispose();
    }

    // WindowPeer implementation

    public native void toFront();
    public native void toBack();

    // FramePeer & DialogPeer partial shared implementation

    public void setTitle(String title) {
        // allow a null title to pass as an empty string.
        if (title == null) {
            title = new String("");
        }
        _setTitle(title);
    }
    native void _setTitle(String title);

    public void setResizable(boolean resizable) {
        _setResizable(resizable);
        ((Component)target).invalidate();  // Insets were updated.
    }

    /* Dummy method */
    public void setActive() {
    }
 
    public native void _setResizable(boolean resizable);
    
    // Toolkit & peer internals

    static Vector allWindows = new Vector();  //!CQ for anchoring windows, frames, dialogs

    PPCWindowPeer(Window target) {
	super(target);
    }

    void initialize() {
        super.initialize();

        updateInsets(insets_);
	allWindows.addElement(this);

        Font f = ((Window)target).getFont();
	if (f == null) {
	    f = new Font("Dialog", Font.PLAIN, 12);
	    ((Window)target).setFont(f);
	    setFont(f);
	}
    }
    
    native void create(PPCComponentPeer parent);

    // Synchronize the insets members (here & in helper) with actual window 
    // state.
    native void updateInsets(Insets i);

    // Called from AwtFrame::WmActivate
    void postFocusOnActivate() {
        PPCToolkit.postEvent(new FocusOnActivate());
    }

    class FocusOnActivate extends AWTEvent implements ActiveEvent {
	public FocusOnActivate() {
	    super((Component)PPCWindowPeer.this.target,0);
	}

	public void dispatch() {
	    PPCComponentPeer	peer;
    
	    peer = getFocusPeer();
	    if (peer == null) {
		// no heavyweight had the focus, so set a default focus
		peer = setDefaultFocus();
	    }
    
	    if (peer != null) {
		requestComponentFocusIfActive(peer);
	    }
	}
    }

    // Requests the focus be set to a given component iff the containing
    // window is activated
    native void requestComponentFocusIfActive(PPCComponentPeer peer);

    // Get the peer of the heavyweight component with focus
    // in this window
    PPCComponentPeer getFocusPeer() {
        Component c = ((Window)target).getFocusOwner();
        while (c != null) {
            ComponentPeer peer = (ComponentPeer)PeerBasedToolkit.getComponentPeer(c);
            if (peer != null && peer instanceof PPCComponentPeer) {
                return (PPCComponentPeer)peer;
            } else {
                // peerless sub-component, try parent.
                c = c.getParent();
            }
        }
        return null;  // no child has focus
    }

    // This method is called to set focus to a Motif-like "reasonable"
    // default.  It's only called if focus was not explicitly set by either the
    // Java app or the user.
    PPCComponentPeer setDefaultFocus(){
        // Find first focus-traversable component.
        Component c = setDefaultFocus((Container)target);
	if (c == null) {
	    // No focus-traversable component found, so set focus the 1st leaf node
	    c = (Component)target;
	    while (c instanceof Container && 
	       	   ((Container)c).getComponentCount() > 0 &&
	           PeerBasedToolkit.getComponentPeer(c) != null) {
		Component child = getContainerElement((Container)c, 0);
		if (child.isVisible() && child.isEnabled()) {
		    c = child;
		} else {
		    break;        // Bug fix for bug #4038896 (Oracle deadlock bug) - Jonathan Locke
		}
	    } 
	}
        ComponentPeer peer = PeerBasedToolkit.getComponentPeer(c);
	if (peer instanceof sun.awt.peer.LightweightPeer)
	    return null;
	else
	    return (PPCComponentPeer)peer;
    }

    // This is similar to Window.activateFocus(), but won't trip over any locks
    // held by client code.
    private Component setDefaultFocus(Container cont) {
        int ncomponents = cont.getComponentCount();
        for (int i = 0; i < ncomponents; i++) {
            Component c = getContainerElement(cont, i);
            if (c.isVisible() && c.isEnabled() && c.isFocusTraversable()) {
                return c;
            }
            if (c instanceof Container && c.isVisible() && c.isEnabled()) {
                Component child = setDefaultFocus((Container)c);
				if (child != null){ 
                    return child;
                }
            }
        }
        return null;
    }

    private Component getContainerElement(Container c, int i) 
    {
	return c.getComponent(i);
    }
}
