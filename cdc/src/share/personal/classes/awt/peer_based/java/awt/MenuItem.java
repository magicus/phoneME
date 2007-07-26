/*
 * @(#)MenuItem.java	1.64 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import sun.awt.peer.MenuItemPeer;
import sun.awt.PeerBasedToolkit;
import java.awt.event.*;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.awt.AWTEventMulticaster;
import java.util.EventListener;

/**
 * All items in a menu must belong to the class
 * <code>MenuItem</code>, or one of its subclasses.
 * <p>
 * The default <code>MenuItem</code> object embodies
 * a simple labeled menu item.
 * <p>
 * This picture of a menu bar shows five menu items:
 * <IMG SRC="images-awt/MenuBar-1.gif"
 * ALIGN=CENTER HSPACE=10 VSPACE=7>
 * <br CLEAR=LEFT>
 * The first two items are simple menu items, labeled
 * <code>"Basic"</code> and <code>"Simple"</code>.
 * Following these two items is a separator, which is itself
 * a menu item, created with the label <code>"-"</code>.
 * Next is an instance of <code>CheckboxMenuItem</code>
 * labeled <code>"Check"</code>. The final menu item is a
 * submenu labeled <code>"More&nbsp;Examples"</code>,
 * and this submenu is an instance of <code>Menu</code>.
 * <p>
 * When a menu item is selected, AWT sends an action event to
 * the menu item. Since the event is an
 * instance of <code>ActionEvent</code>, the <code>processEvent</code>
 * method examines the event and passes it along to
 * <code>processActionEvent</code>. The latter method redirects the
 * event to any <code>ActionListener</code> objects that have
 * registered an interest in action events generated by this
 * menu item.
 * <P>
 * Note that the subclass <code>Menu</code> overrides this behavior and
 * does not send any event to the frame until one of its subitems is
 * selected.
 *
 * @version 1.50, 08/10/01
 * @author Sami Shaio
 */
public class MenuItem extends MenuComponent {
    boolean enabled = true;
    String label;
    String actionCommand;
    // The eventMask is ONLY set by subclasses via enableEvents.
    // The mask should NOT be set when listeners are registered
    // so that we can distinguish the difference between when
    // listeners request events and subclasses request them.
    long eventMask;
    transient ActionListener actionListener;
    private MenuShortcut shortcut = null;
    private static final String base = "menuitem";
    private static int nameCounter = 0;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -21757335363267194L;
    /**
     * Constructs a new MenuItem with an empty label and no keyboard
     * shortcut.
     * @since    JDK1.1
     */
    public MenuItem() {
        this("", null);
    }

    /**
     * Constructs a new MenuItem with the specified label
     * and no keyboard shortcut. Note that use of "-" in
     * a label is reserved to indicate a separator between
     * menu items. By default, all menu items except for
     * separators are enabled.
     * @param       label the label for this menu item.
     * @since       JDK1.0
     */
    public MenuItem(String label) {
        this(label, null);
    }

    /**
     * Create a menu item with an associated keyboard shortcut.
     * Note that use of "-" in a label is reserved to indicate
     * a separator between menu items. By default, all menu
     * items except for separators are enabled.
     * @param       label the label for this menu item.
     * @param       s the instance of <code>MenuShortcut</code>
     *                       associated with this menu item.
     * @since       JDK1.1
     */
    public MenuItem(String label, MenuShortcut s) {
        this.label = label;
        this.shortcut = s;
    }

    /**
     * Construct a name for this MenuComponent.  Called by getName() when
     * the name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }

    /**
     * Creates the menu item's peer.  The peer allows us to modify the
     * appearance of the menu item without changing its functionality.
     */
    public void addNotify() {
        synchronized (getTreeLock()) {
            if (peer == null) {
                peer = ((PeerBasedToolkit) Toolkit.getDefaultToolkit()).createMenuItem(this);
            }
        }
    }

    /**
     * Gets the label for this menu item.
     * @return  the label of this menu item, or <code>null</code>
     if this menu item has no label.
     * @see     java.awt.MenuItem#setLabel
     * @since   JDK1.0
     */
    public String getLabel() {
        return label;
    }

    /**
     * Sets the label for this menu item to the specified label.
     * @param     label   the new label, or <code>null</code> for no label.
     * @see       java.awt.MenuItem#getLabel
     * @since     JDK1.0
     */
    public synchronized void setLabel(String label) {
        this.label = label;
        MenuItemPeer peer = (MenuItemPeer) this.peer;
        if (peer != null) {
            peer.setLabel(label);
        }
    }

    /**
     * Checks whether this menu item is enabled.
     * @see        java.awt.MenuItem#setEnabled
     * @since      JDK1.0
     */
    public boolean isEnabled() {
        return enabled;
    }

    /**
     * Sets whether or not this menu item can be chosen.
     * @param      b  if <code>true</code>, enables this menu item;
     *                       if <code>false</code>, disables it.
     * @see        java.awt.MenuItem#isEnabled
     * @since      JDK1.1
     */
    public synchronized void setEnabled(boolean b) {
        enable(b);
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>setEnabled(boolean)</code>.
     */
    public synchronized void enable() {
        enabled = true;
        MenuItemPeer peer = (MenuItemPeer) this.peer;
        if (peer != null) {
            peer.setEnabled(true);
        }
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>setEnabled(boolean)</code>.
     */
    public void enable(boolean b) {
        if (b) {
            enable();
        } else {
            disable();
        }
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>setEnabled(boolean)</code>.
     */
    public synchronized void disable() {
        enabled = false;
        MenuItemPeer peer = (MenuItemPeer) this.peer;
        if (peer != null) {
            peer.setEnabled(false);
        }
    }

    /**
     * Get the <code>MenuShortcut</code> object associated with this
     * menu item,
     * @return      the menu shortcut associated with this menu item,
     *                   or <code>null</code> if none has been specified.
     * @see         java.awt.MenuItem#setShortcut
     * @since       JDK1.1
     */
    public MenuShortcut getShortcut() {
        return shortcut;
    }

    /**
     * Set the <code>MenuShortcut</code> object associated with this
     * menu item. If a menu shortcut is already associated with
     * this menu item, it is replaced.
     * @param       s  the menu shortcut to associate
     *                           with this menu item.
     * @see         java.awt.MenuItem#getShortcut
     * @since       JDK1.1
     */
    public void setShortcut(MenuShortcut s) {
        shortcut = s;
        MenuItemPeer peer = (MenuItemPeer) this.peer;
        if (peer != null) {
            peer.setLabel(label);
        }
    }

    /**
     * Delete any <code>MenuShortcut</code> object associated
     * with this menu item.
     * @since      JDK1.1
     */
    public void deleteShortcut() {
        shortcut = null;
        MenuItemPeer peer = (MenuItemPeer) this.peer;
        if (peer != null) {
            peer.setLabel(label);
        }
    }

    /*
     * Delete a matching MenuShortcut associated with this MenuItem.
     * Used when iterating Menus.
     */
    void deleteShortcut(MenuShortcut s) {
        if (s.equals(shortcut)) {
            shortcut = null;
            MenuItemPeer peer = (MenuItemPeer) this.peer;
            if (peer != null) {
                peer.setLabel(label);
            }
        }
    }

    /*
     * The main goal of this method is to post an appropriate event
     * to the event queue when menu shortcut is pressed. However,
     * in subclasses this method may do more than just posting 
     * an event.
     */
    void doMenuEvent() {
        Toolkit.getEventQueue().postEvent(
            new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                            getActionCommand()));
    }

    /*
     * Post an ActionEvent to the target (on
     * keydown).  Returns true if there is an associated
     * shortcut.
     */
    boolean handleShortcut(KeyEvent e) {
        MenuShortcut s = new MenuShortcut(e.getKeyCode(),
                (e.getModifiers() & InputEvent.SHIFT_MASK) > 0);
        if (s.equals(shortcut) && enabled) {
            // MenuShortcut match -- issue an event on keydown.
            if (e.getID() == KeyEvent.KEY_PRESSED) {
                doMenuEvent();
            } else {// silently eat key release.
            }
            return true;
        }
        return false;
    }

    MenuItem getShortcutMenuItem(MenuShortcut s) {
        return (s.equals(shortcut)) ? this : null;
    }

    /**
     * Enables event delivery to this menu item for events
     * to be defined by the specified event mask parameter
     * <p>
     * Since event types are automatically enabled when a listener for
     * that type is added to the menu item, this method only needs
     * to be invoked by subclasses of <code>MenuItem</code> which desire to
     * have the specified event types delivered to <code>processEvent</code>
     * regardless of whether a listener is registered.
     * @param       eventsToEnable the event mask defining the event types.
     * @see         java.awt.MenuItem#processEvent
     * @see         java.awt.MenuItem#disableEvents
     * @see         java.awt.Component#enableEvents
     * @since       JDK1.1
     */
    protected final void enableEvents(long eventsToEnable) {
        eventMask |= eventsToEnable;
        newEventsOnly = true;
    }

    /**
     * Disables event delivery to this menu item for events
     * defined by the specified event mask parameter.
     * @param       eventsToDisable the event mask defining the event types.
     * @see         java.awt.MenuItem#processEvent
     * @see         java.awt.MenuItem#enableEvents
     * @see         java.awt.Component#disableEvents
     * @since       JDK1.1
     */
    protected final void disableEvents(long eventsToDisable) {
        eventMask &= ~eventsToDisable;
    }

    /**
     * Sets the command name of the action event that is fired
     * by this menu item.
     * <p>
     * By default, the action command is set to the label of
     * the menu item.
     * @param       command   the action command to be set
     *                                for this menu item.
     * @see         java.awt.MenuItem#getActionCommand
     * @since       JDK1.1
     */
    public void setActionCommand(String command) {
        actionCommand = command;
    }

    /**
     * Gets the command name of the action event that is fired
     * by this menu item.
     * @see         java.awt.MenuItem#setActionCommand
     * @since       JDK1.1
     */
    public String getActionCommand() {
        return (actionCommand == null ? label : actionCommand);
    }

    /**
     * Adds the specified action listener to receive action events
     * from this menu item.
     * @param      l the action listener.
     * @see        java.awt.event.ActionEvent
     * @see        java.awt.event.ActionListener
     * @see        java.awt.MenuItem#removeActionListener
     * @since      JDK1.1
     */
    public synchronized void addActionListener(ActionListener l) {
        actionListener = AWTEventMulticaster.add(actionListener, l);
        newEventsOnly = true;
    }

    /**
     * Removes the specified action listener so it no longer receives
     * action events from this menu item.
     * @param      l the action listener.
     * @see        java.awt.event.ActionEvent
     * @see        java.awt.event.ActionListener
     * @see        java.awt.MenuItem#addActionListener
     * @since      JDK1.1
     */
    public synchronized void removeActionListener(ActionListener l) {
        actionListener = AWTEventMulticaster.remove(actionListener, l);
    }

    /**
     * Returns an array of all the action listeners
     * registered on this menu item.
     *
     * @return all of this menu item's <code>ActionListener</code>s
     *         or an empty array if no action
     *         listeners are currently registered
     *
     * @see        #addActionListener
     * @see        #removeActionListener
     * @see        java.awt.event.ActionEvent
     * @see        java.awt.event.ActionListener
     * @since 1.4
     */
    public synchronized ActionListener[] getActionListeners() {
        return (ActionListener[])AWTEventMulticaster.
            getListeners((EventListener)actionListener, ActionListener.class);
    }

    /**
     * Processes events on this menu item. If the event is an
     * instance of <code>ActionEvent</code>, it invokes
     * <code>processActionEvent</code>, another method
     * defined by <code>MenuItem</code>.
     * <p>
     * Currently, menu items only support action events.
     * @param       e the event.
     * @see         java.awt.MenuItem#processActionEvent
     * @since       JDK1.1
     */
    protected void processEvent(AWTEvent e) {
        if (e instanceof ActionEvent) {
            processActionEvent((ActionEvent) e);
        }
    }

    // NOTE: remove when filtering is done at lower level
    boolean eventEnabled(AWTEvent e) {
        if (e.id == ActionEvent.ACTION_PERFORMED) {
            if ((eventMask & AWTEvent.ACTION_EVENT_MASK) != 0 ||
                actionListener != null) {
                return true;
            }
            return false;
        }
        return super.eventEnabled(e);
    }

    /**
     * Processes action events occurring on this menu item,
     * by dispatching them to any registered
     * <code>ActionListener</code> objects.
     * This method is not called unless action events are
     * enabled for this component. Action events are enabled
     * when one of the following occurs:
     * <p><ul>
     * <li>An <code>ActionListener</code> object is registered
     * via <code>addActionListener</code>.
     * <li>Action events are enabled via <code>enableEvents</code>.
     * </ul>
     * @param       e the action event.
     * @see         java.awt.event.ActionEvent
     * @see         java.awt.event.ActionListener
     * @see         java.awt.MenuItem#enableEvents
     * @since       JDK1.1
     */
    protected void processActionEvent(ActionEvent e) {
        if (actionListener != null) {
            actionListener.actionPerformed(e);
        }
    }

    /**
     * Returns the parameter string representing the state of this menu
     * item. This string is useful for debugging.
     * @return  the parameter string of this menu item.
     * @since   JDK1.0
     */
    public String paramString() {
        String str = ",label=" + label;
        if (shortcut != null) {
            str += ",shortcut=" + shortcut;
        }
        return super.paramString() + str;
    }
    /* Serialization support.
     */

    private int menuItemSerializedDataVersion = 1;
    private void writeObject(ObjectOutputStream s)
        throws IOException {
        s.defaultWriteObject();
        AWTEventMulticaster.save(s, actionListenerK, actionListener);
        s.writeObject(null);
    }

    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();
        Object keyOrNull;
        while (null != (keyOrNull = s.readObject())) {
            String key = ((String) keyOrNull).intern();
            if (actionListenerK == key)
                addActionListener((ActionListener) (s.readObject()));
            else // skip value for unrecognized key
                s.readObject();
        }
    }
}
