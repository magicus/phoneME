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

import sun.awt.SunToolkit;
import sun.awt.AppContext;

public abstract class KeyboardFocusManager implements KeyEventDispatcher, 
                                                      KeyEventPostProcessor {
    public static final int FORWARD_TRAVERSAL_KEYS = 0;
    public static final int BACKWARD_TRAVERSAL_KEYS = 1;
    public static final int UP_CYCLE_TRAVERSAL_KEYS = 2;
    public static final int DOWN_CYCLE_TRAVERSAL_KEYS = 3;
    static final int TRAVERSAL_KEY_LENGTH = DOWN_CYCLE_TRAVERSAL_KEYS + 1;
    static Component focusOwner;
    private static Component permanentFocusOwner;
    private static Window focusedWindow;
    private static Window activeWindow=null;
    private FocusTraversalPolicy defaultPolicy = 
        new DefaultFocusTraversalPolicy();
    private static final String[] defaultFocusTraversalKeyPropertyNames = {
        "forwardDefaultFocusTraversalKeys", 
        "backwardDefaultFocusTraversalKeys",
        "upCycleDefaultFocusTraversalKeys", 
        "downCycleDefaultFocusTraversalKeys"
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
    private static final String notPrivileged = 
    "this KeyboardFocusManager is not installed in the current thread's context";
    private static AWTPermission replaceKeyboardFocusManagerPermission;
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

    static void postSyntheticEvent(AWTEvent e) {
        SunToolkit.postEvent(((Component)e.getSource()).appContext, e);
    }

    static void sendSyntheticEvent(AWTEvent e) {
        Component target = (Component)e.getSource();
        final AppContext targetAppContext = target.appContext;
        final SentEvent se = new SentEvent(e, AppContext.getAppContext());
        
        sendMessage(targetAppContext, se);
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
    static boolean sendMessage(final AppContext targetAppContext, 
                               final SentEvent se) {
        AppContext myAppContext = AppContext.getAppContext();
        if (myAppContext == targetAppContext) {
             se.dispatch();
         } 
         else {
            if (targetAppContext.isDisposed()) {
                return false;
            }
            SunToolkit.postEvent(targetAppContext, se);
            if (EventQueue.isDispatchThread()) {
                EventDispatchThread edt = (EventDispatchThread)
                    Thread.currentThread();
                edt.pumpEvents(SentEvent.ID, new Conditional() {
                        public boolean evaluate() {
                            return !se.dispatched && 
                                !targetAppContext.isDisposed();
                        }
                    });
            } else {
                synchronized (se) {
                    while (!se.dispatched && !targetAppContext.isDisposed()) {
                        try {
                            se.wait(1000);
                        } catch (InterruptedException ie) {
                            break;
                        }
                    }
                }
            }
        }
        return se.dispatched;
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

    private synchronized static void setCurrentKeyboardFocusManager(
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
        if (focusOwner != null) {
            FocusEvent synthetic = new FocusEvent(focusOwner, 
                                                  FocusEvent.FOCUS_LOST,
                                                  false, null);
            // The DKFM.dispatchEvent() while handling this event will
            // set the GlobalFocusOwner, setGlobalPermanentFocusOwner to
            // null if required. We "send" an event (instead of "post"ing)
            // so that the event is handled by DKFM before we exit this
            // method.
            sendSyntheticEvent(synthetic);
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
        Window oldActiveWindow;
        synchronized (KeyboardFocusManager.class) {
	    if ((activeWindow != null) && (!activeWindow.isFocusableWindow()))
	        return;
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
