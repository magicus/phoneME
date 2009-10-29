/*
 * @(#)Component.java	1.53 06/10/10
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

import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.Vector;
import java.util.Locale;
import java.util.EventListener;
import java.util.Timer;
import java.util.TimerTask;
import java.awt.im.InputContext;
import java.awt.im.InputMethodRequests;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.ColorModel;
import java.awt.image.VolatileImage;
import java.awt.event.*;
import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.beans.PropertyChangeListener;
import java.security.AccessController;
import sun.awt.NullGraphics;
import sun.awt.ConstrainableGraphics;
import sun.awt.AppContext;
import sun.awt.SunToolkit;
import sun.security.action.GetPropertyAction;
import java.util.Set;
import java.util.Iterator;
import java.util.HashSet;
import java.util.Collections;

/**
 * A <em>component</em> is an object having a graphical representation
 * that can be displayed on the screen and that can interact with the
 * user. Examples of components are the buttons, checkboxes, and scrollbars
 * of a typical graphical user interface. <p>
 * The <code>Component</code> class is the abstract superclass of
 * the nonmenu-related Abstract Window Toolkit components. Class
 * <code>Component</corede> can also be extended directly to create a
 * lightweight component. A lightweight component is a component that is
 * not associated with a native opaque window.
 *
 * <p>
 * <a name="restrictions">
 * <h4>Restrictions</h4>
 * <em>
 * Implementations of Component in Personal Basis Profile exhibit
 * certain restrictions, specifically:
 * <ul>
 * <li> An implementation may prohibit setting the cursor.  In such
 * a case, attempts to set the cursor will fail silently.
 * See:
 * <ul>
 * <li> {@link #setCursor}
 * </ul>
 * </ul>
 * </em>
 * @version 1.11, 01/16/02
 * @author Nicholas Allen
 */
public abstract class Component implements ImageObserver,
        Serializable {
    /**
     * The parent of the object. It may be null for top-level components.
     * @see #getParent
     */
    transient Container parent;
    /**
     * The x position of the component in the parent's coordinate system.
     *
     * @serial
     * @see #getLocation
     */
    int x;
    /**
     * The y position of the component in the parent's coordinate system.
     *
     * @serial
     * @see #getLocation
     */
    int y;
    /**
     * The width of the component.
     *
     * @serial
     * @see #getSize
     */
    int width;
    /**
     * The height of the component.
     *
     * @serial
     * @see #getSize
     */
    int height;
    /**
     * The foreground color for this component.
     * foreground color can be null.
     *
     * @serial
     * @see #getForeground
     * @see #setForeground
     */
    Color	foreground;
    /**
     * The background color for this component.
     * background can be null.
     *
     * @serial
     * @see #getBackground
     * @see #setBackground
     */
    Color	background;
    /**
     * The font used by this component.
     * The font can be null.
     *
     * @serial
     * @see #getFont
     * @see #setFont
     */
    Font	font;
    /**
     * The cursor displayed when pointer is over this component.
     * This value can be null.
     *
     * @serial
     * @see #getCursor
     * @see #setCursor
     */
    Cursor	cursor;
    /**
     * The locale for the component.
     *
     * @serial
     * @see #getLocale
     * @see #setLocale
     */
    Locale      locale;
    /**
     * A reference to a GraphicsConfiguration object
     * used to describe the characteristics of a graphics
     * destination.
     * This value can be null.
     *
     * @since 1.3
     * @serial
     * @see java.awt.GraphicsConfiguration
     * @see #getGraphicsConfiguration
     */
    //transient GraphicsConfiguration graphicsConfig = null;

    // ### Serialization difference
    // No field GraphicsConfiguration graphicsConfig

    /**
     * True when the object should ignore all repaint events.
     *
     * @since 1.4
     * @serial
     * @see #setIgnoreRepaint
     * @see #getIgnoreRepaint
     */
    boolean ignoreRepaint = false;

    /**
     * True when the object is visible. An object that is not
     * visible is not drawn on the screen.
     *
     * @serial
     * @see #isVisible
     * @see #setVisible
     */
    boolean visible = true;
    /**
     * True when the object is enabled. An object that is not
     * enabled does not interact with the user.
     *
     * @serial
     * @see #isEnabled
     * @see #setEnabled
     */
    boolean enabled = true;
    /**
     * True when the object is displayable. This field is necessary
     * because peers are not used in basis profile.
     *
     * @see #isDisplayable
     */

    transient boolean displayable;
    /**
     * True when the object is valid. An invalid object needs to
     * be layed out. This flag is set to false when the object
     * size is changed.
     *
     * @serial
     * @see #isValid
     * @see #validate
     * @see #invalidate
     */
    boolean valid = false;
    /**
     * A components name.
     * This field can be null.
     *
     * @serial
     * @see getName()
     * @see setName(String)
     */
    private String name;
    /**
     * A bool to determine whether the name has
     * been set explicitly. nameExplicitlySet will
     * be false if the name has not been set and
     * true if it has.
     *
     * @serial
     * @see getName()
     * @see setName(String)
     */
    private boolean nameExplicitlySet = false;
    /**
     * The locking object for AWT component-tree and layout operations.
     *
     * @see #getTreeLock
     */
    static final Object LOCK = new AWTTreeLock();
    static class AWTTreeLock {}
    /**
     * Internal, cached size information.
     * (This field perhaps should have been transient).
     *
     * @serial
     */
    Dimension minSize;
    /** Internal, cached size information
     * (This field perhaps should have been transient).
     *
     * @serial
     */
    Dimension prefSize;
    boolean newEventsOnly = false;
    transient ComponentListener componentListener;
    transient FocusListener focusListener;
    transient KeyListener keyListener;
    transient MouseListener mouseListener;
    transient MouseMotionListener mouseMotionListener;
    transient MouseWheelListener mouseWheelListener;
    transient InputMethodListener inputMethodListener;

    /**
     * The eventMask is ONLY set by subclasses via enableEvents.
     * The mask should NOT be set when listeners are registered
     * so that we can distinguish the difference between when
     * listeners request events and subclasses request them.
     * One bit is used to indicate whether input methods are
     * enabled; this bit is set by enableInputMethods and is
     * on by default.
     *
     * @serial
     * @see enableInputMethods()
     */
    long eventMask = AWTEvent.INPUT_METHODS_ENABLED_MASK;
    /**
     * Static properties for incremental drawing.
     * @see #imageUpdate
     */
    static boolean isInc;
    static int incRate;
    static {
        String s = (String) AccessController.doPrivileged(
                new GetPropertyAction("awt.image.incrementaldraw"));
        isInc = (s == null || s.equals("true"));
        s = (String) AccessController.doPrivileged(
                    new GetPropertyAction("awt.image.redrawrate"));
        incRate = (s != null) ? Integer.parseInt(s) : 100;
    }
    /**
     * Ease-of-use constant for <code>getAlignmentY()</code>.  Specifies an
     * alignment to the top of the component.
     * @see     #getAlignmentY
     */
    public static final float TOP_ALIGNMENT = 0.0f;
    /**
     * Ease-of-use constant for <code>getAlignmentY</code> and
     * <code>getAlignmentX</code>. Specifies an alignment to
     * the center of the component
     * @see     #getAlignmentX
     * @see     #getAlignmentY
     */
    public static final float CENTER_ALIGNMENT = 0.5f;
    /**
     * Ease-of-use constant for <code>getAlignmentY</code>.  Specifies an
     * alignment to the bottom of the component.
     * @see     #getAlignmentY
     */
    public static final float BOTTOM_ALIGNMENT = 1.0f;
    /**
     * Ease-of-use constant for <code>getAlignmentX</code>.  Specifies an
     * alignment to the left side of the component.
     * @see     #getAlignmentX
     */
    public static final float LEFT_ALIGNMENT = 0.0f;
    /**
     * Ease-of-use constant for <code>getAlignmentX</code>.  Specifies an
     * alignment to the right side of the component.
     * @see     #getAlignmentX
     */
    public static final float RIGHT_ALIGNMENT = 1.0f;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -7644114512714619750L;
    /**
     * If any PropertyChangeListeners have been registered, the
     * changeSupport field describes them.
     *
     * @serial
     * @since 1.2
     * @see addPropertyChangeListener
     * @see removePropertyChangeListener
     * @see firePropertyChange
     */
    private java.beans.PropertyChangeSupport changeSupport;
    /** Internal, constants for serialization */
    final static String actionListenerK = "actionL";
    final static String adjustmentListenerK = "adjustmentL";
    final static String componentListenerK = "componentL";
    final static String containerListenerK = "containerL";
    final static String focusListenerK = "focusL";
    final static String itemListenerK = "itemL";
    final static String keyListenerK = "keyL";
    final static String mouseListenerK = "mouseL";
    final static String mouseMotionListenerK = "mouseMotionL";
    final static String mouseWheelListenerK = "mouseWheelL";
    final static String textListenerK = "textL";
    final static String windowListenerK = "windowL";
    final static String inputMethodListenerK = "inputMethodL";
    boolean isPacked = false;
    private static Timer TIMER;

    /**
     * The <code>AppContext</code> of the component.  This is set in
     * the constructor and never changes.
     */
    transient AppContext appContext;

    /**
     * Constructs a new component. Class <code>Component</code> can be
     * extended directly to create a lightweight component that does not
     * utilize an opaque native window. A lightweight component must be
     * hosted by a native container somewhere higher up in the component
     * tree (for example, by a <code>Frame</code> object).
     */
    protected Component() {
       appContext = AppContext.getAppContext();
       SunToolkit.insertTargetMapping(this, appContext);
    }

    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return null; // For strict compliance with prior JDKs, a Component
        // that doesn't set its name should return null from
        // getName();
    }

    /**
     * Gets the name of the component.
     * @return This component's name.
     * @see    #setName
     * @since JDK1.1
     */
    public String getName() {
        if (name == null && !nameExplicitlySet) {
            synchronized (this) {
                if (name == null && !nameExplicitlySet)
                    name = constructComponentName();
            }
        }
        return name;
    }

    /**
     * Sets the name of the component to the specified string.
     * @param name  The string that is to be this
     * component's name.
     * @see #getName
     * @since JDK1.1
     */
    public void setName(String name) {
        synchronized (this) {
            this.name = name;
            nameExplicitlySet = true;
        }
    }

    /**
     * Gets the parent of this component.
     * @return The parent container of this component.
     * @since JDK1.0
     */
    public Container getParent() {
        return parent;
    }

    /**
     * Get the <code>GraphicsConfiguration</code> associated with this
     * <code>Component</code>.
     * If the <code>Component</code> has not been assigned a specific
     * <code>GraphicsConfiguration</code>,
     * the <code>GraphicsConfiguration</code> of the
     * <code>Component</code> object's top-level container is
     * returned.
     * If the <code>Component</code> has been created, but not yet added
     * to a <code>Container</code>, this method returns <code>null</code>.
     * @return the <code>GraphicsConfiguration</code> used by this
     * <code>Component</code> or <code>null</code>
     * @since 1.3
     */
    public GraphicsConfiguration getGraphicsConfiguration() {
        Frame frame = getFrame();
        return (frame != null) ? frame.getGraphicsConfiguration() : null;
    }

    /**
     * Gets the locking object for AWT component-tree and layout
     * Gets this component's locking object (the object that owns the thread
     * sychronization monitor) for AWT component-tree and layout
     * operations.
     * @return This component's locking object.
     */
    public final Object getTreeLock() {
        return LOCK;
    }

    /**
     * Gets the toolkit of this component. Note that
     * the frame that contains a component controls which
     * toolkit is used by that component. Therefore if the component
     * is moved from one frame to another, the toolkit it uses may change.
     * @return  The toolkit of this component.
     * @since JDK1.0
     */
    public Toolkit getToolkit() {
        return Toolkit.getDefaultToolkit();
    }

    /**
     * Determines whether this component is valid. A component is valid
     * when it is correctly sized and positioned within its parent
     * container and all its children are also valid. Components are
     * invalidated when they are first shown on the screen.
     * @return <code>true</code> if the component is valid; <code>false</code>
     * otherwise.
     * @see #validate
     * @see #invalidate
     * @since JDK1.0
     */
    public boolean isValid() {
        return valid;
    }

    /**
     * Determines whether this component is displayable. A component is
     * displayable when it is connected to a native screen resource.
     * <p>
     * A component is made displayable either when it is added to
     * a displayable containment hierarchy or when its containment
     * hierarchy is made displayable.
     * A containment hierarchy is made displayable when its ancestor
     * window is either packed or made visible.
     * <p>
     * A component is made undisplayable either when it is removed from
     * a displayable containment hierarchy or when its containment hierarchy
     * is made undisplayable.  A containment hierarchy is made
     * undisplayable when its ancestor window is disposed.
     *
     * @return <code>true</code> if the component is displayable;
     * <code>false</code> otherwise.
     * @see java.awt.Container#add(java.awt.Component)
     * @see java.awt.Window#pack
     * @see java.awt.Window#show
     * @see java.awt.Container#remove(java.awt.Component)
     * @see java.awt.Window#dispose
     * @since 1.2
     */
    public boolean isDisplayable() {
        return displayable;
    }

    /**
     * Determines whether this component should be visible when its
     * parent is visible. Components are
     * initially visible, with the exception of top level components such
     * as <code>Frame</code> objects.
     * @return <code>true</code> if the component is visible;
     * <code>false</code> otherwise.
     * @see #setVisible
     * @since JDK1.0
     */
    public boolean isVisible() {
        return visible;
    }

    /**
     * Determines whether this component is showing on screen. This means
     * that the component must be visible, and it must be in a container
     * that is visible and showing.
     * @return <code>true</code> if the component is showing;
     * <code>false</code> otherwise.
     * @see #setVisible
     * @since JDK1.0
     */
    public boolean isShowing() {
        Container parent = this.parent;
        return (parent != null) ? (visible && parent.isShowing()) : false;
    }

    /**
     * Determines whether this component is enabled. An enabled component
     * can respond to user input and generate events. Components are
     * enabled initially by default. A component may be enabled or disabled by
     * calling its <code>setEnabled</code> method.
     * @return <code>true</code> if the component is enabled;
     * <code>false</code> otherwise.
     * @see #setEnabled
     * @since JDK1.0
     */
    public boolean isEnabled() {
        return enabled;
    }

    /**
     * Enables or disables this component, depending on the value of the
     * parameter <code>b</code>. An enabled component can respond to user
     * input and generate events. Components are enabled initially by default.
     * @param     b   If <code>true</code>, this component is
     *            enabled; otherwise this component is disabled.
     * @see #isEnabled
     * @since JDK1.1
     */
    public void setEnabled(boolean b) {
        if (enabled && !b) {
            KeyboardFocusManager.clearMostRecentFocusOwner(this);
            synchronized (getTreeLock()) {
                enabled = false;
                if (isFocusOwner()) {
                    // Don't clear the global focus owner. If transferFocus
                    // fails, we want the focus to stay on the disabled
                    // Component so that keyboard traversal, et. al. still
                    // makes sense to the user.
                    autoTransferFocus(false);
                }
            }
        }
        enabled = b;
    }
 
    /**
     * Returns true if this component is painted to an offscreen image
     * ("buffer") that's copied to the screen later.  Component
     * subclasses that support double buffering should override this
     * method to return true if double buffering is enabled.
     *
     * @return false by default
     */
    public boolean isDoubleBuffered() {
        return false;
    }

    /**
     * Enables or disables input method support for this component. If input
     * method support is enabled and the component also processes key events,
     * incoming events are offered to
     * the current input method and will only be processed by the component or
     * dispatched to its listeners if the input method does not consume them.
     * By default, input method support is enabled.
     *
     * @param enable true to enable, false to disable
     * @see #processKeyEvent
     * @since 1.2
     */
    public void enableInputMethods(boolean enable) {
        if (enable) {
            if ((eventMask & AWTEvent.INPUT_METHODS_ENABLED_MASK) != 0)
                return;

            // If this component already has focus, then activate the
            // input method by dispatching a synthesized focus gained
            // event.
            if (isFocusOwner()) {
                InputContext inputContext = getInputContext();
                if (inputContext != null) {
                    FocusEvent focusGainedEvent =
                        new FocusEvent(this, FocusEvent.FOCUS_GAINED);
                    inputContext.dispatchEvent(focusGainedEvent);
                }
            }

            eventMask |= AWTEvent.INPUT_METHODS_ENABLED_MASK;
        } else {
            if (areInputMethodsEnabled()) {
                InputContext inputContext = getInputContext();
                if (inputContext != null) {
                    inputContext.endComposition();
                    inputContext.removeNotify(this);
                }
            }
            eventMask &= ~AWTEvent.INPUT_METHODS_ENABLED_MASK;
        }
    }

    /**
     * Shows or hides this component depending on the value of parameter
     * <code>b</code>.
     * @param b  If <code>true</code>, shows this component;
     * otherwise, hides this component.
     * @see #isVisible
     * @since JDK1.1
     */
    public void setVisible(boolean b) {
        if (b)
            show();
        else hide();
    }

    void show() {
        if (!visible) {
            synchronized (getTreeLock()) {
                visible = true;
                if (isLightweight())
                    repaint();
                if (componentListener != null ||
                    (eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0) {
                    ComponentEvent e = new ComponentEvent(this,
                            ComponentEvent.COMPONENT_SHOWN);
                    Toolkit.getEventQueue().postEvent(e);
                    //SunToolkit.postEvent(appContext, e);
                }
            }
            Container parent = this.parent;
            if (parent != null)
                parent.invalidate();
        }
    }

    void hide() {
        if (visible) {
            clearCurrentFocusCycleRootOnHide();
            clearMostRecentFocusOwnerOnHide();
            synchronized (getTreeLock()) {
                visible = false;
                if (containsFocus()) {
                    autoTransferFocus(true);
                }
                if (componentListener != null ||
                    (eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0) {
                    ComponentEvent e = new ComponentEvent(this,
                                     ComponentEvent.COMPONENT_HIDDEN);
                    Toolkit.getEventQueue().postEvent(e);
                    //SunToolkit.postEvent(appContext, e);
                }
            }
            Container parent = this.parent;
            if (parent != null) {
                parent.invalidate();
            }
        }
    }

    /**
     * Gets the foreground color of this component.
     * @return This component's foreground color. If this component does
     * not have a foreground color, the foreground color of its parent
     * is returned.
     * @see #setForeground
     * @since JDK1.0
     */
    public Color getForeground() {
        Color foreground = this.foreground;
        if (foreground != null) {
            return foreground;
        }
        Container parent = this.parent;
        return (parent != null) ? parent.getForeground() : null;
    }

    /**
     * Sets the foreground color of this component.
     * @param c The color to become this component's
     * foreground color.
     * If this parameter is null then this component will inherit
     * the foreground color of its parent.
     * @see #getForeground
     * @since JDK1.0
     */
    public void setForeground(Color c) {
	Color oldColor = foreground;
        foreground = c;

	// This is a bound property, so report the change to
	// any registered listeners.  (Cheap if there are none.)
	firePropertyChange("foreground", oldColor, c);
    }

    /**
     * Returns whether the foreground color has been explicitly set for this
     * Component. If this method returns <code>false</code>, this Component is
     * inheriting its foreground color from an ancestor.
     *
     * @return <code>true</code> if the foreground color has been explicitly
     *         set for this Component; <code>false</code> otherwise.
     * @since 1.4
     */
    public boolean isForegroundSet() {
        return (foreground != null);
     }

    /**
     * Gets the background color of this component.
     * @return This component's background color. If this component does
     * not have a background color, the background color of its parent
     * is returned.
     * @see java.awt.Component#setBackground(java.awt.Color)
     * @since JDK1.0
     */
    public Color getBackground() {
        Color background = this.background;
        if (background != null) {
            return background;
        }
        Container parent = this.parent;
        return (parent != null) ? parent.getBackground() : null;
    }

    /**
     * Sets the background color of this component.
     * @param c The color to become this component's color.
     *        If this parameter is null then this component will inherit
     *        the background color of its parent.
     * background color.
     * @see #getBackground
     * @since JDK1.0
     */
    public void setBackground(Color c) {
	Color oldColor = background;
        background = c;

	// This is a bound property, so report the change to
	// any registered listeners.  (Cheap if there are none.)
	firePropertyChange("background", oldColor, c);
    }

    /**
     * Returns whether the background color has been explicitly set for this
     * Component. If this method returns <code>false</code>, this Component is
     * inheriting its background color from an ancestor.
     *
     * @return <code>true</code> if the background color has been explicitly
     *         set for this Component; <code>false</code> otherwise.
     * @since 1.4
     */
    public boolean isBackgroundSet() {
        return (background != null);
    }

    /**
     * Gets the font of this component.
     * @return This component's font. If a font has not been set
     * for this component, the font of its parent is returned.
     * @see #setFont
     * @since JDK1.0
     */
    public Font getFont() {
        Font font = this.font;
        if (font != null) {
            return font;
        }
        Container parent = this.parent;
        return (parent != null) ? parent.getFont() : null;
    }

    /**
     * Sets the font of this component.
     * @param f The font to become this component's font.
     * If this parameter is null then this component will inherit
     * the font of its parent.
     * @see #getFont
     * @since JDK1.0
     */
    public void setFont(Font f) {
        Font oldFont = font;
        font = f;

        // This is a bound property, so report the change to
        // any registered listeners.  (Cheap if there are none.)
        firePropertyChange("font", oldFont, font);
    }

    /**
     * Returns whether the font has been explicitly set for this Component. If
     * this method returns <code>false</code>, this Component is inheriting its
     * font from an ancestor.
     *
     * @return <code>true</code> if the font has been explicitly set for this
     *         Component; <code>false</code> otherwise.
     * @since 1.4
     */
    public boolean isFontSet() {
        return (font != null);
    }

    /**
     * Gets the locale of this component.
     * @return This component's locale. If this component does not
     * have a locale, the locale of its parent is returned.
     * @see #setLocale
     * @exception IllegalComponentStateException If the Component
     * does not have its own locale and has not yet been added to
     * a containment hierarchy such that the locale can be determined
     * from the containing parent.
     * @since  JDK1.1
     */
    public Locale getLocale() {
        Locale locale = this.locale;
        if (locale != null) {
            return locale;
        }
        Container parent = this.parent;
        if (parent == null) {
            throw new IllegalComponentStateException("This component must have a parent in order to determine its locale");
        } else {
            return parent.getLocale();
        }
    }

    /**
     * Sets the locale of this component.
     * @param l The locale to become this component's locale.
     * @see #getLocale
     * @since JDK1.1
     */
    public void setLocale(Locale l) {
        locale = l;
        // This could change the preferred size of the Component.
        if (valid) {
            invalidate();
        }
    }

    /**
     * Gets the instance of <code>ColorModel</code> used to display
     * the component on the output device.
     * @return The color model used by this component.
     * @see java.awt.image.ColorModel
     * @see java.awt.peer.ComponentPeer#getColorModel()
     * @see java.awt.Toolkit#getColorModel()
     * @since JDK1.0
     */
    public ColorModel getColorModel() {
        return getToolkit().getColorModel();
    }

    /**
     * Gets the location of this component in the form of a
     * point specifying the component's top-left corner.
     * The location will be relative to the parent's coordinate space.
     * <p>
     * Due to the asynchronous nature of native event handling, this
     * method can return outdated values (for instance, after several calls
     * of <code>setLocation()</code> in rapid succession).  For this
     * reason, the recommended method of obtaining a Component's position is
     * within <code>java.awt.event.ComponentListener.componentMoved()</code>,
     * which is called after the operating system has finished moving the
     * Component.
     * </p>
     * @return An instance of <code>Point</code> representing
     * the top-left corner of the component's bounds in the coordinate
     * space of the component's parent.
     * @see #setLocation
     * @see #getLocationOnScreen
     * @since JDK1.1
     */
    public Point getLocation() {
        return new Point(x, y);
    }

    /**
     * Gets the location of this component in the form of a point
     * specifying the component's top-left corner in the screen's
     * coordinate space.
     * @return An instance of <code>Point</code> representing
     * the top-left corner of the component's bounds in the
     * coordinate space of the screen.
     * @see #setLocation
     * @see #getLocation
     */
    public Point getLocationOnScreen() {
        synchronized (getTreeLock()) {
            Component c = this;
            Point location = new Point(0, 0);
            while (c != null && c.visible) {
                location.x += c.x;
                location.y += c.y;
                if (c instanceof Window)
                    return location;
                c = c.parent;
            }
            throw new IllegalComponentStateException("component must be showing on the screen to determine its location");
        }
    }

    /**
     * Moves this component to a new location. The top-left corner of
     * the new location is specified by the <code>x</code> and <code>y</code>
     * parameters in the coordinate space of this component's parent.
     * @param x The <i>x</i>-coordinate of the new location's
     * top-left corner in the parent's coordinate space.
     * @param y The <i>y</i>-coordinate of the new location's
     * top-left corner in the parent's coordinate space.
     * @see #getLocation
     * @see #setBounds
     * @since JDK1.1
     */
    public void setLocation(int x, int y) {
        setBounds(x, y, width, height);
    }

    /**
     * Moves this component to a new location. The top-left corner of
     * the new location is specified by point <code>p</code>. Point
     * <code>p</code> is given in the parent's coordinate space.
     * @param p The point defining the top-left corner
     * of the new location, given in the coordinate space of this
     * component's parent.
     * @see #getLocation
     * @see #setBounds
     * @since JDK1.1
     */
    public void setLocation(Point p) {
        setLocation(p.x, p.y);
    }

    /**
     * Returns the size of this component in the form of a
     * <code>Dimension</code> object. The <code>height</code>
     * field of the <code>Dimension</code> object contains
     * this component's height, and the <code>width</code>
     * field of the <code>Dimension</code> object contains
     * this component's width.
     * @return A <code>Dimension</code> object that indicates the
     * size of this component.
     * @see #setSize
     * @since JDK1.1
     */
    public Dimension getSize() {
        return new Dimension(width, height);
    }

    /**
     * Resizes this component so that it has width <code>width</code>
     * and <code>height</code>.
     * @param width The new width of this component in pixels.
     * @param height The new height of this component in pixels.
     * @see #getSize
     * @see #setBounds
     * @since JDK1.1
     */
    public void setSize(int width, int height) {
        setBounds(x, y, width, height);
    }

    /**
     * Resizes this component so that it has width <code>d.width</code>
     * and height <code>d.height</code>.
     * @param d The dimension specifying the new size
     * of this component.
     * @see #setSize
     * @see #setBounds
     * @since JDK1.1
     */
    public void setSize(Dimension d) {
        setSize(d.width, d.height);
    }

    /**
     * Gets the bounds of this component in the form of a
     * <code>Rectangle</code> object. The bounds specify this
     * component's width, height, and location relative to
     * its parent.
     * @return A rectangle indicating this component's bounds.
     * @see #setBounds
     * @see #getLocation
     * @see #getSize
     */
    public Rectangle getBounds() {
        return new Rectangle (x, y, width, height);
    }

    /**
     * Moves and resizes this component. The new location of the top-left
     * corner is specified by <code>x</code> and <code>y</code>, and the
     * new size is specified by <code>width</code> and <code>height</code>.
     * @param x The new <i>x</i>-coordinate of this component.
     * @param y The new <i>y</i>-coordinate of this component.
     * @param width The new <code>width</code> of this component.
     * @param height The new <code>height</code> of this
     * component.
     * @see java.awt.Component#getBounds
     * @see java.awt.Component#setLocation(int, int)
     * @see java.awt.Component#setLocation(java.awt.Point)
     * @see java.awt.Component#setSize(int, int)
     * @see java.awt.Component#setSize(java.awt.Dimension)
     * @JDK1.1
     */
    public void setBounds(int x, int y, int width, int height) {
        synchronized (getTreeLock()) {
            boolean resized = (this.width != width) || (this.height != height);
            boolean moved = (this.x != x) || (this.y != y);
            if (resized || moved) {
                // Bug Fix - 4662934
                // Remember the area this component occupied in its parent.
                int oldParentX = this.x;
                int oldParentY = this.y;
                int oldWidth = this.width;
                int oldHeight = this.height;                
                this.x = x;
                this.y = y;
                this.width = width;
                this.height = height;
                if (displayable) {
                    if (resized) {
                        invalidate();
                    }
                    if (parent != null && parent.valid) {
                        parent.invalidate();
                    }
                }
                if (resized) {
                    if (componentListener != null ||
                        (eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0) {
                        ComponentEvent e = new ComponentEvent(this,
                                ComponentEvent.COMPONENT_RESIZED);
                        Toolkit.getEventQueue().postEvent(e);
                        //SunToolkit.postEvent(appContext, e);
                        // Container.dispatchEventImpl will create
                        // HierarchyEvents
                    }
                }
                if (moved) {
                    if (componentListener != null ||
                        (eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0) {
                        ComponentEvent e = new ComponentEvent(this,
                                ComponentEvent.COMPONENT_MOVED);
                        Toolkit.getEventQueue().postEvent(e);
                        //SunToolkit.postEvent(appContext, e);
                    }
                }
                if (visible) {
                    // Bug Fix - 4662934                    
                    // Repaint the old area ...
                    if (parent != null) {
                        parent.repaint(oldParentX, oldParentY, oldWidth, oldHeight);                        
                    }
                    // ... then the new (these areas will be collapsed by
                    // coalesceEvents if they intersect).                    
                    repaint();                                        
                }
            }
        }
    }

    /**
     * Return the current x coordinate of the components origin.
     * This method is preferable to writing component.getBounds().x,
     * or component.getLocation().x because it doesn't cause any
     * heap allocations.
     *
     * @return the current x coordinate of the components origin.
     * @since 1.2
     */
    public int getX() {
        return x;
    }

    /**
     * Return the current y coordinate of the components origin.
     * This method is preferable to writing component.getBounds().y,
     * or component.getLocation().y because it doesn't cause any
     * heap allocations.
     *
     * @return the current y coordinate of the components origin.
     * @since 1.2
     */
    public int getY() {
        return y;
    }

    /**
     * Return the current width of this component.
     * This method is preferable to writing component.getBounds().width,
     * or component.getSize().width because it doesn't cause any
     * heap allocations.
     *
     * @return the current width of this component.
     * @since 1.2
     */
    public int getWidth() {
        return width;
    }

    /**
     * Return the current height of this component.
     * This method is preferable to writing component.getBounds().height,
     * or component.getSize().height because it doesn't cause any
     * heap allocations.
     *
     * @return the current height of this component.
     * @since 1.2
     */
    public int getHeight() {
        return height;
    }

    /**
     * Store the bounds of this component into "return value" <b>rv</b> and
     * return <b>rv</b>.  If rv is null a new Rectangle is allocated.
     * This version of getBounds() is useful if the caller
     * wants to avoid allocating a new Rectangle object on the heap.
     *
     * @param rv the return value, modified to the components bounds
     * @return rv
     */
    public Rectangle getBounds(Rectangle rv) {
        if (rv == null)
            return getBounds();
        rv.x = x;
        rv.y = y;
        rv.width = width;
        rv.height = height;
        return rv;
    }

    /**
     * Store the width/height of this component into "return value" <b>rv</b>
     * and return <b>rv</b>.   If rv is null a new Dimension object is
     * allocated.  This version of getSize() is useful if the
     * caller wants to avoid allocating a new Dimension object on the heap.
     *
     * @param rv the return value, modified to the components size
     * @return rv
     */
    public Dimension getSize(Dimension rv) {
        if (rv == null)
            return getSize();
        rv.width = width;
        rv.height = height;
        return rv;
    }

    /**
     * Store the x,y origin of this component into "return value" <b>rv</b>
     * and return <b>rv</b>.   If rv is null a new Point is allocated.
     * This version of getLocation() is useful if the
     * caller wants to avoid allocating a new Point object on the heap.
     *
     * @param rv the return value, modified to the components location
     * @return rv
     */
    public Point getLocation(Point rv) {
        if (rv == null)
            return getLocation();
        rv.x = x;
        rv.y = y;
        return rv;
    }

    /**
     * Moves and resizes this component to conform to the new
     * bounding rectangle <code>r</code>. This component's new
     * position is specified by <code>r.x</code> and <code>r.y</code>,
     * and its new size is specified by <code>r.width</code> and
     * <code>r.height</code>
     * @param r The new bounding rectangle for this component.
     * @see       java.awt.Component#getBounds
     * @see       java.awt.Component#setLocation(int, int)
     * @see       java.awt.Component#setLocation(java.awt.Point)
     * @see       java.awt.Component#setSize(int, int)
     * @see       java.awt.Component#setSize(java.awt.Dimension)
     * @since     JDK1.1
     */
    public void setBounds(Rectangle r) {
        setBounds(r.x, r.y, r.width, r.height);
    }

    /**
     * Returns true if this component is completely opaque, returns
     * false by default.
     * <p>
     * An opaque component paints every pixel within its
     * rectangular region. A non-opaque component paints only some of
     * its pixels, allowing the pixels underneath it to "show through".
     * A component that does not fully paint its pixels therefore
     * provides a degree of transparency.  Only lightweight
     * components can be transparent.
     * <p>
     * Subclasses that guarantee to always completely paint their
     * contents should override this method and return true.  All
     * of the "heavyweight" AWT components are opaque.
     *
     * @return true if this component is completely opaque.
     * @see #isLightweight
     * @since 1.2
     */
    public boolean isOpaque() {
        return displayable ? !isLightweight() : false;
    }

    /**
     * A lightweight component doesn't have a native toolkit peer.
     * Subclasses of Component and Container, other than the ones
     * defined in this package like Button or Scrollbar, are lightweight.
     * All of the Swing components are lightweights.
     *
     * This method will always return <code>false</code> if this Component
     * is not displayable because it is impossible to determine the
     * weight of an undisplayable Component.
     *
     * @return true if this component has a lightweight peer; false if
     *         it has a native peer or no peer.
     * @see #isDisplayable
     * @since 1.2
     */
    public boolean isLightweight() {
        return displayable ? (!(this instanceof Window)) : false;
    }

    /**
     * Gets the preferred size of this component.
     * @return A dimension object indicating this component's preferred size.
     * @see #getMinimumSize
     * @see java.awt.LayoutManager
     */
    public Dimension getPreferredSize() {
        /* Avoid grabbing the lock if a reasonable cached size value
         * is available.
         */

        Dimension dim = prefSize;
        if (dim != null && isValid()) {
            return dim;
        }
        synchronized (getTreeLock()) {
            prefSize = isDisplayable() ?
                    new Dimension(1, 1) : getMinimumSize();
            return prefSize;
        }
    }

    /**
     * Gets the mininimum size of this component.
     * @return A dimension object indicating this component's minimum size.
     * @see #getPreferredSize
     * @see java.awtLayoutManager
     */
    public Dimension getMinimumSize() {
        /* Avoid grabbing the lock if a reasonable cached size value
         * is available.
         */
        Dimension dim = minSize;
        if (dim != null && isValid()) {
            return dim;
        }
        synchronized (getTreeLock()) {
            minSize = isDisplayable() ?
                    new Dimension(1, 1) : getSize();
            return minSize;
        }
    }

    /**
     * Gets the maximum size of this component.
     * @return A dimension object indicating this component's maximum size.
     * @see #getMinimumSize
     * @see #getPreferredSize
     * @see LayoutManager
     */
    public Dimension getMaximumSize() {
        return new Dimension(Short.MAX_VALUE, Short.MAX_VALUE);
    }

    /**
     * Returns the alignment along the x axis.  This specifies how
     * the component would like to be aligned relative to other
     * components.  The value should be a number between 0 and 1
     * where 0 represents alignment along the origin, 1 is aligned
     * the furthest away from the origin, 0.5 is centered, etc.
     */
    public float getAlignmentX() {
        return CENTER_ALIGNMENT;
    }

    /**
     * Returns the alignment along the y axis.  This specifies how
     * the component would like to be aligned relative to other
     * components.  The value should be a number between 0 and 1
     * where 0 represents alignment along the origin, 1 is aligned
     * the furthest away from the origin, 0.5 is centered, etc.
     */
    public float getAlignmentY() {
        return CENTER_ALIGNMENT;
    }

    /**
     * Prompts the layout manager to lay out this component. This is
     * usually called when the component (more specifically, container)
     * is validated.
     * @see #validate
     * @see LayoutManager
     */
    public void doLayout() {}

    /**
     * Ensures that this component has a valid layout.  This method is
     * primarily intended to operate on instances of <code>Container</code>.
     * @see       java.awt.Component#invalidate
     * @see       java.awt.Component#doLayout()
     * @see       java.awt.LayoutManager
     * @see       java.awt.Container#validate
     * @since     JDK1.0
     */
    public void validate() {
        if (this.cursor != null)
            if (this.cursor.getType() != Cursor.DEFAULT_CURSOR)
                setCursor(this.cursor);
        valid = true;
    }

    /**
     * Invalidates this component.  This component and all parents
     * above it are marked as needing to be laid out.  This method can
     * be called often, so it needs to execute quickly.
     * @see       java.awt.Component#validate
     * @see       java.awt.Component#doLayout
     * @see       java.awt.LayoutManager
     * @since     JDK1.0
     */
    public void invalidate() {
        synchronized (getTreeLock()) {
            /* Nullify cached layout and size information.
             * For efficiency, propagate invalidate() upwards only if
             * some other component hasn't already done so first.
             */
            valid = false;
            prefSize = null;
            minSize = null;
            if (parent != null && parent.valid) {
                parent.invalidate();
            }
        }
    }

    /**
     * Creates a graphics context for this component. This method will
     * return <code>null</code> if this component is currently not
     * displayable.
     * @return A graphics context for this component, or <code>null</code>
     *             if it has none.
     * @see       java.awt.Component#paint
     * @since     JDK1.0
     */
    public Graphics getGraphics() {
        if (displayable) {
            boolean createNullGraphics = false;
            Component c = this;
            int x = 0, y = 0;
            while (c != null) {
                if (!c.visible)
                    createNullGraphics = true;
                if (c instanceof Window) {
                    Graphics g = null;
                    if (createNullGraphics)
                        g = new NullGraphics(c);
                    else g = getToolkit().getGraphics((Window) c);
                    if (g instanceof ConstrainableGraphics) {
                        ((ConstrainableGraphics) g).constrain(x, y, width, height);
                    } else {
                        g.translate(x, y);
                        g.setClip(0, 0, width, height);
                    }
                    return g;
                }
                x += c.x;
                y += c.y;
                c = c.parent;
            }
        }
        return null;
    }

    /**
     * Gets the font metrics for the specified font.
     * @param font The font for which font metrics is to be
     * obtained.
     * @return The font metrics for <code>font</code>.
     * @param     font   the font.
     * @return    the font metrics for the specified font.
     * @see       java.awt.Component#getFont
     * @see       java.awt.Component#getPeer()
     * @see       java.awt.peer.ComponentPeer#getFontMetrics(java.awt.Font)
     * @see       java.awt.Toolkit#getFontMetrics(java.awt.Font)
     * @since     JDK1.0
     */
    public FontMetrics getFontMetrics(Font font) {
        return getToolkit().getFontMetrics(font);
    }

    /**
     * Sets the cursor image to the specified cursor.  This cursor
     * image is displayed when the <code>contains</code> method for
     * this component returns true for the current cursor location, and
     * this Component is visible, displayable, and enabled. Setting the
     * cursor of a <code>Container</code> causes that cursor to be displayed
     * within all of the container's subcomponents, except for those
     * that have a non-null cursor.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param cursor One of the constants defined
     *        by the <code>Cursor</code> class.
     *        If this parameter is null then this component will inherit
     *        the cursor of its parent.
     * @see       #isEnabled
     * @see       #isShowing
     * @see       java.awt.Component#getCursor
     * @see       java.awt.Component#contains
     * @see       java.awt.Toolkit#createCustomCursor
     * @see       java.awt.Cursor
     * @since     JDK1.1
     */
    public void setCursor(Cursor cursor) {
        this.cursor = cursor;
        Frame frame = getFrame();
        if (frame != null && visible)
            frame.updateCursor(this);
    }

    /**
     * Gets the cursor set in the component. If the component does
     * not have a cursor set, the cursor of its parent is returned.
     * If no Cursor is set in the entire hierarchy, Cursor.DEFAULT_CURSOR is
     * returned.
     * @see #setCursor
     * @since      JDK1.1
     */
    public Cursor getCursor() {
        Cursor cursor = this.cursor;
        if (cursor != null) {
            return cursor;
        }
        Container parent = this.parent;
        if (parent != null) {
            return parent.getCursor();
        } else {
            return Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        }
    }

    /**
     * Returns whether the cursor has been explicitly set for this Component.
     * If this method returns <code>false</code>, this Component is inheriting
     * its cursor from an ancestor.
     *
     * @return <code>true</code> if the cursor has been explicitly set for this
     *         Component; <code>false</code> otherwise.
     * @since 1.4
     */
    public boolean isCursorSet() {
        return (cursor != null);
    }

    /**
     * Paints this component.  This method is called when the contents
     * of the component should be painted in response to the component
     * first being shown or damage needing repair.  The clip rectangle
     * in the Graphics parameter will be set to the area which needs
     * to be painted.
     * For performance reasons, Components with zero width or height
     * aren't considered to need painting when they are first shown,
     * and also aren't considered to need repair.
     * @param g The graphics context to use for painting.
     * @see       java.awt.Component#update
     * @since     JDK1.0
     */
    public void paint(Graphics g) {}

    /**
     * Updates this component.
     * <p>
     * The AWT calls the <code>update</code> method in response to a
     * call to <code>repaint</code. The appearance of the
     * component on the screen has not changed since the last call to
     * <code>update</code> or <code>paint</code>. You can assume that
     * the background is not cleared.
     * <p>
     * The <code>update</code>method of <code>Component</code>
     * does the following:
     * <p>
     * <blockquote><ul>
     * <li>Clears this component by filling it
     *      with the background color.
     * <li>Sets the color of the graphics context to be
     *     the foreground color of this component.
     * <li>Calls this component's <code>paint</code>
     *     method to completely redraw this component.
     * </ul></blockquote>
     * <p>
     * The origin of the graphics context, its
     * (<code>0</code>,&nbsp;<code>0</code>) coordinate point, is the
     * top-left corner of this component. The clipping region of the
     * graphics context is the bounding rectangle of this component.
     * @param g the specified context to use for updating.
     * @see       java.awt.Component#paint
     * @see       java.awt.Component#repaint()
     * @since     JDK1.0
     */
    public void update(Graphics g) {
        paint(g);
    }

    /**
     * Paints this component and all of its subcomponents.
     * <p>
     * The origin of the graphics context, its
     * (<code>0</code>,&nbsp;<code>0</code>) coordinate point, is the
     * top-left corner of this component. The clipping region of the
     * graphics context is the bounding rectangle of this component.
     * @param     g   the graphics context to use for painting.
     * @see       java.awt.Component#paint
     * @since     JDK1.0
     */
    public void paintAll(Graphics g) {
        if (isShowing()) {
            validate();
            paint(g);
        }
    }

    /**
     * Repaints this component.
     * <p>
     * This method causes a call to this component's <code>update</code>
     * method as soon as possible.
     * @see       java.awt.Component#update(java.awt.Graphics)
     * @since     JDK1.0
     */
    public void repaint() {
        repaint(0, 0, 0, width, height);
    }

    /**
     * Repaints the component. This will result in a
     * call to <code>update</code> within <em>tm</em> milliseconds.
     * @param tm maximum time in milliseconds before update
     * @see #paint
     * @see java.awt.Component#update(java.awt.Graphics)
     * @since JDK1.0
     */
    public void repaint(long tm) {
        repaint(tm, 0, 0, width, height);
    }

    /**
     * Repaints the specified rectangle of this component.
     * <p>
     * This method causes a call to this component's <code>update</code>
     * method as soon as possible.
     * @param     x   the <i>x</i> coordinate.
     * @param     y   the <i>y</i> coordinate.
     * @param     width   the width.
     * @param     height  the height.
     * @see       java.awt.Component#update(java.awt.Graphics)
     * @since     JDK1.0
     */
    public void repaint(int x, int y, int width, int height) {
        repaint(0, x, y, width, height);
    }

    /**
     * Repaints the specified rectangle of this component within
     * <code>tm</code> milliseconds.
     * <p>
     * This method causes a call to this component's
     * <code>update</code> method.
     * @param     tm   maximum time in milliseconds before update.
     * @param     x    the <i>x</i> coordinate.
     * @param     y    the <i>y</i> coordinate.
     * @param     width    the width.
     * @param     height   the height.
     * @see       java.awt.Component#update(java.awt.Graphics)
     * @since     JDK1.0
     */
    public void repaint(long tm, final int x, final int y, final int width, final int height) {
        if (width <= 0 || height <= 0)
            return;
        Rectangle componentArea = new Rectangle(0, 0, this.width, this.height);
        Rectangle repaintArea = new Rectangle(x, y, width, height);
        if (!componentArea.intersects(repaintArea))
            return;
        final Rectangle clippedRepaintArea = componentArea.intersection(repaintArea);
        if (displayable) {
            Component c = this;
            /* Find outer most heavyweight component to send an PaintEvent.UPDATE event to.
             Beacuse Window, Panel and Canvas are traditionally heavyweight components repaints
             should cause update to be called on these components. Calling repaint on a lightweight
             component should not call update on that component but its heavyweight ancestor. */

            while (c != null) {
                if (c instanceof Window && c.visible) {
                    //Thread.dumpStack();

                    if (tm == 0)
                        Toolkit.getEventQueue().postEvent(new PaintEvent (c, PaintEvent.UPDATE, clippedRepaintArea));
                        //SunToolkit.postEvent(c.appContext, new PaintEvent (c, PaintEvent.UPDATE, clippedRepaintArea));
                    else {
                        final int x1 = x, y1 = y;
                        final Component c1 = c;

                        // 6312976, lazy initialization
                        if (TIMER == null) {
                           TIMER = new Timer();
                        }

                        TIMER.schedule(new TimerTask() {
                                public void run() {
                                    Toolkit.getEventQueue().postEvent(new PaintEvent (c1, PaintEvent.UPDATE, clippedRepaintArea));
                                    //SunToolkit.postEvent(c1.appContext, new PaintEvent (c1, PaintEvent.UPDATE, clippedRepaintArea));
                                }
                            }, tm);
                    }
                    return;
                }
                clippedRepaintArea.x += c.x;
                clippedRepaintArea.y += c.y;
                c = c.parent;
            }
        }
    }

    /**
     * Prints this component. Applications should override this method
     * for components that must do special processing before being
     * printed or should be printed differently than they are painted.
     * <p>
     * The default implementation of this method calls the
     * <code>paint</code> method.
     * <p>
     * The origin of the graphics context, its
     * (<code>0</code>,&nbsp;<code>0</code>) coordinate point, is the
     * top-left corner of this component. The clipping region of the
     * graphics context is the bounding rectangle of this component.
     * @param     g   the graphics context to use for printing.
     * @see       java.awt.Component#paint(java.awt.Graphics)
     * @since     JDK1.0
     */
    public void print(Graphics g) {
        paint(g);
    }

    /**
     * Prints this component and all of its subcomponents.
     * <p>
     * The origin of the graphics context, its
     * (<code>0</code>,&nbsp;<code>0</code>) coordinate point, is the
     * top-left corner of this component. The clipping region of the
     * graphics context is the bounding rectangle of this component.
     * @param     g   the graphics context to use for printing.
     * @see       java.awt.Component#print(java.awt.Graphics)
     * @since     JDK1.0
     */
    public void printAll(Graphics g) {
        if (isShowing()) {
            validate();
            Graphics cg = g.create(0, 0, width, height);
            cg.setFont(getFont());
            try {
                print(g);
            } finally {
                cg.dispose();
            }
        }
    }

    /**
     * Repaints the component when the image has changed.
     * This <code>imageUpdate</code> method of an <code>ImageObserver</code>
     * is called when more information about an
     * image which had been previously requested using an asynchronous
     * routine such as the <code>drawImage</code> method of
     * <code>Graphics</code> becomes available.
     * See the definition of <code>imageUpdate</code> for
     * more information on this method and its arguments.
     * <p>
     * The <code>imageUpdate</code> method of <code>Component</code>
     * incrementally draws an image on the component as more of the bits
     * of the image are available.
     * <p>
     * If the system property <code>awt.image.incrementalDraw</code>
     * is missing or has the value <code>true</code>, the image is
     * incrementally drawn, If the system property has any other value,
     * then the image is not drawn until it has been completely loaded.
     * <p>
     * Also, if incremental drawing is in effect, the value of the
     * system property <code>awt.image.redrawrate</code> is interpreted
     * as an integer to give the maximum redraw rate, in milliseconds. If
     * the system property is missing or cannot be interpreted as an
     * integer, the redraw rate is once every 100ms.
     * <p>
     * The interpretation of the <code>x</code>, <code>y</code>,
     * <code>width</code>, and <code>height</code> arguments depends on
     * the value of the <code>infoflags</code> argument.
     *
     * @param     img   the image being observed.
     * @param     infoflags   see <code>imageUpdate</code> for more information.
     * @param     x   the <i>x</i> coordinate.
     * @param     y   the <i>y</i> coordinate.
     * @param     w   the width.
     * @param     h   the height.
     * @return    <code>false</code> if the infoflags indicate that the
     *            image is completely loaded; <code>true</code> otherwise.
     *
     * @see     java.awt.image.ImageObserver
     * @see     java.awt.Graphics#drawImage(java.awt.Image, int, int, java.awt.Color, java.awt.image.ImageObserver)
     * @see     java.awt.Graphics#drawImage(java.awt.Image, int, int, java.awt.image.ImageObserver)
     * @see     java.awt.Graphics#drawImage(java.awt.Image, int, int, int, int, java.awt.Color, java.awt.image.ImageObserver)
     * @see     java.awt.Graphics#drawImage(java.awt.Image, int, int, int, int, java.awt.image.ImageObserver)
     * @see     java.awt.image.ImageObserver#imageUpdate(java.awt.Image, int, int, int, int, int)
     * @since   JDK1.0
     */
    public boolean imageUpdate(Image img, int flags,
        int x, int y, int w, int h) {
        int rate = -1;
        if ((flags & (FRAMEBITS | ALLBITS)) != 0) {
            rate = 0;
        } else if ((flags & SOMEBITS) != 0) {
            if (isInc) {
                try {
                    rate = incRate;
                    if (rate < 0)
                        rate = 0;
                } catch (Exception e) {
                    rate = 100;
                }
            }
        }
        if (rate >= 0) {
            repaint(rate, 0, 0, width, height);
        }
        return (flags & (ALLBITS | ABORT)) == 0;
    }

    /**
     * Creates an image from the specified image producer.
     * @param     producer  the image producer
     * @return    the image produced.
     * @since     JDK1.0
     */
    public Image createImage(ImageProducer producer) {
        return getToolkit().createImage(producer);
    }

    /**
     * Creates an off-screen drawable image
     *     to be used for double buffering.
     * @param     width the specified width.
     * @param     height the specified height.
     * @return    an off-screen drawable image,
     *            which can be used for double buffering.
     * @since     JDK1.0
     */
    public Image createImage(int width, int height) {
        return (displayable) ? getToolkit().createImage(this, width, height) : null;
    }


    /**
     * Creates a volatile off-screen drawable image
     *     to be used for double buffering.
     * @param     width the specified width.
     * @param     height the specified height.
     * @return    an off-screen drawable image, which can be used for double
     *    buffering.  The return value may be <code>null</code> if the
     *    component is not displayable.  This will always happen if
     *    <code>GraphicsEnvironment.isHeadless()</code> returns
     *    <code>true</code>.
     * @see java.awt.image.VolatileImage
     * @see #isDisplayable
     * @see GraphicsEnvironment#isHeadless
     * @since     1.4
     */
    public VolatileImage createVolatileImage(int width, int height) {
        //6209548
        GraphicsConfiguration gc = getGraphicsConfiguration();
        if ( displayable && gc != null ) {
		    return gc.createCompatibleVolatileImage(width, height);
        }
        return null;
        //6209548
    }

    /**
     * Creates a volatile off-screen drawable image, with the given capabilities.
     * The contents of this image may be lost at any time due
     * to operating system issues, so the image must be managed
     * via the <code>VolatileImage</code> interface.
     * @param width the specified width.
     * @param height the specified height.
     * @param caps the image capabilities
     * @exception AWTException if an image with the specified capabilities cannot
     * be created
     * @return a VolatileImage object, which can be used
     * to manage surface contents loss and capabilities.
     * @see java.awt.image.VolatileImage
     * @since 1.4
     */
    public VolatileImage createVolatileImage(int width, int height, ImageCapabilities caps) throws AWTException {
        //6209548
        GraphicsConfiguration gc = getGraphicsConfiguration();
        if ( displayable && gc != null ) {
		    return gc.createCompatibleVolatileImage(width, height, caps);
        }
        return null;
        //6209548
    }

    /**
     * Prepares an image for rendering on this component.  The image
     * data is downloaded asynchronously in another thread and the
     * appropriate screen representation of the image is generated.
     * @param     image   the <code>Image</code> for which to
     *                    prepare a screen representation.
     * @param     observer   the <code>ImageObserver</code> object
     *                       to be notified as the image is being prepared.
     * @return    <code>true</code> if the image has already been fully prepared;
     <code>false</code> otherwise.
     * @since     JDK1.0
     */
    public boolean prepareImage(Image image, ImageObserver observer) {
        return prepareImage(image, -1, -1, observer);
    }

    /**
     * Prepares an image for rendering on this component at the
     * specified width and height.
     * <p>
     * The image data is downloaded asynchronously in another thread,
     * and an appropriately scaled screen representation of the image is
     * generated.
     * @param     image    the instance of <code>Image</code>
     *            for which to prepare a screen representation.
     * @param     width    the width of the desired screen representation.
     * @param     height   the height of the desired screen representation.
     * @param     observer   the <code>ImageObserver</code> object
     *            to be notified as the image is being prepared.
     * @return    <code>true</code> if the image has already been fully prepared;
     <code>false</code> otherwise.
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public boolean prepareImage(Image image, int width, int height,
        ImageObserver observer) {
        return getToolkit().prepareImage(image, width, height, observer);
    }

    /**
     * Returns the status of the construction of a screen representation
     * of the specified image.
     * <p>
     * This method does not cause the image to begin loading. An
     * application must use the <code>prepareImage</code> method
     * to force the loading of an image.
     * <p>
     * Information on the flags returned by this method can be found
     * with the discussion of the <code>ImageObserver</code> interface.
     * @param     image   the <code>Image</code> object whose status
     *            is being checked.
     * @param     observer   the <code>ImageObserver</code>
     *            object to be notified as the image is being prepared.
     * @return  the bitwise inclusive <b>OR</b> of
     *            <code>ImageObserver</code> flags indicating what
     *            information about the image is currently available.
     * @see      java.awt.Component#prepareImage(java.awt.Image, int, int, java.awt.image.ImageObserver)
     * @see      java.awt.Toolkit#checkImage(java.awt.Image, int, int, java.awt.image.ImageObserver)
     * @see      java.awt.image.ImageObserver
     * @since    JDK1.0
     */
    public int checkImage(Image image, ImageObserver observer) {
        return checkImage(image, -1, -1, observer);
    }

    /**
     * Returns the status of the construction of a screen representation
     * of the specified image.
     * <p>
     * This method does not cause the image to begin loading. An
     * application must use the <code>prepareImage</code> method
     * to force the loading of an image.
     * <p>
     * The <code>checkImage</code> method of <code>Component</code>
     * calls its peer's <code>checkImage</code> method to calculate
     * the flags. If this component does not yet have a peer, the
     * component's toolkit's <code>checkImage</code> method is called
     * instead.
     * <p>
     * Information on the flags returned by this method can be found
     * with the discussion of the <code>ImageObserver</code> interface.
     * @param     image   the <code>Image</code> object whose status
     *                    is being checked.
     * @param     width   the width of the scaled version
     *                    whose status is to be checked.
     * @param     height  the height of the scaled version
     *                    whose status is to be checked.
     * @param     observer   the <code>ImageObserver</code> object
     *                    to be notified as the image is being prepared.
     * @return    the bitwise inclusive <b>OR</b> of
     *            <code>ImageObserver</code> flags indicating what
     *            information about the image is currently available.
     * @see      java.awt.Component#prepareImage(java.awt.Image, int, int, java.awt.image.ImageObserver)
     * @see      java.awt.Toolkit#checkImage(java.awt.Image, int, int, java.awt.image.ImageObserver)
     * @see      java.awt.image.ImageObserver
     * @since    JDK1.0
     */
    public int checkImage(Image image, int width, int height,
        ImageObserver observer) {
        return getToolkit().checkImage(image, width, height, observer);
    }

    /**
     * Sets whether or not paint messages received from the operating system
     * should be ignored.  This does not affect paint events generated in
     * software by the AWT, unless they are an immediate response to an
     * OS-level paint message.
     * <p>
     * This is useful, for example, if running under full-screen mode and
     * better performance is desired, or if page-flipping is used as the
     * buffer strategy.
     *
     * @since 1.4
     * @see #getIgnoreRepaint
     * @see Canvas#createBufferStrategy
     * @see Window#createBufferStrategy
     * @see java.awt.image.BufferStrategy
     * @see GraphicsDevice#setFullScreenWindow
     */
    public void setIgnoreRepaint(boolean ignoreRepaint) {
        this.ignoreRepaint = ignoreRepaint;
    }

    /**
     * @return whether or not paint messages received from the operating system
     * should be ignored.
     *
     * @since 1.4
     * @see #setIgnoreRepaint
     */
    public boolean getIgnoreRepaint() {
        return ignoreRepaint;
    }

    /**
     * Checks whether this component "contains" the specified point,
     * where <code>x</code> and <code>y</code> are defined to be
     * relative to the coordinate system of this component.
     * @param     x   the <i>x</i> coordinate of the point.
     * @param     y   the <i>y</i> coordinate of the point.
     * @see       java.awt.Component#getComponentAt(int, int)
     * @since     JDK1.1
     */
    public boolean contains(int x, int y) {
        return (x >= 0) && (x < width) && (y >= 0) && (y < height);
    }

    /**
     * Checks whether this component "contains" the specified point,
     * where the point's <i>x</i> and <i>y</i> coordinates are defined
     * to be relative to the coordinate system of this component.
     * @param     p     the point.
     * @see       java.awt.Component#getComponentAt(java.awt.Point)
     * @since     JDK1.1
     */
    public boolean contains(Point p) {
        return contains(p.x, p.y);
    }

    /**
     * Determines if this component or one of its immediate
     * subcomponents contains the (<i>x</i>,&nbsp;<i>y</i>) location,
     * and if so, returns the containing component. This method only
     * looks one level deep. If the point (<i>x</i>,&nbsp;<i>y</i>) is
     * inside a subcomponent that itself has subcomponents, it does not
     * go looking down the subcomponent tree.
     * <p>
     * The <code>locate</code> method of <code>Component</code> simply
     * returns the component itself if the (<i>x</i>,&nbsp;<i>y</i>)
     * coordinate location is inside its bounding box, and <code>null</code>
     * otherwise.
     * @param     x   the <i>x</i> coordinate.
     * @param     y   the <i>y</i> coordinate.
     * @return    the component or subcomponent that contains the
     *                (<i>x</i>,&nbsp;<i>y</i>) location;
     *                <code>null</code> if the location
     *                is outside this component.
     * @see       java.awt.Component#contains(int, int)
     * @since     JDK1.0
     */
    public Component getComponentAt(int x, int y) {
        return contains(x, y) ? this : null;
    }

    /**
     * Returns the component or subcomponent that contains the
     * specified point.
     * @param     p   the point.
     * @see       java.awt.Component#contains
     * @since     JDK1.1
     */
    public Component getComponentAt(Point p) {
        return getComponentAt(p.x, p.y);
    }

    /**
     * Dispatches an event to this component or one of its sub components.
     * Calls processEvent() before returning for 1.1-style events which
     * have been enabled for the Component.
     * @param e the event
     */
    public final void dispatchEvent(AWTEvent e) {
        dispatchEventImpl(e);
    }

    void dispatchEventImpl(AWTEvent e) {
        int id = e.getID();
     // 0. Set timestamp and modifiers of current event.
        EventQueue.setCurrentEventAndMostRecentTime(e);
     // 1. Pre-dispatch to KeyboardFocusManager.
        if (!e.focusManagerIsDispatching) {
            if (KeyboardFocusManager.getCurrentKeyboardFocusManager().
                dispatchEvent(e)) {
                return;
            }
        }
     // 2. Allow the Toolkit to pass this to AWTEventListeners.
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        toolkit.notifyAWTEventListeners(e);
     // 3. Allow the KeyboardFocusManager to process key event if not consumed.
        if (!e.isConsumed()) {
            if (e instanceof java.awt.event.KeyEvent) {
                KeyboardFocusManager.getCurrentKeyboardFocusManager().
                    processKeyEvent(this, (KeyEvent)e);
                if (e.isConsumed()) {
                    return;
                }
            }
        }
        /*
         * 4. Allow input methods to process the event
         */
        if (areInputMethodsEnabled()
            && ( 
            // We need to pass on InputMethodEvents since some host
            // input method adapters send them through the Java
            // event queue instead of directly to the component,
            // and the input context also handles the Java composition window
                //((e instanceof InputMethodEvent) && !(this instanceof CompositionArea))
                (e instanceof InputMethodEvent)
            ||
            // Otherwise, we only pass on input and focus events, because
            // a) input methods shouldn't know about semantic or component-level events
            // b) passing on the events takes time
            // c) isConsumed() is always true for semantic events.
            (e instanceof InputEvent) || (e instanceof FocusEvent))) {
            InputContext inputContext = getInputContext();
            if (inputContext != null) {
                inputContext.dispatchEvent(e);
                if (e.isConsumed()) {
                    return;
                }
            }
        }

     // 5. Pre-process any special events before delivery.
        switch(id) {
        case PaintEvent.PAINT:
        case PaintEvent.UPDATE:
            Graphics g = getGraphics();
            if (g != null) {
                try {
                    Rectangle clip = ((PaintEvent) e).getUpdateRect();
                    g.clipRect(clip.x, clip.y, clip.width, clip.height);
                    if (id == PaintEvent.PAINT) {
                        // If a paint event is received for a component that is traditionally
                        // heavyweight then it is necessary to clear the background as this
                        // would normally be done by the heavyweight component prior to posting
                        // a paint event to the event queue.
                        if (this instanceof Window) {
                            g.clearRect(0, 0, width, height);
                        }
                        paint(g);
                    } else {
                        update(g);
                    }
                    getToolkit().sync();
                } finally {
                    g.dispose();
                }
            }
            break;
        case KeyEvent.KEY_PRESSED:
        case KeyEvent.KEY_RELEASED:
            Container p = (Container)((this instanceof Container) ? this : parent);
            if (p != null) {
                p.preProcessKeyEvent((KeyEvent)e);
                if (e.isConsumed()) {
                    return;
                }
            }
            break;
        default:
            break;
        }
        // 6. Deliver event for normal processing.
        if (eventEnabled(e)) {
            processEvent(e);
        }

    } // dispatchEventImpl()


    boolean eventEnabled(AWTEvent e) {
        int type = e.id;
        switch (type) {
        case ComponentEvent.COMPONENT_MOVED:
        case ComponentEvent.COMPONENT_RESIZED:
        case ComponentEvent.COMPONENT_SHOWN:
        case ComponentEvent.COMPONENT_HIDDEN:
            if ((eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0 ||
                componentListener != null) {
                return true;
            }
            break;

        case FocusEvent.FOCUS_GAINED:
        case FocusEvent.FOCUS_LOST:
            if ((eventMask & AWTEvent.FOCUS_EVENT_MASK) != 0 ||
                focusListener != null) {
                return true;
            }
            break;

        case KeyEvent.KEY_PRESSED:
        case KeyEvent.KEY_RELEASED:
        case KeyEvent.KEY_TYPED:
            if ((eventMask & AWTEvent.KEY_EVENT_MASK) != 0 ||
                keyListener != null) {
                return true;
            }
            break;

        case MouseEvent.MOUSE_PRESSED:
        case MouseEvent.MOUSE_RELEASED:
        case MouseEvent.MOUSE_ENTERED:
        case MouseEvent.MOUSE_EXITED:
        case MouseEvent.MOUSE_CLICKED:
            if ((eventMask & AWTEvent.MOUSE_EVENT_MASK) != 0 ||
                mouseListener != null) {
                return true;
            }
            break;
        case MouseEvent.MOUSE_WHEEL:
            if ((eventMask & AWTEvent.MOUSE_WHEEL_EVENT_MASK) != 0 ||
                mouseWheelListener != null) {
                return true;
            }
            break;

        case InputMethodEvent.INPUT_METHOD_TEXT_CHANGED:
        case InputMethodEvent.CARET_POSITION_CHANGED:
            if ((eventMask & AWTEvent.INPUT_METHOD_EVENT_MASK) != 0 ||
                    inputMethodListener != null) {
                return true;
            }
            break;

        case MouseEvent.MOUSE_MOVED:
        case MouseEvent.MOUSE_DRAGGED:
            if ((eventMask & AWTEvent.MOUSE_MOTION_EVENT_MASK) != 0 ||
                mouseMotionListener != null) {
                return true;
            }
            break;

        case ActionEvent.ACTION_PERFORMED:
            if ((eventMask & AWTEvent.ACTION_EVENT_MASK) != 0) {
                return true;
            }
            break;

        case TextEvent.TEXT_VALUE_CHANGED:
            if ((eventMask & AWTEvent.TEXT_EVENT_MASK) != 0) {
                return true;
            }
            break;

        case ItemEvent.ITEM_STATE_CHANGED:
            if ((eventMask & AWTEvent.ITEM_EVENT_MASK) != 0) {
                return true;
            }
            break;

        case AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED:
            if ((eventMask & AWTEvent.ADJUSTMENT_EVENT_MASK) != 0) {
                return true;
            }
            break;

        default:
            break;
        }
        //
        // Always pass on events defined by external programs.
        //
        if (type > AWTEvent.RESERVED_ID_MAX) {
            return true;
        }
        return false;
    }

    // Event source interfaces

    /**
     * Adds the specified component listener to receive component events from
     * this component.
     * If l is null, no exception is thrown and no action is performed.
     * @param    l   the component listener.
     * @see      java.awt.event.ComponentEvent
     * @see      java.awt.event.ComponentListener
     * @see      java.awt.Component#removeComponentListener
     * @since    JDK1.1
     */
    public synchronized void addComponentListener(ComponentListener l) {
        if (l == null) {
            return;
        }
        componentListener = AWTEventMulticaster.add(componentListener, l);
    }

    /**
     * Removes the specified component listener so that it no longer
     * receives component events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If l is null, no exception is thrown and no action is performed.
     * @param    l   the component listener.
     * @see      java.awt.event.ComponentEvent
     * @see      java.awt.event.ComponentListener
     * @see      java.awt.Component#addComponentListener
     * @since    JDK1.1
     */
    public synchronized void removeComponentListener(ComponentListener l) {
        if (l == null) {
            return;
        }
        componentListener = AWTEventMulticaster.remove(componentListener, l);
    }

    /**
     * Returns an array of all the component listeners
     * registered on this component.
     *
     * @return all of this comonent's <code>ComponentListener</code>s
     *         or an empty array if no component
     *         listeners are currently registered
     *
     * @see #addComponentListener
     * @see #removeComponentListener
     * @since 1.4
     */
    public synchronized ComponentListener[] getComponentListeners() {
        return (ComponentListener[]) AWTEventMulticaster.getListeners(
                                     (EventListener)componentListener,
                                     ComponentListener.class);
    }


    /**
     * Adds the specified focus listener to receive focus events from
     * this component when this component gains input focus.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the focus listener.
     * @see      java.awt.event.FocusEvent
     * @see      java.awt.event.FocusListener
     * @see      java.awt.Component#removeFocusListener
     * @since    JDK1.1
     */
    public synchronized void addFocusListener(FocusListener l) {
        if (l == null) {
            return;
        }
        focusListener = AWTEventMulticaster.add(focusListener, l);
    }

    /**
     * Removes the specified focus listener so that it no longer
     * receives focus events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the focus listener.
     * @see      java.awt.event.FocusEvent
     * @see      java.awt.event.FocusListener
     * @see      java.awt.Component#addFocusListener
     * @since    JDK1.1
     */
    public synchronized void removeFocusListener(FocusListener l) {
        if (l == null) {
            return;
        }
        focusListener = AWTEventMulticaster.remove(focusListener, l);
    }

    /**
     * Returns an array of all the focus listeners
     * registered on this component.
     *
     * @return all of this component's <code>FocusListener</code>s
     *         or an empty array if no component
     *         listeners are currently registered
     *
     * @see #addFocusListener
     * @see #removeFocusListener
     * @since 1.4
     */
    public synchronized FocusListener[] getFocusListeners() {
        return (FocusListener[]) AWTEventMulticaster.getListeners(
                                     (EventListener)focusListener,
                                     FocusListener.class);
    }

    /**
     * Adds the specified key listener to receive key events from
     * this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the key listener.
     * @see      java.awt.event.KeyEvent
     * @see      java.awt.event.KeyListener
     * @see      java.awt.Component#removeKeyListener
     * @since    JDK1.1
     */
    public synchronized void addKeyListener(KeyListener l) {
        if (l == null) {
            return;
        }
        keyListener = AWTEventMulticaster.add(keyListener, l);
    }

    /**
     * Removes the specified key listener so that it no longer
     * receives key events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the key listener.
     * @see      java.awt.event.KeyEvent
     * @see      java.awt.event.KeyListener
     * @see      java.awt.Component#addKeyListener
     * @since    JDK1.1
     */
    public synchronized void removeKeyListener(KeyListener l) {
        if (l == null) {
            return;
        }
        keyListener = AWTEventMulticaster.remove(keyListener, l);
    }

    /**
     * Returns an array of all the key listeners
     * registered on this component.
     *
     * @return all of this component's <code>KeyListener</code>s
     *         or an empty array if no key
     *         listeners are currently registered
     *
     * @see      #addKeyListener
     * @see      #removeKeyListener
     * @since    1.4
     */
    public synchronized KeyListener[] getKeyListeners() {
        return (KeyListener[]) AWTEventMulticaster.getListeners(
                                     (EventListener)keyListener,
                                     KeyListener.class);
    }

    /**
     * Adds the specified mouse listener to receive mouse events from
     * this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the mouse listener.
     * @see      java.awt.event.MouseEvent
     * @see      java.awt.event.MouseListener
     * @see      java.awt.Component#removeMouseListener
     * @since    JDK1.1
     */
    public synchronized void addMouseListener(MouseListener l) {
        if (l == null) {
            return;
        }
        mouseListener = AWTEventMulticaster.add(mouseListener, l);
    }

    /**
     * Removes the specified mouse listener so that it no longer
     * receives mouse events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the mouse listener.
     * @see      java.awt.event.MouseEvent
     * @see      java.awt.event.MouseListener
     * @see      java.awt.Component#addMouseListener
     * @since    JDK1.1
     */
    public synchronized void removeMouseListener(MouseListener l) {
        if (l == null) {
            return;
        }
        mouseListener = AWTEventMulticaster.remove(mouseListener, l);
    }

    /**
     * Returns an array of all the mouse listeners
     * registered on this component.
     *
     * @return all of this component's <code>MouseListener</code>s
     *         or an empty array if no mouse
     *         listeners are currently registered
     *
     * @see      #addMouseListener
     * @see      #removeMouseListener
     * @since    1.4
     */
    public synchronized MouseListener[] getMouseListeners() {
        return (MouseListener[]) AWTEventMulticaster.getListeners(
                                     (EventListener)mouseListener,
                                     MouseListener.class);
    }
    /**
     * Adds the specified mouse motion listener to receive mouse motion events from
     * this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the mouse motion listener.
     * @see      java.awt.event.MouseMotionEvent
     * @see      java.awt.event.MouseMotionListener
     * @see      java.awt.Component#removeMouseMotionListener
     * @since    JDK1.1
     */
    public synchronized void addMouseMotionListener(MouseMotionListener l) {
        if (l == null) {
            return;
        }
        mouseMotionListener = AWTEventMulticaster.add(mouseMotionListener, l);
    }

    /**
     * Removes the specified mouse motion listener so that it no longer
     * receives mouse motion events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the mouse motion listener.
     * @see      java.awt.event.MouseMotionEvent
     * @see      java.awt.event.MouseMotionListener
     * @see      java.awt.Component#addMouseMotionListener
     * @since    JDK1.1
     */
    public synchronized void removeMouseMotionListener(MouseMotionListener l) {
        if (l == null) {
            return;
        }
        mouseMotionListener = AWTEventMulticaster.remove(mouseMotionListener, l);
    }

    /**
     * Returns an array of all the mouse motion listeners
     * registered on this component.
     *
     * @return all of this component's <code>MouseMotionListener</code>s
     *         or an empty array if no mouse motion
     *         listeners are currently registered
     *
     * @see      #addMouseMotionListener
     * @see      #removeMouseMotionListener
     * @since    1.4
     */
    public synchronized MouseMotionListener[] getMouseMotionListeners() {
        return (MouseMotionListener[]) AWTEventMulticaster.getListeners(
                                     (EventListener)mouseMotionListener,
                                     MouseMotionListener.class);

    }


    /**
     * Adds the specified mouse wheel listener to receive mouse wheel events
     * from this component.  Containers also receive mouse wheel events from
     * sub-components.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the mouse wheel listener.
     * @see      java.awt.event.MouseWheelEvent
     * @see      java.awt.event.MouseWheelListener
     * @see      #removeMouseWheelListener
     * @see      #getMouseWheelListeners
     * @since    1.4
     */
    public synchronized void addMouseWheelListener(MouseWheelListener l) {
        if (l == null) {
            return;
        }
        mouseWheelListener = AWTEventMulticaster.add(mouseWheelListener,l);
        newEventsOnly = true;
    }

    /**
     * Removes the specified mouse wheel listener so that it no longer
     * receives mouse wheel events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If l is null, no exception is thrown and no action is performed.
     *
     * @param    l   the mouse wheel listener.
     * @see      java.awt.event.MouseWheelEvent
     * @see      java.awt.event.MouseWheelListener
     * @see      #addMouseWheelListener
     * @see      #getMouseWheelListeners
     * @since    1.4
     */
    public synchronized void removeMouseWheelListener(MouseWheelListener l) {
        if (l == null) {
            return;
        }
        mouseWheelListener = AWTEventMulticaster.remove(mouseWheelListener, l);
    }
    /**
     * Returns an array of all the mouse wheel listeners
     * registered on this component.
     *
     * @return all of this component's <code>MouseWheelListener</code>s
     *         or an empty array if no mouse wheel
     *         listeners are currently registered
     *
     * @see      #addMouseWheelListener
     * @see      #removeMouseWheelListener
     * @since    1.4
     */
    public synchronized MouseWheelListener[] getMouseWheelListeners() {
        return (MouseWheelListener[]) AWTEventMulticaster.getListeners(
                                     (EventListener)mouseWheelListener,
                                     MouseWheelListener.class);
    }

    /**
     * Adds the specified input method listener to receive
     * input method events from this component. A component will
     * only receive input method events from input methods
     * if it also overrides <code>getInputMethodRequests</code> to return an
     * <code>InputMethodRequests</code> instance.
     * If listener <code>l</code> is <code>null</code>,
     * no exception is thrown and no action is performed.
     *
     * @param    l   the input method listener
     * @see      java.awt.event.InputMethodEvent
     * @see      java.awt.event.InputMethodListener
     * @see      #removeInputMethodListener
     * @see      #getInputMethodListeners
     * @see      #getInputMethodRequests
     * @since    1.2
     */
    public synchronized void addInputMethodListener(InputMethodListener l) {
        if (l == null) {
            return;
        }
        inputMethodListener = AWTEventMulticaster.add(inputMethodListener, l);
        newEventsOnly = true;
    }

    /**
     * Removes the specified input method listener so that it no longer
     * receives input method events from this component. This method performs
     * no function, nor does it throw an exception, if the listener
     * specified by the argument was not previously added to this component.
     * If listener <code>l</code> is <code>null</code>,
     * no exception is thrown and no action is performed.
     *
     * @param    l   the input method listener
     * @see      java.awt.event.InputMethodEvent
     * @see      java.awt.event.InputMethodListener
     * @see      #addInputMethodListener
     * @see      #getInputMethodListeners
     * @since    1.2
     */
    public synchronized void removeInputMethodListener(InputMethodListener l) {
        if (l == null) {
            return;
        }
        inputMethodListener = AWTEventMulticaster.remove(inputMethodListener, l);
    }

    /**
     * Returns an array of all the input method listeners
     * registered on this component.
     *
     * @return all of this component's <code>InputMethodListener</code>s
     *         or an empty array if no input method
     *         listeners are currently registered
     *
     * @see      #addInputMethodListener
     * @see      #removeInputMethodListener
     * @since    1.4
     */
    public synchronized InputMethodListener[] getInputMethodListeners() {
        return (InputMethodListener[]) AWTEventMulticaster.getListeners(
                                       (EventListener)inputMethodListener,
                                       InputMethodListener.class);
    }

    /**
     * Gets the input method request handler which supports
     * requests from input methods for this component. A component
     * that supports on-the-spot text input must override this
     * method to return an <code>InputMethodRequests</code> instance.
     * At the same time, it also has to handle input method events.
     *
     * @return the input method request handler for this component,
     *          <code>null</code> by default
     * @see #addInputMethodListener
     * @since 1.2
     */
    public InputMethodRequests getInputMethodRequests() {
        return null;
    }

    /**
     * Gets the input context used by this component for handling
     * the communication with input methods when text is entered
     * in this component. By default, the input context used for
     * the parent component is returned. Components may
     * override this to return a private input context.
     *
     * @return the input context used by this component;
     *          <code>null</code> if no context can be determined
     * @since 1.2
     */
    public InputContext getInputContext() {
        Container parent = this.parent;
        if (parent == null) {
            return null;
        } else {
            return parent.getInputContext();
        }
    }

    /**
     * Enables the events defined by the specified event mask parameter
     * to be delivered to this component.
     * <p>
     * Event types are automatically enabled when a listener for
     * that event type is added to the component.
     * <p>
     * This method only needs to be invoked by subclasses of
     * <code>Component</code> which desire to have the specified event
     * types delivered to <code>processEvent</code> regardless of whether
     * or not a listener is registered.
     * @param      eventsToEnable   the event mask defining the event types.
     * @see        java.awt.Component#processEvent
     * @see        java.awt.Component#disableEvents
     * @since      JDK1.1
     */
    protected final synchronized void enableEvents(long eventsToEnable) {
        eventMask |= eventsToEnable;
    }

    /**
     * Disables the events defined by the specified event mask parameter
     * from being delivered to this component.
     * @param      eventsToDisable   the event mask defining the event types
     * @see        java.awt.Component#enableEvents
     * @since      JDK1.1
     */
    protected final synchronized void disableEvents(long eventsToDisable) {
        eventMask &= ~eventsToDisable;
    }

    /**
     * Potentially coalesce an event being posted with an existing
     * event.  This method is called by EventQueue.postEvent if an
     * event with the same ID as the event to be posted is found in
     * the queue (both events must have this component as their source).
     * This method either returns a coalesced event which replaces
     * the existing event (and the new event is then discarded), or
     * null to indicate that no combining should be done (add the
     * second event to the end of the queue).  Either event parameter
     * may be modified and returned, as the other one is discarded
     * unless null is returned.
     * <p>
     * This implementation of coalesceEvents coalesces two event types:
     * mouse move (and drag) events, and paint (and update) events.
     * For mouse move events the last event is always returned, causing
     * intermediate moves to be discarded.  For paint events, the new
     * event is coalesced into a complex RepaintArea in the peer.  The
     * new Event is always returned.
     *
     * @param  existingEvent  the event already on the EventQueue.
     * @param  newEvent       the event being posted to the EventQueue.
     * @return a coalesced event, or null indicating that no coalescing
     *         was done.
     */
    protected AWTEvent coalesceEvents(AWTEvent existingEvent,
        AWTEvent newEvent) {
        int id = existingEvent.getID();
        switch (id) {
        case MouseEvent.MOUSE_MOVED:
        case MouseEvent.MOUSE_DRAGGED: {
                MouseEvent e = (MouseEvent) existingEvent;
                if (e.getModifiers() == ((MouseEvent) newEvent).getModifiers()) {
                    // Just return the newEvent, causing the old to be
                    // discarded.
                    return newEvent;
                }
                break;
            }

        case PaintEvent.PAINT:
        case PaintEvent.UPDATE: {
                // This approach to coalescing paint events seems to be
                // better than any heuristic for unioning rectangles.
                PaintEvent existingPaintEvent = (PaintEvent) existingEvent;
                PaintEvent newPaintEvent = (PaintEvent) newEvent;
                Rectangle existingRect = existingPaintEvent.getUpdateRect();
                Rectangle newRect = newPaintEvent.getUpdateRect();
                if (existingRect.contains(newRect)) {
                    return existingEvent;
                }
                if (newRect.contains(existingRect)) {
                    return newEvent;
                }
                break;
            }
        }
        return null;
    }

    /**
     * Processes events occurring on this component. By default this
     * method calls the appropriate
     * <code>process&lt;event&nbsp;type&gt;Event</code>
     * method for the given class of event.
     * @param     e the event.
     * @see       java.awt.Component#processComponentEvent
     * @see       java.awt.Component#processFocusEvent
     * @see       java.awt.Component#processKeyEvent
     * @see       java.awt.Component#processMouseEvent
     * @see       java.awt.Component#processMouseMotionEvent
     * @see       java.awt.Component#processInputMethodEvent
     * @see       java.awt.Component#processHierarchyEvent
     * @since     JDK1.1
     */
    protected void processEvent(AWTEvent e) {
        if (e instanceof FocusEvent) {
            processFocusEvent((FocusEvent) e);
        } else if (e instanceof MouseEvent) {
            switch (e.getID()) {
            case MouseEvent.MOUSE_PRESSED:
            case MouseEvent.MOUSE_RELEASED:
            case MouseEvent.MOUSE_CLICKED:
            case MouseEvent.MOUSE_ENTERED:
            case MouseEvent.MOUSE_EXITED:
                processMouseEvent((MouseEvent) e);
                break;

            case MouseEvent.MOUSE_MOVED:
            case MouseEvent.MOUSE_DRAGGED:
                processMouseMotionEvent((MouseEvent) e);
                break;
              case MouseEvent.MOUSE_WHEEL:
                processMouseWheelEvent((MouseWheelEvent)e);
                break;
            }
        } else if (e instanceof KeyEvent) {
            processKeyEvent((KeyEvent) e);
        } else if (e instanceof ComponentEvent) {
            processComponentEvent((ComponentEvent) e);
        } else if (e instanceof InputMethodEvent) {
            processInputMethodEvent((InputMethodEvent)e);
        }
    }

    /**
     * Processes component events occurring on this component by
     * dispatching them to any registered
     * <code>ComponentListener</code> objects.
     * <p>
     * This method is not called unless component events are
     * enabled for this component. Component events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>A <code>ComponentListener</code> object is registered
     * via <code>addComponentListener</code>.
     * <li>Component events are enabled via <code>enableEvents</code>.
     * </ul>
     * @param       e the component event.
     * @see         java.awt.event.ComponentEvent
     * @see         java.awt.event.ComponentListener
     * @see         java.awt.Component#addComponentListener
     * @see         java.awt.Component#enableEvents
     * @since       JDK1.1
     */
    protected void processComponentEvent(ComponentEvent e) {
        ComponentListener listener = componentListener;
        if (listener != null) {
            int id = e.getID();
            switch (id) {
            case ComponentEvent.COMPONENT_RESIZED:
                listener.componentResized(e);
                break;

            case ComponentEvent.COMPONENT_MOVED:
                listener.componentMoved(e);
                break;

            case ComponentEvent.COMPONENT_SHOWN:
                listener.componentShown(e);
                break;

            case ComponentEvent.COMPONENT_HIDDEN:
                listener.componentHidden(e);
                break;
            }
        }
    }

    /**
     * Processes focus events occurring on this component by
     * dispatching them to any registered
     * <code>FocusListener</code> objects.
     * <p>
     * This method is not called unless focus events are
     * enabled for this component. Focus events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>A <code>FocusListener</code> object is registered
     * via <code>addFocusListener</code>.
     * <li>Focus events are enabled via <code>enableEvents</code>.
     * </ul>
     * @param       e the focus event.
     * @see         java.awt.event.FocusEvent
     * @see         java.awt.event.FocusListener
     * @see         java.awt.Component#addFocusListener
     * @see         java.awt.Component#enableEvents
     * @since       JDK1.1
     */
    protected void processFocusEvent(FocusEvent e) {
        FocusListener listener = focusListener;
        if (listener != null) {
            int id = e.getID();
            switch (id) {
            case FocusEvent.FOCUS_GAINED:
                listener.focusGained(e);
                break;

            case FocusEvent.FOCUS_LOST:
                listener.focusLost(e);
                break;
            }
        }
    }

    /**
     * Processes key events occurring on this component by
     * dispatching them to any registered
     * <code>KeyListener</code> objects.
     * <p>
     * This method is not called unless key events are
     * enabled for this component. Key events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>A <code>KeyListener</code> object is registered
     * via <code>addKeyListener</code>.
     * <li>Key events are enabled via <code>enableEvents</code>.
     * </ul>
     * @param       e the key event.
     * @see         java.awt.event.KeyEvent
     * @see         java.awt.event.KeyListener
     * @see         java.awt.Component#addKeyListener
     * @see         java.awt.Component#enableEvents
     * @since       JDK1.1
     */
    protected void processKeyEvent(KeyEvent e) {
        KeyListener listener = keyListener;
        if (listener != null) {
            int id = e.getID();
            switch (id) {
            case KeyEvent.KEY_TYPED:
                listener.keyTyped(e);
                break;

            case KeyEvent.KEY_PRESSED:
                listener.keyPressed(e);
                break;

            case KeyEvent.KEY_RELEASED:
                listener.keyReleased(e);
                break;
            }
        }
    }

    /**
     * Processes input method events occurring on this component by
     * dispatching them to any registered
     * <code>InputMethodListener</code> objects.
     * <p>
     * This method is not called unless input method events
     * are enabled for this component. Input method events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>An <code>InputMethodListener</code> object is registered
     * via <code>addInputMethodListener</code>.
     * <li>Input method events are enabled via <code>enableEvents</code>.
     * </ul>
     * <p>Note that if the event parameter is <code>null</code>
     * the behavior is unspecified and may result in an
     * exception.
     *
     * @param       e the input method event
     * @see         java.awt.event.InputMethodEvent
     * @see         java.awt.event.InputMethodListener
     * @see         #addInputMethodListener
     * @see         #enableEvents
     * @since       1.2
     */
    protected void processInputMethodEvent(InputMethodEvent e) {
        InputMethodListener listener = inputMethodListener;
        if (listener != null) {
            int id = e.getID();
            switch (id) {
              case InputMethodEvent.INPUT_METHOD_TEXT_CHANGED:
                listener.inputMethodTextChanged(e);
                break;
              case InputMethodEvent.CARET_POSITION_CHANGED:
                listener.caretPositionChanged(e);
                break;
            }
        }
    }

    /**
     * Processes mouse events occurring on this component by
     * dispatching them to any registered
     * <code>MouseListener</code> objects.
     * <p>
     * This method is not called unless mouse events are
     * enabled for this component. Mouse events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>A <code>MouseListener</code> object is registered
     * via <code>addMouseListener</code>.
     * <li>Mouse events are enabled via <code>enableEvents</code>.
     * </ul>
     * @param       e the mouse event.
     * @see         java.awt.event.MouseEvent
     * @see         java.awt.event.MouseListener
     * @see         java.awt.Component#addMouseListener
     * @see         java.awt.Component#enableEvents
     * @since       JDK1.1
     */
    protected void processMouseEvent(MouseEvent e) {
        MouseListener listener = mouseListener;
        if (listener != null) {
            int id = e.getID();
            switch (id) {
            case MouseEvent.MOUSE_PRESSED:
                listener.mousePressed(e);
                break;

            case MouseEvent.MOUSE_RELEASED:
                listener.mouseReleased(e);
                break;

            case MouseEvent.MOUSE_CLICKED:
                listener.mouseClicked(e);
                break;

            case MouseEvent.MOUSE_EXITED:
                listener.mouseExited(e);
                break;

            case MouseEvent.MOUSE_ENTERED:
                listener.mouseEntered(e);
                break;
            }
        }
    }

    /**
     * Processes mouse motion events occurring on this component by
     * dispatching them to any registered
     * <code>MouseMotionListener</code> objects.
     * <p>
     * This method is not called unless mouse motion events are
     * enabled for this component. Mouse motion events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>A <code>MouseMotionListener</code> object is registered
     * via <code>addMouseMotionListener</code>.
     * <li>Mouse motion events are enabled via <code>enableEvents</code>.
     * </ul>
     * @param       e the mouse motion event.
     * @see         java.awt.event.MouseMotionEvent
     * @see         java.awt.event.MouseMotionListener
     * @see         java.awt.Component#addMouseMotionListener
     * @see         java.awt.Component#enableEvents
     * @since       JDK1.1
     */
    protected void processMouseMotionEvent(MouseEvent e) {
        MouseMotionListener listener = mouseMotionListener;
        if (listener != null) {
            int id = e.getID();
            switch (id) {
            case MouseEvent.MOUSE_MOVED:
                listener.mouseMoved(e);
                break;

            case MouseEvent.MOUSE_DRAGGED:
                listener.mouseDragged(e);
                break;
            }
        }
    }

    /**
     * Processes mouse wheel events occurring on this component by
     * dispatching them to any registered
     * <code>MouseWheelListener</code> objects.
     * <p>
     * This method is not called unless mouse wheel events are
     * enabled for this component. Mouse wheel events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>A <code>MouseWheelListener</code> object is registered
     * via <code>addMouseWheelListener</code>.
     * <li>Mouse wheel events are enabled via <code>enableEvents</code>.
     * </ul>
     * <p>Note that if the event parameter is <code>null</code>
     * the behavior is unspecified and may result in an
     * exception.
     *
     * @param       e the mouse wheel event.
     * @see         java.awt.event.MouseWheelEvent
     * @see         java.awt.event.MouseWheelListener
     * @see         #addMouseWheelListener
     * @see         #enableEvents
     * @since       1.4
     */
    protected void processMouseWheelEvent(MouseWheelEvent e) {
        MouseWheelListener listener = mouseWheelListener;
        if (listener != null) {
            int id = e.getID();
            switch(id) {
                case MouseEvent.MOUSE_WHEEL:
                    listener.mouseWheelMoved(e);
                    break;
            }
        }
    }


    /**
     * Makes this Component displayable by connecting it to a
     * native screen resource.
     * This method is called internally by the toolkit and should
     * not be called directly by programs.
     * @see       java.awt.Component#isDisplayable
     * @see       java.awt.Component#removeNotify
     * @since JDK1.0
     */
    public void addNotify() {
        synchronized (getTreeLock()) {
            // addNotify should only be called on top level contains. In basis this will be the frame.
            // If addNotify is called without a parent then J2SE will throw a NullPointerException
            // during peer construction.
            // To ensure that it is not possible to write a program for basis that doesn't work in
            // J2SE, for example, that behaviour is immitated here. We only need to do this check
            // for components which are not top level containers (ie a Window subclass).

            if (!(this instanceof Window) && (parent == null || !parent.displayable))
                throw new NullPointerException();
            displayable = true;
        }
    }

    /**
     * Makes this Component undisplayable by destroying it native
     * screen resource.
     * This method is called by the toolkit internally and should
     * not be called directly by programs.
     * @see       java.awt.Component#isDisplayable
     * @see       java.awt.Component#addNotify
     * @since JDK1.0
     */
    public void removeNotify() {
        KeyboardFocusManager.clearMostRecentFocusOwner(this);
        if (KeyboardFocusManager.getCurrentKeyboardFocusManager().
                getPermanentFocusOwner() == this)
        {
            KeyboardFocusManager.getCurrentKeyboardFocusManager().
                setGlobalPermanentFocusOwner(null);
        }
        // 5096968
        // (See the similar fix in PP Bug 5095845)
        // The fix involves setting the "displayable" property of this
        // component, before examining if there are any more components
        // that can have focus. By doing this nextFocusHelper() will 
        // return "false" if this is the last component that can have
        // focus and the global focus owner is cleared.
        displayable = false;
        synchronized (getTreeLock()) {
            if (isFocusOwner() && !nextFocusHelper()) {
                KeyboardFocusManager.getCurrentKeyboardFocusManager().
                    clearGlobalFocusOwner();
            }
        }
    }


    /**
     * Returns the value of a flag that indicates whether
     * this component can be traversed using
     * Tab or Shift-Tab keyboard focus traversal.  If this method
     * returns "false", this component may still request the keyboard
     * focus using <code>requestFocus()</code>, but it will not automatically
     * be assigned focus during tab traversal.
     * @return    <code>true</code> if this component is
     *            focus-traverable; <code>false</code> otherwise.
     * @since     JDK1.1
     */
    public boolean isFocusTraversable() {
        if (isFocusTraversableOverridden == FOCUS_TRAVERSABLE_UNKNOWN) {
            isFocusTraversableOverridden = FOCUS_TRAVERSABLE_DEFAULT;
        }
        return focusable;
    }

    /**
     * Requests that this component get the input focus.
     * The component must be visible
     * on the screen for this request to be granted
     * @see FocusEvent
     * @see #addFocusListener
     * @see #processFocusEvent
     * @see #isFocusTraversable
     * @since JDK1.0
     */
    public void requestFocus() {
        requestFocusHelper(false, true);
    }
    /**
     * Transfers the focus to the next component.
     * @see       java.awt.Component#requestFocus
     * @since     JDK1.1s
     */
    public void transferFocus() {
        nextFocusHelper();
    }

    /**
     * Returns true if this Component has the keyboard focus.
     *
     * @return true if this Component has the keyboard focus.
     * @since 1.2
     */
    public boolean hasFocus() {
        return (KeyboardFocusManager.getCurrentKeyboardFocusManager().
            getFocusOwner() == this);
    }

    /*    public void remove(MenuComponent popup) { }
     */


    /**
     * Returns a string representing the state of this component. This
     * method is intended to be used only for debugging purposes, and the
     * content and format of the returned string may vary between
     * implementations. The returned string may be empty but may not be
     * <code>null</code>.
     *
     * @return  a string representation of this component's state.
     * @since     JDK1.0
     */
    protected String paramString() {
        String thisName = getName();
        String str = (thisName != null ? thisName : "") + "," + x + "," + y + "," + width + "x" + height;
        if (!valid) {
            str += ",invalid";
        }
        if (!visible) {
            str += ",hidden";
        }
        if (!enabled) {
            str += ",disabled";
        }
        return str;
    }

    /**
     * Returns a string representation of this component and its values.
     * @return    a string representation of this component.
     * @since     JDK1.0
     */
    public String toString() {
        return getClass().getName() + "[" + paramString() + "]";
    }

    /**
     * Prints a listing of this component to the standard system output
     * stream <code>System.out</code>.
     * @see       java.lang.System#out
     * @since     JDK1.0
     */
    public void list() {
        list(System.out, 0);
    }

    /**
     * Prints a listing of this component to the specified output
     * stream.
     * @param    out   a print stream.
     * @since    JDK1.0
     */
    public void list(PrintStream out) {
        list(out, 0);
    }

    /**
     * Prints out a list, starting at the specified indention, to the
     * specified print stream.
     * @param     out      a print stream.
     * @param     indent   number of spaces to indent.
     * @see       java.io.PrintStream#println(java.lang.Object)
     * @since     JDK1.0
     */
    public void list(PrintStream out, int indent) {
        for (int i = 0; i < indent; i++) {
            out.print(" ");
        }
        out.println(this);
    }

    /**
     * Prints a listing to the specified print writer.
     * @param  out  The print writer to print to.
     * @since JDK1.1
     */
    public void list(PrintWriter out) {
        list(out, 0);
    }

    /**
     * Prints out a list, starting at the specified indention, to
     * the specified print writer.
     * @param out The print writer to print to.
     * @param indent The number of spaces to indent.
     * @see       java.io.PrintStream#println(java.lang.Object)
     * @since JDK1.1
     */
    public void list(PrintWriter out, int indent) {
        for (int i = 0; i < indent; i++) {
            out.print(" ");
        }
        out.println(this);
    }

    /**
     * Adds a PropertyChangeListener to the listener list. The listener is
     * registered for all bound properties of this class, including the
     * following:
     * <ul>
     *    <li>this Component's font ("font")</li>
     *    <li>this Component's background color ("background")</li>
     *    <li>this Component's foreground color ("foreground")</li>
     *    <li>this Component's focusability ("focusable")</li>
     *    <li>this Component's focus traversal keys enabled state
     *        ("focusTraversalKeysEnabled")</li>
     *    <li>this Component's Set of FORWARD_TRAVERSAL_KEYS
     *        ("forwardFocusTraversalKeys")</li>
     *    <li>this Component's Set of BACKWARD_TRAVERSAL_KEYS
     *        ("backwardFocusTraversalKeys")</li>
     *    <li>this Component's Set of UP_CYCLE_TRAVERSAL_KEYS
     *        ("upCycleFocusTraversalKeys")</li>
     * </ul>
     * Note that if this Component is inheriting a bound property, then no
     * event will be fired in response to a change in the inherited property.
     * <p>
     * If listener is null, no exception is thrown and no action is performed.
     *
     * @param    listener  the PropertyChangeListener to be added
     *
     * @see #removePropertyChangeListener
     * @see #getPropertyChangeListeners
     * @see #addPropertyChangeListener(java.lang.String, java.beans.PropertyChangeListener)
     */
    public synchronized void addPropertyChangeListener(
                             PropertyChangeListener listener) {
        if (listener == null) {
            return;
        }
        if (changeSupport == null) {
            changeSupport = new java.beans.PropertyChangeSupport(this);
        }
        changeSupport.addPropertyChangeListener(listener);
    }

    /**
     * Removes a PropertyChangeListener from the listener list. This method
     * should be used to remove PropertyChangeListeners that were registered
     * for all bound properties of this class.
     * <p>
     * If listener is null, no exception is thrown and no action is performed.
     *
     * @param listener the PropertyChangeListener to be removed
     *
     * @see #addPropertyChangeListener
     * @see #getPropertyChangeListeners
     * @see #removePropertyChangeListener(java.lang.String,java.beans.PropertyChangeListener)
     */
    public synchronized void removePropertyChangeListener(
                             PropertyChangeListener listener) {
        if (listener == null || changeSupport == null) {
            return;
        }
        changeSupport.removePropertyChangeListener(listener);
    }

    /**
     * Returns an array of all the property change listeners
     * registered on this component.
     *
     * @return all of this component's <code>PropertyChangeListener</code>s
     *         or an empty array if no property change
     *         listeners are currently registered
     *
     * @see      #addPropertyChangeListener
     * @see      #removePropertyChangeListener
     * @see      #getPropertyChangeListeners(java.lang.String)
     * @see      java.beans.PropertyChangeSupport#getPropertyChangeListeners
     * @since    1.4
     */
    public synchronized PropertyChangeListener[] getPropertyChangeListeners() {
        if (changeSupport == null) {
            return new PropertyChangeListener[0];
        }
        return changeSupport.getPropertyChangeListeners();
    }

    Frame getFrame() {
        Component c = this;
        while (c != null && !(c instanceof Frame))
            c = c.parent;
        return (Frame) c;
    }
    /**
     * Component Serialized Data Version.
     *
     * @serial
     */
    private int componentSerializedDataVersion = 3;
    /**
     * Writes default serializable fields to stream.  Writes
     * a list of serializable ItemListener(s) as optional data.
     * The non-serializable ItemListener(s) are detected and
     * no attempt is made to serialize them.
     *
     * @serialData Null terminated sequence of 0 or more pairs.
     *             The pair consists of a String and Object.
     *             The String indicates the type of object and
     *             is one of the following :
     *             itemListenerK indicating and ItemListener object.
     *
     * @see AWTEventMulticaster.save(ObjectOutputStream, String, EventListener)
     * @see java.awt.Component.itemListenerK
     */
    private void writeObject(ObjectOutputStream s)
        throws IOException {
        s.defaultWriteObject();
        // Compatability with J2SE:
        // Listener fields in J2SE
        AWTEventMulticaster.save(s, componentListenerK, componentListener);
        AWTEventMulticaster.save(s, focusListenerK, focusListener);
        AWTEventMulticaster.save(s, keyListenerK, keyListener);
        AWTEventMulticaster.save(s, mouseListenerK, mouseListener);
        AWTEventMulticaster.save(s, mouseMotionListenerK, mouseMotionListener);
        s.writeObject(null);
        // componentOrientation field in J2SE
        s.writeObject(null);
        // hierarchyListener and hierarchyBoundsListener fields in J2SE
        s.writeObject(null);
        AWTEventMulticaster.save(s, mouseWheelListenerK, mouseWheelListener);
        s.writeObject(null);
    }

    /**
     * Read the ObjectInputStream and if it isnt null
     * add a listener to receive item events fired
     * by the components.
     * Unrecognised keys or values will be Ignored.
     *
     * @see removeActionListener()
     * @see addActionListener()
     */
    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();

        appContext = AppContext.getAppContext();
        SunToolkit.insertTargetMapping(this, appContext);

        if (componentSerializedDataVersion < 4) {
            // These fields are non-transient and rely on default
            // serialization. However, the default values are insufficient,
            // so we need to set them explicitly for object data streams prior
            // to 1.4.
            focusable = true;
            isFocusTraversableOverridden = FOCUS_TRAVERSABLE_UNKNOWN;
            initializeFocusTraversalKeys();
            focusTraversalKeysEnabled = true;
        }

        // Compatability with J2SE:
        // Listener fields in J2SE
        Object keyOrNull;
        while (null != (keyOrNull = s.readObject())) {
            String key = ((String) keyOrNull).intern();
            if (componentListenerK == key) {
                addComponentListener((ComponentListener) (s.readObject()));
            } else if (focusListenerK == key) {
                addFocusListener((FocusListener) (s.readObject()));
            } else if (keyListenerK == key) {
                addKeyListener((KeyListener) (s.readObject()));
            } else if (mouseListenerK == key) {
                addMouseListener((MouseListener) (s.readObject()));
            } else if (mouseMotionListenerK == key) {
                addMouseMotionListener((MouseMotionListener) (s.readObject()));
            } else if (inputMethodListenerK == key) {
                addInputMethodListener((InputMethodListener)(s.readObject()));
            } else { // skip value for unrecognized key
                try {
                    s.readObject();
                } catch (ClassNotFoundException cnfe) {// May be of type that does not exist in
                    // Personal Basis Profile, eg: InputMethodListener
                }
            }
        }
    }

// Focus-related functionality added 
//-------------------------------------------------------------------------
    private boolean focusable = true;
    private static final int FOCUS_TRAVERSABLE_UNKNOWN = 0;
    private static final int FOCUS_TRAVERSABLE_DEFAULT = 1;
    private static final int FOCUS_TRAVERSABLE_SET = 2;
    private int isFocusTraversableOverridden = FOCUS_TRAVERSABLE_UNKNOWN;
    Set[] focusTraversalKeys;
    private static final String[] focusTraversalKeyPropertyNames = {
        "forwardFocusTraversalKeys",
        "backwardFocusTraversalKeys",
        "upCycleFocusTraversalKeys",
        "downCycleFocusTraversalKeys"
    };
    private boolean focusTraversalKeysEnabled = true;

    void initializeFocusTraversalKeys() {
        focusTraversalKeys = new Set[3];
    }

    boolean containsFocus() {
        return isFocusOwner();
    }

    void clearMostRecentFocusOwnerOnHide() {
        KeyboardFocusManager.clearMostRecentFocusOwner(this);
    }

    void clearCurrentFocusCycleRootOnHide() {
        /* do nothing */
    }

    public boolean isFocusable() {
        return isFocusTraversable();
    }

    public void setFocusable(boolean focusable) {
        boolean oldFocusable;
        synchronized (this) {
            oldFocusable = this.focusable;
            this.focusable = focusable;
        }
        isFocusTraversableOverridden = FOCUS_TRAVERSABLE_SET;
        firePropertyChange("focusable",new Boolean(oldFocusable), 
                                       new Boolean(focusable));
        if (oldFocusable && !focusable) {
            if (isFocusOwner()) {
                autoTransferFocus(true);
            }
            KeyboardFocusManager.clearMostRecentFocusOwner(this);
        }
    }

    final boolean isFocusTraversableOverridden() {
        return (isFocusTraversableOverridden != FOCUS_TRAVERSABLE_DEFAULT);
    }

    public void setFocusTraversalKeys(int id, Set keystrokes) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH - 1) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        setFocusTraversalKeys_NoIDCheck(id, keystrokes);
    }

    public Set getFocusTraversalKeys(int id) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH - 1) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        return getFocusTraversalKeys_NoIDCheck(id);
    }

    final void setFocusTraversalKeys_NoIDCheck(int id, Set keystrokes) {
        Set oldKeys;
        synchronized (this) {
            if (focusTraversalKeys == null) {
                initializeFocusTraversalKeys();
            }
            if (keystrokes != null) {
                for (Iterator iter = keystrokes.iterator(); iter.hasNext(); ) {
                    Object obj = iter.next();
                    if (obj == null) {
                        throw new IllegalArgumentException("cannot set null focus traversal key");
                    }
                    // Fix for 6195828/6195836:
                    // According to javadoc this method should throw IAE 
                    // instead of ClassCastException
                    if (!(obj instanceof AWTKeyStroke)) {
                        throw new IllegalArgumentException(
                           "object is expected to be AWTKeyStroke");                                 }

                    AWTKeyStroke keystroke = (AWTKeyStroke)obj;
                    if (keystroke.getKeyChar() != KeyEvent.CHAR_UNDEFINED) {
                        throw new IllegalArgumentException("focus traversal keys cannot map to KEY_TYPED events");
                    }
                    for (int i = 0; i < focusTraversalKeys.length; i++) {
                        if (i == id) {
                            continue;
                        }
                        if (getFocusTraversalKeys_NoIDCheck(i).contains(keystroke)) {
                            throw new IllegalArgumentException("focus traversal keys must be unique for a Component");
                        }
                    }
                }
            }
            oldKeys = focusTraversalKeys[id];
            focusTraversalKeys[id] = (keystrokes != null)
                ? Collections.unmodifiableSet(new HashSet(keystrokes))
                : null;
        }
        firePropertyChange(focusTraversalKeyPropertyNames[id], oldKeys,
            keystrokes);
    }

    final Set getFocusTraversalKeys_NoIDCheck(int id) {
        // Okay to return Set directly because it is an unmodifiable view
        Set keystrokes = (focusTraversalKeys != null)
            ? focusTraversalKeys[id]
            : null;
        if (keystrokes != null) {
            return keystrokes;
        } else {
            Container parent = this.parent;
            if (parent != null) {
                return parent.getFocusTraversalKeys(id);
            } else {
                return KeyboardFocusManager.getCurrentKeyboardFocusManager().
                    getDefaultFocusTraversalKeys(id);
            }
        }
    }

    public boolean areFocusTraversalKeysSet(int id) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        return (focusTraversalKeys != null && focusTraversalKeys[id] != null);
    }

    public void setFocusTraversalKeysEnabled(boolean focusTraversalKeysEnabled) {
        boolean oldFocusTraversalKeysEnabled;
        synchronized (this) {
            oldFocusTraversalKeysEnabled = this.focusTraversalKeysEnabled;
            this.focusTraversalKeysEnabled = focusTraversalKeysEnabled;
        }
        firePropertyChange("focusTraversalKeysEnabled",
               new Boolean(oldFocusTraversalKeysEnabled),
               new Boolean(focusTraversalKeysEnabled));
    }

    public boolean getFocusTraversalKeysEnabled() {
        return focusTraversalKeysEnabled;
    }

    protected boolean requestFocus(boolean temporary) {
        return requestFocusHelper(temporary, true);
    }

    public boolean requestFocusInWindow() {
        return requestFocusHelper(false, false);
    }

    protected boolean requestFocusInWindow(boolean temporary) {
        return requestFocusHelper(temporary, false);
    }

    final boolean requestFocusHelper(boolean temporary, boolean focusedWindowChangeAllowed) {
        // bug 6195570 - if we already have focus, return false
        if (hasFocus()) {
            return false;
        }
        if (isFocusable() && isVisible() 
            // 5092867
            // If we are not "displayable" then we cannot be focused. Only
            // Components that are "focasable", "displayable" and "visble"
            // can be focused. In J2SE, if ( this.peer == null ) then "this"
            // is not displayable and you can see this check done in
            // J2SE also
            && this.displayable ) {
            // 5092867
                Component window = this;
                while (!(window instanceof Window)) {
                    window = window.parent;
                }
            if (window == null || !((Window)window).isFocusableWindow()) {
                return false;
            }
            // Update most-recent map
            KeyboardFocusManager.setMostRecentFocusOwner(this);


            // Synthesize focus events for lightweights
            Component currentFocusOwner = KeyboardFocusManager.getCurrentKeyboardFocusManager().getFocusOwner();
            FocusEvent synthetic = new FocusEvent(this, FocusEvent.FOCUS_GAINED, temporary, currentFocusOwner);
 //           KeyboardFocusManager.postSyntheticEvent(synthetic);
            Toolkit.getEventQueue().postEvent(new SequencedEvent(synthetic));
            return true;// unless we're sure we can't change focus, return true
        }
        return false;
    }

    final void autoTransferFocus(boolean clearOnFailure) {
        Component toTest = KeyboardFocusManager.
            getCurrentKeyboardFocusManager().getFocusOwner();
        if (toTest != this) {
            if (toTest != null) {
                toTest.autoTransferFocus(clearOnFailure);
            }
            return;
        }
        // the following code will execute only if this Component is the focus
        // owner
        if (!(isDisplayable() && isVisible() && isEnabled() && isFocusable())) {
            doAutoTransfer(clearOnFailure);
            return;
        }
        toTest = getParent();
        while (toTest != null && !(toTest instanceof Window)) {
            if (!(toTest.isDisplayable() && toTest.isVisible() &&
                (toTest.isEnabled() || toTest.isLightweight()))) {
                doAutoTransfer(clearOnFailure);
                return;
            }
            toTest = toTest.getParent();
        }
    }

    private void doAutoTransfer(boolean clearOnFailure) {
        if (clearOnFailure) {
            if (!nextFocusHelper()) {
                KeyboardFocusManager.getCurrentKeyboardFocusManager().
                clearGlobalFocusOwner();
            }
        } else {
            transferFocus();
        }
    }

    public Container getFocusCycleRootAncestor() {
        Container rootAncestor = this.parent;
        while (rootAncestor != null && !rootAncestor.isFocusCycleRoot()) {
            rootAncestor = rootAncestor.parent;
        }
        return rootAncestor;
    }

    public boolean isFocusCycleRoot(Container container) {
        Container rootAncestor = getFocusCycleRootAncestor();
        return (rootAncestor == container);
    }

    boolean nextFocusHelper() {
        Container rootAncestor = getFocusCycleRootAncestor();
        Component comp = this;
        while (rootAncestor != null &&
               !(rootAncestor.isShowing() &&
                 rootAncestor.isFocusable() &&
                 rootAncestor.isEnabled())) {
            comp = rootAncestor;
            rootAncestor = comp.getFocusCycleRootAncestor();
        }
        if (rootAncestor != null) {
            FocusTraversalPolicy policy =
                rootAncestor.getFocusTraversalPolicy();
            Component toFocus = policy.getComponentAfter(rootAncestor, comp);
            if (toFocus == null) {
                toFocus = policy.getDefaultComponent(rootAncestor);
            }
            if (toFocus != null) {
                return toFocus.requestFocus(false);
            }
        }
        return false;
    }

    public void transferFocusBackward() {
        Container rootAncestor = getFocusCycleRootAncestor();
        Component comp = this;
        while (rootAncestor != null &&
               !(rootAncestor.isShowing() &&
                 rootAncestor.isFocusable() &&
                 rootAncestor.isEnabled()))
        {
            comp = rootAncestor;
            rootAncestor = comp.getFocusCycleRootAncestor();
        }
        if (rootAncestor != null) {
            FocusTraversalPolicy policy =
                rootAncestor.getFocusTraversalPolicy();
            Component toFocus = policy.getComponentBefore(rootAncestor, comp);
            if (toFocus == null) {
                toFocus = policy.getDefaultComponent(rootAncestor);
            }
            if (toFocus != null) {
                toFocus.requestFocus();
            }
        }
    }

    public void transferFocusUpCycle() {
        Container rootAncestor;
        for (rootAncestor = getFocusCycleRootAncestor();
            rootAncestor != null && !(rootAncestor.isShowing() &&
                rootAncestor.isFocusable() && rootAncestor.isEnabled());
            rootAncestor = rootAncestor.getFocusCycleRootAncestor()) {
        }
        if (rootAncestor != null) {
            Container rootAncestorRootAncestor =
                rootAncestor.getFocusCycleRootAncestor();
            KeyboardFocusManager.getCurrentKeyboardFocusManager().
                setGlobalCurrentFocusCycleRoot(
                    (rootAncestorRootAncestor != null)
                        ? rootAncestorRootAncestor
                        : rootAncestor);
            rootAncestor.requestFocus();
        } else {
            Container window =
                (this instanceof Container) ? ((Container)this) : getParent();
            while (window != null && !(window instanceof Window)) {
                window = window.getParent();
            }
            if (window != null) {
                Component toFocus = window.getFocusTraversalPolicy().
                getDefaultComponent(window);
                if (toFocus != null) {
                    KeyboardFocusManager.getCurrentKeyboardFocusManager().
                    setGlobalCurrentFocusCycleRoot(window);
                    toFocus.requestFocus();
                }
            }
        }
    }

    public boolean isFocusOwner() {
        return hasFocus();
    }

    protected void firePropertyChange(String propertyName,
        Object oldValue, Object newValue) {
        java.beans.PropertyChangeSupport changeSupport = this.changeSupport;
        if (changeSupport == null) {
            return;
        }
        changeSupport.firePropertyChange(propertyName, oldValue, newValue);
    }

    // Input Method related methods
    boolean areInputMethodsEnabled() {
        // in 1.2, we assume input method support is required for all
        // components that handle key events, but components can turn off
        // input methods by calling enableInputMethods(false).
        return ((eventMask & AWTEvent.INPUT_METHODS_ENABLED_MASK) != 0) &&
            ((eventMask & AWTEvent.KEY_EVENT_MASK) != 0 || keyListener != null);
    }

}
