/*
 * @(#)PeerBasedToolkit.java	1.15 04/12/20
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
 * Warning :
 * Two versions of this file exist in this workspace.
 * One for Personal Basis, and one for Personal Profile.
 * Don't edit the wrong one !!!
 */

package sun.awt;

import java.awt.*;
import java.awt.image.*;
import sun.awt.peer.*;
import java.util.Hashtable;

import java.net.URL;
import sun.misc.WeakCache;
import sun.awt.image.ByteArrayImageSource;
import sun.awt.image.FileImageSource;
import sun.awt.image.URLImageSource;


/** The base class for any toolkits whose implementation is peer based. Peers represent
 the native component for an AWT component and are created only when needed. The AWT
 implementation provided in the <code>java.awt</code> package assumes the toolkit to
 be peer based. To use a toolkit with this AWT implementation it is necessary for the
 toolkit class to be a subclass of this class. If this is not the case then a
 ClassCastException will be thrown at run time by the AWT implementation. There is no
 requirement, however, that AWT be implemented using peers. To provide a peerless
 implementation of AWT it will be necessary to write a new implementation of the
 <code>java.awt</code> classes.

 @author Nicholas Allen */

public abstract class PeerBasedToolkit extends SunToolkit {
    /**
     * Creates this toolkit's implementation of <code>Frame</code> using
     * the specified peer interface.
     * @param     target the frame to be implemented.
     * @return    this toolkit's implementation of <code>Frame</code>.
     * @see       java.awt.Frame
     * @see       sun.awt.peer.FramePeer
     * @since     JDK1.0
     */
    public abstract FramePeer  	createFrame(Frame target);
	
    /** Gets the peer for the supplied component. */
    public static native ComponentPeer getComponentPeer(Component c);
	
    /** Gets the peer for the supplied menu component. */
    public static native MenuComponentPeer getMenuComponentPeer(MenuComponent m);

    /**
     * Creates this toolkit's implementation of <code>Canvas</code> using
     * the specified peer interface.
     * @param     target the canvas to be implemented.
     * @return    this toolkit's implementation of <code>Canvas</code>.
     * @see       java.awt.Canvas
     * @see       sun.awt.peer.CanvasPeer
     * @since     JDK1.0
     */
    public abstract CanvasPeer 	createCanvas(Canvas target);

    /**
     * Creates this toolkit's implementation of <code>Panel</code> using
     * the specified peer interface.
     * @param     target the panel to be implemented.
     * @return    this toolkit's implementation of <code>Panel</code>.
     * @see       java.awt.Panel
     * @see       sun.awt.peer.PanelPeer
     * @since     JDK1.0
     */
    public abstract PanelPeer  	createPanel(Panel target);

    /**
     * Creates this toolkit's implementation of <code>Window</code> using
     * the specified peer interface.
     * @param     target the window to be implemented.
     * @return    this toolkit's implementation of <code>Window</code>.
     * @see       java.awt.Window
     * @see       sun.awt.peer.WindowPeer
     * @since     JDK1.0
     */
    public abstract WindowPeer  	createWindow(Window target);

    /**
     * Creates this toolkit's implementation of <code>Dialog</code> using
     * the specified peer interface.
     * @param     target the dialog to be implemented.
     * @return    this toolkit's implementation of <code>Dialog</code>.
     * @see       java.awt.Dialog
     * @see       sun.awt.peer.DialogPeer
     * @since     JDK1.0
     */
    public abstract DialogPeer  	createDialog(Dialog target);

    /**
     * Creates a peer for a component or container.  This peer is windowless
     * and allows the Component and Container classes to be extended directly
     * to create windowless components that are defined entirely in java.
     *
     * @param target The Component to be created.
     */
    public sun.awt.peer.LightweightPeer createComponent(Component target) {
        return new sun.awt.LightweightPeer(target);
    }
    
    /**
     * Give native peers the ability to query the native container
     * given a native component (eg the direct parent may be lightweight).
     */
    public static Container getNativeContainer(Component c) {
        Container p = c.getParent();
        while (p != null && getComponentPeer(p) instanceof sun.awt.peer.LightweightPeer) {
            p = p.getParent();
        }
        return p;
    }
	
    /**
     * Creates this toolkit's implementation of <code>MenuBar</code> using
     * the specified peer interface.
     * @param     target the menu bar to be implemented.
     * @return    this toolkit's implementation of <code>MenuBar</code>.
     * @see       java.awt.MenuBar
     * @see       sun.awt.peer.MenuBarPeer
     * @since     JDK1.0
     */
    public abstract MenuBarPeer  	createMenuBar(MenuBar target);

    /**
     * Creates this toolkit's implementation of <code>Menu</code> using
     * the specified peer interface.
     * @param     target the menu to be implemented.
     * @return    this toolkit's implementation of <code>Menu</code>.
     * @see       java.awt.Menu
     * @see       sun.awt.peer.MenuPeer
     * @since     JDK1.0
     */
    public abstract MenuPeer  	createMenu(Menu target);

    /**
     * Creates this toolkit's implementation of <code>PopupMenu</code> using
     * the specified peer interface.
     * @param     target the popup menu to be implemented.
     * @return    this toolkit's implementation of <code>PopupMenu</code>.
     * @see       java.awt.PopupMenu
     * @see       sun.awt.peer.PopupMenuPeer
     * @since     JDK1.1
     */
    public abstract PopupMenuPeer	createPopupMenu(PopupMenu target);

    /**
     * Creates this toolkit's implementation of <code>MenuItem</code> using
     * the specified peer interface.
     * @param     target the menu item to be implemented.
     * @return    this toolkit's implementation of <code>MenuItem</code>.
     * @see       java.awt.MenuItem
     * @see       sun.awt.peer.MenuItemPeer
     * @since     JDK1.0
     */
    public abstract MenuItemPeer  	createMenuItem(MenuItem target);

    /**
     * Creates this toolkit's implementation of <code>FileDialog</code> using
     * the specified peer interface.
     * @param     target the file dialog to be implemented.
     * @return    this toolkit's implementation of <code>FileDialog</code>.
     * @see       java.awt.FileDialog
     * @see       sun.awt.peer.FileDialogPeer
     * @since     JDK1.0
     */
    public abstract FileDialogPeer	createFileDialog(FileDialog target);

    /**
     * Creates this toolkit's implementation of <code>CheckboxMenuItem</code> using
     * the specified peer interface.
     * @param     target the checkbox menu item to be implemented.
     * @return    this toolkit's implementation of <code>CheckboxMenuItem</code>.
     * @see       java.awt.CheckboxMenuItem
     * @see       sun.awt.peer.CheckboxMenuItemPeer
     * @since     JDK1.0
     */
    public abstract CheckboxMenuItemPeer	createCheckboxMenuItem(CheckboxMenuItem target);

    /**
     * Creates this toolkit's implementation of <code>Font</code> using
     * the specified peer interface.
     * @param     target the font to be implemented.
     * @return    this toolkit's implementation of <code>Font</code>.
     * @see       java.awt.Font
     * @see       sun.awt.peer.FontPeer
     * @since     JDK1.0
     */
    public abstract FontPeer getFontPeer(Font target);
	
    /**
     * Creates this toolkit's implementation of <code>Button</code> using
     * the specified peer interface.
     * @param     target the button to be implemented.
     * @return    this toolkit's implementation of <code>Button</code>.
     * @see       java.awt.Button
     * @see       sun.awt.peer.ButtonPeer
     * @since     JDK1.0
     */
    public abstract ButtonPeer 	createButton(Button target);

    /**
     * Creates this toolkit's implementation of <code>TextField</code> using
     * the specified peer interface.
     * @param     target the text field to be implemented.
     * @return    this toolkit's implementation of <code>TextField</code>.
     * @see       java.awt.TextField
     * @see       sun.awt.peer.TextFieldPeer
     * @since     JDK1.0
     */
    public abstract TextFieldPeer 	createTextField(TextField target);

    /**
     * Creates this toolkit's implementation of <code>Label</code> using
     * the specified peer interface.
     * @param     target the label to be implemented.
     * @return    this toolkit's implementation of <code>Label</code>.
     * @see       java.awt.Label
     * @see       sun.awt.peer.LabelPeer
     * @since     JDK1.0
     */
    public abstract LabelPeer 	createLabel(Label target);

    /**
     * Creates this toolkit's implementation of <code>List</code> using
     * the specified peer interface.
     * @param     target the list to be implemented.
     * @return    this toolkit's implementation of <code>List</code>.
     * @see       java.awt.List
     * @see       sun.awt.peer.ListPeer
     * @since     JDK1.0
     */
    public abstract ListPeer 	createList(List target);

    /**
     * Creates this toolkit's implementation of <code>Checkbox</code> using
     * the specified peer interface.
     * @param     target the check box to be implemented.
     * @return    this toolkit's implementation of <code>Checkbox</code>.
     * @see       java.awt.Checkbox
     * @see       sun.awt.peer.CheckboxPeer
     * @since     JDK1.0
     */
    public abstract CheckboxPeer 	createCheckbox(Checkbox target);

    /**
     * Creates this toolkit's implementation of <code>Scrollbar</code> using
     * the specified peer interface.
     * @param     target the scroll bar to be implemented.
     * @return    this toolkit's implementation of <code>Scrollbar</code>.
     * @see       java.awt.Scrollbar
     * @see       sun.awt.peer.ScrollbarPeer
     * @since     JDK1.0
     */
    public abstract ScrollbarPeer 	createScrollbar(Scrollbar target);

    /**
     * Creates this toolkit's implementation of <code>ScrollPane</code> using
     * the specified peer interface.
     * @param     target the scroll pane to be implemented.
     * @return    this toolkit's implementation of <code>ScrollPane</code>.
     * @see       java.awt.ScrollPane
     * @see       sun.awt.peer.ScrollPanePeer
     * @since     JDK1.1
     */
    public abstract ScrollPanePeer     createScrollPane(ScrollPane target);

    /**
     * Creates this toolkit's implementation of <code>TextArea</code> using
     * the specified peer interface.
     * @param     target the text area to be implemented.
     * @return    this toolkit's implementation of <code>TextArea</code>.
     * @see       java.awt.TextArea
     * @see       sun.awt.peer.TextAreaPeer
     * @since     JDK1.0
     */
    public abstract TextAreaPeer  	createTextArea(TextArea target);

    /**
     * Creates this toolkit's implementation of <code>Choice</code> using
     * the specified peer interface.
     * @param     target the choice to be implemented.
     * @return    this toolkit's implementation of <code>Choice</code>.
     * @see       java.awt.Choice
     * @see       sun.awt.peer.ChoicePeer
     * @since     JDK1.0
     */
    public abstract ChoicePeer	createChoice(Choice target);
    // mapping of components to peers, Hashtable<Component,Peer>
    protected static final Hashtable peerMap = new Hashtable();
    /*
     * Fetch the peer associated with the given target (as specified
     * in the peer creation method).  This can be used to determine
     * things like what the parent peer is.  If the target is null
     * or the target can't be found (either because the a peer was
     * never created for it or the peer was disposed), a null will
     * be returned.
     */
    protected static Object targetToPeer(Object target) {
        if (target != null) {
            return peerMap.get(target);
        }
        return null;
    }

    protected static void targetDisposedPeer(Object target, Object peer) {
        if (target != null && peer != null) {
            synchronized (peerMap) {
                if (peerMap.get(target) == peer) {
                    peerMap.remove(target);
                }
            }
        }
    }

// Begin of: PBP/PP [6262553]
// Take security fixes from J2SE SunToolkit.java into PBP/PP's SunToolkit.java.

//    /* These createImage/getImage impls are brought over from SunToolkit */
//    public java.awt.Image createImage(String filename) {
//        return createImage(new FileImageSource(filename));
//    }
//                                                                             
//    /* These createImage/getImage impls are brought over from SunToolkit */
//    public java.awt.Image createImage(URL url) {
//        return createImage(new URLImageSource(url));
//    }
//                                                                             
//    /* These createImage/getImage impls are brought over from SunToolkit */
//    public java.awt.Image createImage(byte[] data, int offset, int length) {
//        return createImage(new ByteArrayImageSource(data, offset, length));
//    }
//
//    /* These createImage/getImage impls are brought over from SunToolkit */
//    public java.awt.Image getImage(String filename) {
//        return getImageFromCache(filename);
//    }
//
//    /* These createImage/getImage impls are brought over from SunToolkit */
//    public java.awt.Image getImage(URL url) {
//        return getImageFromCache(url);
//    }
//
//    private Hashtable imageMap = new WeakCache();
//    private synchronized java.awt.Image getImageFromCache(Object key) {
//        java.awt.Image img = (java.awt.Image) imageMap.get(key);
//        if (img == null) {
//            /* make sure we can create the image */
//            img = (key instanceof URL)
//                    ? createImage((URL) key)
//                    : createImage((String) key);
//            if (img != null) {
//                imageMap.put(key, img);
//            }
//        }
//        return img;
//    }

// End of: PBP/PP [6262553]

    /**
     * Returns the names of the available fonts in this toolkit.<p>
     * For 1.1, the following font names are deprecated (the replacement
     * name follows):
     * <ul>
     * <li>TimesRoman (use Serif)
     * <li>Helvetica (use SansSerif)
     * <li>Courier (use Monospaced)
     * </ul><p>
     * The ZapfDingbats font is also deprecated in 1.1, but only as a
     * separate fontname.  Unicode defines the ZapfDingbat characters
     * starting at \u2700, and as of 1.1 Java supports those characters.
     * @return    the names of the available fonts in this toolkit.
     * @since     JDK1.0
     */
    public String[] getFontList() {
        String[] hardwiredFontList = {
                "Dialog", "SansSerif", "Serif", "Monospaced", "DialogInput"
                                                                                    
                // -- Obsolete font names from 1.0.2.  It was decided that
                // -- getFontList should not return these old names:
                // , "Helvetica", "TimesRoman", "Courier", "ZapfDingbats"
            };
        return hardwiredFontList;
    }

 
}

/**
 * Implements the LightweightPeer interface for use in lightweight components
 * that have no native window associated with them.  This gets created by
 * default in Component so that Component and Container can be directly
 * extended to create useful components written entirely in java.  These
 * components must be hosted somewhere higher up in the component tree by a
 * native container (such as a Frame).
 *
 * This implementation provides no useful semantics and serves only as a
 * marker.  One could provide alternative implementations in java that do
 * something useful for some of the other peer interfaces to minimize the
 * native code.
 *
 * @author Timothy Prinzing
 */
class LightweightPeer implements sun.awt.peer.LightweightPeer {
    public LightweightPeer(Component target) {}

    public boolean isFocusTraversable() {
        return false;
    }

    public boolean isDoubleBuffered() {
        return false;
    }

    public void setVisible(boolean b) {}

    public void show() {}

    public void hide() {}

    public void setEnabled(boolean b) {}

    public void enable() {}

    public void disable() {}

    public void paint(Graphics g) {}

    public void repaint(long tm, int x, int y, int width, int height) {}

    public void print(Graphics g) {}

    public void clearBackground(Graphics g) {}

    public void setBounds(int x, int y, int width, int height) {}

    public void reshape(int x, int y, int width, int height) {}

    public void handleEvent(java.awt.AWTEvent arg0) {}

    public Dimension getPreferredSize() {
        return new Dimension(1, 1);
    }

    public Dimension getMinimumSize() {
        return new Dimension(1, 1);
    }

    public java.awt.Toolkit getToolkit() {
        return null;
    }

    public ColorModel getColorModel() {
        return null;
    }

    public Graphics getGraphics() {
        return null;
    }

    public FontMetrics	getFontMetrics(Font font) {
        return null;
    }

    public void dispose() {// no native code
    }

    public void setForeground(Color c) {}

    public void setBackground(Color c) {}

    public void setFont(Font f) {}

    public void setCursor(Cursor cursor) {}

    public boolean requestFocus(Component child, Window parent,
                                boolean temporary,
                                boolean focusedWindowChangeAllowed,
                                long time) {
        return false;
    }

    public Image createImage(ImageProducer producer) {
        return null;
    }

    public Image createImage(int width, int height) {
        return null;
    }

    public VolatileImage createVolatileImage(int width, int height) {
        return null;
    }

    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
        return false;
    }

    public int	checkImage(Image img, int w, int h, ImageObserver o) {
        return 0;
    }

    public Dimension preferredSize() {
        return getPreferredSize();
    }

    public Dimension minimumSize() {
        return getMinimumSize();
    }

    public Point getLocationOnScreen() {
        return null;
    }
    public void setFocusable(boolean focusable) {}
}
