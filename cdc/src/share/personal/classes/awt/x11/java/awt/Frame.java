/*
 * @(#)Frame.java	1.10 06/10/10
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

package java.awt;

import java.awt.event.*;
import sun.awt.AppContext;
import java.util.Vector;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import javax.swing.JMenuBar;

/**
 * A Frame is window with additional properties, such as a title bar,
 * a menu bar, a cursor, and an icon image where appropriate.
 * A Personal Profile implementation is not required to support multiple
 * frames. It must allow at least one frame to be created then it may
 * throw an <em>UnsupportedOperationException</em> on subsequent <code>Frame</code> creations.
 * <p>
 * <li>The Title Property
 * The title property can be set when the frame is created and changed
 * at any time. How the title property is displayed and used is platform dependent.
 * <li>The Resizable Property
 * The Resizable property determines if a frame can be resized by the
 * user. Personal Profile and PersonalJava, implementations are not required
 * to support resizable frames.
 * <li>The Cursor Property
 * Not all cursors may be supported
 * <h3>System Properties</h3>
 * <code>java.awt.frame.SupportsMultipleFrames</code>  "true" if the Personal Profile
 *                      implementation supports multiple frames, otherwise "false".
 * <p>
 * <code>java.awt.frame.SupportsResizable</code>       "true" if the Personal Profile
 *                      implementation supports resizable frames, otherwise "false".
 *
 * @version 	1.6, 08/19/02
 * @author 		Nicholas Allen
 * @see WindowEvent
 * @see Window#addWindowListener
 * @since       JDK1.0
 */
public class Frame extends Window implements MenuContainer {
    /* Note: These are being obsoleted;  programs should use the Cursor class
     * variables going forward. See Cursor and Component.setCursor.
     */
	
    /**
     *
     */
    public static final int	DEFAULT_CURSOR = Cursor.DEFAULT_CURSOR;
    /**
     *
     */
    public static final int	CROSSHAIR_CURSOR = Cursor.CROSSHAIR_CURSOR;
    /**
     *
     */
    public static final int	TEXT_CURSOR = Cursor.TEXT_CURSOR;
    /**
     *
     */
    public static final int	WAIT_CURSOR = Cursor.WAIT_CURSOR;
    /**
     *
     */
    public static final int	SW_RESIZE_CURSOR = Cursor.SW_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	SE_RESIZE_CURSOR = Cursor.SE_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	NW_RESIZE_CURSOR = Cursor.NW_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	NE_RESIZE_CURSOR = Cursor.NE_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	N_RESIZE_CURSOR = Cursor.N_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	S_RESIZE_CURSOR = Cursor.S_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	W_RESIZE_CURSOR = Cursor.W_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	E_RESIZE_CURSOR = Cursor.E_RESIZE_CURSOR;
    /**
     *
     */
    public static final int	HAND_CURSOR = Cursor.HAND_CURSOR;
    /**
     *
     */
    public static final int	MOVE_CURSOR = Cursor.MOVE_CURSOR;
    String 	title = "Untitled";
    Image  	icon;
    MenuBar	menuBar;
    boolean	resizable = true;
    boolean     mbManagement = false;   /* used only by the Motif impl. */
    /*
     * The Windows owned by the Frame.
     */
    Vector ownedWindows;
    private static final String base = "frame";
    private static int nameCounter = 0;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 2673458971256075116L;
    /**
     * Constructs a new <code>Frame</code>
     * <h3>Compatibility</h3>
     * A Personal Profile implementation is not required to support multiple
     * Frames (the current Frame must be destroyed before a subsequent Frame
     * can be created). Support for menu objects is independent of the level
     * of Frame support.
     * <p>
     * A PersonalJava implementation, which fully supported Frame, also
     * supported CheckBoxMenuItem, Menu, MenuBar and MenuShortcut.
     * JDK Fully supports multiple frames.
     * @exception UnsupportedOperationException if multiple frames are created
     *            on a platform which only supports a single frame.
     * @see Component#setSize
     * @see Component#setVisible
     * @since JDK1.0
     */
    public Frame() {
        this("");
    }
	
    /**
     * Constructs a new <code>Frame</code>
     * <h3>Compatibility</h3>
     * A Personal Profile implementation is not required to support multiple
     * Frames (the current Frame must be destroyed before a subsequent Frame
     * can be created). Support for menu objects is independent of the level
     * of Frame support.
     * <p>
     * A PersonalJava implementation, which fully supported Frame, also
     * supported CheckBoxMenuItem, Menu, MenuBar and MenuShortcut.
     * JDK Fully supports multiple frames.
     * @param title is the string specifying the frames title.
     * @exception UnsupportedOperationException if multiple frames are created
     *            on a platform which only supports a single frame.
     * @see java.awt.Component#setSize
     * @see java.awt.Component#setVisible
     * @since JDK1.0
     */
    public Frame(String title) {
        this.title = title;
        visible = false;
        setLayout(new BorderLayout());
    }
	
    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }
	
    /**
     * Adds the specified window to the list of windows owned by
     * the frame.
     * @param window the window to be added
     */
    Window addOwnedWindow(Window window) {
        if (window != null) {
            if (ownedWindows == null) {
                ownedWindows = new Vector();
            }
            ownedWindows.addElement(window);
        }
        return window;
    }
	
    /**
     * Removes the specified window from the list of windows owned by
     * the frame.
     * @param window the window to be added
     */
    void removeOwnedWindow(Window window) {
        if (window != null) {
            if (ownedWindows != null) {
                ownedWindows.removeElement(window);
            }
        }
    }
	
    ComponentXWindow createXWindow() {
        return new FrameXWindow(this);
    }
	
    public Graphics getGraphics() {
        Graphics g = super.getGraphics();
        // Clip graphics so cannot draw in menu bar area
		
        if (g != null && menuBar != null) {
            Insets insets = getInsets();
            g.clipRect(0, insets.top, width, height);
        }
        return g;
    }
	
    public void addNotify() {
        super.addNotify();
        MenuBar menuBar = this.menuBar;
        if (menuBar != null)
            menuBar.addNotify();
    }
	
    public void removeNotify() {
        super.removeNotify();
        MenuBar menuBar = this.menuBar;
        if (menuBar != null)
            menuBar.removeNotify();
    }
	
    /**
     * Gets the title of the frame.
     * @return    the title of this frame, or <code>null</code>
     *                if this frame doesn't have a title.
     * @see       java.awt.Frame#setTitle
     * @since     JDK1.0
     */
    public String getTitle() {
        return title;
    }
	
    /**
     * Sets the title for this frame to the specified title.
     * @param    title    the specified title of this frame.
     * @see      java.awt.Frame#getTitle
     * @since    JDK1.0
     */
    public synchronized void setTitle(String title) {
        this.title = title;
        FrameXWindow xwindow = (FrameXWindow) this.xwindow;
        if (xwindow != null) {
            xwindow.setTitle(title);
        }
    }
	
    /**
     * Gets the icon image for this frame.
     * @return    the icon image for this frame, or <code>null</code>
     *                    if this frame doesn't have an icon image.
     * @see       java.awt.Frame#setIconImage
     * @since     JDK1.0
     */
    public Image getIconImage() {
        return icon;
    }
	
    public Insets getInsets() {
        Insets insets = super.getInsets();
        if (menuBar != null)
            insets.top += menuBar.jMenuBar.getHeight();
        return insets;
    }
	
    public void validate() {
        MenuBar menuBar = this.menuBar;
        if (menuBar != null) {
            Insets insets = super.getInsets();
            menuBar.jMenuBar.setBounds(insets.left, insets.top, width - insets.left - insets.right, menuBar.jMenuBar.getPreferredSize().height);
            menuBar.jMenuBar.validate();
        }
        super.validate();
    }
	
    void dispatchEventImpl(AWTEvent e) {
        super.dispatchEventImpl(e);
        // Paint the JMenuBar if a menubar is attatched to this frame.
		
        if (e instanceof PaintEvent) {
            PaintEvent paintEvent = (PaintEvent) e;
            MenuBar menuBar = this.menuBar;
            if (menuBar != null) {
                // Get super graphics because our graphics is clipped to prevent drawing
                // in the menu bar area.
				
                Graphics g = super.getGraphics();
                if (g != null) {
                    Rectangle clip = paintEvent.getUpdateRect();
                    g.clipRect(clip.x, clip.y, clip.width, clip.height);
                    Component c = menuBar.jMenuBar;
                    Graphics cg = g.create(c.x, c.y, c.width, c.height);
                    try {
                        c.paint(cg);
                    } finally {
                        cg.dispose();
                        g.dispose();
                        Toolkit.getDefaultToolkit().sync();
                    }
                }
            }
        }
    }
	
    /**
     * Sets the image to display when this frame is iconized.
     * Not all platforms support the concept of iconizing a window.
     * @param     image the icon image to be displayed
     * @see       java.awt.Frame#getIconImage
     * @since     JDK1.0
     */
    public synchronized void setIconImage(Image image) {
        this.icon = image;
        FrameXWindow xwindow = (FrameXWindow) this.xwindow;
        if (xwindow != null) {
            xwindow.setIconImage(image);
        }
    }
	
    /**
     * Gets the menu bar for this frame.
     * @return    the menu bar for this frame, or <code>null</code>
     *                   if this frame doesn't have a menu bar.
     * @see       java.awt.Frame#setMenuBar
     * @since     JDK1.0
     */
    public MenuBar getMenuBar() {
        return menuBar;
    }
	
    /**
     
     * Sets this Frame's menu bar.
     * <h3>Compatability</h3>
     * In PersonalJava and PersonalProfile, this operation is optional and throws an
     * UnsupportedOperationException if it is not supported.
     * @param     mb the menu bar being set
     * @exception UnsupportedOperationException if it is not supported.
     * @see       java.awt.Frame#getMenuBar
     * @see       java.awt.MenuBar
     * @since     JDK1.0
     */
    public void setMenuBar(MenuBar mb) {
        synchronized (getTreeLock()) {
            if (menuBar == mb) {
                return;
            }
            if (menuBar != null) {
                remove(menuBar);
            }
            menuBar = mb;
            if (mb != null) {
                if (mb.parent != null)
                    mb.parent.remove(mb);
                mb.parent = this;
                if (xwindow != null)
                    mb.addNotify();
            }
            EventQueue.invokeLater(new Runnable() {
                    public void run() {
                        invalidate();
                        validate();
                    }
                }
            );
        }
    }
	
    /**
     * Indicates whether this frame is resizable.
     * By default, all frames are initially resizable.
     * @return    <code>true</code> if the user can resize this frame;
     *                        <code>false</code> otherwise.
     * @see       java.awt.Frame#setResizable
     * @since     JDK1.0
     */
    public boolean isResizable() {
        return resizable;
    }
	
    /**
     * Sets the resizable Property
     * <h3>Compatability</h3>
     * In Personal Profile this method can have no effect. The
     * <code>isResizable()</code> method may be used to verify this.
     * Also the system property java.awt.frame.SupportsResizable is
     * <code>"true"</code> if the platform supports resizable frames.
     * @param resizable if <code>true</code>, the frame becomes resizable,
     *                  otherwise the frame become non resizable.
     * @see      java.awt.Frame#isResizable
     * @since    JDK1.0
     */
    public synchronized void setResizable(boolean resizable) {
        this.resizable = resizable;
        FrameXWindow xwindow = (FrameXWindow) this.xwindow;
        if (xwindow != null) {
            xwindow.setResizable(resizable);
        }
    }
	
    /**
     * Removes the specified menu bar from this frame.
     * @param    m   the menu component to remove.
     * @since    JDK1.0
     */
    public void remove(MenuComponent m) {
        synchronized (getTreeLock()) {
            if (m != null && m == menuBar) {
                menuBar.parent = null;
                menuBar = null;
                if (xwindow != null)
                    m.removeNotify();
            } else {
                super.remove(m);
            }
        }
    }
	
    /**
     * Disposes of the Frame. This method must
     * be called to release the resources that
     * are used for the frame.  All components
     * contained by the frame and all windows
     * owned by the frame will also be destroyed.
     * @since JDK1.0
     */
    public void dispose() {     // synch removed.
        synchronized (getTreeLock()) {
            if (ownedWindows != null) {
                int ownedWindowCount = ownedWindows.size();
                Window ownedWindowCopy[] = new Window[ownedWindowCount];
                ownedWindows.copyInto(ownedWindowCopy);
                for (int i = 0; i < ownedWindowCount; i++) {
                    ownedWindowCopy[i].dispose();
                }
            }
            if (menuBar != null) {
                remove(menuBar);
                menuBar = null;
            }
        }
        super.dispose();
    }
	
    void postProcessKeyEvent(KeyEvent e) {
        if (menuBar != null && menuBar.handleShortcut(e)) {
            e.consume();
            return;
        }
        super.postProcessKeyEvent(e);
    }
	
    /**
     * Returns the parameter String of this Frame.
     */
    protected String paramString() {
        String str = super.paramString();
        if (resizable) {
            str += ",resizable";
        }
        if (title != null) {
            str += ",title=" + title;
        }
        return str;
    }
	
    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>Component.setCursor(Cursor)</code>.
     */
    public synchronized void setCursor(int cursorType) {
        if (cursorType < DEFAULT_CURSOR || cursorType > MOVE_CURSOR) {
            throw new IllegalArgumentException("illegal cursor type");
        }
        setCursor(Cursor.getPredefinedCursor(cursorType));
    }
	
    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>Component.getCursor()</code>.
     */
    public int getCursorType() {
        return (getCursor().getType());
    }
    
    /**
     * Returns an array containing all Frames created by the application.
     * If called from an applet, the array will only include the Frames
     * accessible by that applet.
     * @since 1.2
     */
    public static Frame[] getFrames() {
        synchronized (Frame.class) {
            Frame realCopy[];
            Vector frameList =
                (Vector) AppContext.getAppContext().get(Frame.class);
            if (frameList != null) {
                // Recall that frameList is actually a Vector of WeakReferences
                // and calling get() on one of these references may return
                // null. Make two arrays-- one the size of the Vector
                // (fullCopy with size fullSize), and one the size of all
                // non-null get()s (realCopy with size realSize).
                int fullSize = frameList.size();
                int realSize = 0;
                Frame fullCopy[] = new Frame[fullSize];
                for (int i = 0; i < fullSize; i++) {
                    fullCopy[realSize] = (Frame)
                            (((WeakReference) (frameList.elementAt(i))).get());
                    if (fullCopy[realSize] != null) {
                        realSize++;
                    }
                }
                if (fullSize != realSize) {
                    realCopy = new Frame[realSize];
                    System.arraycopy(fullCopy, 0, realCopy, 0, realSize);
                } else {
                    realCopy = fullCopy;
                }
            } else {
                realCopy = new Frame[0];
            }
            return realCopy;
        }
    }
    /* Serialization support.  If there's a MenuBar we restore
     * its (transient) parent field here.  Likewise for top level
     * windows that are "owned" by this frame.
     */
	
    private int frameSerializedDataVersion = 1;
    private void writeObject(ObjectOutputStream s)
        throws IOException {
        s.defaultWriteObject();
    }
	
    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();
        if (menuBar != null)
            menuBar.parent = this;
        if (ownedWindows != null) {
            for (int i = 0; i < ownedWindows.size(); i++) {
                Window child = (Window) (ownedWindows.elementAt(i));
                child.parent = this;
            }
        }
    }
}
