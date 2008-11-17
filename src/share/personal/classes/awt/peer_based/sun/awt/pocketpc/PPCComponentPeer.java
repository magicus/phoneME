/*
 * @(#)PPCComponentPeer.java	1.11 02/12/16
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

import java.awt.*;
import sun.awt.peer.*;
import sun.awt.ScreenUpdater;
import sun.awt.UpdateClient;
import sun.awt.image.ImageRepresentation;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.VolatileImage;
import java.awt.event.PaintEvent;
import sun.awt.PeerBasedToolkit;


abstract class PPCComponentPeer extends PPCObjectPeer implements ComponentPeer, 
    UpdateClient {

    private static native void initIDs();
    
    static 
    {
	initIDs();
    }

    // ComponentPeer implementation

    private boolean repaintPending;
    private Rectangle repaintRect;

    public synchronized native void show();
    public synchronized native void hide();
    public synchronized native void enable();
    public synchronized native void disable();

    /* New 1.1 API */
    public native Point getLocationOnScreen();

    /* New 1.1 API */
    public void setVisible(boolean b) {
	if (b) {
    	    show();
    	} else {
    	    hide();
    	}
    }

    /* New 1.1 API */
    public void setEnabled(boolean b) {
    	if (b) {
	    enable();
	} else {
	    disable();
	}
    }


    /* New 1.1 API */
    public void setBounds(int x, int y, int width, int height) {
	reshape(x, y, width, height);
    }

    public void paint(Graphics g) {
	g.setColor(((Component)target).getForeground());
	g.setFont(((Component)target).getFont());
        ((Component)target).paint(g);
    }

    /* Dummy method */
    public VolatileImage createVolatileImage(int width, int height) {	
        return null;
    }

    /* Dummy method */
    public void setFocusable(boolean focusable) {
    }

    /* Dummy method */
    public boolean requestFocus(Component child, Window parent, boolean temporary, boolean focusedWindowChangeAllowed, long time) {
        return false;
    }

    public void repaint(long tm, int x, int y, int width, int height)
    {
	addRepaintArea(x, y, width, height);
	ScreenUpdater.updater.notify(this, tm);
    }
    public void print(Graphics g) {
	/*
        ((Component)target).print(g);
	((PPCGraphics)g).print(this);
	*/
    }
    public synchronized native void reshape(int x, int y, int width, int height);
    native void nativeHandleEvent(AWTEvent e);

    public void handleEvent(AWTEvent e) {
        int id = e.getID();

        switch(id) {
	    //might want to update the paint stuff with PP code
          case PaintEvent.PAINT:
          case PaintEvent.UPDATE:
            Graphics g = null ;
            Rectangle r = ((PaintEvent)e).getUpdateRect();
	    synchronized (this) {
                g = ((Component)target).getGraphics();
	    }
	    if (g == null) {
	        return;
	    }
	    g.clipRect(r.x, r.y, r.width, r.height);
            if (id == PaintEvent.PAINT) {
                clearRectBeforePaint(g, r);
                ((Component)target).paint(g);
            } else {
                ((Component)target).update(g);
            }
            g.dispose();
        }

        // Call the native code
        nativeHandleEvent(e);
    }

    // Do a clearRect() before paint for most Components.  This is
    // overloaded to do nothing for native components.
    void clearRectBeforePaint(Graphics g, Rectangle r) {
        g.clearRect(r.x, r.y, r.width, r.height);
    }

    public Dimension getMinimumSize() {
	return ((Component)target).getSize();
    }

    public Dimension getPreferredSize() {
	return getMinimumSize();
    }

    public boolean isFocusTraversable() {
	return false;
    }

    public boolean isDoubleBuffered() {
      return false;
    }

    public ColorModel getColorModel() {
	return PPCToolkit.getStaticColorModel();
    }

    public java.awt.Toolkit getToolkit() {
	return Toolkit.getDefaultToolkit();
    }

    public Graphics getGraphics() {
	Graphics g = new PPCGraphics(this);
	g.setColor(((Component)target).getForeground());
	g.setFont(((Component)target).getFont());
	return g;
    }
    public FontMetrics getFontMetrics(Font font) {
	return PPCFontMetrics.getFontMetrics(font);
    }

    /*
     * Subclasses should override disposeImpl() instead of dispose(). Client
     * code should always invoke dispose(), never disposeImpl().
     */
    public void	dispose() {
	/* Can this be removed?
        if (!(this instanceof PPCWindowPeer)) {
            Container parent =
		PeerBasedToolkit.getNativeContainer((Component)target);
            if (parent != null) {
                PPCContainerPeer parentPeer = (PPCContainerPeer) PPCToolkit.getComponentPeer(parent);
                if (parentPeer != null)
                    parentPeer.remove(this);
            }
        }
	*/
        disposeNative();
    }

    private native void disposeNative();

    public synchronized void setForeground(Color c) {_setForeground(c.getRGB());}
    public synchronized void setBackground(Color c) {_setBackground(c.getRGB());}

    public native void _setForeground(int rgb);
    public native void _setBackground(int rgb);
    
    public synchronized native void setFont(Font f);
    public synchronized native void requestFocus();
    public synchronized native void setCursor(Cursor c);
    public Image createImage(ImageProducer producer) {
	return new PPCImage(producer);
    }
    public Image createImage(int width, int height) {
	return new PPCImage(((Component)target), width, height);
    }
    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
	return PPCToolkit.prepareScrImage(img, w, h, o);
    }
    public int checkImage(Image img, int w, int h, ImageObserver o) {
	return PPCToolkit.checkScrImage(img, w, h, o);
    }

    // UpdateClient implementation

    public void updateClient(Object arg) {
      	// bug 4073091.
       	// Integrated fix as suggested by Oracle.
	if (((Component) target).isDisplayable()) {
	    Rectangle updateRect = null;
	    synchronized (this) {
		if (repaintPending) {
		    updateRect = repaintRect;
		    repaintPending = false;
       		}
	    }
	    if (updateRect != null) {
		postEvent(new PaintEvent((Component)target, PaintEvent.UPDATE,
					 updateRect));
	    }
	}
    }

    // Object overrides

    public String toString() 
    {
	return getClass().getName() + "[" + target + "]";
    }

    // Toolkit & peer internals

    private int updateX1, updateY1, updateX2, updateY2;

    PPCComponentPeer(Component target) {
	this.target = target;
        this.repaintRect = new Rectangle();
        this.repaintPending = false;
	Container parent = PPCToolkit.getNativeContainer(target);
	PPCComponentPeer parentPeer = (PPCComponentPeer)
	    PeerBasedToolkit.getComponentPeer(parent);

	if (parent != null && parentPeer == null) {
	    parent.addNotify();
	    parentPeer = (PPCComponentPeer)
		PeerBasedToolkit.getComponentPeer(parent);
	}
	create(parentPeer);
	initialize();
	start();  // Initialize enable/disable state, turn on callbacks
    }
    abstract void create(PPCComponentPeer parent); 

    synchronized native void start();

    void initialize() {
        initZOrderPosition();

	if (((Component)target).isVisible()) {
	    show();  // the wnd starts hidden
	}
	Color fg = ((Component)target).getForeground();
	if (fg != null) {
	    setForeground(fg);
	}
        // Set background color in C++, to avoid inheriting a parent's color.
	Font  f = ((Component)target).getFont();
	if (f != null) {
	    setFont(f);
	}
	if (! ((Component)target).isEnabled()) {
	    disable();
	}

	Rectangle r = ((Component)target).getBounds();
	setBounds(r.x, r.y, r.width, r.height);

	// Set cursor if necessary.
	Cursor c = ((Component)target).getCursor();
        if (c != Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR)) {
            setCursor(c);
        }
    }

    private synchronized void addRepaintArea(int x, int y, int w, int h) {
        if (repaintPending == false) {
            repaintPending = true;
            repaintRect = new Rectangle(x, y, w, h);
        } else {
            /* expand the repaint area */
            repaintRect = repaintRect.union(new Rectangle(x, y, w, h));
        }
    }

    // Callbacks for window-system events to the frame

    // Invoke a update() method call on the target
    void handleRepaint(int x, int y, int w, int h) {
        // Repaints are posted from updateClient now...
    }

    // Invoke a paint() method call on the target, after clearing the
    // damaged area.
    void handleExpose(int x, int y, int w, int h) {
        // Bug ID 4081126 - can't do the clearRect() here, since it
        // interferes with the java thread working in the same window
        // on multi-processor NT machines.

        postEvent(new PaintEvent((Component)target, PaintEvent.PAINT, 
                                 new Rectangle(x, y, w, h)));
    }

    /* Invoke a paint() method call on the target, without clearing the
     * damaged area.  This is normally called by a native control after
     * it has painted itself. */
    void handlePaint(int x, int y, int w, int h) {
        postEvent(new PaintEvent((Component)target, PaintEvent.PAINT,
                                  new Rectangle(x, y, w, h)));
    }

    /*
     * Post an event. Queue it for execution by the callback thread.
     */
    void postEvent(AWTEvent event) {
        PPCToolkit.postEvent(event);
    }

    protected void finalize() throws Throwable {
        // Calling dispose() here is essentially a NOP since the current
        // implementation prohibts gc before an explicit call to dispose().
        dispose();
        super.finalize();
    }

    // Routines to support deferred window positioning.
    public void beginValidate() {

      // Fix for oracle repaint bugs #4045627, #4045617 and #4040638
      // Removed call to clearRect that was formerly here.

        _beginValidate();
    }
    public native void _beginValidate();
    public native void endValidate();

    public void initZOrderPosition() {
        Container p = ((Component)target).getParent();
        PPCComponentPeer peerAbove = null;

        if (p != null) {
            Component children[] = p.getComponents();
            for (int i = 0; i < children.length; i++) {
                if (children[i] == target) {
                    break;
                } else {
                    Object cpeer = PeerBasedToolkit.getComponentPeer(children[i]);
                    if (cpeer != null && 
                        !(cpeer instanceof sun.awt.peer.LightweightPeer)) {
                        peerAbove = (PPCComponentPeer)cpeer;
                    }
                }
            }

        }
        setZOrderPosition(peerAbove);
    }

    native void setZOrderPosition(PPCComponentPeer compAbove);

    /**
     * DEPRECATED
     */
    public Dimension minimumSize() {
	return getMinimumSize();
    }

    /**
     * DEPRECATED
     */
    public Dimension preferredSize() {
	return getPreferredSize();
    }
}
