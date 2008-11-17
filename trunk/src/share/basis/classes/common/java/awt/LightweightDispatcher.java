/*
 * @(#)LightweightDispatcher.java	1.16 06/10/10
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
 */

package java.awt;

import java.awt.event.AWTEventListener;
import java.awt.event.MouseEvent;
import java.awt.event.FocusEvent;
import java.awt.event.KeyEvent;

import sun.awt.SunToolkit;

/**
 * Class to manage the dispatching of events to the lightweight
 * components contained by a native container. In basis profile,
 * there is only one native container which is the Frame. A
 * LightweightDispatcher is associated with the Frame and forwards
 * mouse events to the lightweight components contained in it.
 *
 */
class LightweightDispatcher implements java.io.Serializable {
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 5184291520170872969L;
    LightweightDispatcher(Window window) {
        this.window = window;
        mouseEventTarget = null;
    }
	
    /**
     * Dispatches an event to a lightweight sub-component if necessary, and
     * returns whether or not the event was forwarded to a lightweight
     * sub-component.
     *
     * @param e the event
     */
    boolean dispatchEvent(AWTEvent e) {
        boolean ret = false;
        if (e instanceof MouseEvent) {
            MouseEvent me = (MouseEvent) e;
            ret = processMouseEvent(me);
        }
         if (e instanceof MouseEvent) {
            // find out what component the mouse moved in
            MouseEvent me = (MouseEvent) e;
            if (me.getID() == MouseEvent.MOUSE_MOVED) {
                cursorOn = window.getCursorTarget(me.getX(), me.getY());
                updateCursor(cursorOn);
            }
        }
        return ret;
    }
	
    Component getFocusOwner() {
        return focus;
    }
	
    /**
     * This method attempts to distribute a mouse event to a lightweight
     * component.  It tries to avoid doing any unnecessary probes down
     * into the component tree to minimize the overhead of determining
     * where to route the event, since mouse movement events tend to
     * come in large and frequent amounts.
     */
    private boolean processMouseEvent(MouseEvent e) {
        int id = e.getID();
        Component targetOver;
        Component lwOver;
        targetOver = window.getMouseEventTarget(e.getX(), e.getY(), true);
        trackMouseEnterExit(targetOver, e);
        if (mouseEventTarget == null) {
            if (id == MouseEvent.MOUSE_MOVED ||
                id == MouseEvent.MOUSE_PRESSED) {
                lwOver = (targetOver != window) ? targetOver : null;
                mouseEventTarget = lwOver;
            }
        }
        if (mouseEventTarget != null) {
            // pbp specific, check if the mouseEventTarget belongs to a 
            // different AppContext from the original target
            // If they're different, we cannot dispatch the event 
            // with the current EventQueue's dispatcher thread, and 
            // need to repost the event with the retargetted component's
            // EventQueue.
            boolean repost = !appContextEquals((Component)e.getSource(), mouseEventTarget);

            // we are currently forwarding to some component, check
            // to see if we should continue to forward.
            switch (id) {
            case MouseEvent.MOUSE_DRAGGED:
                if (dragging) {
                    retargetMouseEvent(mouseEventTarget, id, e, repost);
                }
                break;

            case MouseEvent.MOUSE_PRESSED:
                dragging = true;
                retargetMouseEvent(mouseEventTarget, id, e, repost);
                break;

            case MouseEvent.MOUSE_RELEASED:
                Component releasedTarget = mouseEventTarget;
                dragging = false;
                retargetMouseEvent(mouseEventTarget, id, e, repost);
                lwOver = window.getMouseEventTarget(e.getX(), e.getY(), false);
                mouseEventTarget = lwOver;
                // fix 4155217 component was hidden or moved in user
                // code MOUSE_RELEASED handling
                isClickOrphaned = lwOver != releasedTarget;
                break;

            case MouseEvent.MOUSE_CLICKED:
                if (!isClickOrphaned) {
                    retargetMouseEvent(mouseEventTarget, id, e, repost);
                }
                isClickOrphaned = false;
                break;

            case MouseEvent.MOUSE_ENTERED:
                break;

            case MouseEvent.MOUSE_EXITED:
                if (!dragging) {
                    mouseEventTarget = null;
                }
                break;

            case MouseEvent.MOUSE_MOVED:
                lwOver = window.getMouseEventTarget(e.getX(), e.getY(), false);
                mouseEventTarget = lwOver;
                retargetMouseEvent(mouseEventTarget, id, e, repost);
                break;
            }
            e.consume();
        }
        return e.isConsumed();
    }
	
    /*
     * Generates enter/exit events as mouse moves over lw components
     * @param targetOver	Target mouse is over (including native container)
     * @param e			Mouse event in native container
     */
    private void trackMouseEnterExit(Component targetOver, MouseEvent e) {
        Component	targetEnter = null;
        int		id = e.getID();
        if (id != MouseEvent.MOUSE_EXITED &&
            id != MouseEvent.MOUSE_DRAGGED &&
            isMouseInNativeContainer == false) {
            // any event but an exit or drag means we're in the native container
            isMouseInNativeContainer = true;
        } else if (id == MouseEvent.MOUSE_EXITED) {
            isMouseInNativeContainer = false;
        }
        if (isMouseInNativeContainer) {
            targetEnter = targetOver;
        }
        //System.out.println("targetEnter = " + targetEnter);
        //System.out.println("targetLastEntered = " + targetLastEntered);
		
        if (targetLastEntered == targetEnter) {
            return;
        }
        retargetMouseEvent(targetLastEntered, MouseEvent.MOUSE_EXITED, e, 
                           !appContextEquals((Component)e.getSource(), targetLastEntered));
        if (id == MouseEvent.MOUSE_EXITED) {
            // consume native exit event if we generate one
            e.consume();
        }
        retargetMouseEvent(targetEnter, MouseEvent.MOUSE_ENTERED, e,
                           !appContextEquals((Component)e.getSource(), targetEnter));
        if (id == MouseEvent.MOUSE_ENTERED) {
            // consume native enter event if we generate one
            e.consume();
        }
        //System.out.println("targetLastEntered: " + targetLastEntered);
        targetLastEntered = targetEnter;
    }
	
    /**
     * Sends a mouse event to the current mouse event recipient using
     * the given event (sent to the windowed host) as a srcEvent.  If
     * the mouse event target is still in the component tree, the
     * coordinates of the event are translated to those of the target.
     * If the target has been removed, we don't bother to send the
     * message.
    * void retargetMouseEvent(Component target, int id, MouseEvent e) {
    *   retargetMouseEvent(target, id, e, false) {
    * }
     */

    /*
     * @param repost - true if the new target belongs to a different AppContext 
     *                 from the original target
     */
    void retargetMouseEvent(Component target, int id, MouseEvent e, boolean repost) {
        if (target == null) {
            return; // mouse is over another hw component
        }
        int x = e.getX(), y = e.getY();
        Component component;
        for (component = target;
            component != null && component != window;
            component = component.getParent()) {
            x -= component.x;
            y -= component.y;
        }
        if (component != null) {
            MouseEvent retargeted = new MouseEvent(target,
                    id,
                    e.getWhen(),
                    e.getModifiers(),
                    x,
                    y,
                    e.getClickCount(),
                    e.isPopupTrigger());
            if (target == window) {
                // avoid recursively calling LightweightDispatcher...
                window.dispatchEventToSelf(retargeted);
            } else {
                //target.dispatchEvent(retargeted);
                if (repost)  {
                   /* can't dispatch event with this EventQueue's 
                    * dispatcher thread. Need to repost to the right EQ.
                   **/
                   SunToolkit.postEvent(target.appContext, retargeted);
                } else {
                   target.dispatchEvent(retargeted);
                }
            }
        }
    }
	
    /**
     * Set the cursor for a lightweight component
     * Enforce that null cursor means inheriting from parent
     */
    void updateCursor(Component comp) {
        // if user wants to change the cursor, we do it even mouse is dragging
        // so LightweightDispatcher's dragging state is not checked here
        // get the current mouse location
	Point currentLoc = GraphicsEnvironment.getLocalGraphicsEnvironment().getMouseLocation();
        cursorOn = window.getCursorTarget(currentLoc.x, currentLoc.y);
        if (comp != cursorOn) {
            return;
        }
         
        if (comp == null) {
            comp = window;
        }
        Cursor cursor = comp.getCursor();
        while (cursor == null && comp != window) {
            comp = comp.getParent();
            if (comp == null) {
                cursor = window.getCursor();
                break;
            }
            cursor = comp.getCursor();
        }
        if (cursor != lightCursor) {
            lightCursor = cursor;
            window.changeCursor(lightCursor);
        }
    }
    // --- member variables -------------------------------
	
    /**
     * The windowed container that might be hosting events for
     * lightweight components.
     */
    private Window window;
    /**
     * The current lightweight component that has focus that is being
     * hosted by this container.  If this is a null reference then
     * there is currently no focus on a lightweight component being
     * hosted by this container
     */
    private Component focus;
    /**
     * The current lightweight component being hosted by this windowed
     * component that has mouse events being forwarded to it.  If this
     * is null, there are currently no mouse events being forwarded to
     * a lightweight component.
     */
    private transient Component mouseEventTarget;
    /**
     *  lightweight component the mouse cursor is on
     */
    private transient Component cursorOn;
    /**
     * The last component entered
     */
    private transient Component targetLastEntered;
    /**
     * Is the mouse over the native container
     */
    private transient boolean isMouseInNativeContainer = false;
    /**
     * Is the next click event orphaned because the component hid/moved
     */
    private transient boolean isClickOrphaned = false;
    /**
     * Indicates if the mouse pointer is currently being dragged...
     * this is needed because we may receive exit events while dragging
     * and need to keep the current mouse target in this case.
     */
    private boolean dragging;
    /**
     * The cursor that is currently displayed for the lightwieght
     * components.  Remember this cursor, so we do not need to
     * change cursor on every mouse event.
     */
    private Cursor lightCursor;

    private boolean appContextEquals(Component origTarget, Component newTarget) {
       if (origTarget == null || newTarget == null) {
          return true;
       }
       return (origTarget.appContext.getThreadGroup() == newTarget.appContext.getThreadGroup());
    }
}
