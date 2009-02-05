/*
 * @(#)QtComponentPeer.java	1.44 06/10/10
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
/*
 * @(#)QtComponentPeer.java	1.27 02/10/29
 */
package sun.awt.qt;


import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.awt.datatransfer.*;
import sun.awt.*;
import java.io.UnsupportedEncodingException;
import java.awt.image.VolatileImage;

/**
 * QtComponentPeer.java
 *
 * @(#)QtComponentPeer.java	1.27 02/10/29
 *
 * @author Indrayana Rustandi
 * @author Nicholas Allen
 */

abstract class QtComponentPeer implements ComponentPeer, UpdateClient,
					  ClipboardOwner
{
    private static native void initIDs();
    private native void postFocusEventToQt(FocusEvent e);
    public native void setFocusable(boolean focusable);
    public native boolean nativeRequestFocus(Component lightweightChild, 
					  boolean temporary,
					  boolean focusedWindowChangeAllowed, 
					  long time);
	
    // 6182409: Window.pack does not work correctly.
    // Native function to check the Component.isPacked field value.
    static native boolean isPacked(Component component);

    static
    {
	initIDs();
    }
	
    QtComponentPeer (QtToolkit toolkit, Component target)
    {
	this.toolkit = toolkit;
	this.target = target;

        // Allow the top level window to have an influence on its geometry.
        // An example is the QtFileDialogPeer which basically lays out its
        // components using Qt widgets and does not have any associated Java
        // layer window manager.
        // This and the change in QtFileDialogPeer.cc (1.19) fixed bug 4760172.
        if (this instanceof QtWindowPeer) {
            create(null);
        }

        // Determine the heavyweight parent for this peer. If a
        // lightweight container is found on the way up which is not
        // visible then make sure this peer is invisible too. The
	// NativeInLightFixer class will make this peer visible
        // should a lightweight parent be made visible.
	
	Container parent;
	boolean visible = target.isVisible();
	Rectangle bounds = target.getBounds();
	
	for (parent = target.getParent();
	     parent != null && parent.isLightweight();
	     parent = parent.getParent()) {
	    
	    if (!parent.isVisible()) {
		visible = false;
	    }
	    
	    bounds.x += parent.getX();
	    bounds.y += parent.getY();
	}
	
	// parent now refers to the heavyweight ancestor.
	
	// As long as this peer isn't a window we can add it to its parent.
	// We need to add the peer to its native parent peer and not necessarily
	// its direct parent as this could be lightweight.
		
	if (!(this instanceof QtWindowPeer)) {
	    parent = QtToolkit.getNativeContainer(target);
	    QtComponentPeer parentPeer = (QtComponentPeer) QtToolkit.getComponentPeer(parent);
	    create(parentPeer);

	    if (parentPeer != null && parentPeer instanceof
		QtContainerPeer) {
		((QtContainerPeer) parentPeer).add(this);
	    }
	}
		
	setEnabled(target.isEnabled());
        //Use visible here instead of target.isVisible() because 
        //peer should be visible only if the target has any visible 
        //lightweight ancestor
	setVisible(visible);
	setBounds (bounds.x, bounds.y, bounds.width, bounds.height);
    }
	
    /** Decides whether it is possible for this component to have a
        pixmap background (i.e. a non solid color).
	We allow all components except Window, Frame, Dialog, Panel and
	Canvas to have pixmaps as the user can, and typically does,
	override the paint for these components. Thus update can be
	called which will clear the background with a solid color for
	these components. However, we would still like to have support
	for Qt themes where a button, for example, may hava a pixmap
	background. */
	
    protected boolean canHavePixmapBackground () {return true;}
	
    /** Creates this component peer. This requires setting the data
        field to point to a struct QtComponentData. */
	
    protected abstract void create(QtComponentPeer parentPeer);
	
    public void	dispose() {
	if (!(this instanceof QtWindowPeer)) {
            Container parent = PeerBasedToolkit.getNativeContainer(target);
	    
            if (parent != null) {
                QtContainerPeer parentPeer = (QtContainerPeer) QtToolkit.getComponentPeer(parent);
		
                if (parentPeer != null)
                    parentPeer.remove(this);
            }
        }
	
	disposeNative();
    }
	
    private synchronized native void disposeNative();

    public void setVisible(boolean b)
    {
	if (b)
	    show();
			
	else hide();
    }
	
    public native void setEnabled(boolean b);
	
    public void	paint(Graphics g)
    {
	target.paint(g);
    }
	
    public void update (Graphics g)
    {
	target.update(g);
    }
	
    public void repaint(long tm, int x, int y, int width, int height)
    {
	addRepaintArea(x, y, width, height);
	ScreenUpdater.updater.notify(this, tm);
    }
	
    public void	print(Graphics g) { }
	
    public void	clearBackground(Graphics g) { }

    public void	setBounds(int x, int y, int width, int height)
    {
	// Fix for 4744238.  Let a set size attempt of 0x0 go through but guard
        // against any < 0 attempt.  See class javadoc of java.awt.Rectangle in
        // regard to "undefined behavior" for rectangles with negative width or
        // height.

	if (width < 0 || height < 0)
	    return;

	Container nativeContainer = PeerBasedToolkit.getNativeContainer(target);

	// Give the native window peer a chance to translate the
        // position of components added to it if it needs to. This
        // is necessary in the case of a Frame for example
	// where the size of the frame includes the borders add
        // possibly an optional menu bar. The layout manager lays
        //out components relative to the top left  of the frame
	// but we may wish to add components to a native container
        // inside the frame. In this case we need to translate all
        // components so their coordinates were relative to the
	// native container inside the frame instead of to the frame itself.
	

    /*
     * We need to check if the target is not a Window, since Window's parent
     * (actually the owner) is set to another Window ( typiclly Frame) and
     * we will be incorrectly changing the location of the "target" based
     * on the parent's Insets. This is done for the bug fix to get 
     * Insets working on all java.awt.Window
     */	
	if (!(target instanceof Window) && nativeContainer != null) {
	    QtContainerPeer peer = 
                (QtContainerPeer)toolkit.getComponentPeer(nativeContainer);
            if ( peer == null ) {
                nativeContainer.addNotify();
                peer = (QtContainerPeer)toolkit.getComponentPeer(nativeContainer);
            }

            x += peer.getOriginX();
            y += peer.getOriginY();
	}
		
	setBoundsNative (x, y, width, height);
    }
	
    native void setBoundsNative (int x, int y, int width, int height);

    public void handleEvent(AWTEvent event)
    {
	int id = event.getID();
		
	if (event instanceof PaintEvent) {
            Graphics g = null;
            Rectangle r = ((PaintEvent) event).getUpdateRect();
	    
            try {
                synchronized (target.getTreeLock()) {
                    g = target.getGraphics();
		    
                    if (g == null)
                        return;
		    
                    g.clipRect(r.x, r.y, r.width, r.height);
                }
		
                if (id == PaintEvent.PAINT)
                    paint(g);
		
                else
                    update(g);
		
                toolkit.sync();
            } finally {	// Always dispose of graphics even if exception ocurrs during paint
		if (g != null)
		    g.dispose();
	    }
        } else if (event instanceof MouseEvent) {
	    MouseEvent mouseEvent = (MouseEvent)event;

	    if (id == MouseEvent.MOUSE_PRESSED) {
                if (target == event.getSource() && 
                    !((InputEvent)event).isConsumed() &&
                    shouldFocusOnClick() && !target.isFocusOwner() &&
                    canBeFocusedByClick(target)) {
                    boolean result = target.requestFocusInWindow();
                }
            }
	    
            if (id != MouseEvent.MOUSE_CLICKED) { // not a synthetic event
                if (!mouseEvent.isConsumed()) {
                    postMouseEventToQt(mouseEvent);
                } else {
                    eventConsumed(mouseEvent);
                }
	    }
	} else if (event instanceof KeyEvent) {
	    KeyEvent keyEvent = (KeyEvent)event;
	    
        if ( id != KeyEvent.KEY_TYPED) { // not a synthetic event
	        if (!keyEvent.isConsumed() ) {
		        postKeyEventToQt(keyEvent);
            }
            else {
                eventConsumed(keyEvent);
            }
	    }
	} else if (event instanceof FocusEvent) {
	    FocusEvent focusEvent = (FocusEvent)event;
	    postFocusEventToQt(focusEvent);
	}
    }
    
    /** Posts a mouse event back to Qt.
	After an event has been processed by Java it is posted back to
	Qt if it has not been consumed. */ 
	
    private native void postMouseEventToQt (MouseEvent event);
    
    /** Tells the native layer to cleanup the event, since it has
     * been consumed bu the java layer
	 * After an event has been processed by Java its native memory is
	 * freed if it has been consumed. 
     */ 
    private native void eventConsumed(AWTEvent event);
    
    /** Posts a key event back to Qt.
	After an event has been processed by Java it is posted back to
	Qt if it has not been consumed. */ 
    
    private native void postKeyEventToQt (KeyEvent event);
    
    /** A utility function called from native code to create a byte
	array of characters from a unicode character in UTF-8
	format. This is called when the keyChar has been modified for
	a key event. The GDK key event requires a pointer to a
	sequence of UTF-8 characters. This could probably be done a
	lot more efficiently but it is not used often and works.... */
    
    private byte [] getUTF8Bytes (char c) {
	try {return new String(new char [] {c}).getBytes("UTF-8");}
	catch (UnsupportedEncodingException e) {throw new AWTError ("UTF-8 encoding not supported");}
    }
    
    private synchronized void addRepaintArea(int x, int y, int w, int h)
    {
        if (repaintPending == false) {
	    repaintPending = true;
	    repaintRect = new Rectangle(x, y, w, h);
	}
	
	else {
	    /* expand the repaint area */
	    repaintRect = repaintRect.union(new Rectangle(x, y, w, h));
	}
    }
	
    /** Called by the ScreenUpdater to update the component. */
	
    public void updateClient(Object arg)
    {
        if (target.isDisplayable()) {
	    Rectangle updateRect = null;
				
	    synchronized (this) {
		if (repaintPending) {
		    updateRect = repaintRect;
		    repaintPending = false;
		}
	    }
			
	    if (updateRect != null)
		QtToolkit.postEvent(new PaintEvent((Component)target,
						   PaintEvent.UPDATE, 
						   updateRect));
	}
    }
	
    public native Point	getLocationOnScreen();

    public native Dimension getPreferredSize();

    public Dimension getMinimumSize() 
    {
	return getPreferredSize();
    }
	;
    public ColorModel getColorModel() {return toolkit.getColorModel();}
    public Toolkit getToolkit() {return toolkit;}
	
    public Graphics getGraphics()
    {
	return QtGraphics.createFromComponent(this);
    }
	
    private native void updateWidgetStyle();
	
    public FontMetrics getFontMetrics(Font font) 
    {
	return QtFontPeer.getFontPeer(font);
    }

    public void setForeground(Color c)
    {
	// This is required to prevent recursion in some badly written
	// programs where they set a color on the component during a
	// paint method. updateWidgetStyle will also cause a paint
	// event to be posted to the event queue so recursion will
	// occur. 
		
	if (c != null && !c.equals(lastForeground)) {
	    lastForeground = c;
	    updateWidgetStyle();
	}
    }
	
    public void	setBackground(Color c)
    {
	// This is required to prevent recursion in some badly written
	// programs where they set a color on the component during a
	// paint method. updateWidgetStyle will also cause a paint
	// event to be posted to the event queue so recursion will
	// occur. 
		
	if (c != null && !c.equals(lastBackground)) {
	    lastBackground = c;
	    updateWidgetStyle();
	}
    }
	
    public void	setFont(Font f)
    {
	// This is required to prevent recursion in some badly written
	// programs where they set a font on the component during a
	// paint method. updateWidgetStyle will also cause a paint
	// event to be posted to the event queue so recursion will
	// occur. 
		
	if (f != null && !f.equals(lastFont)) {
	    lastFont = f;
	    updateWidgetStyle();
	}
    }
	
    public void setCursor(Cursor cursor) 
    {
	this.cursor = cursor;
    // 6201639
    // Previously we were passing the cursor object itself and it was 
    // never used. The native code directly accessed the "target.cursor"
    // field which was the cause for the cursor not getting updated for
    // lightweights. Since the native code only needs the "type" field to
    // map to the Qt type, we only send the type.
	setCursorNative(cursor.getType());
    // 6201639
    }
    
    // 6201639 
    // Parameter changed from Cursor to int
    private native void setCursorNative(int cursorType);
    // 6201639
	

    public boolean isFocusTraversable() 
    {
	return false;
    }
	
    public Image createImage(ImageProducer producer)
    {
	return new QtImage (producer);
    }
	
    public Image createImage(int width, int height)
    {
	return new QtImage (target, width, height);
    }
	
    public boolean prepareImage(Image img, int w, int h, ImageObserver o)
    {
	return QtToolkit.prepareScrImage(img, w, h, o);
    }
	
    public int checkImage(Image img, int w, int h, ImageObserver o)
    {
	return QtToolkit.checkScrImage(img, w, h, o);
    }

    /**
     * Shows this widget.
     */
    native void show();

    /**
     * Hides this widget
     */
    native void hide();
	
    public void lostOwnership(Clipboard clipboard, Transferable contents) { }
	
    private native void setNativeEvent (AWTEvent e, int nativeEvent);
 
    void postEvent(AWTEvent event) {
        QtToolkit.postEvent(event);
    }
	
    /** Posts a paint event for this component. This is called when an
        area of the component is exposed. The area that needs to be
        painted is specified by the parameters. */
	
    void postPaintEvent (int x, int y, int width, int height)
    {
	Rectangle r = new Rectangle (x, y, width, height);
		
	QtToolkit.postEvent (new PaintEvent (target, PaintEvent.PAINT, r));
    }
	
    /* Posts a mouse event for this component. */
	
    void postMouseEvent (int id, long when, int modifiers, int x, 
			 int y, int clickCount, boolean popupTrigger,
			 int nativeEvent)
    {
	MouseEvent e = new MouseEvent (target, id, when, modifiers, x,
				       y, clickCount, popupTrigger);
	setNativeEvent(e, nativeEvent);
	QtToolkit.postEvent(e);
    }
	
    /* Posts a key event for this component. */
	
    private void postKeyEvent (int id, long when, int modifiers, 
			       int keyCode, char keyChar, 
			       int nativeEvent) 
    {
	KeyEvent e = new KeyEvent (target, id, when, modifiers,
				   keyCode, keyChar);
	setNativeEvent (e, nativeEvent);
	QtToolkit.postEvent(e);
    }


    private void postFocusEvent (int id, boolean temporary, int nativeEvent) 
    {
	FocusEvent e = new FocusEvent(target, id, temporary);
	QtToolkit.postEvent(e);
    }
	
    /** The Toolkit that created this peer. */
	
    QtToolkit toolkit;
	
    /** Used by native code as a pointer to the Qt widget. */
	
    int data;
	
    /** The Component this is the peer for. */
	
    Component target;

    private Cursor cursor;
    private boolean repaintPending;
    private Rectangle repaintRect;
    private Color lastForeground, lastBackground;
    private Font lastFont;

    protected byte[] stringToNulMultiByte(String string)
    {
	if(string!=null) {
	    byte[] encString = string.getBytes();
	    byte[] encStringNul = new byte[encString.length+1];

	    System.arraycopy(encString, 0, encStringNul, 0, encString.length);
	    return encStringNul;
	}

	return null;
    }

    public long getNativeComponent() {
        return getQWidget(data) ;
    }
    private native static long getQWidget(int data) ;
   
    public VolatileImage createVolatileImage(int width, int height) {
        return new QtVolatileImage(this.target, width, height);
    } 

    public boolean requestFocus(Component child, Window parent,
				boolean temporary,
         boolean focusedWindowChangeAllowed, long time) {

   	 // Qt doesn't allow window focus changes - so ignore 
	 // isWindowFocusChangeAllowed and don't allow window focus changes
	 if (!(target.isEnabled() && parent.isFocused())) {
	     return false;
	}
	else {
 //  	    System.out.println(this + " requesting focus on " + child);
//    System.out.println("calling nativeRequestFocus on " + this + " for " + child);
	    return nativeRequestFocus(child, temporary, 
                                     focusedWindowChangeAllowed, time); 
	}
    }

    static boolean canBeFocusedByClick(Component component) {
        if (component == null) 
            return false;
        else {
            return component.isDisplayable() && component.isVisible() &&
                   component.isEnabled() && component.isFocusable();
        }
    }

    public boolean isFocusable() {
        return false;
    }

    protected boolean shouldFocusOnClick() {
        return isFocusable();
    }
} 
