/*
 * @(#)WindowEvent.java	1.24 06/10/10
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

package java.awt.event;

import java.awt.Window;

import sun.awt.AppContext;
import sun.awt.SunToolkit;

/**
 * The window-level event.
 *
 * @version 1.17 08/19/02
 * @author Carl Quinn
 * @author Amy Fowler
 */
public class WindowEvent extends ComponentEvent {
    /**
     * Marks the first integer id for the range of window event ids.
     */
    public static final int WINDOW_FIRST = 200;

    /**
     * The window opened event type.  This event is delivered only
     * the first time a window is made visible.
     */
    public static final int WINDOW_OPENED = WINDOW_FIRST; // 200
    /**
     * The window closing event type. This event is delivered when
     * the user selects "Quit" from the window's system menu.  If
     * the program does not explicitly hide or destroy the window as
     * a result of this event, the window close operation will be
     * cancelled.
     */
    public static final int WINDOW_CLOSING = 1 + WINDOW_FIRST; //Event.WINDOW_DESTROY
    /**
     * The window closed event type. This event is delivered after
     * the window has been closed as the result of a call to hide or
     * destroy.
     */
    public static final int WINDOW_CLOSED = 2 + WINDOW_FIRST;
    /**
     * The window iconified event type.
     */
    public static final int WINDOW_ICONIFIED = 3 + WINDOW_FIRST; //Event.WINDOW_ICONIFY
    /**
     * The window deiconified event type.
     */
    public static final int WINDOW_DEICONIFIED = 4 + WINDOW_FIRST; //Event.WINDOW_DEICONIFY
    /**
     * The window activated event type.
     */
    public static final int WINDOW_ACTIVATED = 5 + WINDOW_FIRST;
    /**
     * The window deactivated event type.
     */
    public static final int WINDOW_DEACTIVATED = 6 + WINDOW_FIRST;

    /**
     * The window gained focus event type.
     */
    public static final int WINDOW_GAINED_FOCUS = 7 + WINDOW_FIRST;

    /**
     * The window lost focus event type.
     */
    public static final int WINDOW_LOST_FOCUS = 8 + WINDOW_FIRST;

    /**
     * Marks the last integer id for the range of window event ids.
     */
    public static final int WINDOW_LAST = WINDOW_LOST_FOCUS;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = -1567959133147912127L;

    transient Window opposite;
    //int oldState;
    //int newState;

    /**
     * Constructs a WindowEvent object with the specified source window 
     * and type.
     * @param source the component where the event originated
     * @id the event type
     */
    public WindowEvent(Window source, int id) {
        super(source, id);
    }

    /* 
    public WindowEvent(Window source, int id, Window opposite,
	    int oldState, int newState) {
        super(source, id);
        this.opposite = opposite;
        this.oldState = oldState;
        this.newState = newState;
    }
    */

    public WindowEvent(Window source, int id, Window opposite) {
        super(source, id);
        this.opposite = opposite;
    }

    /*
    public WindowEvent(Window source, int id, int oldState, int newState) {
        this(source, id, null, oldState, newState);
    }
    */

    /**
     * Returns the window where this event originated.
     */
    public Window getWindow() {
        return (source instanceof Window) ? (Window) source : null;
    } 

    public Window getOppositeWindow() {
        if (opposite == null) {
            return null;
        }
        return (SunToolkit.targetToAppContext(opposite) ==
                AppContext.getAppContext()) ? opposite : null;
    }

/*
    public int getOldState() {
        return oldState;
    }

    public int getNewState() {
        return newState;
    }
*/

    public String paramString() {
        String typeStr;
        switch (id) {
        case WINDOW_OPENED:
            typeStr = "WINDOW_OPENED";
            break;

        case WINDOW_CLOSING:
            typeStr = "WINDOW_CLOSING";
            break;

        case WINDOW_CLOSED:
            typeStr = "WINDOW_CLOSED";
            break;

        case WINDOW_ICONIFIED:
            typeStr = "WINDOW_ICONIFIED";
            break;

        case WINDOW_DEICONIFIED:
            typeStr = "WINDOW_DEICONIFIED";
            break;

        case WINDOW_ACTIVATED:
            typeStr = "WINDOW_ACTIVATED";
            break;

        case WINDOW_DEACTIVATED:
            typeStr = "WINDOW_DEACTIVATED";
            break;
	    case WINDOW_GAINED_FOCUS:
	        typeStr = "WINDOW_GAINED_FOCUS";
	        break;
	    case WINDOW_LOST_FOCUS:
	        typeStr = "WINDOW_LOST_FOCUS";
	        break;
        default:
            typeStr = "unknown type";
        }
        return typeStr + ",opposite=" + getOppositeWindow();// + 
//            ",oldState=" + oldState + ",newState=" + newState;
     }
}
