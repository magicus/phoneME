/*
 * @(#)Window.java	1.48 06/10/10
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
package java.awt;

import java.awt.event.*;
import java.util.Vector;
import java.util.Locale;
import java.util.EventListener;
import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.io.OptionalDataException;
import java.util.ResourceBundle;
import java.util.Set;
import java.lang.ref.WeakReference;
import java.lang.reflect.InvocationTargetException;
import java.security.AccessController;
import java.util.EventListener;
import java.awt.AWTEventMulticaster;
import java.awt.im.InputContext;
import sun.security.action.GetPropertyAction;

import sun.awt.NullGraphics;
import sun.awt.SunToolkit;

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
 * with {@link Window(Window, GraphicsConfiguration)}.  The
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
 * <!-- PBP/PP [4692065] -->
 * <!-- The following two paragraphs are borrowed from J2SE 1.5. -->
 * <a name="geometry">
 * <em>
 * Note: the location and size of top-level windows (including
 * <code>Window</code>s, <code>Frame</code>s, and <code>Dialog</code>s)
 * are under the control of the <strike>desktop's</strike> window management system.
 * Calls to <code>setLocation</code>, <code>setSize</code>, and
 * <code>setBounds</code> are requests (not directives) which are
 * forwarded to the window management system.  <strike>Every effort will be
 * made to honor such requests.</strike>  <strike>However, i</strike>In some cases the window
 * management system may ignore such requests, or modify the requested
 * geometry in order to place and size the <code>Window</code> <strike>in a way
 * that more closely matches the desktop settings.</strike>
 * <em>appropriately</em>.
 *
 * Due to the asynchronous nature of native event handling, the results
 * returned by <code>getBounds</code>, <code>getLocation</code>,
 * <code>getLocationOnScreen</code>, and <code>getSize</code> might not 
 * reflect the actual geometry of the Window on screen until the last
 * request has been processed.  During the processing of subsequent
 * requests these values might change accordingly while the window
 * management system fulfills the requests.
 * </em>
 * <p>
 * <!-- The following language is specific to PBP, but might be picked up
 * in J2SE 1.6.-->
 * <em>
 * An application may set the size and location of an
 * invisible <code>Window</code> arbitrarily,
 * but the window management system may subsequently change
 * its size and/or location if and when the <code>Window</code> is
 * made visible.  One or more <code>ComponentEvent</code>s will
 * be generated to indicate the new geometry.
 * </em>
 *
 * <p>
 * Windows are capable of generating the following window events:
 * WindowOpened, WindowClosed.
 *
 * @version 	1.137, 02/09/01
 * @author 	Sami Shaio
 * @author 	Arthur van Hoff
 * @see WindowEvent
 * @see #addWindowListener
 * @see java.awt.BorderLayout
 * @since       JDK1.0
 */
public class Window extends Container {
    /**
     * This represents the warning message that is
     * to be displayed in a non secure window. ie :
     * a window that has a security manager installed for
     * which calling SecurityManager.checkTopLevelWindow()
     * is false.  This message can be displayed anywhere in
     * the window.
     *
     * @serial
     * @see getWarningString()
     */
    String      warningString;
    private int warningLabelHeight = 0;   //6221221
    static final int OPENED = 0x01;
    /**
     * An Integer value representing the Window State.
     *
     * @serial
     * @since 1.2
     * @see show()
     */
    int state;
    // ### Serialization problem.  FocusManager is package private

    /**
     * The Focus for the Window in question, and its components.
     *
     * @serial
     * @since 1.2
     * @See java.awt.FocusManager
     */
    //private FocusManager focusMgr;

    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 4497834738069338734L;
    transient GraphicsConfiguration graphicsConfig;
    transient WindowListener windowListener;
    transient LightweightDispatcher dispatcher;

    transient InputContext inputContext;

    /**
     * Constructs a new window in the default size.
     *
     * <p>First, if there is a security manager, its
     * <code>checkTopLevelWindow</code>
     * method is called with <code>this</code>
     * as its argument
     * to see if it's ok to display the window without a warning banner.
     * If the default implementation of <code>checkTopLevelWindow</code>
     * is used (that is, that method is not overriden), then this results in
     * a call to the security manager's <code>checkPermission</code> method with an
     * <code>AWTPermission("showWindowWithoutWarningBanner")</code>
     * permission. It that method raises a SecurityException,
     * <code>checkTopLevelWindow</code> returns false, otherwise it
     * returns true. If it returns false, a warning banner is created.
     *
     * @see java.lang.SecurityManager#checkTopLevelWindow
     */
    Window(GraphicsConfiguration gc) {
        setWarningString();   //6221221

        if (gc == null)
            gc = GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration();
        GraphicsDevice device = gc.getDevice();
        if (device.getType() != GraphicsDevice.TYPE_RASTER_SCREEN)
            throw new IllegalArgumentException("Windows can only be created on screen devices");
        if (device.getWindow() != null)
            throw new UnsupportedOperationException("Cannot create more than one window per graphics device");

        // The following bounds setting code is commented out and the functionality is moved
        // to the Window.show() method in order to fix 6270960.
        /*
        Rectangle bounds = gc.getBounds();
        x = bounds.x;
        y = bounds.y;
        width = bounds.width;
        height = bounds.height;
        */
        graphicsConfig = gc;
        foreground = Color.black;
        background = Color.lightGray;
        font = new Font(null, -1, 12);
        visible = false;
        dispatcher = new LightweightDispatcher(this);
        setLayout(new BorderLayout());
        device.setWindow(this);

        //6221221
        if (warningString != null) {
            setWarningString(warningString);
        }
    }

    // 6270960.
    // Was: always ignore setBounds() request.
    // Now: ignore whenever Window.isVisible() returns true.
    public void setBounds(int x, int y, int width, int height) {
        if (isVisible()) {
            // Don't allow window to be moved or resized. The window always occupies
            // the whole screen area.
        } else {
            super.setBounds(x, y, width, height);
        }
    }

    void dispatchEventImpl(AWTEvent e) {
        if (dispatcher.dispatchEvent(e)) {
            // event was sent to a lightweight component.  The
            // native-produced event sent to the native container
            // must be properly disposed of by the peer, so it
            // gets forwarded.  If the native host has been removed
            // as a result of the sending the lightweight event,
            // the peer reference will be null.
            e.consume();
            return;
        }
        switch (e.id) {
        case ComponentEvent.COMPONENT_RESIZED:
            invalidate();
            validate();
            repaint();
            break;
        }
        super.dispatchEventImpl(e);
    }

    /*
     * Dispatches an event to this component, without trying to forward
     * it to any sub components
     * @param e the event
     */
    void dispatchEventToSelf(AWTEvent e) {
        super.dispatchEventImpl(e);
    }

    /**
     * Disposes of the input methods and context, and removes the WeakReference
     * which formerly pointed to this Window from the parent's owned Window
     * list.
     */
    //protected void finalize() throws Throwable {}

    /**
     * Causes this Window to be sized to fit the preferred size
     * and layouts of its subcomponents.  If the window and/or its owner
     * are not yet displayable, both are made displayable before
     * calculating the preferred size.  The Window will be validated
     * after the preferredSize is calculated.
     * @see Component#isDisplayable
     */
    public void pack() {
        Container parent = this.parent;
        if (parent != null && !parent.displayable) {
            parent.addNotify();
        }
        if (!displayable) {
            addNotify();
        }
        setSize(getPreferredSize());
        validate();
    }

    /**
     * Makes the Window visible. If the Window and/or its owner
     * are not yet displayable, both are made displayable.  The
     * Window will be validated prior to being made visible.
     * If the Window is already visible, this will bring the Window
     * to the front.
     * @see       java.awt.Component#isDisplayable
     * @see       java.awt.Window#toFront
     * @see       java.awt.Component#setVisible
     */
    public void show() {
        if (!displayable) {
            addNotify();
        }

        // The following bounds setting code is moved from Window.ctor() method
        // in order to fix 6270960.
        // Basis toplevel always occupies the whole screen bounded by the
        // graphics device.  We need to call setBounds() before validate()
        // in order to calculate the right size for the containees.
        Rectangle bounds = graphicsConfig.getBounds();
        setBounds(bounds.x, bounds.y, bounds.width, bounds.height);

        validate();

        if (visible) {
            toFront();
        } else {
            // 6229858 - need to call pShow because this is where we actually
            // show the native widget
            pShow();
            super.show();
            // If first time shown, generate WindowOpened event
            if ((state & OPENED) == 0) {
                Toolkit.getEventQueue().postEvent(new WindowEvent(this, WindowEvent.WINDOW_OPENED));
                //SunToolkit.postEvent(appContext, new WindowEvent(this, WindowEvent.WINDOW_OPENED));
                state |= OPENED;
            }
            // Using WINDOW_GAINED_FOCUS to bootstrap the process so that
            // we could resue J2SE's implementation of 
            // DefaultKeyboardFocusManager.dispatchEvent(). Previously we
            // we were posting WINDOW_ACTIVATED
            SunToolkit.postEvent(appContext,new WindowEvent(this, WindowEvent.WINDOW_GAINED_FOCUS));
            postPaintEvent();
        }
    }

    // 6229858 - don't show the window until we actually show the frame
    private native void pShow();

    /**
     * Releases all of the native screen resources used by this Window,
     * its subcomponents, and all of its owned children. That is, the
     * resources for these Components will be destroyed, any memory
     * they consume will be returned to the OS, and they will be marked
     * as undisplayable.
     * <p>
     * The Window and its subcomponents can be made displayable again
     * by rebuilding the native resources with a subsequent call to
     * <code>pack</code> or <code>show</code>. The states of the recreated
     * Window and its subcomponents will be identical to the states of these
     * objects at the point where the Window was disposed (not accounting for
     * additional modifcations between those actions).
     * </p>
     * @see Component#isDisplayable
     * @see Window#pack
     * @see Window#show
     */
    public void dispose() {
        setVisible(false);
        removeNotify();
        Toolkit.getEventQueue().postEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSED));
        //SunToolkit.postEvent(appContext, new WindowEvent(this, WindowEvent.WINDOW_CLOSED));

        /* IM cleanup currently as Window.dispose() 
           is not called for 
        *if (inputContext != null) {
        *   InputContext toDispose = inputContext;
        *   inputContext = null;
        *   toDispose.dispose();
        *}
        */
    }

    public void update(Graphics g) {
        if (isShowing()) {
            g.clearRect(0, 0, width, height);
            paint(g);
        }
    }

    /**
     * Brings this window to the front.
     * Places this window at the top of the stacking order and
     * shows it in front of any other windows.
     * @see       java.awt.Window#toBack
     */
    public void toFront() {}

    /**
     * Sends this window to the back.
     * Places this window at the bottom of the stacking order and
     * makes the corresponding adjustment to other visible windows.
     * @see       java.awt.Window#toFront
     */
    public void toBack() {}

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
     */
    public final String getWarningString() {
        return warningString;
    }

    /** 6221221 */
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
     * 6221221
     *
     * Sets the warning string label and returns the height
     * of the label.  The label height must be taken into
     * account when calculating the insets.
     */
    private void setWarningString(String warningString) {
        warningLabelHeight = pSetWarningString(warningString);
    }

    /**
     * Determines the insets of this window, which indicate the size
     * of the window's border.
     * <p>
     * A <code>Frame</code> object, for example, has a top inset that
     * corresponds to the height of the frame's title bar.
     * @return    the insets of this window.
     * @see       java.awt.Insets
     * @see       java.awt.LayoutManager
     * @since     JDK1.1
     */
    public Insets getInsets() {
        //6221221 return  new Insets(0, 0, 0, 0);
        return  new Insets(0, 0, warningLabelHeight, 0);   //6221221
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
        if (locale == null)
            return Locale.getDefault();
        return locale;
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
     * Set the cursor image to a specified cursor.
     * @param <code>cursor</code> One of the constants defined
     *            by the <code>Cursor</code> class. If this parameter is null
     *            then the cursor for this window will be set to the type
     *            Cursor.DEFAULT_CURSOR.
     * @see       java.awt.Component#getCursor
     * @see       java.awt.Cursor
     * @since     JDK1.1
     */
    public void setCursor(Cursor cursor) {
        if (cursor == null) {
            cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        }
        super.setCursor(cursor);
    }

    public void setBackground(Color c) {
        Color oldBackground = background;
        background = c;
        if (c != null && !c.equals(oldBackground)) {
            // Normally a Window is a heavyweight component that paints its own
            // background and is opaque. Setting the background on a heavywieghht causes
            // the background to be updated immediately (on lightweight components the background is not
            // repainted until repaint is called). It is necessary to post a paint event
            // here to ensure that the background is repainted. We can't just call repaint because
            // the update method may be overridden so as to not paint the background.

            postPaintEvent();
        }
    }

    /** Posts a paint event to the event queue for this window. This method should be called whenever
     a paint event would traditionally be generated by a heavyweight window. */

    private void postPaintEvent() {
        //Toolkit.getEventQueue().postEvent(new PaintEvent(this, PaintEvent.PAINT, new Rectangle(0, 0, width, height)));
        SunToolkit.postEvent(appContext, new PaintEvent(this, PaintEvent.PAINT, new Rectangle(0, 0, width, height)));
    }

    /**
     * Adds the specified window listener to receive window events from
     * this window.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param 	l the window listener
     */
    public synchronized void addWindowListener(WindowListener l) {
        if (l == null) {
            return;
        }
        windowListener = AWTEventMulticaster.add(windowListener, l);
    }

    /**
     * Removes the specified window listener so that it no longer
     * receives window events from this window.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param 	l the window listener
     */
    public synchronized void removeWindowListener(WindowListener l) {
        if (l == null) {
            return;
        }
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

    /** Posts a mouse button event to the event queue for this window. This method will determine the click count for the
     event. It will also post a mouse click event if the mouse was released at the last click coordinate.
     This is called from native code when a mouse event is received. It could also
     be called from Robot implementations if it is not possible to generate mouse events at
     the native level (that is why it has been made package protected and not private).
     @param button the number of the button that was pressed or released
     (MouseEvent.BUTTON1, MouseEvent.BUTTON2, or MouseEvent.BUTTON3)
     @paran id either MouseEvent.MOUSE_PRESSED or MouseEvent.MOUSE_RELEASED
     @param modifiers the modifiers for the event (these are defined using the extended modifiers in InputEvent)
     @param x the x coordinate for the event
     @param y the y coordinate for the event */

    void postMouseButtonEvent(int button, int id, int modifiers, int x, int y) {
        long when = System.currentTimeMillis();
        if (id == MouseEvent.MOUSE_PRESSED) {
            if (x == lastClickX && y == lastClickY && when - lastClickTime <= 200)
                clickCount++;
            else clickCount = 1;
            lastClickX = x;
            lastClickY = y;
            lastClickButton = button;
        }
        boolean popupTrigger = id == MouseEvent.MOUSE_PRESSED && button == InputEvent.BUTTON3_MASK;
        //Toolkit.getEventQueue().postEvent(new MouseEvent (this, id, when, modifiers, x, y, clickCount, popupTrigger));
        SunToolkit.postEvent(appContext, new MouseEvent (this, id, when, modifiers, x, y, clickCount, popupTrigger));
        if (id == MouseEvent.MOUSE_RELEASED &&
            lastClickX == x &&
            lastClickY == y &&
            lastClickButton == button) {
            //Toolkit.getEventQueue().postEvent(new MouseEvent (this, MouseEvent.MOUSE_CLICKED, when, modifiers, x, y, clickCount, popupTrigger));
            SunToolkit.postEvent(appContext,new MouseEvent (this, MouseEvent.MOUSE_CLICKED, when, modifiers, x, y, clickCount, popupTrigger));
            lastClickTime = when;
        }
    }

    /** Posts mouse events, other than button presses or releases, to the event queue for this
     window. This method is called from native code when a mouse event is received. It could also
     be called from Robot implementations if it is not possible to generate mouse events at
     the native level (that is why it has been made package protected and not private). */

    void postMouseEvent(int id, int modifiers, int x, int y) {
        //Toolkit.getEventQueue().postEvent(new MouseEvent (this, id, System.currentTimeMillis(), modifiers, x, y, 0, false));
        SunToolkit.postEvent(appContext, new MouseEvent (this, id, System.currentTimeMillis(), modifiers, x, y, 0, false));
    }

    void postKeyEvent(int id, int modifiers, 
                              int keyCode, char keyChar) {
    
        long when = System.currentTimeMillis();

        // get the current focus owner. if one exists then dispatch the
        // key events to the owner's event queue. If not check if the
        // window is focusable and dispatch it to the window's event queue

        // not using getFocusOwner() since we will incur a appcontext 
        // check and not get the focus owner, so use getGlobalFocusOwner()
        Component focusOwner = 
            KeyboardFocusManager.getCurrentKeyboardFocusManager().
            getGlobalFocusOwner();
        sun.awt.AppContext context = this.appContext ;
        if ( focusOwner != null )
            context = focusOwner.appContext;
        SunToolkit.postEvent(context, 
           new KeyEvent (this, id, when, modifiers, keyCode, keyChar));
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

    /**
     * Checks if this Window is showing on screen.
     * @see java.awt.Component#setVisible(boolean)
     */
    public boolean isShowing() {
        return visible;
    }
    /**
     * The window serialized data version.
     *
     * @serial
     */
    private int windowSerializedDataVersion = 1;
    /**
     * Writes default serializable fields to stream.  Writes
     * a list of serializable ItemListener(s) as optional data.
     * The non-serializable ItemListener(s) are detected and
     * no attempt is made to serialize them. Write a list of
     * child Windows as optional data.
     *
     * @serialData Null terminated sequence of 0 or more pairs.
     *             The pair consists of a String and Object.
     *             The String indicates the type of object and
     *             is one of the following :
     *             itemListenerK indicating an ItemListener object.
     * @serialData Null terminated sequence of 0 or more pairs.
     *             The pair consists of a String and Object.
     *             The String indicates the type of object and
     *             is one of the following :
     *             ownedWindowK indicating a child Window object.
     *
     * @see AWTEventMulticaster.save(ObjectOutputStream, String, EventListener)
     * @see java.awt.Component.itemListenerK
     * @see java.awt.Component.ownedWindowK
     */
    private void writeObject(ObjectOutputStream s)
        throws IOException {}

    /**
     * Read the default ObjectInputStream, a possibly null listener to
     * receive item events fired by the Window, and a possibly null
     * list of child Windows.
     * Unrecognised keys or values will be Ignored.
     *
     * @see removeActionListener()
     * @see addActionListener()
     */
    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {}

    /**
     * Update the cursor is decided by the dispatcher
     * This method is used when setCursor on a lightweight
     */
    void updateCursor(Component comp) {
        // 6229858 - if this component is the toplevel frame, change the 
        // cursor here instead of deferring to the LightweightDispatcher
        if (comp instanceof Window) {
            Cursor cursor = comp.getCursor();
            changeCursor(cursor);
        }
        else {
            dispatcher.updateCursor(comp);
        }
    }

    /* Native wrapper for changing cursor bitmap */
    private native void pChangeCursor(int cursorType);

    /* 
     * 6221221
     * Native method for setting the warning string
     */
    private native int pSetWarningString(String warningString);
    
    /**
     * Changes the cursor displayed in this frame to the supplied cursor.
     * This is used for lightweight components. It does not affect the
     * return value for getCursor on this frame.
     * @see LightweightDispatcher
     */
    void changeCursor(Cursor cursor) {
        pChangeCursor(cursor.type);
    }

    public Graphics getGraphics() {
        Graphics g = null;
        if (displayable) {
            if (this.visible)
                g = getToolkit().getGraphics(this);
            else
                g = new NullGraphics(this);
        }
        return g;
    }

    /**
     * This method returns the GraphicsConfiguration used by this Window.
     */
    public GraphicsConfiguration getGraphicsConfiguration() {
        return graphicsConfig;
    }
    /** The time the last mouse click event ocurred at. */

    private long lastClickTime;
    /** The click count used for mouse events. */

    private int clickCount = 1;
    /** Position of last mouse click. */

    private int lastClickX, lastClickY;
    /** Modifiers used the last time the mouse was clicked. */

    //private int lastClickModifiers;

    /** The button number that was last clicked. */

    private int lastClickButton;

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
    }

//    public synchronized WindowFocusListener[] getWindowFocusListeners() {
//        return (WindowFocusListener[])(getListeners(WindowFocusListener.class));
//    }

    public synchronized void removeWindowFocusListener(WindowFocusListener l) {
        if (l == null) {
            return;
        }
        windowFocusListener = AWTEventMulticaster.remove(windowFocusListener, l);
    }

    /**
     * Returns an array of all the window focus listeners
     * registered on this window.
     *
     * @return all of this window's <code>WindowFocusListener</code>s
     *         or an empty array if no window focus
     *         listeners are currently registered
     *
     * @see #addWindowFocusListener
     * @see #removeWindowFocusListener
     * @since 1.4
     */
    public synchronized WindowFocusListener[] getWindowFocusListeners() {
        return (WindowFocusListener[]) AWTEventMulticaster.getListeners(
                                       (EventListener)windowFocusListener,
                                        WindowFocusListener.class);
    }

    public final boolean isFocusableWindow() {
        // If a Window/Frame was made non-focusable,
        // then it is always non-focusable.
        if (!getFocusableWindowState()) {
            return false;
        }
        // All other tests apply only to Windows.
        if (this instanceof Frame) {
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
            if (owner instanceof Frame) {
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

    /**
     * Returns whether this Window is active. Only a Frame or a Dialog may be
     * active. The native windowing system may denote the active Window or its
     * children with special decorations, such as a highlighted title bar. The
     * active Window is always either the focused Window, or the first Frame or
     * Dialog that is an owner of the focused Window.
     *
     * @return whether this is the active Window.
     * @see #isFocused
     * @since 1.4
     */
    public boolean isActive() {
        return (KeyboardFocusManager.getCurrentKeyboardFocusManager().
                getActiveWindow() == this);
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

    private Window getOwner() {
        return (Window)parent;
    }
}
