/*
 * @(#)WindowFactory.java	1.12 06/10/10
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

package sun.porting.windowsystem;

/**
 * Window objects are created via a factory; this allows an implementation
 * to cleanly extend the definition of a Window if necessary.
 *
 * @version 1.7, 08/19/02
 */
public interface WindowFactory {
    /**
     * The default window type.
     */
    static final int CHILD_WINDOW = 0;
    /**
     * The window type for a top-level frame (corresponds to java.awt.Frame).
     */
    static final int TOP_LEVEL_FRAME = 1;
    /**
     * The window type for a dialog frame (corresponds to java.awt.Dialog and
     * java.awt.FileDialog).
     */
    static final int TOP_LEVEL_DIALOG = 2;
    /**
     * The window type for a frameless, top-level window, e.g. the kind that
     * might be used to implement a popup menu (corresponds to java.awt.Window).
     */
    static final int TOP_LEVEL_FRAMELESS = 3;
    /**
     * Create a new window as a child of the given parent window,
     * and having the specified dimensions.  Parent should not be null.
     * @param parent The parent window.
     * @param x the x coordinate for the window's upper left hand corner,
     * in the coordinate system of the parent window
     * @param y the y coordinate for the window's upper left hand corner,
     * in the coordinate system of the parent window
     * @param w the width of the window
     * @param h the height of the window
     * @return The new Window object.
     * @throws IllegalArgumentException if parent is null.
     */
    Window makeWindow(Window parent, int x, int y, int w, int h) 
        throws IllegalArgumentException;
    /**
     * Create a new top-level window of the given type
     * and having the specified dimensions.
     * @param type The type of the window.
     * @param x the x coordinate for the window's upper left hand corner,
     * in the global coordinate system of the root.
     * @param y the y coordinate for the window's upper left hand corner,
     * in the global coordinate system of the root.
     * @param w the width of the window
     * @param h the height of the window
     * @return The new Window object.
     * @throws IllegalArgumentException if type is not valid.
     */
    Window makeTopLevelWindow(int type, int x, int y, int w, int h)
        throws IllegalArgumentException;
}
