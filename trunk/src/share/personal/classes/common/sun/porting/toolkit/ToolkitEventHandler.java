/*
 * @(#)ToolkitEventHandler.java	1.17 06/10/10
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

package sun.porting.toolkit;

import sun.porting.windowsystem.Window;

/**
 * <code>ToolkitEventHandler</code>
 * is the only interface that a toolkit presents to the underlying window 
 * system.  It is the means for delivering events up into the toolkit.  
 * All of the other interactions between the window system and the toolkit are 
 * in the other direction, i.e. the toolkit issues a directive and the window 
 * system executes it.)
 *
 * @version 1.12, 08/19/02
 */
public interface ToolkitEventHandler {
    /**
     * Delimiting number: a pointer event will be greater than POINTER_EVENT_START.
     */
    final int POINTER_EVENT_START = 0100;
    /**
     * Event ID for a pointer press (i.e. a press associated with position
     * information, like a mouse click, touchscreen press, etc).
     */
    final int POINTER_PRESSED = 0101;
    /**
     * Event ID for a pointer release (i.e. a press associated with position
     * information, like a mouse click, touchscreen press, etc).  The window
     * system must guarantee that the release is delivered to the same window
     * that got the press, unless the focus was explicitly redirected by
     * the toolkit.
     */
    final int POINTER_RELEASED = 0102;
    /**
     * Event ID for a drag, i.e. a position change that occurs between
     * POINTER_PRESSED and POINTER_RELEASED.  The window system is responsible
     * for delivering all drags to the same window that got the press, unless
     * the focus is explicitly redirected by the toolkit.
     */
    final int POINTER_DRAGGED = 0104;
    /** 
     * Event ID for a position change.  This ID is used when the pointer is
     * being moved but no buttons (etc) are pressed, i.e. not a "drag" event.
     */
    final int POINTER_MOVED = 0103;
    /**
     * Delimiting number: a pointer event will be less than POINTER_EVENT_END.
     * (Note that this must be updated if new pointer events are added.)
     */
    final int POINTER_EVENT_END = 0104;
    /**
     * Called by the window system in order to deliver a pointer event.
     * @param w The window in which the event occurred.
     * @param when Time at which the event occurred, in milliseconds since
     *        1/1/70 UTC.
     * @param x x coordinate of position at which the event occurred.
     * Coordinates are relative to the window.
     * @param y y coordinate of position at which the event occurred.
     * Coordinates are relative to the window.
     * @param ID Type identifier indicating the kind of event
     * @param number Used to distinguish among several buttons (etc) that 
     *        could have been pressed or released.
     * @see java.lang.System#currentTimeMillis
     */
    void pointerEventOccurred(Window w, long when,
        int x, int y, 
        int ID, int number);
    /**
     * Delimiting number: a key event will be greater than KEY_EVENT_START.
     */
    final int KEY_EVENT_START = 0200;
    /**
     * Event ID for a key press corresponding to a typewriter-style keyboard.
     */
    final int KEY_PRESSED = 0201;
    /**
     * Event ID for a key press corresponding to a typewriter-style keyboard.
     */
    final int KEY_RELEASED = 0202;
    /**
     * Event ID for an event indicating that a character was typed.  This
     * event is only generated if the window system has taken responsiblity
     * for translating KEY_PRESSED/KEY_RELEASED events into characters.
     */
    final int KEY_TYPED = 0203;
    /**
     * Event ID for a press of special "non-keyboard" keys, like
     * a "GO" key on a remote control.
     */
    final int ACTION_PRESSED = 0204;
    /**
     * Event ID for a release of special "non-keyboard" keys, like
     * a "GO" key on a remote control.
     */
    final int ACTION_RELEASED = 0205;
    /**
     * Delimiting number: a key event will be less than KEY_EVENT_END.
     * (Note that this must be updated if new key events are added.)
     */
    final int KEY_EVENT_END = 0206;
    /**
     * Called by the window system in order to deliver a keyboard event.
     * Keyboard events are delivered to the window that currently has input
     * focus; whether this is the window that contains the pointer depends on
     * the focus policy implemented by the toolkit.
     * @param win The window to which the event is directed (the focus window).
     * @param when Time at which the event occurred, in milliseconds since
     *        1/1/70 UTC.
     * @param ID Type identifier indicating the kind of event
     * @param keycode A code identifying which key was pressed.
     * @param keychar Unicode character -- for KEY_TYPED only.
     * @see java.awt.event.KeyEvent
     * @see java.lang.System#currentTimeMillis
     */
    void keyboardEventOccurred(Window win, long when,
        int ID, int keycode, char keychar);
    /**
     * Delimiting number: a window event will be greater than WINDOW_EVENT_START.
     */
    final int WINDOW_EVENT_START = 0300;
    /**
     * Event ID for indicating that a window has been reshaped (moved and/or
     * resized).  For this event, {x, y, w, h} are DELTA VALUES, not absolute.
     */
    final int WINDOW_RESHAPED = 0301;
    /**
     * Event ID for indicating that a window has been assigned keyboard focus.
     */
    final int WINDOW_GOT_FOCUS = 0302;
    /**
     * Event ID for indicating that a window no longer has the keyboard focus.
     */
    final int WINDOW_LOST_FOCUS = 0303;
    /**
     * Event ID for indicating that a region of a window needs to be repainted.
     */
    final int WINDOW_DAMAGED = 0304;
    /**
     * Event ID for indicating that the pointing device has entered the window.
     */
    final int WINDOW_ENTERED = 0305;
    /**
     * Event ID for indicating that the pointing device has left the window.
     */
    final int WINDOW_EXITED = 0306;
    /**
     * Event ID for indicating that a window has been mapped (made visible).
     * The toolkit is allowed to use this event for optimization
     * purposes but must not depend on it, because it is optional.
     */
    final int WINDOW_MAPPED = 0307;
    /**
     * Event ID for indicating that a window has been unmapped (made invisible).
     * The toolkit is allowed to use this event for optimization
     * purposes but must not depend on it, because it is optional.
     */
    final int WINDOW_UNMAPPED = 0310;
    /**
     * Event ID for indicating that a mapped window has been clipped and/or 
     * completely covered by one or more other windows (this could include 
     * its children).  When the window is no longer completely invisible,
     * window system is expected to generate WINDOW_DAMAGED events for the
     * newly-visible portions of the window.)
     * WINDOW_INVISIBLE events must not be generated when the window is
     * unmapped.
     * The toolkit is allowed to use this event for optimization
     * purposes but must not depend on it, because it is optional.
     */
    final int WINDOW_INVISIBLE = 0311;
    /**
     * Delimiting number: a window event will be less than WINDOW_EVENT_END.
     * (Note that this must be updated if new window events are added.)
     */
    final int WINDOW_EVENT_END = 0312;
    /**
     * Called by the window system in order to deliver a window event.
     * @param win The window on which the event occurred.
     * @param when Time at which the event occurred, in milliseconds since
     *        1/1/70 UTC.
     * @param x x coordinate of position at which the event occurred.
     * @param y y coordinate of position at which the event occurred.  
     *        Coordinates are relative
     *        to the window's parent for a reshape or map event, but relative 
     *        to the window itself for other events (enter, exit, and damage).
     * @param w Width of window (reshape or map event) or of the
     *          area to be repainted (damage event).
     * @param h Width of window (reshape or map event) or of the
     *          area to be repainted (damage event).
     * @param ID Type identifier indicating the kind of event
     * @see java.awt.event.KeyEvent
     * @see java.lang.System#currentTimeMillis
     */
    void windowEventOccurred(Window win, long when,
        int x, int y, int w, int h, 
        int ID);
}
