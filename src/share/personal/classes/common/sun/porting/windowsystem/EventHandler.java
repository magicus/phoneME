/*
 * @(#)EventHandler.java	1.15 06/10/10
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
 * The <code>EventHandler</code> interface is presented to the underlying
 * operating system (by registering with the underlying <code>GraphicsSystem</code>)
 * in order to allow events to flow into the system.
 *
 * @version 1.10, 08/19/02
 */
public interface EventHandler {
    /* event handling */

    /**
     * Neither press nor release.  This is used for events that have no "click"
     * associated with them, e.g. a mouse move or drag.
     */
    static final int NONE = 0;
    /**
     * An actuator was pressed.  This could be a mouse button, or a key on a
     * keyboard, keypad, remote control, etc. which signifies an event that can
     * be represented by a virtual key code (i.e. a constant in 
     * <code>java.awt.event.KeyEvent</code> whose name begins with <code>VK_</code>)
     */
    static final int PRESSED = 1;
    /**
     * An actuator was released.  This could be a mouse button, or a key on a
     * keyboard, keypad, remote control, etc. which signifies an event that can
     * be represented by a virtual key code (i.e. a constant in 
     * <code>java.awt.event.KeyEvent</code> whose name begins with <code>VK_</code>)
     */
    static final int RELEASED = 2;
    /**
     * The <code>ACTION</code> event is sent whenever an action button
     * is activated.  This event is used for special keys
     * (e.g. a "GO" key on a keypad) which cannot be represented by a virtual 
     * key code and are supposed to be handled or translated by the toolkit.
     */
    static final int ACTION = 3;
    /**
     * The <code>TYPED</code> event is used to deliver translated characters
     * into the system.  If translation from virtual keys to Unicode character codes
     * is done at a low level, this event type is used to deliver them; if the
     * translation must performed at a higher level (e.g. in the toolkit) then these
     * events will never be seen.
     */
    static final int TYPED = 4;
    /**
     * Called when an event on a pointing object (e.g. a mouse, track ball,
     * joystick etc.) has occurred.
     * @param when Time at which the event occurred, in milliseconds since
     *        1/1/70 UTC.
     * @param x Current position of pointer - x coordinate
     * @param y Current position of pointer - y coordinate
     * @param ID Type identifier indicating the kind of event
     * @param number Used to distinguish among several buttons (etc) that 
     *        could have been pressed or released.
     * @see java.lang.System#currentTimeMillis
     */
    void pointerEventOccurred(long when, int x, int y, int ID, int number);
    /**
     * Called when an event on a key object (typewriter-style keyboard key,
     * button on a remote control, etc) has occurred.
     * @param when Time at which the event occurred, in milliseconds since
     *        1/1/70 UTC.
     * @param ID Type identifier indicating the kind of event
     * @param keycode A code identifying which key was pressed.
     * @param keychar Unicode character -- for TYPED events only.  (Events
     * with the TYPED id will only occur if the system is doing key-to-char
     * translation at a low level.)
     * @see java.awt.event.KeyEvent
     * @see java.lang.System#currentTimeMillis
     */
    void keyboardEventOccurred(long when, int ID, int keycode, char keychar);
    /**
     * Indicates that some change or another happened at the graphics layer
     * which has made the current state of the 
     * This is a bit that can be logically OR-ed with the other flags.
     */
    static final int GRAPHICS_INVALID_FLAG = 1;
    /**
     * Indicates that the pixel definition of the root window has changed.
     * This is a bit that can be logically OR-ed with the other flags.
     */
    static final int COLORMODEL_CHANGE_FLAG = 2;
    /**
     * Indicates that the size of the root window area has changed.
     * This is a bit that can be logically OR-ed with the other flags.
     */
    static final int SIZE_CHANGE_FLAG = 4;
    /**
     * Called when a change occurred in the graphics system, e.g. the
     * depth or size of the screen was changed.  <code>GRAPHICS_INVALID_FLAG</code>
     * forces the window system to repaint all of its windows; it may
     * be sent all by itself if the system is an emulator and needs a
     * refresh.  The <code>Region</code> structure, if supplied, is used to clip the
     * repaint.  <code>GRAPHICS_INVALID_FLAG</code> should always be sent if the
     * event is a change in color model.  But it is permissible *not*
     * to repaint on a size change in some circumstances (most likely
     * in an emulator).  Other actions are to be taken as appropriate
     * (e.g. resetting the window system's idea of the root window
     * clip area.)
     */
    void graphicsChange(long when, sun.porting.graphicssystem.Region r, 
        int flags);
}
