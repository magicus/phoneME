/*
 * @(#)MenuComponent.java	1.14 06/10/10
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

import java.awt.event.ActionEvent;

/**
 * The abstract class <code>MenuComponent</code> is the superclass
 * of all menu-related components. In this respect, the class
 * <code>MenuComponent</code> is analogous to the abstract superclass
 * <code>Component</code> for AWT components.
 * <p>
 * Menu components receive and process AWT events, just as components do,
 * through the method <code>processEvent</code>.
 *
 * @version 	1.3, 08/22/01
 * @author 	Arthur van Hoff
 * @since       JDK1.0
 */
public abstract class MenuComponent implements java.io.Serializable {
    transient MenuContainer parent;
    Font font;
    private String name;
    private boolean nameExplicitlySet = false;
    boolean newEventsOnly = false;
    /*
     * Internal constants for serialization
     */
    final static String actionListenerK = Component.actionListenerK;
    final static String itemListenerK = Component.itemListenerK;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -4536902356223894379L;
    /**
     * Construct a name for this MenuComponent.  Called by getName() when
     * the name is null.
     */
    String constructComponentName() {
        return null; // For strict compliance with prior JDKs, a MenuComponent
        // that doesn't set its name should return null from
        // getName()
    }

    /**
     * Gets the name of the menu component.
     * @return        the name of the menu component.
     * @see           java.awt.MenuComponent#setName(java.lang.String)
     * @since         JDK1.1
     */
    public String getName() {
        if (name == null && !nameExplicitlySet) {
            synchronized (this) {
                if (name == null && !nameExplicitlySet) {
                    name = constructComponentName();
                }
            }
        }
        return name;
    }

    /**
     * Sets the name of the component to the specified string.
     * @param         name    the name of the menu component.
     * @see           java.awt.MenuComponent#getName
     * @since         JDK1.1
     */
    public void setName(String name) {
        synchronized (this) {
            this.name = name;
            nameExplicitlySet = true;
        }
    }

    /**
     * Returns the parent container for this menu component.
     * @return    the menu component containing this menu component,
     *                 or <code>null</code> if this menu component
     *                 is the outermost component, the menu bar itself.
     * @since     JDK1.0
     */
    public MenuContainer getParent() {
        return parent;
    }

    /**
     * Gets the font used for this menu component.
     * @return   the font used in this menu component, if there is one;
     *                  <code>null</code> otherwise.
     * @see     java.awt.MenuComponent#setFont
     * @since   JDK1.0
     */
    public Font getFont() {
        Font font = this.font;
        if (font != null) {
            return font;
        }
        MenuContainer parent = this.parent;
        if (parent != null) {
            return parent.getFont();
        }
        return null;
    }

    /**
     * Sets the font to be used for this menu component to the specified
     * font. This font is also used by all subcomponents of this menu
     * component, unless those subcomponents specify a different font.
     * @param     f   the font to be set.
     * @see       java.awt.MenuComponent#getFont
     * @since     JDK1.0
     */
    public void setFont(Font f) {
        font = f;
    }

    /**
     * Removes the menu component's peer.  The peer allows us to modify the
     * appearance of the menu component without changing the functionality of
     * the menu component.
     */
    public void removeNotify() {}

    /*
     * Delivers an event to this component or one of its sub components.
     * @param e the event
     */
    public final void dispatchEvent(AWTEvent e) {
        dispatchEventImpl(e);
    }

    void dispatchEventImpl(AWTEvent e) {
        if ((parent != null && parent instanceof MenuComponent &&
                ((MenuComponent) parent).newEventsOnly)) {
            if (eventEnabled(e)) {
                processEvent(e);
            } else if (e instanceof ActionEvent && parent != null) {
                ((MenuComponent) parent).dispatchEvent(new ActionEvent(parent,
                        e.getID(),
                        ((ActionEvent) e).getActionCommand()));
            }
        }
    }

    // NOTE: remove when filtering is done at lower level
    boolean eventEnabled(AWTEvent e) {
        return false;
    }

    /**
     * Processes events occurring on this menu component.
     * @param e the event
     * @since JDK1.1
     */
    protected void processEvent(AWTEvent e) {}

    /**
     * Returns the parameter string representing the state of this
     * menu component. This string is useful for debugging.
     * @return     the parameter string of this menu component.
     * @since      JDK1.0
     */
    protected String paramString() {
        String thisName = getName();
        return (thisName != null ? thisName : "");
    }

    /**
     * Returns a representation of this menu component as a string.
     * @return  a string representation of this menu component.
     * @since     JDK1.0
     */
    public String toString() {
        return getClass().getName() + "[" + paramString() + "]";
    }

    /**
     * Gets this component's locking object (the object that owns the thread
     * sychronization monitor) for AWT component-tree and layout
     * operations.
     * @return This component's locking object.
     */
    protected final Object getTreeLock() {
        return Component.LOCK;
    }
}
