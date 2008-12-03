/*
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

import java.awt.event.FocusEvent;
import java.awt.event.KeyEvent;
import java.awt.event.WindowEvent;
import java.beans.PropertyChangeListener;
import java.util.LinkedList;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.Set;

import sun.awt.AppContext;
import sun.awt.SunToolkit;

class DefaultKeyboardFocusManager extends KeyboardFocusManager {
    private int inSendMessage;
    private boolean consumeNextKeyTyped;
    private Window mostRecentFocusedWindow;
    private Window pendingWindow;
    private Component pendingComponent;
    private LinkedList enqueuedKeyEvents = new LinkedList();
    private LinkedList typeAheadMarkers = new LinkedList();
    private boolean doDebug=false;

    // added to support vetoable changes
    private Window realOppositeWindow;
    private Component realOppositeComponent;


    private static class TypeAheadMarker {
        long after;
        Component untilFocused;
        TypeAheadMarker(long after, Component untilFocused) {
            this.after = after;
            this.untilFocused = untilFocused;
        }
    }

    /*
     * This series of restoreFocus methods is used for recovering from a
     * rejected focus or activation change. Rejections typically occur when
     * the user attempts to focus a non-focusable Component or Window.
     */
    private void restoreFocus(FocusEvent fe, Window newFocusedWindow) {
        Component realOppositeComponent = this.realOppositeComponent;
        Component vetoedComponent = fe.getComponent();
        if (newFocusedWindow != null && restoreFocus(newFocusedWindow,
                                                     vetoedComponent, false))
        {
        } else if (realOppositeComponent != null &&
                   restoreFocus(realOppositeComponent, false)) {
        } else if (fe.getOppositeComponent() != null &&
                   restoreFocus(fe.getOppositeComponent(), false)) {
        } else {
            clearGlobalFocusOwner();
        }
    }

    private void restoreFocus(WindowEvent we) {
        Window realOppositeWindow = this.realOppositeWindow;
        if (realOppositeWindow != null && restoreFocus(realOppositeWindow,
                                                       null, false)) {
        } else if (we.getOppositeWindow() != null &&
                   restoreFocus(we.getOppositeWindow(), null, false)) {
        } else {
            clearGlobalFocusOwner();
        }
    }

    private boolean restoreFocus(Window aWindow, Component vetoedComponent,
                                 boolean clearOnFailure) {
        Component toFocus =
            KeyboardFocusManager.getMostRecentFocusOwner(aWindow);
        if (toFocus != null && toFocus != vetoedComponent && restoreFocus(toFocus, false)) {
            return true;
        } else if (clearOnFailure) {
            clearGlobalFocusOwner();
            return true;
        } else {
            return false;
        }
    }

    private boolean restoreFocus(Component toFocus, boolean clearOnFailure) {
        if (doDebug)
            System.out.println("     restoreFocus to " + toFocus);
                                                                              
        // 6198823: vetoableChangeListener not working as expected when trying
        // to veto a FOCUS_LOST event.
        if (toFocus.hasFocus()) {
            return true;
        }
        // 6198823: vetoableChangeListener not working as expected when trying
        // to veto a FOCUS_LOST event.
        //
        // The new focus management system puts a check at the beginning of
        // the Component.requestFocusHelper() method to see whether the
        // component hasFocus(), if true, just returns false without going
        // through the steps of asking peer to request the focus.  This has
        // the implication that if the clearGlobalFocusOwner() call was vetoed
        // by a vetoable change listener and the logic flow falls back to the
        // restoreFocus() call, after trying the first if branch and then the
        // second if branch without success, calling the third branch will 
        // lead to an infinite recursion leading to stack overflow.

        if (toFocus.isShowing() && toFocus.isFocusable() &&
            toFocus.requestFocus(false)) {
            return true;
        } else if (toFocus.nextFocusHelper()) {
            return true;
        } else if (clearOnFailure) {
            clearGlobalFocusOwner();
            return true;
        } else {
            return false;
        }
    }

    
    private Window getOwningFrame(Window window) {
        while (window != null && !(window instanceof Frame)) {
            window = (Window)window.getParent();
        }
        return window;
    }

    /**
     * A special type of SentEvent which updates a counter in the target
     * KeyboardFocusManager if it is an instance of
     * DefaultKeyboardFocusManager.
     */
    private static class DefaultKeyboardFocusManagerSentEvent
        extends SentEvent {
        public DefaultKeyboardFocusManagerSentEvent(AWTEvent nested,
                                                    AppContext toNotify) {
            super(nested, toNotify);
        }
        public final void dispatch() {
            KeyboardFocusManager manager =
                KeyboardFocusManager.getCurrentKeyboardFocusManager();
            DefaultKeyboardFocusManager defaultManager =
                (manager instanceof DefaultKeyboardFocusManager)
                ? (DefaultKeyboardFocusManager)manager
                : null;
            if (defaultManager != null) {
                synchronized (defaultManager) {
                    defaultManager.inSendMessage++;
                }
            }
            super.dispatch();
            if (defaultManager != null) {
                synchronized (defaultManager) {
                    defaultManager.inSendMessage--;
                }
            }
        }
    }

    /**
     * Sends a synthetic AWTEvent to a Component. If the Component is in
     * the current AppContext, then the event is immediately dispatched.
     * If the Component is in a different AppContext, then the event is
     * posted to the other AppContext's EventQueue, and this method blocks
     * until the event is handled or target AppContext is disposed.
     * Returns true if successfuly dispatched event, false if failed
     * to dispatch.
     */
    boolean sendMessage(Component target, AWTEvent e) {
        AppContext myAppContext = AppContext.getAppContext();
        final SentEvent se =
            new DefaultKeyboardFocusManagerSentEvent(e, myAppContext);
        return sendMessage(target.appContext,se);
    }

    public boolean dispatchEvent(AWTEvent e) {
        if (!(e instanceof WindowEvent) && 
            !(e instanceof FocusEvent) &&
            !(e instanceof KeyEvent)) {
            return false;
        }

        switch( e.getID() ) {
        case WindowEvent.WINDOW_GAINED_FOCUS: {
            WindowEvent we = (WindowEvent) e;
            Window oldFocusedWindow = getGlobalFocusedWindow();
            Window newFocusedWindow = we.getWindow();
            Component oldFocusOwner = getGlobalFocusOwner();
            if (newFocusedWindow == oldFocusedWindow) {
                break;
            }

            // If there exists a current focused window, then notify it
            // that it has lost focus.
            if (oldFocusedWindow != null) {
                boolean isEventDispatched = sendMessage(oldFocusedWindow,
                        new WindowEvent(oldFocusedWindow,
                                        WindowEvent.WINDOW_LOST_FOCUS,
                                        newFocusedWindow));
                // Failed to dispatch, clear by ourselfves
                if (!isEventDispatched) {
                    setGlobalFocusOwner(null);
                    setGlobalFocusedWindow(null);
                }
            }

            // Because the native libraries do not post WINDOW_ACTIVATED
            // events, we need to synthesize one if the active Window
            // changed.
            Window newActiveWindow = getOwningFrame(newFocusedWindow);
            Window currentActiveWindow = getGlobalActiveWindow();
            if (newActiveWindow != currentActiveWindow) {
                sendMessage(newActiveWindow,
                            new WindowEvent(newActiveWindow,
                                            WindowEvent.WINDOW_ACTIVATED,
                                            currentActiveWindow));
                if (newActiveWindow != getGlobalActiveWindow()) {
                    // Activation change was rejected. Unlikely, but
                    // possible.
                    restoreFocus(we);
                    break;
                }
            }

            setGlobalFocusedWindow(newFocusedWindow);

            if (newFocusedWindow != getGlobalFocusedWindow()) {
                // Focus change was rejected. Will happen if
                // newFocusedWindow is not a focusable Window.
                restoreFocus(we);
                break;
            }

            mostRecentFocusedWindow = newFocusedWindow; // Pbp

            if ( inSendMessage == 0 ) {
                Component toFocus = 
                KeyboardFocusManager.getMostRecentFocusOwner(newFocusedWindow);
                if (toFocus == null && newFocusedWindow.isFocusableWindow()) {
                    toFocus = newFocusedWindow.getFocusTraversalPolicy().
                        getInitialComponent(newFocusedWindow);
                }
                if (toFocus != null) {
                    toFocus.requestFocusInWindow();
                }
            }

            // Does not apply as of now, since we support only one Frame
// J2SE
//            Window realOppositeWindow = this.realOppositeWindow;
//             if (realOppositeWindow != we.getOppositeWindow()) {
//                 we = new WindowEvent(newFocusedWindow,
//                                      WindowEvent.WINDOW_GAINED_FOCUS,
//                                      realOppositeWindow);
//             }
// J2SE

            return typeAheadAssertions(newFocusedWindow, we);
        }

        case WindowEvent.WINDOW_ACTIVATED: {
            WindowEvent we = (WindowEvent)e;
            Window oldActiveWindow = getGlobalActiveWindow();
            Window newActiveWindow = we.getWindow();
            if (oldActiveWindow == newActiveWindow) {
                break;
            }
            // If there exists a current active window, then notify it that
            // it has lost activation.
            if (oldActiveWindow != null) {
                boolean isEventDispatched =
                    sendMessage(oldActiveWindow,
                                new WindowEvent(oldActiveWindow,
                                    WindowEvent.WINDOW_DEACTIVATED,
                                                newActiveWindow));
                // Failed to dispatch, clear by ourselfves
                if (!isEventDispatched) {
                    setGlobalActiveWindow(null);
                }
                if (getGlobalActiveWindow() != null) {
                    // Activation change was rejected. Unlikely, but
                    // possible.
                    break;
                }
            }
            setGlobalActiveWindow(newActiveWindow);
            if (newActiveWindow != getGlobalActiveWindow()) {
                // Activation change was rejected. Unlikely, but
                // possible.             
                break;
            }
            return typeAheadAssertions(newActiveWindow, we);
        }

        case FocusEvent.FOCUS_GAINED: {
            FocusEvent fe = (FocusEvent)e;
            Component oldFocusOwner = getGlobalFocusOwner();
            Component newFocusOwner = fe.getComponent();
            if (oldFocusOwner == newFocusOwner) {
                break;
            }

            // ignore the event if the component is not displayable.
            // Component.removeNotify() could have been called after 
            // requesting focus on the component.
            if ( !newFocusOwner.isDisplayable() ) {
                break;
            }

            // If there exists a current focus owner, then notify it that
            // it has lost focus.
            if (oldFocusOwner != null) {
                boolean isEventDispatched = sendMessage(oldFocusOwner,
                                     new FocusEvent(oldFocusOwner,
                                                    FocusEvent.FOCUS_LOST,
                                                    fe.isTemporary(),
                                                    newFocusOwner));
                // Failed to dispatch, clear by ourselfves
                if (!isEventDispatched) {
                    setGlobalFocusOwner(null);
                    if (!fe.isTemporary()) {
                        setGlobalPermanentFocusOwner(null);
                    }
                }
            }
            
            
            // Because the native windowing system has a different notion
            // of the current focus and activation states, it is possible
            // that a Component outside of the focused Window receives a
            // FOCUS_GAINED event. We synthesize a WINDOW_GAINED_FOCUS
            // event in that case.
            Component newFocusedWindow = newFocusOwner;
            while (newFocusedWindow != null &&
                   !(newFocusedWindow instanceof Window)) {
                newFocusedWindow = newFocusedWindow.parent;
            }
            Window currentFocusedWindow = getGlobalFocusedWindow();
            if (newFocusedWindow != null &&
                newFocusedWindow != currentFocusedWindow) {
                sendMessage(newFocusedWindow,
                            new WindowEvent((Window)newFocusedWindow,
                                            WindowEvent.WINDOW_GAINED_FOCUS,
                                            currentFocusedWindow));
                if (newFocusedWindow != getGlobalFocusedWindow()) {
                    // Focus change was rejected. Will happen if
                    // newFocusedWindow is not a focusable Window.
                    
                    // Need to recover type-ahead, but don't bother
                    // restoring focus. That was done by the
                    // WINDOW_GAINED_FOCUS handler
                    dequeueKeyEvents(-1, newFocusOwner);
                    break;
                }
            }

            setGlobalFocusOwner(newFocusOwner);

            if (newFocusOwner != getGlobalFocusOwner()) {
                // Focus change was rejected. Will happen if
                // newFocusOwner is not focus traversable.
                dequeueKeyEvents(-1, newFocusOwner);
                restoreFocus(fe, (Window)newFocusedWindow);
                break;
            }

            if (!fe.isTemporary()) {
                setGlobalPermanentFocusOwner(newFocusOwner);
                if (newFocusOwner != getGlobalPermanentFocusOwner()) {
                    // Focus change was rejected. Unlikely, but possible.
                    dequeueKeyEvents(-1, newFocusOwner);
                    restoreFocus(fe, (Window)newFocusedWindow);
                    break;
                }
            }

            Component realOppositeComponent = this.realOppositeComponent;
            if (realOppositeComponent != null &&
                realOppositeComponent != fe.getOppositeComponent()) {
                fe = new FocusEvent(newFocusOwner,
                                    FocusEvent.FOCUS_GAINED,
                                    fe.isTemporary(),
                                    realOppositeComponent);
            }
            return typeAheadAssertions(newFocusOwner, fe);
        }

	    case FocusEvent.FOCUS_LOST: {
	        FocusEvent fe = (FocusEvent)e;
            Component currentFocusOwner = getGlobalFocusOwner();
            if (currentFocusOwner == null) {
                break;
            }
            // Ignore cases where a Component loses focus to itself.
            // If we make a mistake because of retargeting, then the
            // FOCUS_GAINED handler will correct it.
            if (currentFocusOwner == fe.getOppositeComponent()) {
                break;
            }

            setGlobalFocusOwner(null);

            if (getGlobalFocusOwner() != null) {
                // Focus change was rejected. Unlikely, but possible.
                restoreFocus(currentFocusOwner, true);
                break;
            }

            if (!fe.isTemporary()) {                    
                setGlobalPermanentFocusOwner(null);

                if (getGlobalPermanentFocusOwner() != null) {
                    // Focus change was rejected. Unlikely, but possible.
                    restoreFocus(currentFocusOwner, true);
                    break;
                }
            } 
            // We dont handle temporary focus changes
// J2SE
//            else {
//                 Window owningWindow = getContainingWindow(currentFocusOwner);
//                 if (owningWindow != null) {
//                     owningWindow.setTemporaryLostComponent(currentFocusOwner);
//                 }
//             }
// J2SE
		
            // 6238261 - now that AWTEvent.setSource is package protected,
            // need to cast focus event as AWTEvent so we're in the same
            // package before calling setSource
            ((AWTEvent)fe).setSource(currentFocusOwner);

            realOppositeComponent = (fe.getOppositeComponent() != null)
                ? currentFocusOwner : null;

            return typeAheadAssertions(currentFocusOwner, fe);
	    }

	    case WindowEvent.WINDOW_DEACTIVATED: {
	        WindowEvent we = (WindowEvent)e;
            Window currentActiveWindow = getGlobalActiveWindow();
            if (currentActiveWindow == null) {
                break;
            }
            
            setGlobalActiveWindow(null);
            if (getGlobalActiveWindow() != null) {
                // Activation change was rejected. Unlikely, but possible.
                break;
            }

            // 6238261 - now that AWTEvent.setSource is package protected,
            // need to cast focus event as AWTEvent so we're in the same
            // package before calling setSource
            ((AWTEvent)we).setSource(currentActiveWindow);
            return typeAheadAssertions(currentActiveWindow, we);
	    }

	    case WindowEvent.WINDOW_LOST_FOCUS: {
	        WindowEvent we = (WindowEvent)e;
            Window currentFocusedWindow = getGlobalFocusedWindow();
            if (currentFocusedWindow == null) {
                break;
            }
            
	        // Special case -- if the native windowing system posts an
            // event claiming that the active Window has lost focus to the
            // focused Window, then discard the event. This is an artifact
            // of the native windowing system not knowing which Window is
            // really focused.
            Window losingFocusWindow = we.getWindow();
            Window activeWindow = getGlobalActiveWindow();
            Window oppositeWindow = we.getOppositeWindow();
            if (inSendMessage == 0 && losingFocusWindow == activeWindow &&
                oppositeWindow == currentFocusedWindow) {
                break;
            }

            Component currentFocusOwner = getGlobalFocusOwner();
            if (currentFocusOwner != null) {
                // The focus owner should always receive a FOCUS_LOST event
                // before the Window is defocused.
                Component oppositeComp = null;
                if (oppositeWindow != null) {
                    oppositeComp = oppositeWindow.getTemporaryLostComponent();
                    if (oppositeComp == null) {
                        oppositeComp = 
                            oppositeWindow.getMostRecentFocusOwner();
                    }
                }
                if (oppositeComp == null) {
                    oppositeComp = oppositeWindow;
                }
                sendMessage(currentFocusOwner,
                            new FocusEvent(currentFocusOwner,
                                           FocusEvent.FOCUS_LOST,
                                           true,
                                           oppositeComp));
            }

            setGlobalFocusedWindow(null);
            if (getGlobalFocusedWindow() != null) {
                // Focus change was rejected. Unlikely, but possible.
                restoreFocus(currentFocusedWindow, null, true);
                break;
            }

            // 6238261 - now that AWTEvent.setSource is package protected,
            // need to cast focus event as AWTEvent so we're in the same
            // package before calling setSource
            ((AWTEvent)we).setSource(currentFocusedWindow);
            realOppositeWindow = (oppositeWindow != null)
                ? currentFocusedWindow
                : null;
            typeAheadAssertions(currentFocusedWindow, we);

            if (oppositeWindow == null) {
                // Then we need to deactive the active Window as well.
                // No need to synthesize in other cases, because
                // WINDOW_ACTIVATED will handle it if necessary.
                sendMessage(activeWindow,
                            new WindowEvent(activeWindow,
                                            WindowEvent.WINDOW_DEACTIVATED,
                                            null));
                if (getGlobalActiveWindow() != null) {
                    // Activation change was rejected. Unlikely,
                    // but possible.
                    restoreFocus(currentFocusedWindow, null, true);
                }
            }
            break;
	    }

        case KeyEvent.KEY_TYPED:
        case KeyEvent.KEY_PRESSED:
        case KeyEvent.KEY_RELEASED: {
            return typeAheadAssertions(null, e);
        }

        default :
            return false;
        }

        return true;
    }

    public boolean dispatchKeyEvent(KeyEvent e) {
        Component focusOwner = getFocusOwner();
        if (focusOwner != null && focusOwner.isShowing()
                && focusOwner.isFocusable() && focusOwner.isEnabled()) {
            if (!e.isConsumed()) {
                Component comp = e.getComponent();
                if (comp != null && comp.isEnabled()) {
                    redispatchEvent(comp, e);
                }
            }
        }
        boolean stopPostProcessing = false;
        java.util.List processors = getKeyEventPostProcessors();
        if (processors != null) {
            for (java.util.Iterator iter = processors.iterator();
                    !stopPostProcessing && iter.hasNext();) {
                stopPostProcessing = (((KeyEventPostProcessor) (iter.next())).postProcessKeyEvent(e));
            }
        }
        if (!stopPostProcessing) {
            postProcessKeyEvent(e);
        }
        return true;
    }

    public boolean postProcessKeyEvent(KeyEvent e) {
        if (!e.isConsumed()) {
            Component target = e.getComponent();
            if (target != null) {
                Container p = (Container)
                    (target instanceof Container ? target : target.getParent());
                if (p != null) {
                    p.postProcessKeyEvent(e);
                }
            }
        }
        return true;
    }

    private void pumpApprovedKeyEvents() {
        KeyEvent ke;
        do {
            ke = null;
            synchronized (this) {
                if (enqueuedKeyEvents.size() != 0) {
                    ke = (KeyEvent) enqueuedKeyEvents.getFirst();
                    if (typeAheadMarkers.size() != 0) {
                        TypeAheadMarker marker = (TypeAheadMarker)
                                typeAheadMarkers.getFirst();
                        if (ke.getWhen() > marker.after) {
                            ke = null;
                        }
                    }
                    if (ke != null) {
                        enqueuedKeyEvents.removeFirst();
                    }
                }
            }
            if (ke != null) {
                preDispatchKeyEvent(ke);
            }
        } while (ke != null);
    }

    private boolean typeAheadAssertions(Component target, AWTEvent e) {
        // Clear any pending events here as well as in the FOCUS_GAINED
        // handler. We need this call here in case a marker was removed in
        // response to a call to dequeueKeyEvents.
        pumpApprovedKeyEvents();
        switch (e.getID()) {
        case KeyEvent.KEY_TYPED:
        case KeyEvent.KEY_PRESSED:
        case KeyEvent.KEY_RELEASED: {
            KeyEvent ke = (KeyEvent) e;
            synchronized (this) {
                if (typeAheadMarkers.size() != 0) {
                    TypeAheadMarker marker = (TypeAheadMarker)
                            typeAheadMarkers.getFirst();
                    if (ke.getWhen() > marker.after) {
                        enqueuedKeyEvents.addLast(ke);
                        return true;
                    }
                }
            }
            // KeyEvent was posted before focus change request
            return preDispatchKeyEvent(ke);
        }
        case FocusEvent.FOCUS_GAINED:
            // Search the marker list for the first marker tied to the
            // Component which just gained focus. Then remove that marker
            // and any markers which immediately follow and are tied to
            // the same Component. This handles the case where multiple
            // focus requests were made for the same Component in a row.
            // Since FOCUS_GAINED events will not be generated for these
            // additional requests, we need to clear those markers too.
            synchronized (this) {
                boolean found = false;
                for (Iterator iter = typeAheadMarkers.iterator();
                        iter.hasNext();) {
                    if (((TypeAheadMarker) iter.next()).untilFocused == target) {
                        iter.remove();
                        found = true;
                    } else if (found) {
                        break;
                    }
                }			
            }
            redispatchEvent(target, e);
            // Now, dispatch any pending KeyEvents which have been
            // released because of the FOCUS_GAINED event so that we don't
            // have to wait for another event to be posted to the queue.
            pumpApprovedKeyEvents();
            return true;
        default:
            redispatchEvent(target, e);
            return true;
        }
    }

    private boolean preDispatchKeyEvent(KeyEvent ke) {
        Component focusOwner = getFocusOwner();
        ke.setSource(((focusOwner != null) ? focusOwner : getFocusedWindow()));
        if (ke.getSource() == null) {
            return true;
        }
        EventQueue.setCurrentEventAndMostRecentTime(ke);
        java.util.List dispatchers = getKeyEventDispatchers();
        if (dispatchers != null) {
            for (java.util.Iterator iter = dispatchers.iterator();
                    iter.hasNext();) {
                if (((KeyEventDispatcher) (iter.next())).dispatchKeyEvent(ke)) {
                    return true;
                }
            }
        }
        return dispatchKeyEvent(ke);
    }

    public void processKeyEvent(Component focusedComponent, KeyEvent e) {
        if (e.getID() == KeyEvent.KEY_TYPED) {
            if (consumeNextKeyTyped) {
                e.consume();
                consumeNextKeyTyped = false;
            }
            return;
        }
        if (focusedComponent.getFocusTraversalKeysEnabled() && !e.isConsumed()) {
            AWTKeyStroke stroke = AWTKeyStroke.getAWTKeyStrokeForEvent(e),
                    oppStroke = AWTKeyStroke.getAWTKeyStroke(stroke.getKeyCode(),
                    stroke.getModifiers(), !stroke.isOnKeyRelease());
            Set toTest;
            boolean contains, containsOpp;
            toTest = focusedComponent.getFocusTraversalKeys(KeyboardFocusManager.FORWARD_TRAVERSAL_KEYS);
            contains = toTest.contains(stroke);
            containsOpp = toTest.contains(oppStroke);
            if (contains || containsOpp) {
                if (contains) {
                    focusNextComponent(focusedComponent);
                }
                e.consume();
                consumeNextKeyTyped = (e.getID() == KeyEvent.KEY_PRESSED);
                return;
            }
            toTest = focusedComponent.getFocusTraversalKeys(KeyboardFocusManager.BACKWARD_TRAVERSAL_KEYS);
            contains = toTest.contains(stroke);
            containsOpp = toTest.contains(oppStroke);
            if (contains || containsOpp) {
                if (contains) {
                    focusPreviousComponent(focusedComponent);
                }
                e.consume();
                consumeNextKeyTyped = (e.getID() == KeyEvent.KEY_PRESSED);
                return;
            }
            toTest = focusedComponent.getFocusTraversalKeys(KeyboardFocusManager.UP_CYCLE_TRAVERSAL_KEYS);
            contains = toTest.contains(stroke);
            containsOpp = toTest.contains(oppStroke);
            if (contains || containsOpp) {
                if (contains) {
                    upFocusCycle(focusedComponent);
                }
                e.consume();
                consumeNextKeyTyped = (e.getID() == KeyEvent.KEY_PRESSED);
                return;
            }
            if (!((focusedComponent instanceof Container)
                    && ((Container) focusedComponent).isFocusCycleRoot())) {
                return;
            }
            toTest = focusedComponent.getFocusTraversalKeys(KeyboardFocusManager.DOWN_CYCLE_TRAVERSAL_KEYS);
            contains = toTest.contains(stroke);
            containsOpp = toTest.contains(oppStroke);
            if (contains || containsOpp) {
                if (contains) {
                    downFocusCycle((Container) focusedComponent);
                }
                e.consume();
                consumeNextKeyTyped = (e.getID() == KeyEvent.KEY_PRESSED);
            }
        }
    }

    protected synchronized void enqueueKeyEvents(long after,
            Component untilFocused) {
        if (untilFocused == null) {
            return;
        }
        int insertionIndex = 0,
                i = typeAheadMarkers.size();
        ListIterator iter = typeAheadMarkers.listIterator(i);
        for (; i > 0; i--) {
            TypeAheadMarker marker = (TypeAheadMarker) iter.previous();
            if (marker.after <= after) {
                insertionIndex = i;
                break;
            }
        }
        typeAheadMarkers.add(insertionIndex, new TypeAheadMarker(after, untilFocused));
    }

    protected synchronized void dequeueKeyEvents(long after,
            Component untilFocused) {
        if (untilFocused == null) {
            return;
        }
        TypeAheadMarker marker;
        ListIterator iter = typeAheadMarkers.listIterator
                ((after >= 0) ? typeAheadMarkers.size() : 0);
        if (after < 0) {
            while (iter.hasNext()) {
                marker = (TypeAheadMarker) iter.next();
                if (marker.untilFocused == untilFocused) {
                    iter.remove();
                    return;
                }
            }
        } else {
            while (iter.hasPrevious()) {
                marker = (TypeAheadMarker) iter.previous();
                if (marker.untilFocused == untilFocused && marker.after == after) {
                    iter.remove();
                    return;
                }
            }
        }
    }

    protected synchronized void discardKeyEvents(Component comp) {
        if (comp == null) {
            return;
        }
        long start = -1;
        for (Iterator iter = typeAheadMarkers.iterator(); iter.hasNext();) {
            TypeAheadMarker marker = (TypeAheadMarker) iter.next();
            Component toTest = marker.untilFocused;
            boolean match = (toTest == comp);
            while (!match && toTest != null && !(toTest instanceof Window)) {
                toTest = toTest.getParent();
                match = (toTest == comp);
            }
            if (match) {
                if (start < 0) {
                    start = marker.after;
                }
                iter.remove();
            } else if (start >= 0) {
                purgeStampedEvents(start, marker.after);
                start = -1;
            }
        }
        purgeStampedEvents(start, -1);
    }

    // Notes:
    // * must be called inside a synchronized block
    // * if 'start' is < 0, then this function does nothing
    // * if 'end' is < 0, then all KeyEvents from 'start' to the end of the
    // queue will be removed
    private void purgeStampedEvents(long start, long end) {
        if (start < 0) {
            return;
        }
        for (Iterator iter = enqueuedKeyEvents.iterator(); iter.hasNext();) {
            KeyEvent ke = (KeyEvent) iter.next();
            long time = ke.getWhen();
            if (start < time && (end < 0 || time <= end)) {
                iter.remove();
            }
            if (end >= 0 && time > end) {
                break;
            }
        }
    }

    public void focusPreviousComponent(Component aComponent) {
        if (aComponent != null) {
            aComponent.transferFocusBackward();
        }
    }

    public void focusNextComponent(Component aComponent) {
        if (aComponent != null) {
            aComponent.transferFocus();
        }
    }

    public void upFocusCycle(Component aComponent) {
        if (aComponent != null) {
            aComponent.transferFocusUpCycle();
        }
    }

    public void downFocusCycle(Container aContainer) {
        if (aContainer != null && aContainer.isFocusCycleRoot()) {
            aContainer.transferFocusDownCycle();
        }
    }
}
