/*
 * @(#)Window.java	1.12 06/10/10
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
import java.util.Vector;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.lang.reflect.Constructor;
import sun.awt.FocusManagement;
import sun.awt.im.InputContext;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;

/**
 * A <code>Window</code> object is a top-level window with no borders and no
 * menubar. It could be used to implement a pop-up menu.
 * The default layout for a window is <code>BorderLayout</code>.
 * A <code>Window</code> object blocks input to other application
 * windows when it is shown.
 * <p>
 * Windows are capable of generating the following window events:
 * WindowOpened, WindowClosed.
 * <h3>Compatibility</h3>
 * The direct creation of Window objects is an optional feature
 * in PersonalJava and Personal Profile implementations.  If the
 * implementation prohibits the direct creation of Window objects,
 * the constructor throws and <code>UnsupportedOperationException</code>.
 * <h3>System Properties</h3>
 * <code>java.awt.SupportsWindow</code> is set to <code>"true"</code>
 * or <code>"false"</code> indicating if the platform supports direct creation
 * of window objects.
 *
 * @version 	1.7, 08/19/02
 * @author 	Nicholas Allen
 * @see WindowEvent
 * @see #addWindowListener
 * @see java.awt.BorderLayout
 * @since       JDK1.0
 */
public class Window extends Container {
    String      warningString;
    static final int OPENED = 0x01;
    int state;
    transient WindowListener windowListener;
    private transient boolean active;
    transient InputContext inputContext;
    //private FocusManager focusMgr; // For serialization.  SUPERCEDED by:
    transient private FocusManagement focusMgmt;
    private static final String base = "win";
    private static int nameCounter = 0;
    /* Personal Java allows only one top-level window */
    private static  boolean madeOneAlready = false;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 4497834738069338734L;
    Window() {
        setWarningString();
        this.cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        checkSecurity();
        // Compatibility for serialization
        //this.focusMgr = new FocusManager(this);
		
        // Instantiate a pluggable focus manager instead
        try {
            Class paramTypes[] = new Class[1];
            paramTypes[0] = Class.forName("java.awt.Container");
            Object initArgs[] = new Object[1];
            initArgs[0] = this;
            String s = (String) AccessController.doPrivileged(
                    new GetPropertyAction("awt.focusManger"));
            if (s != null) {
                Class  c = Class.forName(s);
                Constructor cons = c.getConstructor(paramTypes);
                this.focusMgmt = (FocusManagement) cons.newInstance(initArgs);
            }
        } catch (Throwable t) {
            System.err.println("Problem instantiating focus manager:" + t);
            t.printStackTrace();
        }
        if (this.focusMgmt == null) {
            this.focusMgmt = new sun.awt.TabFocusManager(this);
        }
        this.visible = false;
        this.foreground = Color.black;
        this.background = Color.lightGray;
        this.font = new Font ("Dialog", Font.PLAIN, 12);
    }
	
    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }
	
    boolean isLightweightWhenDisplayable() {
        return false;
    }
	
    /**
     * Constructs a new invisible window.
     * <p>
     * The window is not initially visible. Call the <code>show</code>
     * method to cause the window to become visible.
     * @param     parent   the main application frame.
     * @exception UnsupportedOperationException if the implementation does
     *             not allow the direct creation of window objects.
     * @see       java.awt.Window#show
     * @see       java.awt.Component#setSize
     * @since     JDK1.0
     */
    public Window(Frame parent) {
        this();
        if (parent == null) {
            throw new IllegalArgumentException("null parent frame");
        }
        this.parent = parent;
        parent.addOwnedWindow(this);
        setLayout(new BorderLayout());
    }
	
    private void checkSecurity() {/*if (this instanceof FileDialog) {
         // FileDialog not allowed if minawt
         throw new UnsupportedOperationException();
         } else if (this instanceof Dialog) {
         // Other Dialog is always OK
         } else if (this instanceof sun.awt.EmbeddedFrame) {
         // EmbeddedFrame is always OK
         } else if (this instanceof Frame) {
         if (madeOneAlready) {
         // if minawt, only one regular Frame is allowed
         throw new UnsupportedOperationException();
         } else {
         madeOneAlready = true;
         }
         } else {
         // We've checked everything else, so this must be a plain Window.
         // Not allowed if minawt
         throw new UnsupportedOperationException();
         }*/}
	
    ComponentXWindow createXWindow() {
        return new WindowXWindow(this);
    }
	
    /**
     * Causes subcomponents of this window to be laid out at their
     * preferred size.
     * @since     JDK1.0
     */
    public void pack() {
        Container parent = this.parent;
        if (parent != null && parent.xwindow == null) {
            parent.addNotify();
        }
        if (xwindow == null) {
            addNotify();
        }
        setSize(getPreferredSize());
        validate();
    }
	
    /**
     * Creates a graphics context for this component. This method will
     * return <code>null</code> if this component is currently not on
     * the screen.
     * @return A graphics context for this component, or <code>null</code>
     *             if it has none.
     * @see       java.awt.Component#paint
     * @since     JDK1.0
     */
    public Graphics getGraphics() {
        WindowXWindow xwindow = (WindowXWindow) this.xwindow;
        if (xwindow != null)
            return new X11Graphics(xwindow);
        return null;
    }
	
    public void validate() {
        if (!valid) {
            super.validate();
            WindowXWindow xwindow = (WindowXWindow) this.xwindow;
            if (xwindow != null) {
                xwindow.setSizeHints(getMinimumSize(), getMaximumSize());
            }
        }
    }
	
    public void update(Graphics g) {
        g.clearRect(0, 0, width, height);
        paint(g);
    }
	
    /**
     * Shows this window, and brings it to the front.
     * <p>
     * If this window is not yet visible, <code>show</code>
     * makes it visible. If this window is already visible,
     * then this method brings it to the front.
     * @see       java.awt.Window#toFront
     * @see       java.awt.Component#setVisible
     * @since     JDK1.0
     */
    public void show() {
        Container parent = this.parent;
        if (parent != null && parent.xwindow == null) {
            parent.addNotify();
        }
        if (xwindow == null) {
            addNotify();
        }
        validate();
        if (visible) {
            toFront();
        } else {
            super.show();
        }
        // If first time shown, generate WindowOpened event
        if ((state & OPENED) == 0) {
            postWindowEvent(WindowEvent.WINDOW_OPENED);
            state |= OPENED;
        }
    }
	
    synchronized void postWindowEvent(int id) {
        if (windowListener != null ||
            (eventMask & AWTEvent.WINDOW_EVENT_MASK) != 0) {
            WindowEvent e = new WindowEvent(this, id);
            Toolkit.getEventQueue().postEvent(e);
        }
    }
	
    /**
     * Disposes of this window. This method must
     * be called to release the resources that
     * are used for the window.
     * @since JDK1.0
     */
    public void dispose() {
        synchronized (getTreeLock()) {
            if (inputContext != null) {
                InputContext toDispose = inputContext;
                inputContext = null;
                toDispose.dispose();
            }
            hide();
            removeNotify();
            if (parent != null) {
                Frame parent = (Frame) this.parent;
                parent.removeOwnedWindow(this);
            }
            postWindowEvent(WindowEvent.WINDOW_CLOSED);
        }
        // allows next instance of Frame to be created
        // as per PJAE spec 1.2.
        if (this instanceof Frame) {
            madeOneAlready = false;
        }
    }
	
    /**
     * Brings this window to the front.
     * Places this window at the top of the stacking order and
     * shows it in front of any other windows.
     * @see       java.awt.Window#toBack
     * @since     JDK1.0
     */
    public void toFront() {
        WindowXWindow xwindow = (WindowXWindow) this.xwindow;
        if (xwindow != null) {
            xwindow.toFront();
        }
    }
	
    /**
     * Sends this window to the back.
     * Places this window at the bottom of the stacking order and
     * makes the corresponding adjustment to other visible windows.
     * @see       java.awt.Window#toFront
     * @since     JDK1.0
     */
    public void toBack() {
        WindowXWindow xwindow = (WindowXWindow) this.xwindow;
        if (xwindow != null) {
            xwindow.toBack();
        }
    }
	
    /**
     * Returns the toolkit of this frame.
     * @return    the toolkit of this window.
     * @see       java.awt.Toolkit
     * @see       java.awt.Toolkit#getDefaultToolkit()
     * @see       java.awt.Component#getToolkit()
     * @since     JDK1.0
     */
    public Toolkit getToolkit() {
        return Toolkit.getDefaultToolkit();
    }
	
    /**
     * Gets the warning string that is displayed with this window.
     * If this window is insecure, the warning string is displayed
     * somewhere in the visible area of the window. A window is
     * insecure if there is a security manager, and the security
     * manager's <code>checkTopLevelWindow</code> method returns
     * <code>false</code> when this window is passed to it as an
     * argument.
     * <p>
     * If the window is secure, then <code>getWarningString</code>
     * returns <code>null</code>. If the window is insecure, this
     * method checks for the system property
     * <code>awt.appletWarning</code>
     * and returns the string value of that property.
     * @return    the warning string for this window.
     * @see       java.lang.SecurityManager#checkTopLevelWindow(java.lang.Object)
     * @since     JDK1.0
     */
    public final String getWarningString() {
        return warningString;
    }
	
    private void setWarningString() {
        warningString = null;
        SecurityManager sm = System.getSecurityManager();
        if (sm != null) {
            if (!sm.checkTopLevelWindow(this)) {
                // make sure the privileged action is only
                // for getting the property! We don't want the
                // above checkTopLevelWindow call to always succeed!
                warningString = (String) AccessController.doPrivileged(
                            new GetPropertyAction("awt.appletWarning",
                                "Warning: Applet Window"));
            }
        }
    }
	
    /**
     * Gets the <code>Locale</code> object that is associated
     * with this window, if the locale has been set.
     * If no locale has been set, then the default locale
     * is returned.
     * @return    the locale that is set for this window.
     * @see       java.util.Locale
     * @since     JDK1.1
     */
	
    public Locale getLocale() {
        if (this.locale == null) {
            return Locale.getDefault();
        }
        return this.locale;
    }
	
    public Insets getInsets() {
        WindowXWindow xwindow = (WindowXWindow) this.xwindow;
        if (xwindow != null)
            return new Insets (xwindow.topBorder, xwindow.leftBorder, xwindow.bottomBorder, xwindow.rightBorder);
        return super.getInsets();
    }
	
    /**
     * Gets the input context for this window. A window always has an input context,
     * which is shared by subcomponents unless they create and set their own.
     * @see Component#getInputContext
     */
	
    synchronized InputContext getInputContext() {
        if (inputContext == null) {
            inputContext = InputContext.getInstance();
        }
        return inputContext;
    }
	
    public void setBackground(Color c) {
        super.setBackground(c);
        WindowXWindow xwindow = (WindowXWindow) this.xwindow;
        if (xwindow != null)
            xwindow.setBackground(c);
    }
	
    /**
     * Set the cursor image to a predefined cursor.
     * @param <code>cursor</code> One of the constants defined
     *            by the <code>Cursor</code> class. If this parameter is null
     *            then the cursor for this window will be set to the type
     *            Cursor.DEFAULT_CURSOR .
     * @see       java.awt.Component#getCursor
     * @see       java.awt.Cursor
     * @since     JDK1.1
     */
    public synchronized void setCursor(Cursor cursor) {
        if (cursor == null) {
            cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        }
        super.setCursor(cursor);
    }
	
    /**
     * Adds the specified window listener to receive window events from
     * this window.
     * @param l the window listener
     */
    public synchronized void addWindowListener(WindowListener l) {
        windowListener = AWTEventMulticaster.add(windowListener, l);
        checkEnableNewEventsOnly(l);
    }
	
    /**
     * Removes the specified window listener so that it no longer
     * receives window events from this window.
     * @param l the window listener
     */
    public synchronized void removeWindowListener(WindowListener l) {
        windowListener = AWTEventMulticaster.remove(windowListener, l);
    }
	
    // NOTE: remove when filtering is handled at lower level
    boolean eventEnabled(AWTEvent e) {
        switch (e.id) {
        case WindowEvent.WINDOW_OPENED:
        case WindowEvent.WINDOW_CLOSING:
        case WindowEvent.WINDOW_CLOSED:
        case WindowEvent.WINDOW_ICONIFIED:
        case WindowEvent.WINDOW_DEICONIFIED:
        case WindowEvent.WINDOW_ACTIVATED:
        case WindowEvent.WINDOW_DEACTIVATED:
            if ((eventMask & AWTEvent.WINDOW_EVENT_MASK) != 0 ||
                windowListener != null) {
                return true;
            }
            return false;

        default:
            break;
        }
        return super.eventEnabled(e);
    }
	
    /**
     * Processes events on this window. If the event is an WindowEvent,
     * it invokes the processWindowEvent method, else it invokes its
     * superclass's processEvent.
     * @param e the event
     */
    protected void processEvent(AWTEvent e) {
        if (e instanceof WindowEvent) {
            processWindowEvent((WindowEvent) e);
            return;
        }
        super.processEvent(e);
    }
	
    /**
     * Processes window events occurring on this window by
     * dispatching them to any registered WindowListener objects.
     * NOTE: This method will not be called unless window events
     * are enabled for this component; this happens when one of the
     * following occurs:
     * a) A WindowListener object is registered via addWindowListener()
     * b) Window events are enabled via enableEvents()
     * @see Component#enableEvents
     * @param e the window event
     */
    protected void processWindowEvent(WindowEvent e) {
        if (windowListener != null) {
            switch (e.getID()) {
            case WindowEvent.WINDOW_OPENED:
                windowListener.windowOpened(e);
                break;

            case WindowEvent.WINDOW_CLOSING:
                windowListener.windowClosing(e);
                break;

            case WindowEvent.WINDOW_CLOSED:
                windowListener.windowClosed(e);
                break;

            case WindowEvent.WINDOW_ICONIFIED:
                windowListener.windowIconified(e);
                break;

            case WindowEvent.WINDOW_DEICONIFIED:
                windowListener.windowDeiconified(e);
                break;

            case WindowEvent.WINDOW_ACTIVATED:
                windowListener.windowActivated(e);
                break;

            case WindowEvent.WINDOW_DEACTIVATED:
                windowListener.windowDeactivated(e);
                break;

            default:
                break;
            }
        }
    }
	
    void preProcessKeyEvent(KeyEvent e) {
        // Dump the list of child windows to System.out.
        if (e.isActionKey() && e.getKeyCode() == KeyEvent.VK_F1 &&
            e.isControlDown() && e.isShiftDown()) {
            list(System.out, 0);
        }
    }
	
    void postProcessKeyEvent(KeyEvent e) {
        if (focusMgmt.handleKeyEvent(e)) {
            e.consume();
            return;
        }
    }
	
    boolean isActive() {
        return active;
    }
	
    public void repaint(long tm, int x, int y, final int width, final int height) {
        if (tm == 0)
            Toolkit.getEventQueue().postEvent(new PaintEvent (this, PaintEvent.UPDATE, new Rectangle(x, y, width, height)));
        else {
            final int x1 = x;
            final int y1 = y;
            updateTimer.schedule(new TimerTask() {
                    public void run() {
                        Toolkit.getEventQueue().postEvent(new PaintEvent (Window.this, PaintEvent.UPDATE, new Rectangle(x1, y1, width, height)));
                    }
                }, tm);
        }
    }
	
    void setFocusOwner(Component c) {
        Component oldFocusOwner = focusMgmt.getFocusOwner();
        if (oldFocusOwner != c) {
            focusMgmt.setFocusOwner(c);
            if (oldFocusOwner != null)
                Toolkit.getEventQueue().postEvent(new FocusEvent (oldFocusOwner, FocusEvent.FOCUS_LOST, false));
            Toolkit.getEventQueue().postEvent(new FocusEvent (c, FocusEvent.FOCUS_GAINED, !active));
        }
    }
	
    void transferFocus(Component base) {
        nextFocus(base);
    }
	
    /**
     * Returns the child component of this Window which has focus if and
     * only if this Window is active.
     * @return the component with focus, or null if no children have focus
     * assigned to them.
     */
    public Component getFocusOwner() {
        //System.out.println("Window.getFocusOwner(), active:"+active+", owner:"+focusMgmt.getFocusOwner());
        if (active)
            return focusMgmt.getFocusOwner();
        else
            return null;
    }
	
    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>transferFocus(Component)</code>.
     */
    void nextFocus(Component base) {
        focusMgmt.focusNext(base);
    }
	
    /*
     * Dispatches an event to this window or one of its sub components.
     * @param e the event
     */
    void dispatchEventImpl(AWTEvent e) {
        Thread.yield();
        Component focusOwner;
        switch (e.id) {
        case ComponentEvent.COMPONENT_RESIZED:
            int width = this.width;
            int height = this.height;
            if (width != lastResizeWidth || height != lastResizeHeight) {
                invalidate();
                validate();
                repaint();
                lastResizeWidth = width;
                lastResizeHeight = height;
            }
            break;
				
        case WindowEvent.WINDOW_ACTIVATED:
            active = true;
            focusOwner = focusMgmt.getFocusOwner();
            if (focusOwner == null)
                setFocusOwner(this);
            else
                Toolkit.getEventQueue().postEvent(new FocusEvent (focusOwner, FocusEvent.FOCUS_GAINED, false));
            break;
				
        case WindowEvent.WINDOW_DEACTIVATED:
            active = false;
            focusOwner = focusMgmt.getFocusOwner();
            if (focusOwner != null)
                Toolkit.getEventQueue().postEvent(new FocusEvent (focusOwner, FocusEvent.FOCUS_LOST, true));
            break;
				
        case KeyEvent.KEY_PRESSED:
        case KeyEvent.KEY_RELEASED:
        case KeyEvent.KEY_TYPED:
            focusOwner = focusMgmt.getFocusOwner();
            KeyEvent keyEvent = (KeyEvent) e;
            if (focusOwner != null && focusOwner != this)
                focusOwner.dispatchEvent(new KeyEvent (focusOwner,
                        keyEvent.id,
                        keyEvent.getWhen(),
                        keyEvent.getModifiers(),
                        keyEvent.getKeyCode(),
                        keyEvent.getKeyChar()));
            break;
				
        default:
            break;
        }
        super.dispatchEventImpl(e);
    }
	
    /**
     * @deprecated As of JDK version 1.1
     * replaced by <code>dispatchEvent(AWTEvent)</code>.
     */
    public boolean postEvent(Event e) {
        if (handleEvent(e)) {
            e.consume();
            return true;
        }
        return false;
    }
	
    /**
     * Checks if this Window is showing on screen.
     * @see java.awt.Component#setVisible(boolean)
     */
    public boolean isShowing() {
        return visible;
    }
    /* Serialization support.
     */
	
    private int windowSerializedDataVersion = 1;
    private void writeObject(ObjectOutputStream s)
        throws IOException {
        s.defaultWriteObject();
        AWTEventMulticaster.save(s, windowListenerK, windowListener);
        s.writeObject(null);
    }
	
    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();
        checkSecurity();
        Object keyOrNull;
        while (null != (keyOrNull = s.readObject())) {
            String key = ((String) keyOrNull).intern();
            if (windowListenerK == key)
                addWindowListener((WindowListener) (s.readObject()));
            else // skip value for unrecognized key
                s.readObject();
        }
        setWarningString();
    }
    private int lastResizeWidth = -1;
    private int lastResizeHeight = -1;
} // class Window
