/*
 * @(#)MenuShortcut.java	1.27 06/10/10
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

import java.awt.event.KeyEvent;

/**
 * A class which represents a keyboard accelerator for a MenuItem.
 *
 * @version %I, 08/19/02
 * @author Thomas Ball
 */
public class MenuShortcut implements java.io.Serializable {
    int key;
    boolean usesShift;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 143448358473180225L;
    /**
     * Constructs a new MenuShortcut for the specified key.
     * @param key the raw keycode for this MenuShortcut, as would be returned
     * in the keyCode field of a KeyEvent if this key were pressed.
     * @exception UnsupportedOperationException if the class is not supported
     **/
    public MenuShortcut(int key) {
        this(key, false);
    }

    /**
     * Constructs a new MenuShortcut for the specified key.
     * @param key the raw keycode for this MenuShortcut, as would be returned
     * in the keyCode field of a KeyEvent if this key were pressed.
     * @param useShiftModifier indicates whether this MenuShortcut is invoked
     * with the SHIFT key down.
     * @exception UnsupportedOperationException if the class is not supported
     **/
    public MenuShortcut(int key, boolean useShiftModifier) {
        this.key = key;
        this.usesShift = useShiftModifier;
    }

    /**
     * Return the raw keycode of this MenuShortcut.
     */
    public int getKey() {
        return key;
    }

    /**
     * Return whether this MenuShortcut must be invoked using the SHIFT key.
     */
    public boolean usesShiftModifier() {
        return usesShift;
    }

    /**
     * Returns whether this MenuShortcut is the same as another:
     * equality is defined to mean that both MenuShortcuts use the same key
     * and both either use or don't use the SHIFT key.
     * @param s the MenuShortcut to compare with this.
     */
    public boolean equals(MenuShortcut s) {
        return (s != null && (s.getKey() == key) &&
                (s.usesShiftModifier() == usesShift));
    }
	
    /**
     * Returns whether this MenuShortcut is the same as another:
     * equality is defined to mean that both MenuShortcuts use the same key
     * and both either use or don't use the SHIFT key.
     * @param obj the Object to compare with this.
     * @return <code>true</code> if this MenuShortcut is the same as another,
     * <code>false</code> otherwise.
     * @since 1.2
     */
    public boolean equals(Object obj) {
        if (obj instanceof MenuShortcut) {
            return equals((MenuShortcut) obj);
        }
        return false;
    }
	
    /**
     * Returns the hashcode for this MenuShortcut.
     * @return the hashcode for this MenuShortcut.
     * @since 1.2
     */
    public int hashCode() {
        return (usesShift) ? (~key) : key;
    }

    /**
     * Returns an internationalized description of the MenuShortcut.
     */
    public String toString() {
        int modifiers = Toolkit.getDefaultToolkit().getMenuShortcutKeyMask();
        if (usesShiftModifier()) {
            modifiers |= Event.SHIFT_MASK;
        }
        return KeyEvent.getKeyModifiersText(modifiers) + "+" +
            KeyEvent.getKeyText(key);
    }

    protected String paramString() {
        String str = "key=" + key;
        if (usesShiftModifier()) {
            str += ",usesShiftModifier";
        }
        return str;
    }
}
