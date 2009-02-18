/*
 * @(#)GComponentPeer.java	1.24 06/10/10
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

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.awt.datatransfer.*;
import sun.awt.*;
import java.io.UnsupportedEncodingException;

/**
 * GComponentPeer.java
 *
 * @author Nicholas Allen
 */

abstract class GComponentPeer implements ComponentPeer, UpdateClient, ClipboardOwner {
    private static native void initIDs();
    static {
        initIDs();
    }
    GComponentPeer (GToolkit toolkit, Component target) {
        this.toolkit = toolkit;
        this.target = target;
        // Create the peer
		
        create();
        // Determine the heavyweight parent for this peer. If a lightweight container is found
        // on the way up which is not visible then make sure this peer is invisible too. The
        // NativeInLightFixer class will make this peer visible should a lightweight parent
        // be made visible.
		
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
		
        // As long as this peer isn't a window we can add it to its heavyweight parent.
        // We need to add the peer to its native parent peer and not necessarily
        // its direct parent as this could be lightweight.
		
        if (!(this instanceof GWindowPeer)) {
            if (parent != null) {
                GContainerPeer parentPeer = (GContainerPeer) GToolkit.getComponentPeer(parent);
                parentPeer.add(this);
            }
        }
        setEnabled(target.isEnabled());
        setVisible(visible);
        setBounds(bounds.x, bounds.y, bounds.width, bounds.height);
    }
	
    /** Decides whether it is possible for this component to have a pixmap background (i.e. a non solid color).
     We allow all components except Window, Frame, Dialog, Panel and Canvas to have pixmaps as the user
     can, and typically does, override the paint for these components. Thus update can be called which will clear
     the background with a solid color for these components. However, we would still like to have support for Gtk
     themes where a button, for example, may hava a pixmap background. */
	
    protected boolean canHavePixmapBackground() {
        return true;
    }
	
    /** Creates this component peer. This requires setting the data field to point to a
     struct GComponentData. */
	
    protected abstract void create();
	
    public void	dispose() {
        if (!(this instanceof GWindowPeer)) {
            Container parent = PeerBasedToolkit.getNativeContainer(target);
            if (parent != null) {
                GContainerPeer parentPeer = (GContainerPeer) GToolkit.getComponentPeer(parent);
                if (parentPeer != null)
                    parentPeer.remove(this);
            }
        }
        disposeNative();
    }
	
    private native void disposeNative();
	
    public void setVisible(boolean b) {
        if (b)
            show();
        else hide();
    }
	
    public native void setEnabled(boolean b);
	
    public void	paint(Graphics g) {
        target.paint(g);
    }
	
    public void update(Graphics g) {
        target.update(g);
    }
	
    public void repaint(long tm, int x, int y, int width, int height) {
        addRepaintArea(x, y, width, height);
        ScreenUpdater.updater.notify(this, tm);
    }
	
    public void		print(Graphics g) {}
	
    public void	setBounds(int x, int y, int width, int height) {
        // Gtk doesn't like setting widget sizes to 0 so we ignore these requests.
		
        if (width == 0 && height == 0)
            return;
        Container nativeContainer = PeerBasedToolkit.getNativeContainer(target);
        // Give the native window peer a chance to translate the position of components
        // added to it if it needs to. This is necessary in the case of a Frame for example
        // where the size of the frame includes the borders add possibly an optional menu
        // bar. The layout manager lays out components relative to the top left of the frame
        // but we may wish to add components to a native container inside the frame. In this case
        // we need to translate all components so their coordinates were relative to the
        // native container inside the frame instead of to the frame itself.
		
        if (nativeContainer != null) {
            GContainerPeer peer = (GContainerPeer) toolkit.getComponentPeer(nativeContainer);
            x += peer.getOriginX();
            y += peer.getOriginY();
        }
        setBoundsNative(x, y, width, height);
    }
	
    native void setBoundsNative(int x, int y, int width, int height);
	
    public void handleEvent(AWTEvent event) {
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
            } finally 	// Always dispose of graphics even if exception ocurrs during paint
            {
                if (g != null)
                    g.dispose();
            }
        } else if (event instanceof MouseEvent) {
            MouseEvent mouseEvent = (MouseEvent) event;
            if (!mouseEvent.isConsumed() && id != MouseEvent.MOUSE_CLICKED) {
                postMouseEventToGtk(mouseEvent);
            }
        } else if (event instanceof KeyEvent) {
            KeyEvent keyEvent = (KeyEvent) event;
            if (!keyEvent.isConsumed() && id != KeyEvent.KEY_TYPED) {
                postKeyEventToGtk(keyEvent);
            }
        }
    }
	
    /** Posts a mouse event back to Gtk.
     After an event has been processed by Java it is posted back to Gtk if it has not been consumed. */
	
    private native void postMouseEventToGtk(MouseEvent event);
	
    /** Posts a key event back to Gtk.
     After an event has been processed by Java it is posted back to Gtk if it has not been consumed. */
	
    private native void postKeyEventToGtk(KeyEvent event);
	
    /** A utility function called from native code to create a byte array of characters
     from a unicode character in UTF-8 format. This is called when the keyChar has been
     modified for a key event. The GDK key event requires a pointer to a sequence of UTF-8
     characters. This could probably be done a lot more efficiently but it is not used often
     and works.... */
	
    private byte[] getUTF8Bytes(char c) {
        try {
            return new String(new char[] {c}
                ).getBytes("UTF-8");
        } catch (UnsupportedEncodingException e) {
            throw new AWTError ("UTF-8 encoding not supported");
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
	
    /** Called by the ScreenUpdater to update the component. */
	
    public void updateClient(Object arg) {
        if (target.isDisplayable()) {
            Rectangle updateRect = null;
            synchronized (this) {
                if (repaintPending) {
                    updateRect = repaintRect;
                    repaintPending = false;
                }
            }
            if (updateRect != null)
                GToolkit.postEvent(new PaintEvent((Component) target, PaintEvent.UPDATE,
                        updateRect));
        }
    }
	
    public native Point	getLocationOnScreen();

    public native Dimension		getPreferredSize();

    public Dimension		getMinimumSize() {
        return getPreferredSize();
    }

    public ColorModel		getColorModel() {
        return toolkit.getColorModel();
    }

    public Toolkit	getToolkit() {
        return toolkit;
    }
	
    public Graphics	getGraphics() {
        return GdkGraphics.createFromComponent(this);
    }
	
    private native void updateWidgetStyle();
	
    public FontMetrics getFontMetrics(Font font) {
        return (FontMetrics) toolkit.getFontPeer(font);
    }
	
    public void setForeground(Color c) {
        // This is required to prevent recursion in some badly written programs where they
        // set a color on the component during a paint method. updateWidgetStyle will also
        // cause a paint event to be posted to the event queue so recursion will occurr.
		
        if (c != null && !c.equals(lastForeground)) {
            lastForeground = c;
            updateWidgetStyle();
        }
    }
	
    public void	setBackground(Color c) {
        // This is required to prevent recursion in some badly written programs where they
        // set a color on the component during a paint method. updateWidgetStyle will also
        // cause a paint event to be posted to the event queue so recursion will occurr.
		
        if (c != null && !c.equals(lastBackground)) {
            lastBackground = c;
            updateWidgetStyle();
        }
    }
	
    public void	setFont(Font f) {
        // This is required to prevent recursion in some badly written programs where they
        // set a font on the component during a paint method. updateWidgetStyle will also
        // cause a paint event to be posted to the event queue so recursion will occurr.
		
        if (f != null && !f.equals(lastFont)) {
            lastFont = f;
            updateWidgetStyle();
        }
    }

    public void setCursor(Cursor cursor) {
        this.cursor = cursor;
        setCursorNative(cursor);
    }
	
    private native void setCursorNative(Cursor cursor);
	
    public native void requestFocus();

    public boolean isFocusTraversable() {
        return false;
    }
	
    public Image createImage(ImageProducer producer) {
        return new GdkImage (producer);
    }
	
    public Image createImage(int width, int height) {
        return new GdkImage (target, width, height);
    }
	
    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
        return GToolkit.prepareScrImage(img, w, h, o);
    }
	
    public int checkImage(Image img, int w, int h, ImageObserver o) {
        return GToolkit.checkScrImage(img, w, h, o);
    }
	
    /**
     * Shows this widget.
     */
    native void show();
	
    /**
     * Hides this widget
     */
    native void hide();

    public void lostOwnership(Clipboard clipboard, Transferable contents) {}
	
    private native void setNativeEvent(InputEvent e, int nativeEvent);
	
    /** Posts a paint event for this component. This is called when an area of the component is exposed.
     The area that needs to be painted is specified by the parameters. */
	
    void postPaintEvent(int x, int y, int width, int height) {
        Rectangle r = new Rectangle (x, y, width, height);
        GToolkit.postEvent(new PaintEvent (target, PaintEvent.PAINT, r));
    }
	
    /* Posts a mouse event for this component. */
	
    void postMouseEvent(int id, long when, int modifiers, int x, int y, int clickCount, boolean popupTrigger, int nativeEvent) {
        MouseEvent e = new MouseEvent (target, id, when, modifiers, x, y, clickCount, popupTrigger);
        setNativeEvent(e, nativeEvent);
        GToolkit.postEvent(e);
    }
	
    /* Posts a key event for this component. */
	
    private void postKeyEvent(int id, long when, int modifiers, int keyCode, char keyChar, int nativeEvent) {
        KeyEvent e = new KeyEvent (target, id, when, modifiers, keyCode, keyChar);
        setNativeEvent(e, nativeEvent);
        GToolkit.postEvent(e);
    }
    /** The Toolkit that created this peer. */
	
    GToolkit toolkit;
    /** Used by native code as a pointer to the Gtk widget. */
	
    int data;
    /** The Component this is the peer for. */
	
    Component target;
    private Cursor cursor;
    private boolean repaintPending;
    private Rectangle repaintRect;
    private Color lastForeground, lastBackground;
    private Font lastFont;
	
    protected byte[] stringToNulMultiByte(String string) {
		byte[] encStringNul = null;
        
		if (string != null) {
			try {
				byte[] encString = string.getBytes("UTF-8");

				encStringNul = new byte[encString.length + 1];
				System.arraycopy(encString, 0, encStringNul, 0, encString.length);
				return encStringNul;
			} catch(Exception e)
			{
				/* Oh well... */
			}
		}
		
        return encStringNul;
    }
	
	
	
    /** Draws a Multichar string on a GTK component - assumes byte array is UTF8 */
	
    private void drawMCString(byte[] string, int x, int y, int gtkData) {
			GFontPeer p = GFontPeer.getFontPeer(lastFont);
			CharsetString[] cs = null;
			
			try {
				cs = p.gpf.makeMultiCharsetString(new String(string, "UTF-8"));
			} catch(Exception e)
			{
				/* We couldn't work out what the string is */
				return;
			}
			
			for(int i=0;i<cs.length;i++)
			{
				byte[] s = new byte[cs[i].length*3];
				int len;
				
				try {
					len = cs[i].fontDescriptor.fontCharset.convertAny(cs[i].charsetChars, cs[i].offset, cs[i].length, s, 0, s.length);
				} catch(Exception e)
				{
					/* FIXME ... */
					continue;
				}
					
				int gdkfont = p.gpf.getGdkFont(cs[i].fontDescriptor);
				
				drawMCStringNative(s, len, gdkfont, x, y, gtkData);
				x+= p.stringWidthNative(s, len, gdkfont);
			}
    }

    private native void drawMCStringNative(byte[] string, int len, int gdkfont, int x, int y, int gtkData);	
}
