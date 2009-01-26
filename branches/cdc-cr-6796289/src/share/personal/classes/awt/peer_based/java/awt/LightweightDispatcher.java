/*
 * @(#)LightweightDispatcher.java	1.15 06/10/10
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
import sun.awt.peer.*;

/**
 * Class to manage the dispatching of events to the lightweight
 * components contained by a native container.
 *
 * @author Timothy Prinzing
 */
class LightweightDispatcher implements java.io.Serializable,
        AWTEventListener {
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 5184291520170872969L;
    /*
     * Our own mouse event for when we're dragged over from another hw container
     */
    private static final int  LWD_MOUSE_DRAGGED_OVER = AWTEvent.RESERVED_ID_MAX + 1;
    LightweightDispatcher(Container nativeContainer) {
        this.nativeContainer = nativeContainer;
        mouseEventTarget = null;
        eventMask = 0;
    }

    /*
     * Disposes any external resources allocated by the dispatcher
     */
    void dispose() {
        stopListeningForOtherDrags();
    }

    /**
     * Enables events to lightweight components.
     */
    void enableEvents(long events) {
        eventMask |= events;
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
        if ((eventMask & PROXY_EVENT_MASK) != 0) {
            if ((e instanceof MouseEvent) &&
                ((eventMask & MOUSE_MASK) != 0)) {
                MouseEvent me = (MouseEvent) e;
                ret = processMouseEvent(me);
            }
        }
        if (e instanceof MouseEvent) {
            // find out what component the mouse moved in
            MouseEvent me = (MouseEvent) e;
            if (me.getID() == MouseEvent.MOUSE_MOVED) {
                cursorOn = nativeContainer.getCursorTarget(me.getX(), me.getY());
                // 6201639
                // cursorOn can be null in the following cases
                // 1) mouse position is on the nativeContainer
                // 2) mouse position is on a heavyweight
                // The updateCursor() method always treats a null argument
                // as the nativeContainer, so we need to make this non-null
                // check. 
                // (See the related fix in Container.getCursorTarget())
                if ( cursorOn != null ) {
                // 6201639
                    updateCursor(cursorOn);
                }
            }
        }
        return ret;
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
        targetOver = nativeContainer.getMouseEventTarget(e.getX(), e.getY(), true);
        trackMouseEnterExit(targetOver, e);

        if (id == MouseEvent.MOUSE_MOVED ||
            id == MouseEvent.MOUSE_PRESSED ||
            id == MouseEvent.MOUSE_RELEASED ) {
            lwOver = (targetOver != nativeContainer) ? targetOver : null;
            setMouseTarget(lwOver, e);
        }
        if (mouseEventTarget != null) {
            // we are currently forwarding to some component, check
            // to see if we should continue to forward.
            switch (id) {
            case MouseEvent.MOUSE_DRAGGED:
                if (dragging) {
                    retargetMouseEvent(mouseEventTarget, id, e);
                }
                break;

            case MouseEvent.MOUSE_PRESSED:
                dragging = true;
                retargetMouseEvent(mouseEventTarget, id, e);
                break;

            case MouseEvent.MOUSE_RELEASED:
                dragging = false;
                retargetMouseEvent(mouseEventTarget, id, e);
                break;

            // MOUSE_CLICKED should never be dispatched to a Component
            // other than that which received the MOUSE_PRESSED event.  If the
            // mouse is now over a different Component, don't dispatch the event.
            // The previous fix for a similar problem was associated with bug
            // 4155217.        
            case MouseEvent.MOUSE_CLICKED:
                if (targetOver == mouseEventTarget) {
                    retargetMouseEvent(mouseEventTarget, id, e);
                }
                break;

            case MouseEvent.MOUSE_ENTERED:
                break;

            case MouseEvent.MOUSE_EXITED:
                if (!dragging) {
                    setMouseTarget(null, e);
                }
                break;

            case MouseEvent.MOUSE_MOVED:
                retargetMouseEvent(mouseEventTarget, id, e);
                break;
            }
            e.consume();
        }
        return e.isConsumed();
    }

    /**
     * Change the current target of mouse events.
     */
    private void setMouseTarget(Component target, MouseEvent e) {
        if (target != mouseEventTarget) {
            //System.out.println("setMouseTarget: " + target);
            mouseEventTarget = target;
        }
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
            id != LWD_MOUSE_DRAGGED_OVER &&
            isMouseInNativeContainer == false) {
            // any event but an exit or drag means we're in the native container
            isMouseInNativeContainer = true;
            startListeningForOtherDrags();
        } else if (id == MouseEvent.MOUSE_EXITED) {
            isMouseInNativeContainer = false;
            stopListeningForOtherDrags();
        }
        if (isMouseInNativeContainer) {
            targetEnter = targetOver;
        }
        //System.out.println("targetEnter = " + targetEnter);
        //System.out.println("targetLastEntered = " + targetLastEntered);

        if (targetLastEntered == targetEnter) {
            return;
        }
        retargetMouseEvent(targetLastEntered, MouseEvent.MOUSE_EXITED, e);
        if (id == MouseEvent.MOUSE_EXITED) {
            // consume native exit event if we generate one
            e.consume();
        }
        retargetMouseEvent(targetEnter, MouseEvent.MOUSE_ENTERED, e);
        if (id == MouseEvent.MOUSE_ENTERED) {
            // consume native enter event if we generate one
            e.consume();
        }
        //System.out.println("targetLastEntered: " + targetLastEntered);
        targetLastEntered = targetEnter;
    }

    private void startListeningForOtherDrags() {
        java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction() {
                public Object run() {
                    nativeContainer.getToolkit().addAWTEventListener(LightweightDispatcher.this, AWTEvent.MOUSE_EVENT_MASK |
                        AWTEvent.MOUSE_MOTION_EVENT_MASK);
                    return null;
                }
            }
        );
    }

    private void stopListeningForOtherDrags() {
        java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction() {
                public Object run() {
                    nativeContainer.getToolkit().removeAWTEventListener(LightweightDispatcher.this);
                    return null;
                }
            }
        );
        // removed any queued up dragged-over events
        Toolkit.getEventQueue().removeEvents(MouseEvent.class, LWD_MOUSE_DRAGGED_OVER);
    }

    /*
     * (Implementation of AWTEventListener)
     * Listen for drag events posted in other hw components so we can
     * track enter/exit regardless of where a drag originated
     */
    public void eventDispatched(AWTEvent e) {
        boolean isForeignDrag = (e instanceof MouseEvent) &&
            (e.id == MouseEvent.MOUSE_DRAGGED) &&
            (e.getSource() != nativeContainer);
        if (!isForeignDrag) {
            // only interested in drags from other hw components
            return;
        }
        // execute trackMouseEnterExit on EventDispatchThread
        Toolkit.getEventQueue().postEvent(
            new TrackEnterExitEvent(nativeContainer, (MouseEvent) e)
        );
    }
    /*
     * ActiveEvent that calls trackMouseEnterExit as a result of a drag
     * originating in a 'foreign' hw container. Normally, we'd only be
     * able to track mouse events in our own hw container.
     */
    private class TrackEnterExitEvent extends AWTEvent implements ActiveEvent {
        MouseEvent	srcEvent;
        public TrackEnterExitEvent(Component trackSrc, MouseEvent e) {
            super(trackSrc, 0);
            srcEvent = e;
        }

        public void dispatch() {
            MouseEvent	me;
            synchronized (nativeContainer.getTreeLock()) {
                Component srcComponent = srcEvent.getComponent();
                // component may have disappeared since drag event posted
                // (i.e. Swing hierarchical menus)
                if (!srcComponent.isShowing() ||
                    !nativeContainer.isShowing()) {
                    return;
                }
                //
                // create an internal 'dragged-over' event indicating
                // we are being dragged over from another hw component
                //
                me = new MouseEvent(nativeContainer,
                            LWD_MOUSE_DRAGGED_OVER,
                            srcEvent.getWhen(),
                            srcEvent.getModifiers(),
                            srcEvent.getX(),
                            srcEvent.getY(),
                            srcEvent.getClickCount(),
                            srcEvent.isPopupTrigger());
                // translate coordinates to this native container
                Point	ptSrcOrigin = srcComponent.getLocationOnScreen();
                Point	ptDstOrigin = nativeContainer.getLocationOnScreen();
                me.translatePoint(ptSrcOrigin.x - ptDstOrigin.x, ptSrcOrigin.y - ptDstOrigin.y);
            }
            //System.out.println("Track event: " + me);
            // feed the 'dragged-over' event directly to the enter/exit
            // code (not a real event so don't pass it to dispatchEvent)
            Component targetOver = nativeContainer.getMouseEventTarget(me.getX(), me.getY(), true);
            trackMouseEnterExit(targetOver, me);
        }
    }
    /**
     * Sends a mouse event to the current mouse event recipient using
     * the given event (sent to the windowed host) as a srcEvent.  If
     * the mouse event target is still in the component tree, the
     * coordinates of the event are translated to those of the target.
     * If the target has been removed, we don't bother to send the
     * message.
     */
    void retargetMouseEvent(Component target, int id, MouseEvent e) {
        if (target == null) {
            return; // mouse is over another hw component
        }
        int x = e.getX(), y = e.getY();
        Component component;
        for (component = target;
            component != null && component != nativeContainer;
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
            if (target == nativeContainer) {
                // avoid recursively calling LightweightDispatcher...
                ((Container) target).dispatchEventToSelf(retargeted);
            } else {
                target.dispatchEvent(retargeted);
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
        if (comp != cursorOn) {
            return;
        }

        if (comp == null) {
            comp = nativeContainer;
        }
        Cursor cursor = comp.getCursor();
        while (cursor == null && comp != nativeContainer) {
            comp = comp.getParent();
            if (comp == null) {
                cursor = nativeContainer.getCursor();
                break;
            }
            cursor = comp.getCursor();
        }
        if (cursor != lightCursor) {
            lightCursor = cursor;
            // Only change the cursor on the peer, because we want client code to think
            // that the Container's cursor only changes in response to setCursor calls.
            ComponentPeer ncPeer = nativeContainer.peer;
            if (ncPeer != null) {
                ncPeer.setCursor(cursor);
            }
        }
    }

    /**
     * get the lightweight component mouse cursor is on
     * null means the nativeContainer
     */
    Component getCursorOn() {
        return cursorOn;
    }
    // --- member variables -------------------------------

    /**
     * The windowed container that might be hosting events for
     * lightweight components.
     */
    private Container nativeContainer;
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
    /**
     * The event mask for contained lightweight components.  Lightweight
     * components need a windowed container to host window-related
     * events.  This seperate mask indicates events that have been
     * requested by contained lightweight components without effecting
     * the mask of the windowed component itself.
     */
    private long eventMask;
    /**
     * The kind of events routed to lightweight components from windowed
     * hosts.
     */
    private static final long PROXY_EVENT_MASK =
        AWTEvent.FOCUS_EVENT_MASK |
        AWTEvent.KEY_EVENT_MASK |
        AWTEvent.MOUSE_EVENT_MASK |
        AWTEvent.MOUSE_MOTION_EVENT_MASK;
    private static final long MOUSE_MASK =
        AWTEvent.MOUSE_EVENT_MASK | AWTEvent.MOUSE_MOTION_EVENT_MASK;
}
