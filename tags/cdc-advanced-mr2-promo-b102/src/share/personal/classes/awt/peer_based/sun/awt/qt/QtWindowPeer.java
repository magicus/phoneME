/*
 * @(#)QtWindowPeer.java	1.47 06/10/10
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

import sun.awt.peer.WindowPeer;
import java.awt.*;
import java.awt.event.*;

/**
 * QtWindowPeer.java
 *
 * @author Indrayana Rustandi
 * @author Nicholas Allen
 */

class QtWindowPeer extends QtPanelPeer implements WindowPeer
{
    private static native void initIDs();
    public native void setActiveNative();
    public native void setFocusInWindow(QtComponentPeer toFocus);
	
    // 6182409: Window.pack does not work correctly.
    // The winsets object, if set, overwrites the insets object in the
    // QtContainerPeer.  It is necessary to introduce a different object
    // because we are guessing/setting winsets in the peer constructor
    // and this conflicts with QtContainerPeer constructor where insets
    // is initialized to new Insets(0, 0, 0, 0).  If we update super.insets
    // instead of winsets, the super.insets gets overwritten at the end.
    // See also setInsets() and updateInsets().
    protected Insets winsets;
    private Rectangle paintRect = null;
	
    static
    {
        initIDs();
    }
	
    QtWindowPeer (QtToolkit toolkit, Window target)
    {
        super (toolkit, target);
    }
    
    protected native void create (QtComponentPeer parentPeer);
	
    public void show() 
    {
        /*
         * Fixed (partially) bug 4828042 where the initial splash screen
         * (an instance of java.awt.Window) was setMainWidget()'ed.
         * We now dictate that only java.awt.Frame can be honored as the
         * main widget.
         */
        if (this instanceof QtFramePeer) {
            toolkit.setMainWidget(this);
        }

	// bug 6186499 - we need to set the focus in native before showing
        // the window.  Otherwise, Qt will first focus the frame, then the
        // widget that we want to have the focus
        Component toFocus = ((Window)target).getMostRecentFocusOwner();
	if (toFocus == null)
            toFocus = ((Window)target).getFocusTraversalPolicy().getInitialComponent((Window)target); 
        sun.awt.peer.ComponentPeer peer = QtToolkit.getComponentPeer(toFocus);
	if (peer instanceof QtComponentPeer)
            setFocusInWindow((QtComponentPeer)peer);
	// bug 6186499
        super.show();

        // 6224133.
        // No longer needed.
        // toolkit.setRun(true);
        // 6224133.

        computeInsets();   //6182365

        /* Don't call setBounds here - bounds is already set at QtComponentPeer's
           constructor, and we don't know the insets of this window right after
           the show() anyway. */
        //Rectangle bounds = target.getBounds();
        //setBounds(bounds.x, bounds.y, bounds.width, bounds.height);
        //don't return from this method until we receive notifiation that
        //we're actually visible on the screen

    }

    // 6182409: Window.pack does not work correctly.
    // Called from computeInsets() native method impl.
    // The 6182365 comment is no longer valid.
    /* 6182365.  For x86 only.  Called from Qt's paint event delivery */
    private void restoreBounds() {
        Rectangle bounds = target.getBounds();
        if (QtComponentPeer.isPacked(target)) {
            if (insetsChanged) {

                // must obey the interior size constraint of our target.
                // Now that we have an accurate insets, update the Java layer
                // about the correct target bounds if necessary.
                bounds.setSize(bounds.width + getWidthDelta(),
                               bounds.height + getHeightDelta());
                target.setSize(bounds.width, bounds.height);
            }
        } else {
            // Window was NOT packed before show and the insets may or may not
            // have changed.
        }

        // The bounds may have been set correctly by the application before
        // target.show(), but we have found out that it is necessary to do
        // another peer.setBounds(), followed by, if necessary, the layout
        // of the Window so that the window is positioned correctly on the
        // screen.
        setBounds(bounds.x, bounds.y, bounds.width, bounds.height);
        if (insetsChanged) {
            target.invalidate();
            target.validate();
        }

        resetInsetsDeltas();
    }

    // 6182409: Window.pack does not work correctly.
    // The follow delta related functions are for pack() to work correctly.
    // Subclasses may override them, for example, QtFramePeer.
    protected int getWidthDelta() {
        return leftBorderDelta + rightBorderDelta;
    }

    protected int getHeightDelta() {
        return topBorderDelta + bottomBorderDelta;
    }

    protected void resetInsetsDeltas() {
        topBorderDelta = leftBorderDelta = bottomBorderDelta = rightBorderDelta = 0;
        insetsChanged = false;
    }

    public native void computeInsets();   //6182365

    public native void toFront ();

    public native void toBack ();

    public native int warningLabelHeight();   //6233632

    public native void setResizable(boolean resizable);
	
    /** This is overridden to set the bounds on the window manager's
        frame. */

    public void setBounds (int x, int y, int w, int h)
    {
        super.setBounds (x + leftBorder,
                         y + topBorder,
                         Math.max (w - leftBorder - rightBorder, 0), 
                         Math.max (h - topBorder - bottomBorder, 0));
    }
	
    // Bug 5108647.
    // Overrides QtComponentPeer.setBoundsNative() in order to
    // check whether this peer is user resizable.
    native void setBoundsNative (int x, int y, int width, int height);

    public Graphics getGraphics ()
    {
        Graphics g = super.getGraphics();
		
        if (g != null) {
            g.translate(-leftBorder, -topBorder) ;
        }

        return g;
    }
	
    void postMouseEvent (int id, long when, int modifiers, int x, 
                         int y, int clickCount, boolean popupTrigger, 
                         int nativeEvent)
    {
        super.postMouseEvent (id, when, modifiers, x + leftBorder,
                              y + topBorder, clickCount,
                              popupTrigger, nativeEvent);
    }
	
    /**
     * Called to inform the Frame that its size has changed and it
     * should layout its children.
     */
	
    private void postResizedEvent ()
    {
        QtToolkit.postEvent(new ComponentEvent(target,
                                               ComponentEvent.COMPONENT_RESIZED));
    }
	
    private void postMovedEvent ()
    {
        QtToolkit.postEvent(new ComponentEvent(target, ComponentEvent.COMPONENT_MOVED));
    }
	
    private void postShownEvent() 
    {
        QtToolkit.postEvent(new ComponentEvent(target, ComponentEvent.COMPONENT_SHOWN));
    }

    protected void postWindowEvent (int type) {
	WindowEvent we = new WindowEvent((Window)target, type);
        if (type == WindowEvent.WINDOW_ACTIVATED) {
            if (!postedActivatedEvent) {
		QtToolkit.postEvent(we);
                postedActivatedEvent = true;
            }
        }
        else if (type == WindowEvent.WINDOW_DEACTIVATED) {
            if (postedActivatedEvent) {
                QtToolkit.postEvent(we);
                postedActivatedEvent = false;
            }
        }
        else {
            QtToolkit.postEvent(we);
        }
    }
    // Bug 6211281
    protected void postWindowEvent(AWTEvent event) {
        QtToolkit.postEvent(event);
    }
	
    // 6182409: Window.pack does not work correctly.
    // Overrides QtContainerPeer.getInsets() so that the native layer can
    // update the Java window peer about new insets value when they become
    // available.
    public Insets getInsets() 
    {
        Insets newInsets;
        if (winsets != null) {
            //6233632
            newInsets = new Insets(winsets.top,
                                   winsets.left,
                                   winsets.bottom + warningLabelHeight(),
                                   winsets.right);
            return newInsets;
            //6233632 return winsets;
        } else {
            return super.getInsets();
        }
    }

    // 6182409: Window.pack does not work correctly.
    /**
     * Sets the window peer's insets.
     */
    protected void setInsets(Insets newInsets, boolean isInsetsChanged)
    {
        this.winsets = newInsets;
        this.insetsChanged = isInsetsChanged;
    }

    /** Calculates the insets using any values appropriate (such as borders). */
    Insets calculateInsets ()
    {
	return new Insets (topBorder, leftBorder, bottomBorder, rightBorder);
    }
    
    // 6182409: Window.pack does not work correctly.
    /**
     * Sets this Frame/Dialog's inset values.
     * Invoked by the frame/dialog native layer which guesses the borders
     * for decorated windows.  Pass isInsetsChanged = false.
     */
    private void setInsets(int top, int left, int bottom, int right)
    {
        setInsets(top, left, bottom, right, false);
    }
    private void setInsets(int top, int left, int bottom, int right, boolean isInsetsChanged)
    {
        topBorder = top;
        leftBorder = left;
        bottomBorder = bottom;
        rightBorder = right;

	setInsets(calculateInsets(), isInsetsChanged);
    }

    // 6182409: Window.pack does not work correctly.
    /**
     * Updates this Frame/Dialog's inset values.
     * Invoked by the native layer after the window is realized.
     */
    protected void updateInsets(int top, int left, int bottom, int right)
    {
        topBorderDelta = top - topBorder;
        leftBorderDelta = left - leftBorder;
        bottomBorderDelta = bottom - bottomBorder;
        rightBorderDelta = right - rightBorder;

        if (topBorderDelta != 0 || leftBorderDelta != 0 ||
            rightBorderDelta != 0 && bottomBorderDelta != 0)
        {
            setInsets(top, left, bottom, right, true);
            
        }
    }

    static void setQWSCoords (int x, int y) 
    {
        qwsX = x;
        qwsY = y;
    }

    int getOriginX () {return -leftBorder;}
    int getOriginY () {return -topBorder;}
	
    int topBorder, leftBorder, bottomBorder, rightBorder;

    // 6182409: Window.pack does not work correctly.
    // Indicates the deltas for decorations before/after peer.show().
    int topBorderDelta, leftBorderDelta, bottomBorderDelta, rightBorderDelta;

    // 6182409: Window.pack does not work correctly.
    // Indicates whether insets have changed before/after peer.show().
    boolean insetsChanged;

    static int qwsX, qwsY;
    static boolean qwsInit = false;

    private boolean postedActivatedEvent;

    // 5108404 
    // Called from the native code with qt-library lock held, so call the
    // hide method in the event queue thread
    void hideLater() {
        EventQueue.invokeLater(new Runnable() {
                public void run() {
                    target.hide() ;
                }
            });
    }
    // 5108404 

    public void setActive() {
        if (target.isVisible() && target.isFocusable())
	    setActiveNative();
    }

    public void hide() {
        // bug 6186499
	// if we are the focused window, generate a WINDOW_LOST_FOCUS event
	// before hiding ourselves - because Qt won't necessarily do so
	if (((Window)target).isFocused())
	    postWindowEvent(WindowEvent.WINDOW_LOST_FOCUS);
	super.hide();
        // bug 6186499
    }
}
