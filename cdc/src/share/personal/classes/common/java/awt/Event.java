/*
 * @(#)Event.java	1.66 06/10/10
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
import java.io.*;

/**
 * <code>Event</code> is a platform-independent class that  
 * encapsulates events from the platform's Graphical User 
 * Interface in the Java&nbsp;1.0 event model. In Java&nbsp;1.1 
 * and later versions, the <code>Event</code> class is maintained 
 * only for backwards compatibilty. The information in this
 * class description is provided to assist programmers in
 * converting Java&nbsp;1.0 programs to the new event model.
 * <p>
 * In the Java&nbsp;1.0 event model, an event contains an 
 * <a href="#id"><code>id</code></a> field 
 * that indicates what type of event it is and which other 
 * <code>Event</code> variables are relevant for the event.
 * <p>
 * For keyboard events, <a href="#key"><code>key</code></a> 
 * contains a value indicating which key was activated, and 
 * <a href="#modifiers"><code>modifiers</code></a> contains the 
 * modifiers for that event.  For the KEY_PRESS and KEY_RELEASE  
 * event ids, the value of <code>key</code> is the unicode 
 * character code for the key. For KEY_ACTION and 
 * KEY_ACTION_RELEASE, the value of <code>key</code> is
 * one of the defined action-key identifiers in the 
 * <code>Event</code> class (<code>PGUP</code>,  
 * <code>PGDN</code>, <code>F1</code>, <code>F2</code>, etc).
 *
 * @version 1.62 08/19/02
 * @author     Sami Shaio
 * @since      JDK1.0
 */
public class Event implements java.io.Serializable {
    private transient int data;
    /* Modifier constants */

    /**
     * This flag indicates that the Shift key was down when the event 
     * occurred. 
     * @since   JDK1.0
     */
    public static final int SHIFT_MASK = 1 << 0;
    /**
     * This flag indicates that the Control key was down when the event 
     * occurred. 
     * @since   JDK1.0
     */
    public static final int CTRL_MASK = 1 << 1;
    /** 
     * This flag indicates that the Meta key was down when the event 
     * occurred. For mouse events, this flag indicates that the right 
     * button was pressed or released. 
     * @since    JDK1.0
     */
    public static final int META_MASK = 1 << 2;
    /** 
     * This flag indicates that the Alt key was down when 
     * the event occurred. For mouse events, this flag indicates that the 
     * middle mouse button was pressed or released. 
     * @since   JDK1.0
     */
    public static final int ALT_MASK = 1 << 3;
    /* Action keys */
    
    /** 
     * The Home key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int HOME = 1000;
    /** 
     * The End key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int END = 1001;
    /**
     * The Page Up key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int PGUP = 1002;
    /**
     * The Page Down key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int PGDN = 1003;
    /**
     * The Up Arrow key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int UP = 1004;
    /**
     * The Down Arrow key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int DOWN = 1005;
    /**
     * The Left Arrow key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int LEFT = 1006;
    /**
     * The Right Arrow key, a non-ASCII action key. 
     * @since   JDK1.0
     */
    public static final int RIGHT = 1007;
    /**
     * The F1 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F1 = 1008;
    /**
     * The F2 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F2 = 1009;
    /**
     * The F3 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F3 = 1010;
    /**
     * The F4 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F4 = 1011;
    /**
     * The F5 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F5 = 1012;
    /**
     * The F6 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F6 = 1013;
    /**
     * The F7 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F7 = 1014;
    /**
     * The F8 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F8 = 1015;
    /**
     * The F9 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F9 = 1016;
    /**
     * The F10 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F10 = 1017;
    /**
     * The F11 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F11 = 1018;
    /**
     * The F12 function key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int F12 = 1019;
    /**
     * The Print Screen key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int PRINT_SCREEN = 1020;
    /**
     * The Scroll Lock key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int SCROLL_LOCK = 1021;
    /**
     * The Caps Lock key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int CAPS_LOCK = 1022;
    /**
     * The Num Lock key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int NUM_LOCK = 1023;
    /**
     * The Pause key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int PAUSE = 1024;
    /**
     * The Insert key, a non-ASCII action key.
     * @since   JDK1.0
     */
    public static final int INSERT = 1025;
    /* Non-action keys */
    
    /**
     * The Enter key.
     * @since   JDK1.0
     */
    public static final int ENTER = '\n';
    /**
     * The BackSpace key.
     * @since   JDK1.0
     */
    public static final int BACK_SPACE = '\b';
    /**
     * The Tab key.
     * @since   JDK1.0
     */
    public static final int TAB = '\t';
    /**
     * The Escape key.
     * @since   JDK1.0
     */
    public static final int ESCAPE = 27;
    /**
     * The Delete key.
     * @since   JDK1.0
     */
    public static final int DELETE = 127;
    /* Base for all window events. */
    private static final int WINDOW_EVENT = 200;
    /**
     * The user has asked the window manager to kill the window.
     * @since    JDK1.0
     */
    public static final int WINDOW_DESTROY = 1 + WINDOW_EVENT;
    /**
     * The user has asked the window manager to expose the window.
     * @since    JDK1.0
     */
    public static final int WINDOW_EXPOSE = 2 + WINDOW_EVENT;
    /** 
     * The user has asked the window manager to iconify the window.
     * @since    JDK1.0
     */
    public static final int WINDOW_ICONIFY = 3 + WINDOW_EVENT;
    /** 
     * The user has asked the window manager to de-iconify the window.
     * @since    JDK1.0
     */
    public static final int WINDOW_DEICONIFY = 4 + WINDOW_EVENT;
    /**
     * The user has asked the window manager to move the window.
     * @since    JDK1.0
     */
    public static final int WINDOW_MOVED = 5 + WINDOW_EVENT;
    /* Base for all keyboard events. */
    private static final int KEY_EVENT = 400;
    /**
     * The user has pressed a normal key.  
     * @since   JDK1.0
     */
    public static final int KEY_PRESS = 1 + KEY_EVENT;
    /**
     * The user has released a normal key.  
     * @since   JDK1.0
     */
    public static final int KEY_RELEASE = 2 + KEY_EVENT;
    /** 
     * The user has pressed a non-ASCII <em>action</em> key.  
     * The <code>key</code> field contains a value that indicates
     * that the event occurred on one of the action keys, which
     * comprise the 12 function keys, the arrow (cursor) keys,
     * Page Up, Page Down, Home, End, Print Screen, Scroll Lock,
     * Caps Lock, Num Lock, Pause, and Insert.
     * @since   JDK1.0
     */
    public static final int KEY_ACTION = 3 + KEY_EVENT;
    /** 
     * The user has released a non-ASCII <em>action</em> key.  
     * The <code>key</code> field contains a value that indicates
     * that the event occurred on one of the action keys, which
     * comprise the 12 function keys, the arrow (cursor) keys,
     * Page Up, Page Down, Home, End, Print Screen, Scroll Lock,
     * Caps Lock, Num Lock, Pause, and Insert.
     * @since   JDK1.0
     */
    public static final int KEY_ACTION_RELEASE = 4 + KEY_EVENT;
    /* Base for all mouse events. */
    private static final int MOUSE_EVENT = 500;
    /**
     * The user has pressed the mouse button. The <code>ALT_MASK</code> 
     * flag indicates that the middle button has been pressed. 
     * The <code>META_MASK</code>flag indicates that the 
     * right button has been pressed. 
     * @see     java.awt.Event#ALT_MASK
     * @see     java.awt.Event#META_MASK
     * @since   JDK1.0
     */
    public static final int MOUSE_DOWN = 1 + MOUSE_EVENT;
    /**
     * The user has released the mouse button. The <code>ALT_MASK</code> 
     * flag indicates that the middle button has been released. 
     * The <code>META_MASK</code>flag indicates that the 
     * right button has been released. 
     * @see     java.awt.Event#ALT_MASK
     * @see     java.awt.Event#META_MASK
     * @since   JDK1.0
     */
    public static final int MOUSE_UP = 2 + MOUSE_EVENT;
    /**
     * The mouse has moved with no button pressed. 
     * @since   JDK1.0
     */
    public static final int MOUSE_MOVE = 3 + MOUSE_EVENT;
    /**
     * The mouse has entered a component. 
     * @since   JDK1.0
     */
    public static final int MOUSE_ENTER = 4 + MOUSE_EVENT;
    /**
     * The mouse has exited a component. 
     * @since   JDK1.0
     */
    public static final int MOUSE_EXIT = 5 + MOUSE_EVENT;
    /** 
     * The user has moved the mouse with a button pressed. The 
     * <code>ALT_MASK</code> flag indicates that the middle 
     * button is being pressed. The <code>META_MASK</code> flag indicates 
     * that the right button is being pressed. 
     * @see     java.awt.Event#ALT_MASK
     * @see     java.awt.Event#META_MASK
     * @since   JDK1.0
     */
    public static final int MOUSE_DRAG = 6 + MOUSE_EVENT;
    /* Scrolling events */
    private static final int SCROLL_EVENT = 600;
    /** 
     * The user has activated the <em>line up</em>  
     * area of a scroll bar. 
     * @since   JDK1.0
     */
    public static final int SCROLL_LINE_UP = 1 + SCROLL_EVENT;
    /**
     * The user has activated the <em>line down</em>  
     * area of a scroll bar. 
     * @since   JDK1.0
     */
    public static final int SCROLL_LINE_DOWN = 2 + SCROLL_EVENT;
    /**
     * The user has activated the <em>page up</em>  
     * area of a scroll bar. 
     * @since   JDK1.0
     */
    public static final int SCROLL_PAGE_UP = 3 + SCROLL_EVENT;
    /**
     * The user has activated the <em>page down</em>  
     * area of a scroll bar. 
     * @since   JDK1.0
     */
    public static final int SCROLL_PAGE_DOWN = 4 + SCROLL_EVENT;
    /**
     * The user has moved the bubble (thumb) in a scroll bar,
     * moving to an "absolute" position, rather than to
     * an offset from the last postion.
     * @since   JDK1.0
     */
    public static final int SCROLL_ABSOLUTE = 5 + SCROLL_EVENT;
    /**
     * The scroll begin event.
     * @since   JDK1.0
     */
    public static final int SCROLL_BEGIN = 6 + SCROLL_EVENT;
    /**
     * The scroll end event.
     * @since   JDK1.0
     */
    public static final int SCROLL_END = 7 + SCROLL_EVENT;
    /* List Events */
    private static final int LIST_EVENT = 700;
    /**
     * An item in a list has been selected. 
     * @since   JDK1.0
     */
    public static final int LIST_SELECT = 1 + LIST_EVENT;
    /**
     * An item in a list has been deselected. 
     * @since   JDK1.0
     */
    public static final int LIST_DESELECT = 2 + LIST_EVENT;
    /* Misc Event */
    private static final int MISC_EVENT = 1000;
    /**
     * This event indicates that the user wants some action to occur. 
     * @since   JDK1.0
     */
    public static final int ACTION_EVENT = 1 + MISC_EVENT;
    /**
     * A file loading event.
     * @since   JDK1.0
     */
    public static final int LOAD_FILE = 2 + MISC_EVENT;
    /**
     * A file saving event.
     * @since   JDK1.0
     */
    public static final int SAVE_FILE = 3 + MISC_EVENT;
    /**
     * A component gained the focus.
     * @since   JDK1.0
     */
    public static final int GOT_FOCUS = 4 + MISC_EVENT;
    /**
     * A component lost the focus.
     * @since   JDK1.0
     */
    public static final int LOST_FOCUS = 5 + MISC_EVENT;
    /**
     * The target component. This indicates the component over which the 
     * event occurred or with which the event is associated. 
     * @since   JDK1.0
     */
    public Object target;
    /**
     * The time stamp.
     * @since   JDK1.0
     */
    public long when;
    /**
     * Indicates which type of event the event is, and which 
     * other <code>Event</code> variables are relevant for the event.
     * @since   JDK1.0
     */
    public int id;
    /** 
     * The <i>x</i> coordinate of the event. 
     * @since   JDK1.0
     */
    public int x;
    /** 
     * The <i>y</i> coordinate of the event. 
     * @since   JDK1.0
     */
    public int y;
    /** 
     * The key code of the key that was pressed in a keyboard event. 
     * @since   JDK1.0
     */
    public int key;
    /** 
     * The key character that was pressed in a keyboard event. 
     * @since   JDK1.0
     */
    //    public char keyChar;

    /** 
     * The state of the modifier keys.
     * <p>
     * NOTE:  changing the modifier keys is not recommended, because many
     * native implementations do not recognize modifier changes.  This is
     * especially true when the shift modifier is changed.
     *
     * @since     JDK1.0
     */
    public int modifiers;
    /**
     * For <code>MOUSE_DOWN</code> events, this field indicates the 
     * number of consecutive clicks. For other events, its value is 
     * <code>0</code>. 
     * @since   JDK1.0
     */
    public int clickCount;
    /**
     * An arbitrary argument of the event. The value of this field 
     * depends on the type of event. 
     * @since   JDK1.0
     */
    public Object arg;
    /**
     * The next event. This field is set when putting events into a 
     * linked list. 
     * @since   JDK1.0
     */
    public Event evt;
    /* table for mapping old Event action keys to KeyEvent virtual keys. */
    private static final int actionKeyCodes[][] = {
            
            /*    virtual key              action key   */
            { KeyEvent.VK_HOME, Event.HOME         },
            { KeyEvent.VK_END, Event.END          },
            { KeyEvent.VK_PAGE_UP, Event.PGUP         },
            { KeyEvent.VK_PAGE_DOWN, Event.PGDN         },
            { KeyEvent.VK_UP, Event.UP           },
            { KeyEvent.VK_DOWN, Event.DOWN         },
            { KeyEvent.VK_LEFT, Event.LEFT         },
            { KeyEvent.VK_RIGHT, Event.RIGHT        },
            { KeyEvent.VK_F1, Event.F1           },
            { KeyEvent.VK_F2, Event.F2           },
            { KeyEvent.VK_F3, Event.F3           },
            { KeyEvent.VK_F4, Event.F4           },
            { KeyEvent.VK_F5, Event.F5           },
            { KeyEvent.VK_F6, Event.F6           },
            { KeyEvent.VK_F7, Event.F7           },
            { KeyEvent.VK_F8, Event.F8           },
            { KeyEvent.VK_F9, Event.F9           },
            { KeyEvent.VK_F10, Event.F10          },
            { KeyEvent.VK_F11, Event.F11          },
            { KeyEvent.VK_F12, Event.F12          },
            { KeyEvent.VK_PRINTSCREEN, Event.PRINT_SCREEN },
            { KeyEvent.VK_SCROLL_LOCK, Event.SCROLL_LOCK  },
            { KeyEvent.VK_CAPS_LOCK, Event.CAPS_LOCK    },
            { KeyEvent.VK_NUM_LOCK, Event.NUM_LOCK     },
            { KeyEvent.VK_PAUSE, Event.PAUSE        },
            { KeyEvent.VK_INSERT, Event.INSERT       }
        };
    // This field controls whether or not the event is sent back
    // down to the peer once the target has processed it -
    // false means it's sent to the peer, true means it's not.
    private boolean consumed = false;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = 5488922509400504703L;
    /**
     * Creates an instance of <code>Event</code> with the specified target 
     * component, time stamp, event type, <i>x</i> and <i>y</i> 
     * coordinates, keyboard key, state of the modifier keys, and 
     * argument. 
     * @param     target     the target component.
     * @param     when       the time stamp.
     * @param     id         the event type.
     * @param     x          the <i>x</i> coordinate.
     * @param     y          the <i>y</i> coordinate.
     * @param     key        the key pressed in a keyboard event.
     * @param     modifiers  the state of the modifier keys.
     * @param     arg        the specified argument.
     * @since     JDK1.0
     */
    public Event(Object target, long when, int id, int x, int y, int key,
        int modifiers, Object arg) {
        this.target = target;
        this.when = when;
        this.id = id;
        this.x = x;
        this.y = y;
        this.key = key;
        this.modifiers = modifiers;
        this.arg = arg;
        this.data = 0;
        this.clickCount = 0;
        switch (id) {
        case ACTION_EVENT:
        case WINDOW_DESTROY:
        case WINDOW_ICONIFY:
        case WINDOW_DEICONIFY:
        case WINDOW_MOVED:
        case SCROLL_LINE_UP:
        case SCROLL_LINE_DOWN:
        case SCROLL_PAGE_UP:
        case SCROLL_PAGE_DOWN:
        case SCROLL_ABSOLUTE:
        case SCROLL_BEGIN:
        case SCROLL_END:
        case LIST_SELECT:
        case LIST_DESELECT:
            consumed = true; // these types are not passed back to peer
            break;

        default:
        }
    }

    /**
     * Creates an instance of <code>Event</code>, with the specified target 
     * component, time stamp, event type, <i>x</i> and <i>y</i> 
     * coordinates, keyboard key, state of the modifier keys, and an 
     * argument set to <code>null</code>. 
     * @param     target     the target component.
     * @param     when       the time stamp.
     * @param     id         the event type.
     * @param     x          the <i>x</i> coordinate.
     * @param     y          the <i>y</i> coordinate.
     * @param     key        the key pressed in a keyboard event.
     * @param     modifiers  the state of the modifier keys.
     * @since    JDK1.0
     */
    public Event(Object target, long when, int id, int x, int y, int key, int modifiers) {
        this(target, when, id, x, y, key, modifiers, null);
    }

    /**
     * Creates an instance of <code>Event</code> with the specified  
     * target component, event type, and argument. 
     * @param     target     the target component.
     * @param     id         the event type.
     * @param     arg        the specified argument.
     * @since     JDK1.0
     */
    public Event(Object target, int id, Object arg) {
        this(target, 0, id, 0, 0, 0, 0, arg);
    }

    /** 
     * Translates this event so that its <i>x</i> and <i>y</i> 
     * coordinates are increased by <i>dx</i> and <i>dy</i>, 
     * respectively. 
     * <p>
     * This method translates an event relative to the given component. 
     * This involves, at a minimum, translating the coordinates into the
     * local coordinate system of the given component. It may also involve
     * translating a region in the case of an expose event.
     * @param     dx     the distance to translate the <i>x</i> coordinate.
     * @param     dy     the distance to translate the <i>y</i> coordinate.
     * @since     JDK1.0
     */
    public void translate(int x, int y) {
        this.x += x;
        this.y += y;
    }

    /**
     * Checks if the Shift key is down.
     * @return    <code>true</code> if the key is down; 
     *            <code>false</code> otherwise.
     * @see       java.awt.Event#modifiers
     * @see       java.awt.Event#controlDown
     * @see       java.awt.Event#metaDown
     * @since     JDK1.0
     */
    public boolean shiftDown() {
        return (modifiers & SHIFT_MASK) != 0;
    }

    /**
     * Checks if the Control key is down.
     * @return    <code>true</code> if the key is down; 
     *            <code>false</code> otherwise.
     * @see       java.awt.Event#modifiers
     * @see       java.awt.Event#shiftDown
     * @see       java.awt.Event#metaDown
     * @since     JDK1.0
     */
    public boolean controlDown() {
        return (modifiers & CTRL_MASK) != 0;
    }

    /**
     * Checks if the Meta key is down.
     * @return    <code>true</code> if the key is down; 
     *            <code>false</code> otherwise.
     * @see       java.awt.Event#modifiers
     * @see       java.awt.Event#shiftDown
     * @see       java.awt.Event#controlDown
     * @since     JDK1.0
     */
    public boolean metaDown() {
        return (modifiers & META_MASK) != 0;
    }

    void consume() {
        switch (id) {
        case KEY_PRESS:
        case KEY_RELEASE:
        case KEY_ACTION:
        case KEY_ACTION_RELEASE:
            consumed = true;
            break;

        default:
            // event type cannot be consumed
        }
    }

    boolean isConsumed() {
        return consumed;
    }

    /*
     * Returns the integer key-code associated with the key in this event,
     * as described in java.awt.Event.  
     */
    static int getOldEventKey(KeyEvent e) {
        int keyCode = e.getKeyCode();
        for (int i = 0; i < actionKeyCodes.length; i++) {
            if (actionKeyCodes[i][0] == keyCode) {
                return actionKeyCodes[i][1];
            }
        }
        return (int) e.getKeyChar();
    }

    /*
     * Returns a new KeyEvent char which corresponds to the int key
     * of this old event.
     */
    char getKeyEventChar() {
        for (int i = 0; i < actionKeyCodes.length; i++) {
            if (actionKeyCodes[i][1] == key) {
                return KeyEvent.CHAR_UNDEFINED;
            }
        }
        return (char) key;
    }

    /**
     * Returns the parameter string representing this event. 
     * This string is useful for debugging.
     * @return    the parameter string of this event.
     * @since     JDK1.0 
     */
    protected String paramString() {
        String str = "id=" + id + ",x=" + x + ",y=" + y;
        if (key != 0) {
            str += ",key=" + key;
        }
        if (shiftDown()) {
            str += ",shift";
        }
        if (controlDown()) {
            str += ",control";
        }
        if (metaDown()) {
            str += ",meta";
        }
        if (target != null) {
            str += ",target=" + target;
        }
        if (arg != null) {
            str += ",arg=" + arg;
        }
        return str;
    }

    /**
     * Returns a representation of this event's values as a string.
     * @return    a string that represents the event and the values
     *                 of its member fields.
     * @see       java.awt.Event#paramString
     * @since     JDK1.1
     */
    public String toString() {
        return getClass().getName() + "[" + paramString() + "]";
    }
}
