/*
 * @(#)Window.java	1.115 06/10/10
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

import sun.awt.peer.WindowPeer;
import sun.awt.PeerBasedToolkit;
import java.awt.im.InputContext;
import java.awt.event.*;
import java.util.Vector;
import java.util.Locale;
import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.io.OptionalDataException;
import java.lang.reflect.Constructor;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;
import java.util.Set;
import java.util.EventListener;
import java.awt.AWTEventMulticaster;

/**
 * A <code>Window</code> object is a top-level window with no borders and no
 * menubar.  
 * The default layout for a window is <code>BorderLayout</code>.
 * <p>
 * A window must have either a frame, dialog, or another window defined as its
 * owner when it's constructed. 
 * <p>
 * In a multi-screen environment, you can create a <code>Window</code>
 * on a different screen device by constructing the <code>Window</code>
 * with {@link #Window(Window, GraphicsConfiguration)}.  The 
 * <code>GraphicsConfiguration</code> object is one of the 
 * <code>GraphicsConfiguration</code> objects of the target screen device.  
 * <p>
 * In a virtual device multi-screen environment in which the desktop 
 * area could span multiple physical screen devices, the bounds of all
 * configurations are relative to the virtual device coordinate system.  
 * The origin of the virtual-coordinate system is at the upper left-hand 
 * corner of the primary physical screen.  Depending on the location of
 * the primary screen in the virtual device, negative coordinates are 
 * possible, as shown in the following figure.
 * <p>
 * <img src="doc-files/MultiScreen.gif"
 * ALIGN=center HSPACE=10 VSPACE=7>
 * <p>  
 * In such an environment, when calling <code>setLocation</code>, 
 * you must pass a virtual coordinate to this method.  Similarly,
 * calling <code>getLocationOnScreen</code> on a <code>Window</code> returns 
 * virtual device coordinates.  Call the <code>getBounds</code> method 
 * of a <code>GraphicsConfiguration</code> to find its origin in the virtual
 * coordinate system.
 * <p>
 * The following code sets the location of a <code>Window</code> 
 * at (10, 10) relative to the origin of the physical screen
 * of the corresponding <code>GraphicsConfiguration</code>.  If the 
 * bounds of the <code>GraphicsConfiguration</code> is not taken 
 * into account, the <code>Window</code> location would be set 
 * at (10, 10) relative to the virtual-coordinate system and would appear
 * on the primary physical screen, which might be different from the
 * physical screen of the specified <code>GraphicsConfiguration</code>.
 *
 * <pre>
 *	Window w = new Window(Window owner, GraphicsConfiguration gc);
 *	Rectangle bounds = gc.getBounds();
 *	w.setLocation(10 + bounds.x, 10 + bounds.y);
 * </pre>
 *
 * <p>
 * Windows are capable of generating the following window events:
 * WindowOpened, WindowClosed.
 *
 * @version 	@(#)Window.java	1.100 02/08/21
 * @author 	Sami Shaio
 * @author 	Arthur van Hoff
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
    transient InputContext inputContext;

    // 6182409: Window.pack does not work correctly.
    // See also Component.isPacked.
    transient boolean beforeFirstShow = true;

    /**
     * Unused. Maintained for serialization backward-compatibility.
     *
     * @serial
     * @since 1.2
     */
    private transient FocusManager focusMgr;

    private static final String base = "win";
    private static int nameCounter = 0;

    /**
     * A vector containing all the windows this
     * window currently owns.
     */
    transient Vector ownedWindows = new Vector();
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 4497834738069338734L;
    Window() {
        this((GraphicsConfiguration) null);
    }

    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
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
        this(parent, (GraphicsConfiguration) null);        
    }

    public Window(Window owner) {
        this(owner, (GraphicsConfiguration) null);
    }
    
    Window(GraphicsConfiguration gc) {
        setWarningString();
        this.cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        checkSecurity();
        // Get default graphics configuration if gc == null
        if (gc == null) {
            this.graphicsConfig =
                    GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration();
        } else {
            this.graphicsConfig = gc;
        }          
        // Compatibility for serialization
        this.focusMgr = new FocusManager();
        this.visible = false;
        setLayout(new BorderLayout());       
    }

    public Window(Window owner, GraphicsConfiguration gc) {
        this(gc);
        if (owner == null) {
            throw new IllegalArgumentException("null parent window");
        }
        this.parent = owner;
        owner.addOwnedWindow(this);        
    }
    
    /**
     * Adds the specified window to the list of windows owned by
     * the frame.
     * @param window the window to be added
     */
    Window addOwnedWindow(Window window) {
        if (window != null) {
            ownedWindows.addElement(window);
        }
        return window;
    }
    
    /**
     * Hide this Window, its subcomponents, and all of its owned children. 
     * The Window and its subcomponents can be made visible again
     * with a call to <code>show</code>. 
     * </p>
     * @see Window#show
     * @see Window#dispose
     */
    public void hide() {
        synchronized (ownedWindows) {
            for (int i = 0; i < ownedWindows.size(); i++) {
                Window child = (Window) ownedWindows.elementAt(i);
                if (child != null) {
                    child.hide();
                }
            }
        }
        super.hide();
    }    

    // added to fix bug 6186499 - this is in J2SE; we missed in when bringing
    // in focus mgmt changes - we don't want to clear the focus owner for a 
    // window on hide
    final void clearMostRecentFocusOwnerOnHide() {
        /* do nothing */
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

    void connectOwnedWindow(Window child) {
        child.parent = this;
	addOwnedWindow(child);
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

    /**
     * Creates the Window's peer.  The peer allows us to modify the
     * appearance of the Window without changing its functionality.
     */
    public void addNotify() {
        synchronized (getTreeLock()) {
            if (peer == null) {
                peer = ((PeerBasedToolkit) getToolkit()).createWindow(this);
            }
            super.addNotify();
        }
    }

    /**
     * Causes subcomponents of this window to be laid out at their
     * preferred size.
     * @since     JDK1.0
     */
    public void pack() {
        Container parent = this.parent;
        if (parent != null && parent.peer == null) {
            parent.addNotify();
        }
        if (peer == null) {
            addNotify();
        }

        // 6182409: Window.pack does not work correctly.
        if (peer != null) {
            setSize(getPreferredSize());
        }
        if (beforeFirstShow) {
            isPacked = true;
        }
        validate();
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
        if (parent != null && parent.peer == null) {
            parent.addNotify();
        }
        if (peer == null) {
            addNotify();
        }
        validate();
        if (visible) {
            toFront();
        } else {
            // 6182409: Window.pack does not work correctly.
            beforeFirstShow = false;
            super.show();
        }
        // If first time shown, generate WindowOpened event
        if ((state & OPENED) == 0) {
            //postWindowEvent(WindowEvent.WINDOW_OPENED);
            WindowEvent e = new WindowEvent(this, WindowEvent.WINDOW_OPENED);
            Toolkit.getEventQueue().postEvent(e);
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
            synchronized (ownedWindows) {
                for (int i = 0; i < ownedWindows.size(); i++) {
                    Window child = (Window) (ownedWindows.elementAt(i));
                    if (child != null) {
                        child.dispose();
                    }
                }
            }          
            hide();
            // 6182409: Window.pack does not work correctly.
            beforeFirstShow = true;
            removeNotify();
            if (parent != null) {
                Window parent = (Window) this.parent;
                parent.removeOwnedWindow(this);
            }             
            postWindowEvent(WindowEvent.WINDOW_CLOSED);
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
        WindowPeer peer = (WindowPeer) this.peer;
        if (peer != null) {
            peer.toFront();
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
        WindowPeer peer = (WindowPeer) this.peer;
        if (peer != null) {
            peer.toBack();
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
                                "Java Applet Window"));
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

    /**
     * Gets the input context for this window. A window always has an input context,
     * which is shared by subcomponents unless they create and set their own.
     * @see Component#getInputContext
     */

    public synchronized InputContext getInputContext() {
        if (inputContext == null) {
            inputContext = InputContext.getInstance();
        }
        return inputContext;
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
        newEventsOnly = true;
    }

    /**
     * Removes the specified window listener so that it no longer
     * receives window events from this window.
     * @param l the window listener
     */
    public synchronized void removeWindowListener(WindowListener l) {
        windowListener = AWTEventMulticaster.remove(windowListener, l);
    }

    /**
     * Returns an array of all the window listeners
     * registered on this window.
     *
     * @return all of this window's <code>WindowListener</code>s
     *         or an empty array if no window
     *         listeners are currently registered
     *
     * @see #addWindowListener
     * @see #removeWindowListener
     * @since 1.4
     */
    public synchronized WindowListener[] getWindowListeners() {
        return (WindowListener[]) AWTEventMulticaster.getListeners(
                                  (EventListener)windowListener,
                                  WindowListener.class);
    }

    boolean eventEnabled(AWTEvent e) {
        switch(e.id) {
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
        case WindowEvent.WINDOW_GAINED_FOCUS:
        case WindowEvent.WINDOW_LOST_FOCUS:
            if ((eventMask & AWTEvent.WINDOW_FOCUS_EVENT_MASK) != 0 ||
                windowFocusListener != null) {
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
            switch (e.getID()) {
            case WindowEvent.WINDOW_OPENED:
            case WindowEvent.WINDOW_CLOSING:
            case WindowEvent.WINDOW_CLOSED:
            case WindowEvent.WINDOW_ICONIFIED:
            case WindowEvent.WINDOW_DEICONIFIED:
            case WindowEvent.WINDOW_ACTIVATED:
            case WindowEvent.WINDOW_DEACTIVATED:
                processWindowEvent((WindowEvent)e);
                break;
            case WindowEvent.WINDOW_GAINED_FOCUS:
            case WindowEvent.WINDOW_LOST_FOCUS:
                processWindowFocusEvent((WindowEvent)e);
                break;
            default:
                break;
            }
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
        // Do nothing.
    }

    public boolean isActive() {
        return (KeyboardFocusManager.getCurrentKeyboardFocusManager().
        	getActiveWindow() == this);
    }

    /**
     * Returns the child component of this Window which has focus if and
     * only if this Window is active.
     * @return the component with focus, or null if no children have focus
     * assigned to them.
     */
    public Component getFocusOwner() {
        return (isFocused()) ? 
            KeyboardFocusManager.getCurrentKeyboardFocusManager().getFocusOwner()
            : null;
    }

    /*
     * Dispatches an event to this window or one of its sub components.
     * @param e the event
     */
    void dispatchEventImpl(AWTEvent e) {
        if (e.getID() == ComponentEvent.COMPONENT_RESIZED) {
            invalidate();
            validate();
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

	synchronized (ownedWindows) {
	    for (int i = 0; i < ownedWindows.size(); i++) {
                Window child = (Window) ownedWindows.elementAt(i);
		if (child != null) {
		    s.writeObject(ownedWindowK);
		    s.writeObject(child);
		}
	    }
	}
	s.writeObject(null);
    }

    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();
        checkSecurity();

        ownedWindows = new Vector();

        Object keyOrNull;

        while (null != (keyOrNull = s.readObject())) {
            String key = ((String) keyOrNull).intern();
            if (windowListenerK == key)
                addWindowListener((WindowListener) (s.readObject()));
            else // skip value for unrecognized key
                s.readObject();
        }

       try {
	   while (null != (keyOrNull = s.readObject())) {
               
	       String key = ((String)keyOrNull).intern();
	       if (ownedWindowK == key)
		   connectOwnedWindow((Window) s.readObject());

	       else // skip value for unrecognized key
		   s.readObject();
	   }
       }
       catch (OptionalDataException e) {
	   // 1.1 serialized form
	   // ownedWindowList will be updated by Frame.readObject
       }

        setWarningString();
    }
// Focus-related functionality added 
//-------------------------------------------------------------------------
    private transient Component temporaryLostComponent;
    transient WindowFocusListener windowFocusListener;
    private boolean focusableWindowState = true;

    public synchronized void addWindowFocusListener(WindowFocusListener l) {
        if (l == null) {
            return;
        }
        windowFocusListener = AWTEventMulticaster.add(windowFocusListener, l);
        newEventsOnly = true;
    }

    public synchronized WindowFocusListener[] getWindowFocusListeners() {
        return (WindowFocusListener[]) AWTEventMulticaster.getListeners(
                                       (EventListener)windowFocusListener,
                                       WindowFocusListener.class);
    }

    public synchronized void removeWindowFocusListener(WindowFocusListener l) {
        if (l == null) {
            return;
        }
        windowFocusListener = AWTEventMulticaster.remove(windowFocusListener, l);
    }

    public final boolean isFocusableWindow() {
        // If a Window/Frame/Dialog was made non-focusable,
        // then it is always non-focusable.
        if (!getFocusableWindowState()) {
            return false;
        }
        // All other tests apply only to Windows.
        if (this instanceof Frame || this instanceof Dialog) {
            return true;
        }
        // A Window must have at least one Component in its root focus
        // traversal cycle to be focusable.
        if (getFocusTraversalPolicy().getDefaultComponent(this) == null) {
            return false;
        }
        // A Window's nearest owning Frame or Dialog must be showing
        // on the screen.
        for (Window owner = getOwner(); owner != null;
             owner = owner.getOwner()) {
            if (owner instanceof Frame || owner instanceof Dialog) {
                return owner.isShowing();
            }
        }
        return false;
    }

    public boolean getFocusableWindowState() {
        return focusableWindowState;
    }

    public void setFocusableWindowState(boolean focusableWindowState) {
        boolean oldFocusableWindowState;
        synchronized (this) {
            oldFocusableWindowState = this.focusableWindowState;
            this.focusableWindowState = focusableWindowState;
        }
        firePropertyChange("focusableWindowState", 
                           new Boolean(oldFocusableWindowState),
                           new Boolean(focusableWindowState));
        if (oldFocusableWindowState && !focusableWindowState && isFocused()) {
            for (Window owner = (Window)getParent(); owner != null;
                owner = (Window)owner.getParent()) {
                Component toFocus =
                    KeyboardFocusManager.getMostRecentFocusOwner(owner);
                if (toFocus != null && toFocus.requestFocus(false)) {
                    return;
                }
            }
            KeyboardFocusManager.getCurrentKeyboardFocusManager().
                clearGlobalFocusOwner();
        }
    }

    protected void processWindowFocusEvent(WindowEvent e) {
        WindowFocusListener listener = windowFocusListener;
        if (listener != null) {
            switch (e.getID()) {
            case WindowEvent.WINDOW_GAINED_FOCUS:
                listener.windowGainedFocus(e);
                break;
            case WindowEvent.WINDOW_LOST_FOCUS:
                listener.windowLostFocus(e);
                break;
            default:
                break;
            }
        }
    }

    public Component getMostRecentFocusOwner() {
        if (isFocused()) {
            return getFocusOwner();
        } else {
            Component mostRecent =
            KeyboardFocusManager.getMostRecentFocusOwner(this);
            if (mostRecent != null) {
                return mostRecent;
            } else {
                return (isFocusableWindow()) ?
                    getFocusTraversalPolicy().getInitialComponent(this) : null;
            }
        }
    }

    public boolean isFocused() {
        return (KeyboardFocusManager.getCurrentKeyboardFocusManager().
            getGlobalFocusedWindow() == this);
    }

    public Set getFocusTraversalKeys(int id) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        // Okay to return Set directly because it is an unmodifiable view
        Set keystrokes = (focusTraversalKeys != null) ? 
            focusTraversalKeys[id] : null;
        if (keystrokes != null) {
            return keystrokes;
        } else {
            return KeyboardFocusManager.getCurrentKeyboardFocusManager().
                getDefaultFocusTraversalKeys(id);
        }
    }

    public final void setFocusCycleRoot(boolean focusCycleRoot) {
    }

    public final boolean isFocusCycleRoot() {
        return true;
    }

    public final Container getFocusCycleRootAncestor() {
        return null;
    }

    Component getTemporaryLostComponent() {
        return temporaryLostComponent;
    }

    Component setTemporaryLostComponent(Component component) {
        Component previousComp = temporaryLostComponent;
        // Check that "component" is an acceptable focus owner and don't store it otherwise 
        // - or later we will have problems with opposite while handling  WINDOW_GAINED_FOCUS
        if (component == null ||
            (component.isDisplayable() && component.isVisible() && 
             component.isEnabled() && component.isFocusable())) {
            temporaryLostComponent = component;
        } else {
            temporaryLostComponent = null;
        }
        return previousComp;
    }

    public Window getOwner() {
        return (Window)parent;
    }
} // class Window


/**
 * This class is no longer used, but is maintained for Serialization
 * backward-compatibility.
 */
class FocusManager implements java.io.Serializable {
    Container focusRoot;
    Component focusOwner;

    /*
     * JDK 1.1 serialVersionUID
     */
    static final long serialVersionUID = 2491878825643557906L;
}
