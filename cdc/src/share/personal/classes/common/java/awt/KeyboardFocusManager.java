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
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.WindowEvent;
import java.beans.*;
import java.util.Set;
import java.util.HashSet;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.ListIterator;
import java.util.StringTokenizer;
import java.util.WeakHashMap;
import java.lang.ref.WeakReference;
import sun.awt.peer.LightweightPeer;
import sun.awt.SunToolkit;
import sun.awt.AppContext;

public abstract class KeyboardFocusManager implements KeyEventDispatcher, KeyEventPostProcessor {
    public static final int FORWARD_TRAVERSAL_KEYS = 0;
    public static final int BACKWARD_TRAVERSAL_KEYS = 1;
    public static final int UP_CYCLE_TRAVERSAL_KEYS = 2;
    public static final int DOWN_CYCLE_TRAVERSAL_KEYS = 3;
    static final int TRAVERSAL_KEY_LENGTH = DOWN_CYCLE_TRAVERSAL_KEYS + 1;
    private static Component focusOwner;
    private static Component permanentFocusOwner;
    private static Window focusedWindow;
    private static Window activeWindow=null;
    private FocusTraversalPolicy defaultPolicy = new DefaultFocusTraversalPolicy();
    private static final String[] defaultFocusTraversalKeyPropertyNames = {
        "forwardDefaultFocusTraversalKeys", "backwardDefaultFocusTraversalKeys",
        "upCycleDefaultFocusTraversalKeys", "downCycleDefaultFocusTraversalKeys"
    };
    private static final String[] defaultFocusTraversalKeyStrings = {
        "TAB,ctrl TAB", "shift TAB,ctrl shift TAB", "", ""
    };
    private Set[] defaultFocusTraversalKeys = new Set[4];
    private static Container currentFocusCycleRoot;
    private VetoableChangeSupport vetoableSupport;
    private PropertyChangeSupport changeSupport;
    private java.util.LinkedList keyEventDispatchers;
    private java.util.LinkedList keyEventPostProcessors;
    private static java.util.Map mostRecentFocusOwners = new WeakHashMap();
    private static final String notPrivileged = "this KeyboardFocusManager is not installed in the current thread's context";
    private static AWTPermission replaceKeyboardFocusManagerPermission;
    static KeyboardFocusManager manager;

    private native void _clearGlobalFocusOwner();
    static native Component getNativeFocusOwner();
    static native Window getNativeFocusedWindow();
    /**
     * Initialize JNI field and method IDs
     */
    private static native void initIDs();
    private boolean initialized=false;

    /*
     * SequencedEvent which is currently dispatched in AppContext.
     */
    transient SequencedEvent currentSequencedEvent = null;
                                                                               
    final void setCurrentSequencedEvent(SequencedEvent current) {
        synchronized (SequencedEvent.class) {
            //assert(current == null || currentSequencedEvent == null);
            currentSequencedEvent = current;
        }
    }
                                                                                
    final SequencedEvent getCurrentSequencedEvent() {
        synchronized (SequencedEvent.class) {
            return currentSequencedEvent;
        }
    }

   private static final class LightweightFocusRequest {
        final Component component;
        final boolean temporary;

        LightweightFocusRequest(Component component, boolean temporary) {
            this.component = component;
            this.temporary = temporary;
        }
        public String toString() {
            return "LightweightFocusRequest[component=" + component +
                ",temporary=" + temporary + "]";
        }
    }

    private static final class HeavyweightFocusRequest {
        final Component heavyweight;
        final LinkedList lightweightRequests;

        static final HeavyweightFocusRequest CLEAR_GLOBAL_FOCUS_OWNER =
            new HeavyweightFocusRequest();

        private HeavyweightFocusRequest() {
            heavyweight = null;
            lightweightRequests = null;
        }

        HeavyweightFocusRequest(Component heavyweight, Component descendant,
                                boolean temporary) {
/*
            if (dbg.on) {
                dbg.assertion(heavyweight != null);
            }
*/

            this.heavyweight = heavyweight;
            this.lightweightRequests = new LinkedList();
            addLightweightRequest(descendant, temporary);
        }
        boolean addLightweightRequest(Component descendant,
                                      boolean temporary) {
/*
            if (dbg.on) {
                dbg.assertion(this != HeavyweightFocusRequest.
                              CLEAR_GLOBAL_FOCUS_OWNER);
                dbg.assertion(descendant != null);
            }
*/

            Component lastDescendant = ((lightweightRequests.size() > 0)
                ? ((LightweightFocusRequest)lightweightRequests.getLast()).
                  component
                : null);

            if (descendant != lastDescendant) {
                // Not a duplicate request
                lightweightRequests.add
                    (new LightweightFocusRequest(descendant, temporary));
                return true;
            } else {
                return false;
            }
        }

        LightweightFocusRequest getFirstLightweightRequest() {
            if (this == CLEAR_GLOBAL_FOCUS_OWNER) {
                return null;
            }
            return (LightweightFocusRequest)lightweightRequests.getFirst();
        }
        public String toString() {
            boolean first = true;
            String str = "HeavyweightFocusRequest[heavweight=" + heavyweight +
                ",lightweightRequests=";
            if (lightweightRequests == null) {
                str += null;
            } else {
                str += "[";
                for (Iterator iter = lightweightRequests.iterator();
                     iter.hasNext(); )
                {
                    if (first) {
                        first = false;
                    } else {
                        str += ",";
                    }
                    str += iter.next();
                }
                str += "]";
            }
            str += "]";
            return str;
        }
    }

    private static boolean focusedWindowChanged(Component a, Component b) {
        Window wa = getContainingWindow(a);
        Window wb = getContainingWindow(b);

        if (wa == null || wb == null) {
            return false;
        }
        return (wa != wb);
    }
    static Window getContainingWindow(Component comp) {
        while (comp != null && !(comp instanceof Window)) {
            comp = comp.getParent();
        }

        return (Window)comp;
    }

    private static Component getHeavyweight(Component comp) {
        if (comp == null || comp.peer == null) {
            return null;
        } else if (comp.peer instanceof LightweightPeer) {
            return comp.getNativeContainer();
        } else {
            return comp;
        }
    }
    /*
     * heavyweightRequests is used as a monitor for synchronized changes of 
     * currentLightweightRequests, clearingCurrentLightweightRequests and 
     * newFocusOwner.
     */
    private static LinkedList heavyweightRequests = new LinkedList();
    private static LinkedList currentLightweightRequests;
    private static boolean clearingCurrentLightweightRequests;
    private static Component newFocusOwner = null;

    int requestCount() {
        return heavyweightRequests.size();
    }

    static final int SNFH_FAILURE = 0;
    static final int SNFH_SUCCESS_HANDLED = 1;
    static final int SNFH_SUCCESS_PROCEED = 2;

   /**
     * Indicates whether the native implementation should proceed with a
     * pending, native focus request. Before changing the focus at the native
     * level, the AWT implementation should always call this function for
     * permission. This function will reject the request if a duplicate request
     * preceded it, or if the specified heavyweight Component already owns the
     * focus and no native focus changes are pending. Otherwise, the request
     * will be approved and the focus request list will be updated so that,
     * if necessary, the proper descendant will be focused when the
     * corresponding FOCUS_GAINED event on the heavyweight is received.
     * 
     * An implementation must ensure that calls to this method and native
     * focus changes are atomic. If this is not guaranteed, then the ordering
     * of the focus request list may be incorrect, leading to errors in the
     * type-ahead mechanism. Typically this is accomplished by only calling
     * this function from the native event pumping thread, or by holding a
     * global, native lock during invocation.
     */
    static int shouldNativelyFocusHeavyweight
        (Component heavyweight, Component descendant, boolean temporary,
         boolean focusedWindowChangeAllowed, long time)
    {
/*
        if (dbg.on) {
            dbg.assertion(heavyweight != null);
            dbg.assertion(time != 0);
        }
*/

        if (descendant == null) {
            // Focus transfers from a lightweight child back to the
            // heavyweight Container should be treated like lightweight
            // focus transfers.
            descendant = heavyweight;
        }

        synchronized (heavyweightRequests) {
            HeavyweightFocusRequest hwFocusRequest = (HeavyweightFocusRequest)
                ((heavyweightRequests.size() > 0)
                 ? heavyweightRequests.getLast() : null);
            if (hwFocusRequest == null &&
                heavyweight == getNativeFocusOwner())
            {
                Component currentFocusOwner = getCurrentKeyboardFocusManager().
                    getGlobalFocusOwner();

                if (descendant == currentFocusOwner) {
                    // Redundant request.
                    return SNFH_FAILURE;
                }

                // 'heavyweight' owns the native focus and there are no pending
                // requests. 'heavyweight' must be a Container and
                // 'descendant' must not be the focus owner. Otherwise,
                // we would never have gotten this far.
                getCurrentKeyboardFocusManager
		    (SunToolkit.targetToAppContext(descendant)).
                    enqueueKeyEvents(time, descendant);

                hwFocusRequest =
                    new HeavyweightFocusRequest(heavyweight, descendant,
                                                temporary);
                heavyweightRequests.add(hwFocusRequest);

                if (currentFocusOwner != null) {
                    FocusEvent currentFocusOwnerEvent =
                        new FocusEvent(currentFocusOwner,
                                       FocusEvent.FOCUS_LOST,
                                       temporary, descendant);
		    SunToolkit.postEvent(currentFocusOwner.appContext,
					 currentFocusOwnerEvent);

                }
                FocusEvent newFocusOwnerEvent =
                    new FocusEvent(descendant, FocusEvent.FOCUS_GAINED,
                                   temporary, currentFocusOwner);
		 SunToolkit.postEvent(descendant.appContext,
				      newFocusOwnerEvent);
                return SNFH_SUCCESS_HANDLED;
            } else if (hwFocusRequest != null &&
                       hwFocusRequest.heavyweight == heavyweight) {
                // 'heavyweight' doesn't have the native focus right now, but
                // if all pending requests were completed, it would. Add
                // descendant to the heavyweight's list of pending
                // lightweight focus transfers.
                if (hwFocusRequest.addLightweightRequest(descendant,
                                                         temporary)) {
                    getCurrentKeyboardFocusManager
                        (SunToolkit.targetToAppContext(descendant)).
                        enqueueKeyEvents(time, descendant);
                }
                
                return SNFH_SUCCESS_HANDLED;
            } else {
                if (!focusedWindowChangeAllowed) {
                    // For purposes of computing oldFocusedWindow, we should
                    // look at the second to last HeavyweightFocusRequest on
                    // the queue iff the last HeavyweightFocusRequest is
                    // CLEAR_GLOBAL_FOCUS_OWNER. If there is no second to last
                    // HeavyweightFocusRequest, null is an acceptable value.
                    if (hwFocusRequest ==
                        HeavyweightFocusRequest.CLEAR_GLOBAL_FOCUS_OWNER) 
                    {
                        int size = heavyweightRequests.size();
                        hwFocusRequest = (HeavyweightFocusRequest)((size >= 2)
                            ? heavyweightRequests.get(size - 2)
                            : null);
                    }

                    if (focusedWindowChanged(heavyweight,
                                             (hwFocusRequest != null)
                                             ? hwFocusRequest.heavyweight
                                             : getNativeFocusedWindow())) {
                        return SNFH_FAILURE;
                    }
                }
                getCurrentKeyboardFocusManager
                    (SunToolkit.targetToAppContext(descendant)).
                    enqueueKeyEvents(time, descendant);
                heavyweightRequests.add
                    (new HeavyweightFocusRequest(heavyweight, descendant,
                                                 temporary));
                return SNFH_SUCCESS_PROCEED;
            }
        }
    }

    static void processCurrentLightweightRequests() {
        KeyboardFocusManager manager = getCurrentKeyboardFocusManager();
        LinkedList localLightweightRequests = null;
        
        synchronized(heavyweightRequests) {
            if (currentLightweightRequests != null) {
                clearingCurrentLightweightRequests = true;
                localLightweightRequests = currentLightweightRequests;
                currentLightweightRequests = null;
            } else {
                // do nothing
                return;
            }
        }
        try {
            if (localLightweightRequests != null) {
                for (Iterator iter = localLightweightRequests.iterator();
                     iter.hasNext(); )
                {
                    Component currentFocusOwner = manager.
                        getGlobalFocusOwner();
                    if (currentFocusOwner == null) {
                        // If this ever happens, a focus change has been
                        // rejected. Stop generating more focus changes.
                        break;
                    }

                    LightweightFocusRequest lwFocusRequest =
                        (LightweightFocusRequest)iter.next();
                    FocusEvent currentFocusOwnerEvent =
                        new FocusEvent(currentFocusOwner,
                                       FocusEvent.FOCUS_LOST,
                                       lwFocusRequest.temporary,
                                       lwFocusRequest.component);
                    FocusEvent newFocusOwnerEvent =
                        new FocusEvent(lwFocusRequest.component,
                                       FocusEvent.FOCUS_GAINED,
                                       lwFocusRequest.temporary,
                                       currentFocusOwner);

                    currentFocusOwner.dispatchEvent(currentFocusOwnerEvent);
                    lwFocusRequest.component.
                        dispatchEvent(newFocusOwnerEvent);
                }
            }
        } finally {
            clearingCurrentLightweightRequests = false;
            localLightweightRequests = null;
        }
    }


    static FocusEvent retargetUnexpectedFocusEvent(FocusEvent fe) {
        synchronized (heavyweightRequests) {
            // Any other case represents a failure condition which we did
            // not expect. We need to clearFocusRequestList() and patch up
            // the event as best as possible.

            if (removeFirstRequest()) {
                return (FocusEvent)retargetFocusEvent(fe);
            }

            Component source = fe.getComponent();
            Component opposite = fe.getOppositeComponent();
            boolean temporary = false;
            if (fe.getID() == FocusEvent.FOCUS_LOST &&
                (opposite == null || focusedWindowChanged(source, opposite)))
            {
                temporary = true;
            }
            return new FocusEvent(source, fe.getID(), temporary, opposite);
        }
    }

    static FocusEvent retargetFocusGained(FocusEvent fe) {
       // assert (fe.getID() == FocusEvent.FOCUS_GAINED);

        Component currentFocusOwner = getCurrentKeyboardFocusManager().
            getGlobalFocusOwner();
        Component source = fe.getComponent();
        Component opposite = fe.getOppositeComponent();
        Component nativeSource = getHeavyweight(source);

        synchronized (heavyweightRequests) {
            HeavyweightFocusRequest hwFocusRequest = (HeavyweightFocusRequest)
                ((heavyweightRequests.size() > 0)
                 ? heavyweightRequests.getFirst() : null);

            if (hwFocusRequest == HeavyweightFocusRequest.CLEAR_GLOBAL_FOCUS_OWNER)
            {
                return retargetUnexpectedFocusEvent(fe);
            }

            if (source != null && nativeSource == null && hwFocusRequest != null) {
                // if source w/o peer and 
                // if source is equal to first lightweight
                // then we should correct source and nativeSource
                if (source == hwFocusRequest.getFirstLightweightRequest().component) 
                {
                    source = hwFocusRequest.heavyweight;
                    nativeSource = source; // source is heavuweight itself
                }
            }
            if (hwFocusRequest != null &&
                nativeSource == hwFocusRequest.heavyweight)
            {
                // Focus change as a result of a known call to requestFocus(),
                // or known click on a peer focusable heavyweight Component.

                heavyweightRequests.removeFirst();

                LightweightFocusRequest lwFocusRequest =
                    (LightweightFocusRequest)hwFocusRequest.
                    lightweightRequests.removeFirst();

                Component newSource = lwFocusRequest.component;
                if (currentFocusOwner != null) {
                    /*
                     * Since we receive FOCUS_GAINED when current focus 
                     * owner is not null, correcponding FOCUS_LOST is supposed
                     * to be lost.  And so,  we keep new focus owner 
                     * to determine synthetic FOCUS_LOST event which will be 
                     * generated by KeyboardFocusManager for this FOCUS_GAINED.
                     *
                     * This code based on knowledge of 
                     * DefaultKeyboardFocusManager's implementation and might 
                     * be not applicable for another KeyboardFocusManager.
                     */
                    newFocusOwner = newSource;
                }
		// LMK - comment out the check for opposite == null - it 
		// always is for us
                boolean temporary = (//opposite == null || 
                                     focusedWindowChanged(newSource, opposite))
                        ? false
                        : lwFocusRequest.temporary;

                if (hwFocusRequest.lightweightRequests.size() > 0) {
                    currentLightweightRequests =
                        hwFocusRequest.lightweightRequests;
                    EventQueue.invokeLater(new Runnable() {
                            public void run() {
                                processCurrentLightweightRequests();
                            }
                        });
                }

                // 'opposite' will be fixed by
                // DefaultKeyboardFocusManager.realOppositeComponent
                return new FocusEvent(newSource,
                                      FocusEvent.FOCUS_GAINED, temporary,
                                      opposite);
            }

            if (currentFocusOwner != null
                && getContainingWindow(currentFocusOwner) == source
                && (hwFocusRequest == null || source != hwFocusRequest.heavyweight)) 
            {
                // Special case for FOCUS_GAINED in top-levels
                // If it arrives as the result of activation we should skip it
                // This event will not have appropriate request record and
                // on arrival there will be already some focus owner set.
                return new FocusEvent(currentFocusOwner, FocusEvent.FOCUS_GAINED, false, null);
            }

            return retargetUnexpectedFocusEvent(fe);
        } // end synchronized(heavyweightRequests)
    }

    static FocusEvent retargetFocusLost(FocusEvent fe) {
//        assert (fe.getID() == FocusEvent.FOCUS_LOST);

        Component currentFocusOwner = getCurrentKeyboardFocusManager().
            getGlobalFocusOwner();
        Component opposite = fe.getOppositeComponent();
        Component nativeOpposite = getHeavyweight(opposite);

        synchronized (heavyweightRequests) {
            HeavyweightFocusRequest hwFocusRequest = (HeavyweightFocusRequest)
                ((heavyweightRequests.size() > 0)
                 ? heavyweightRequests.getFirst() : null);

            if (hwFocusRequest == HeavyweightFocusRequest.CLEAR_GLOBAL_FOCUS_OWNER)
            {
                if (currentFocusOwner != null) {
                    // Call to KeyboardFocusManager.clearGlobalFocusOwner()
                    heavyweightRequests.removeFirst();
                    return new FocusEvent(currentFocusOwner,
                                          FocusEvent.FOCUS_LOST, false, null);
                }

                // Otherwise, fall through to failure case below

            } else if (opposite == null)
            {
                // Focus leaving application
                if (currentFocusOwner != null) {
                    return new FocusEvent(currentFocusOwner, 
                                          FocusEvent.FOCUS_LOST,
                                          true, null);
                } else {
                    return fe;
                }
            } else if (hwFocusRequest != null &&
                       (nativeOpposite == hwFocusRequest.heavyweight || 
                        nativeOpposite == null && 
                        opposite == hwFocusRequest.getFirstLightweightRequest().component))
            {
                if (currentFocusOwner == null) {
                    return fe;
                }
                // Focus change as a result of a known call to requestFocus(),
                // or click on a peer focusable heavyweight Component.
                
                // If a focus transfer is made across top-levels, then the
                // FOCUS_LOST event is always temporary, and the FOCUS_GAINED
                // event is always permanent. Otherwise, the stored temporary
                // value is honored.

                LightweightFocusRequest lwFocusRequest =
                    (LightweightFocusRequest)hwFocusRequest.
                    lightweightRequests.getFirst();

                boolean temporary = focusedWindowChanged(opposite,
                                                         currentFocusOwner)
                    ? true
                    : lwFocusRequest.temporary;

                return new FocusEvent(currentFocusOwner, FocusEvent.FOCUS_LOST,
                                      temporary, lwFocusRequest.component);
            } 

            return retargetUnexpectedFocusEvent(fe);
        }  // end synchronized(heavyweightRequests)
    }

    static AWTEvent retargetFocusEvent(AWTEvent event) {
        if (clearingCurrentLightweightRequests) {
            return event;
        }

        KeyboardFocusManager manager = getCurrentKeyboardFocusManager();

        synchronized(heavyweightRequests) {
            /*
             * This code handles FOCUS_LOST event which is generated by 
             * DefaultKeyboardFocusManager for FOCUS_GAINED.
             *
             * This code based on knowledge of DefaultKeyboardFocusManager's
             * implementation and might be not applicable for another 
             * KeyboardFocusManager.
             * 
             * Fix for 4472032
             */
            if (newFocusOwner != null &&
                event.getID() == FocusEvent.FOCUS_LOST) 
            {
                FocusEvent fe = (FocusEvent)event;

                if (manager.getGlobalFocusOwner() == fe.getComponent() &&
                    fe.getOppositeComponent() == newFocusOwner) 
                {
                    newFocusOwner = null;
                    return event;
                }
            }
        }

        processCurrentLightweightRequests();

        switch (event.getID()) {
            case FocusEvent.FOCUS_GAINED: {
                event = retargetFocusGained((FocusEvent)event);
                break;
            }
            case FocusEvent.FOCUS_LOST: {
                event = retargetFocusLost((FocusEvent)event);
                break;
            }
            default:
                /* do nothing */
        }
        return event;
    }

    static boolean removeFirstRequest() {
        KeyboardFocusManager manager =
            KeyboardFocusManager.getCurrentKeyboardFocusManager();

        synchronized(heavyweightRequests) {
            HeavyweightFocusRequest hwFocusRequest = (HeavyweightFocusRequest)

                ((heavyweightRequests.size() > 0) 
                 ? heavyweightRequests.getFirst() : null);
            if (hwFocusRequest != null) {
                heavyweightRequests.removeFirst();
                if (hwFocusRequest.lightweightRequests != null) {
                    for (Iterator lwIter = hwFocusRequest.lightweightRequests.
                             iterator();
                         lwIter.hasNext(); )
                    {
                        manager.dequeueKeyEvents
                            (-1, ((LightweightFocusRequest)lwIter.next()).
                             component);
                    }
                }
            }
            return (heavyweightRequests.size() > 0);
        }        
    }
    static void removeLastFocusRequest(Component heavyweight) {
	/*  if (dbg.on) {
            dbg.assertion(heavyweight != null);
        }
	*/

        synchronized(heavyweightRequests) {
            HeavyweightFocusRequest hwFocusRequest = (HeavyweightFocusRequest)
                ((heavyweightRequests.size() > 0) 
                 ? heavyweightRequests.getLast() : null);
            if (hwFocusRequest != null && 
                hwFocusRequest.heavyweight == heavyweight) {
                heavyweightRequests.removeLast();
            }
        }
    }
    static void removeFocusRequest(Component component) {
/*
        if (dbg.on) {
            dbg.assertion(component != null);
        }
*/

        Component heavyweight = (component.peer instanceof LightweightPeer)
            ? component.getNativeContainer() : component;
        if (heavyweight == null) {
            return;
        }

        synchronized (heavyweightRequests) {
            if ((currentLightweightRequests != null) && 
                (currentLightweightRequests.size() > 0)) {
                LightweightFocusRequest lwFocusRequest =
                    (LightweightFocusRequest)currentLightweightRequests.getFirst();
                Component comp = lwFocusRequest.component;
                Component currentHeavyweight = 
                    (comp.peer instanceof LightweightPeer) 
                    ? comp.getNativeContainer() : comp;

                if (currentHeavyweight == component) {
                    currentLightweightRequests = null;
                } else {
                    for (Iterator iter = currentLightweightRequests.iterator();
                         iter.hasNext(); ) {
                        if (((LightweightFocusRequest)iter.next()).
                            component == component)
                        {
                            iter.remove();
                        }
                    }
                    if (currentLightweightRequests.size() == 0) {
                        currentLightweightRequests = null;
                    }
                }
            }
            for (Iterator iter = heavyweightRequests.iterator();
                 iter.hasNext(); )
            {
                HeavyweightFocusRequest hwFocusRequest =
                    (HeavyweightFocusRequest)iter.next();
                if (hwFocusRequest.heavyweight == heavyweight) {
                    if (heavyweight == component) {
                        iter.remove();
                        continue;
                    }
                    for (Iterator lwIter = hwFocusRequest.lightweightRequests.
                             iterator();
                         lwIter.hasNext(); )
                    {
                        if (((LightweightFocusRequest)lwIter.next()).
                            component == component)
                        {
                            lwIter.remove();
                        }
                    }
		    if (hwFocusRequest.lightweightRequests.size() == 0) {
			iter.remove();
		    }
                }
            }
        }
    }
    private static void clearFocusRequestList() {
        KeyboardFocusManager manager =
            KeyboardFocusManager.getCurrentKeyboardFocusManager();

        synchronized (heavyweightRequests) {
            for (Iterator iter = heavyweightRequests.iterator();
                 iter.hasNext(); )
            {
                HeavyweightFocusRequest hwFocusRequest =
                    (HeavyweightFocusRequest)iter.next();
                if (hwFocusRequest.lightweightRequests == null) {
                    continue;
                }
                for (Iterator lwIter = hwFocusRequest.lightweightRequests.
                         iterator();
                     lwIter.hasNext(); )
                {
                    manager.dequeueKeyEvents
                        (-1, ((LightweightFocusRequest)lwIter.next()).
                         component);
                }
            }
            heavyweightRequests.clear();
        }        
    }

    public static KeyboardFocusManager getCurrentKeyboardFocusManager() {
        return getCurrentKeyboardFocusManager(AppContext.getAppContext());
    }
                   
    synchronized static KeyboardFocusManager
        getCurrentKeyboardFocusManager(AppContext appcontext)
    {
        KeyboardFocusManager manager = (KeyboardFocusManager)
            appcontext.get(KeyboardFocusManager.class);
        if (manager == null) {
            manager = new DefaultKeyboardFocusManager();
            appcontext.put(KeyboardFocusManager.class, manager);
        }
        return manager;
    }

    // Bug #6223096: TCK: PP-TCK_11 Signature Test fails with CDC-PP-Sec
    /*
    synchronized static void setCurrentKeyboardFocusManager(
            KeyboardFocusManager newManager) throws SecurityException {
       AppContext appcontext = AppContext.getAppContext();
        if (newManager != null) {
            SecurityManager security = System.getSecurityManager();
            if (security != null) {
                if (replaceKeyboardFocusManagerPermission == null) {
                    replaceKeyboardFocusManagerPermission = 
                        new AWTPermission("replaceKeyboardFocusManager");
                }
                security.
                    checkPermission(replaceKeyboardFocusManagerPermission);
            }
            appcontext.put(KeyboardFocusManager.class, newManager);
        }
        else {
            appcontext.remove(KeyboardFocusManager.class);
        }
    }
    */

    static Set initFocusTraversalKeysSet(String value, Set targetSet) {
        StringTokenizer tokens = new StringTokenizer(value, ",");
        while (tokens.hasMoreTokens()) {
            targetSet.add(AWTKeyStroke.getAWTKeyStroke(tokens.nextToken()));
        }
        return (targetSet.isEmpty())
                ? Collections.EMPTY_SET
                : Collections.unmodifiableSet(targetSet);
    }
    
    KeyboardFocusManager() {
        for (int i = 0; i < TRAVERSAL_KEY_LENGTH; i++) {
            defaultFocusTraversalKeys[i] = initFocusTraversalKeysSet(defaultFocusTraversalKeyStrings[i],
                    new HashSet());
        }
    }


    public Component getFocusOwner() {
        synchronized (KeyboardFocusManager.class) {
            if (focusOwner == null) {
                return null;
            }
            return (focusOwner.appContext == AppContext.getAppContext())
                ? focusOwner : null;
        }
    }

    Component getGlobalFocusOwner() throws SecurityException {
        synchronized (KeyboardFocusManager.class) {
            if (this == getCurrentKeyboardFocusManager()) {
                return focusOwner;
            } else {
                throw new SecurityException(notPrivileged);
            }
        }
    }

    void setGlobalFocusOwner(Component focusOwner) {
        Component oldFocusOwner = null;
        boolean shouldFire = false;
        if (focusOwner == null || focusOwner.isFocusable()) {
            synchronized (KeyboardFocusManager.class) {
                oldFocusOwner = getFocusOwner();
                try {
                    fireVetoableChange("focusOwner", oldFocusOwner, focusOwner);
                } catch (PropertyVetoException e) {
                    return;
                }
                KeyboardFocusManager.focusOwner = focusOwner;
                if (focusOwner != null
                        && (getCurrentFocusCycleRoot() == null
                                || !focusOwner.isFocusCycleRoot(getCurrentFocusCycleRoot()))) {
                    Container rootAncestor = focusOwner.getFocusCycleRootAncestor();
                    if (rootAncestor == null && (focusOwner instanceof Window)) {
                        rootAncestor = (Container) focusOwner;
                    }
                    if (rootAncestor != null) {
                        setGlobalCurrentFocusCycleRoot(rootAncestor);
                    }
                }
                shouldFire = true;
            }
        }
        if (shouldFire) {
            firePropertyChange("focusOwner", oldFocusOwner, focusOwner);
        }
    }

    public void clearGlobalFocusOwner() {
  	if (!GraphicsEnvironment.isHeadless()) {
            // Toolkit must be fully initialized, otherwise
            // _clearGlobalFocusOwner will crash or throw an exception
            Toolkit.getDefaultToolkit();
            if (!initialized) {
                // 6253293 - don't want to initialize the toolkit in the 
                // constructor as this starts a bunch of threads up and doesn't
                // allow the VM to exit.  Don't do it till we need it
                initIDs(); 
                initialized=true;
            }
            _clearGlobalFocusOwner();
        }
    }

    /**
     * Returns the Window which will be active after processing this request,
     * or null if this is a duplicate request. The active Window is useful
     * because some native platforms do not support setting the native focus
     * owner to null. On these platforms, the obvious choice is to set the
     * focus owner to the focus proxy of the active Window.
     */
    static Window markClearGlobalFocusOwner() {
        synchronized (heavyweightRequests) {
            HeavyweightFocusRequest hwFocusRequest = (HeavyweightFocusRequest)
                ((heavyweightRequests.size() > 0)
                 ? heavyweightRequests.getLast() : null);
            if (hwFocusRequest ==
                HeavyweightFocusRequest.CLEAR_GLOBAL_FOCUS_OWNER)
            {
                // duplicate request
                return null;
            }
                                                                
            heavyweightRequests.add
                (HeavyweightFocusRequest.CLEAR_GLOBAL_FOCUS_OWNER);
                                                                               
            Component activeWindow = ((hwFocusRequest != null)
                ? getContainingWindow(hwFocusRequest.heavyweight)
                : getNativeFocusedWindow());
            while (activeWindow != null &&
                   !((activeWindow instanceof Frame) ||
                     (activeWindow instanceof Dialog)))
            {
                activeWindow = activeWindow.getParent();
            }
                                                                               
            return (Window)activeWindow;
        }
    }


    public Component getPermanentFocusOwner() {
        synchronized (KeyboardFocusManager.class) {
            if (permanentFocusOwner == null) {
                return null;
            }
            return (permanentFocusOwner.appContext ==
                    AppContext.getAppContext()) ? permanentFocusOwner : null;
        }
    }

    Component getGlobalPermanentFocusOwner()
        throws SecurityException {
        synchronized (KeyboardFocusManager.class) {
            if (this == getCurrentKeyboardFocusManager()) {
                return permanentFocusOwner;
            } else {
                throw new SecurityException(notPrivileged);
            }
        }
    }

    void setGlobalPermanentFocusOwner(Component permanentFocusOwner) {
        Component oldPermanentFocusOwner = null;
        boolean shouldFire = false;
        if (permanentFocusOwner == null || permanentFocusOwner.isFocusable()) {
            synchronized (KeyboardFocusManager.class) {
                oldPermanentFocusOwner = getPermanentFocusOwner();
                try {
                    fireVetoableChange("permanentFocusOwner",
                            oldPermanentFocusOwner, permanentFocusOwner);
                } catch (PropertyVetoException e) {
                    return;
                }
                KeyboardFocusManager.permanentFocusOwner = permanentFocusOwner;
                KeyboardFocusManager.setMostRecentFocusOwner(permanentFocusOwner);
                shouldFire = true;
            }
        }
	
        if (shouldFire) {
            firePropertyChange("permanentFocusOwner", oldPermanentFocusOwner,
                    permanentFocusOwner);
        }
    }

    public Window getFocusedWindow() {

        synchronized (KeyboardFocusManager.class) {
            if (focusedWindow == null) {
                return null;
            }
            return (focusedWindow.appContext == AppContext.getAppContext())
                ? focusedWindow : null;
        }
    }

    Window getGlobalFocusedWindow() throws SecurityException {
        synchronized (KeyboardFocusManager.class) {
            if (this == getCurrentKeyboardFocusManager()) {
                return focusedWindow;
            } else {
                throw new SecurityException(notPrivileged);
            }
        }
    }

    void setGlobalFocusedWindow(Window focusedWindow) {
        Window oldFocusedWindow = null;
        boolean shouldFire = false;
        if (focusedWindow == null || focusedWindow.isFocusableWindow()) {
            synchronized (KeyboardFocusManager.class) {
                oldFocusedWindow = getFocusedWindow();
                try {
                    fireVetoableChange("focusedWindow", oldFocusedWindow,
                            focusedWindow);
                } catch (PropertyVetoException e) {
                    return;
                }
                KeyboardFocusManager.focusedWindow = focusedWindow;
                shouldFire = true;
            }
        }
        if (shouldFire) {
            firePropertyChange("focusedWindow", oldFocusedWindow, focusedWindow);
        }
    }

    public Window getActiveWindow() {
        synchronized (KeyboardFocusManager.class) {
            if (activeWindow == null) {
                return null;
            }
            return (activeWindow.appContext == AppContext.getAppContext())
                ? activeWindow : null;
        }
    }

    Window getGlobalActiveWindow() throws SecurityException {
        synchronized (KeyboardFocusManager.class) {
            if (this == getCurrentKeyboardFocusManager()) {
                return activeWindow;
            } else {
                throw new SecurityException(notPrivileged);
            }
        }
    }

    void setGlobalActiveWindow(Window activeWindow) {
        // 6205919:
        // PP-TCK: api/java_awt/Event/WinEventTests.html#WinEventTest0008 hangs.
        //
        // The following code was once performed within the synchronized block
        // of KeyboardFocusManager.class and caused a deadlock because the call
        // activeWindow.isFocusableWindow() can potentially try to grab the AWT
        // tree lock, while another thread might be doing a dispose call,
        // acquiring the AWT tree lock before trying to acquire the
        // KeyboardFocusManager.class lock.
        if ((activeWindow != null) && (!activeWindow.isFocusableWindow())) {
            return;
        }

        Window oldActiveWindow;
        synchronized (KeyboardFocusManager.class) {
            oldActiveWindow = getActiveWindow();
            try {
                fireVetoableChange("activeWindow", oldActiveWindow, activeWindow);
            } catch (PropertyVetoException e) {
                return;
            }
            KeyboardFocusManager.activeWindow = activeWindow;
        }
        firePropertyChange("activeWindow", oldActiveWindow, activeWindow);
    }

    public synchronized FocusTraversalPolicy getDefaultFocusTraversalPolicy() {
        return defaultPolicy;
    }

    public void setDefaultFocusTraversalPolicy(FocusTraversalPolicy
            defaultPolicy) {
        if (defaultPolicy == null) {
            throw new IllegalArgumentException("default focus traversal policy cannot be null");
        }
        FocusTraversalPolicy oldPolicy;
        synchronized (this) {
            oldPolicy = this.defaultPolicy;
            this.defaultPolicy = defaultPolicy;
        }
        firePropertyChange("defaultFocusTraversalPolicy", oldPolicy,
                defaultPolicy);
    }

    public void setDefaultFocusTraversalKeys(int id, Set keystrokes) {
        if (id < 0 || id >= TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        if (keystrokes == null) {
            throw new IllegalArgumentException("cannot set null Set of default focus traversal keys");
        }
        Set oldKeys;
        synchronized (this) {
            for (Iterator iter = keystrokes.iterator(); iter.hasNext();) {
                Object obj = iter.next();
                if (obj == null) {
                    throw new IllegalArgumentException("cannot set null focus traversal key");
                }
                AWTKeyStroke keystroke = (AWTKeyStroke) obj;
                if (keystroke.getKeyChar() != KeyEvent.CHAR_UNDEFINED) {
                    throw new IllegalArgumentException("focus traversal keys cannot map to KEY_TYPED events");
                }
                for (int i = 0; i < TRAVERSAL_KEY_LENGTH; i++) {
                    if (i == id) {
                        continue;
                    }
                    if (defaultFocusTraversalKeys[i].contains(keystroke)) {
                        throw new IllegalArgumentException("focus traversal keys must be unique for a Component");
                    }
                }
            }
            oldKeys = defaultFocusTraversalKeys[id];
            defaultFocusTraversalKeys[id] = Collections.unmodifiableSet(new HashSet(keystrokes));
        }
        firePropertyChange(defaultFocusTraversalKeyPropertyNames[id], oldKeys,
                keystrokes);
    }

    public Set getDefaultFocusTraversalKeys(int id) {
        if (id < 0 || id >= TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        return defaultFocusTraversalKeys[id];
    }

    public Container getCurrentFocusCycleRoot() {

        synchronized (KeyboardFocusManager.class) {
            if (currentFocusCycleRoot == null) {
                return null;
            }
            return (currentFocusCycleRoot.appContext ==
                    AppContext.getAppContext()) ? currentFocusCycleRoot:null;
        }
    }

    Container getGlobalCurrentFocusCycleRoot()
        throws SecurityException {
        synchronized (KeyboardFocusManager.class) {
            if (this == getCurrentKeyboardFocusManager()) {
                return currentFocusCycleRoot;
            } else {
                throw new SecurityException(notPrivileged);
            }
        }
    }

    void setGlobalCurrentFocusCycleRoot(Container newFocusCycleRoot) {
        Container oldFocusCycleRoot;
        synchronized (KeyboardFocusManager.class) {
            oldFocusCycleRoot = getCurrentFocusCycleRoot();
            currentFocusCycleRoot = newFocusCycleRoot;
        }
        firePropertyChange("currentFocusCycleRoot", oldFocusCycleRoot,
                newFocusCycleRoot);
    }

    public void addPropertyChangeListener(PropertyChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (changeSupport == null) {
                    changeSupport = new PropertyChangeSupport(this);
                }
                changeSupport.addPropertyChangeListener(listener);
            }
        }
    }

    public void removePropertyChangeListener(PropertyChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (changeSupport != null) {
                    changeSupport.removePropertyChangeListener(listener);
                }
            }
        }
    }

    public synchronized PropertyChangeListener[] getPropertyChangeListeners() {
        if (changeSupport == null) {
            changeSupport = new PropertyChangeSupport(this);
        }
        return changeSupport.getPropertyChangeListeners();
    }

/*
    public void addPropertyChangeListener(String propertyName,
            PropertyChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (changeSupport == null) {
                    changeSupport = new PropertyChangeSupport(this);
                }
                changeSupport.addPropertyChangeListener(propertyName, listener);
            }
        }
    }

    public void removePropertyChangeListener(String propertyName,
            PropertyChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (changeSupport != null) {
                    changeSupport.removePropertyChangeListener(propertyName,
                            listener);
                }
            }
        }
    }

    public synchronized PropertyChangeListener[] getPropertyChangeListeners(String propertyName) {
        if (changeSupport == null) {
            changeSupport = new PropertyChangeSupport(this);
        }
        return changeSupport.getPropertyChangeListeners(propertyName);
    }

*/
    void firePropertyChange(String propertyName, Object oldValue,
            Object newValue) {
        PropertyChangeSupport changeSupport = this.changeSupport;
        if (changeSupport != null) {
            changeSupport.firePropertyChange(propertyName, oldValue, newValue);
        }

    }

    public void addVetoableChangeListener(VetoableChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (vetoableSupport == null) {
                    vetoableSupport = new VetoableChangeSupport(this);
                }
                vetoableSupport.addVetoableChangeListener(listener);
            }
        }
    }

    public void removeVetoableChangeListener(VetoableChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (vetoableSupport != null) {
                    vetoableSupport.removeVetoableChangeListener(listener);
                }
            }
        }
    }

    public synchronized VetoableChangeListener[] getVetoableChangeListeners() {
        if (vetoableSupport == null) {
            vetoableSupport = new VetoableChangeSupport(this);
        }
        return vetoableSupport.getVetoableChangeListeners();
    }

/*
    public void addVetoableChangeListener(String propertyName,
            VetoableChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (vetoableSupport == null) {
                    vetoableSupport = new VetoableChangeSupport(this);
                }
                vetoableSupport.addVetoableChangeListener(propertyName, listener);
            }
        }
    }

    public void removeVetoableChangeListener(String propertyName,
            VetoableChangeListener listener) {
        if (listener != null) {
            synchronized (this) {
                if (vetoableSupport != null) {
                    vetoableSupport.removeVetoableChangeListener(propertyName,
                            listener);
                }
            }
        }
    }

    public synchronized VetoableChangeListener[] getVetoableChangeListeners(String propertyName) {
        if (vetoableSupport == null) {
            vetoableSupport = new VetoableChangeSupport(this);
        }
        return vetoableSupport.getVetoableChangeListeners(propertyName);
    }

*/
    void fireVetoableChange(String propertyName, Object oldValue,
            Object newValue)
        throws PropertyVetoException {
        VetoableChangeSupport vetoableSupport = this.vetoableSupport;
        if (vetoableSupport != null) {
            vetoableSupport.fireVetoableChange(propertyName, oldValue, newValue);
        }
    }

    public void addKeyEventDispatcher(KeyEventDispatcher dispatcher) {
        if (dispatcher != null) {
            synchronized (this) {
                if (keyEventDispatchers == null) {
                    keyEventDispatchers = new java.util.LinkedList();
                }
                keyEventDispatchers.add(dispatcher);
            }
        }
    }

    public void removeKeyEventDispatcher(KeyEventDispatcher dispatcher) {
        if (dispatcher != null) {
            synchronized (this) {
                if (keyEventDispatchers != null) {
                    keyEventDispatchers.remove(dispatcher);
                }
            }
        }
    }

    synchronized java.util.List getKeyEventDispatchers() {
        return (keyEventDispatchers != null)
                ? (java.util.List) keyEventDispatchers.clone()
                : null;
    }

    public void addKeyEventPostProcessor(KeyEventPostProcessor processor) {
        if (processor != null) {
            synchronized (this) {
                if (keyEventPostProcessors == null) {
                    keyEventPostProcessors = new java.util.LinkedList();
                }
                keyEventPostProcessors.add(processor);
            }
        }
    }

    public void removeKeyEventPostProcessor(KeyEventPostProcessor processor) {
        if (processor != null) {
            synchronized (this) {
                if (keyEventPostProcessors != null) {
                    keyEventPostProcessors.remove(processor);
                }
            }
        }
    }

    java.util.List getKeyEventPostProcessors() {
        return (keyEventPostProcessors != null)
                ? (java.util.List) keyEventPostProcessors.clone()
                : null;
    }

    static void setMostRecentFocusOwner(Component component) {
        Component window = component;
        while (window != null && !(window instanceof Window)) {
            window = window.parent;
        }
        if (window != null) {
            setMostRecentFocusOwner((Window) window, component);
        }
    }

    static synchronized void setMostRecentFocusOwner(Window window,
            Component component) {
        WeakReference weakValue = null;
        if (component != null) {
            weakValue = new WeakReference(component);
        }
        mostRecentFocusOwners.put(window, weakValue);
    }

    static void clearMostRecentFocusOwner(Component comp) {
        Container window;
        if (comp == null) {
            return;
        }
        synchronized (comp.getTreeLock()) {
            window = comp.getParent();
            while (window != null && !(window instanceof Window)) {
                window = window.getParent();
            }
        }
        synchronized (KeyboardFocusManager.class) {
            if ((window != null)
                    && (getMostRecentFocusOwner((Window) window) == comp)) {
                setMostRecentFocusOwner((Window) window, null);
            }
            if (window != null) {
                Window realWindow = (Window) window;
                if (realWindow.getTemporaryLostComponent() == comp) {
                    realWindow.setTemporaryLostComponent(null);
                }
            }
        }
    }

    static synchronized Component getMostRecentFocusOwner(Window window) {
        WeakReference weakValue = (WeakReference) mostRecentFocusOwners.get(window);
        return weakValue == null ? null : (Component) weakValue.get();
    }

    public abstract boolean dispatchEvent(AWTEvent e);

    public final void redispatchEvent(Component target, AWTEvent e) {
        e.focusManagerIsDispatching = true;
        target.dispatchEvent(e);
        e.focusManagerIsDispatching = false;
    }

    public abstract boolean dispatchKeyEvent(KeyEvent e);

    public abstract boolean postProcessKeyEvent(KeyEvent e);

    public abstract void processKeyEvent(Component focusedComponent, KeyEvent e);

    public abstract void focusNextComponent(Component aComponent);

    public abstract void focusPreviousComponent(Component aComponent);

    public abstract void upFocusCycle(Component aComponent);

    public abstract void downFocusCycle(Container aContainer);

    abstract void enqueueKeyEvents(long after, Component untilFocused);

    abstract void dequeueKeyEvents(long after, Component untilFocused);

    abstract void discardKeyEvents(Component comp);

    public final void focusNextComponent() {
        Component focusOwner = getFocusOwner();
        if (focusOwner != null) {
            focusNextComponent(focusOwner);
        }
    }

    public final void focusPreviousComponent() {
        Component focusOwner = getFocusOwner();
        if (focusOwner != null) {
            focusPreviousComponent(focusOwner);
        }
    }

    public final void upFocusCycle() {
        Component focusOwner = getFocusOwner();
        if (focusOwner != null) {
            upFocusCycle(focusOwner);
        }
    }

    public final void downFocusCycle() {
        Component focusOwner = getFocusOwner();
        if (focusOwner instanceof Container) {
            downFocusCycle((Container) focusOwner);
        }
    }
}

