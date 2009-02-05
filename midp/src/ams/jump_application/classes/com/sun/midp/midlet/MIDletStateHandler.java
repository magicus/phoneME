/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.midlet;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;

/**
 * This MIDletStateHandler is a synchronous class that simply forwards
 * central AMS actions to a single MIDlet running in isolation.
 * <p>
 * The MIDletStateHandler is a singleton for the suite being run and
 * is retrieved with getMIDletStateHandler(). This allow the
 * MIDletStateHandler to be the anchor of trust internally for the MIDP API,
 * restricted methods can obtain the MIDletStateHandler for a MIDlet suite
 * inorder to check the properties and actions of a suite.
 * Because of this, there MUST only be one a MIDlet suite per
 * MIDletStateHandler.
 * <p>
 * The MIDlet methods are protected in the javax.microedition.midlet package
 * so the MIDletStateHandler can not call them directly.  The MIDletState
 * object and
 * MIDletTunnel subclass class allow the MIDletStateHandler to hold the state
 * of the
 * MIDlet and to invoke methods on it.  The MIDletState instance is created
 * by the MIDlet when it is constructed.
 * <p>
 * When a MIDlet changes its or requests to change its state to
 * the MIDlet state listener is notified of the change,
 * which in turn sends the notification onto the central AMS.
 * <p>
 * When a MIDlet state is changed by the central AMS there is no call to a
 * listener, since this is synchronous handler.
 *
 * @see MIDlet
 * @see MIDletPeer
 * @see MIDletLoader
 * @see MIDletStateHandler
 */

public class MIDletStateHandler {
    /** The event handler of all MIDlets in an Isolate. */
    private static MIDletStateHandler stateHandler;
    /** The listener for the state of all MIDlets in an Isolate. */
    private static MIDletStateListener listener;
    /** Serializes the creation of MIDlets. */
    private static Object midletLock = new Object();
    /** New MIDlet peer waiting for the next MIDlet created to claim it. */
    private static MIDletPeer newMidletPeer;

    /**
     * Gets the MIDletStateHandler that manages the lifecycle states of
     * MIDlets running in an Isolate.
     * <p>
     * If the instance of the MIDletStateHandler has already been created
     * it is returned.  If not it is created.
     * The instance becomes the MIDletStateHandler for this suite.
     * <p>
     * The fact that there is one handler per Isolate
     * is a security feature. Also a security feature, is that
     * getMIDletStateHandler is
     * static, so API can find out what suite is calling, if in the future
     * multiple suites can be run in the same VM, the MIDlet state handler
     * for each suite
     * should be loaded in a different classloader or have some other way
     * having multiple instances of static class data.
     *
     * @return the MIDlet state handler for this Isolate
     */
    public static synchronized MIDletStateHandler getMidletStateHandler() {
        /*
         * If the midlet state handler has not been created, create one now.
         */
        if (stateHandler == null) {
            /* This is the default scheduler class */
            stateHandler = new MIDletStateHandler();
        }

        return stateHandler;
    }

    /**
     * Called by the MIDlet constructor to a new MIDletPeer object.
     *
     * @param token security token for authorizing the caller
     * @param m the MIDlet for which this state is being created;
     *          must not be <code>null</code>.
     * @return the preallocated MIDletPeer for the MIDlet being constructed by
     *         {@link #createMIDlet}
     *
     * @exception SecurityException AMS permission is not granted and
     * if is constructor is not being called in the context of
     * <code>createMIDlet</code>.
     */
    public static MIDletPeer newMIDletPeer(SecurityToken token, MIDlet m) {
        token.checkIfPermissionAllowed(Permissions.MIDP);

        synchronized (midletLock) {
            MIDletPeer temp;

            if (newMidletPeer == null) {
                throw new SecurityException(
                    "MIDlet not constructed by createMIDlet.");
            }

            temp = newMidletPeer;
            newMidletPeer = null;
            temp.midlet = m;
            return temp;
        }
    }

    /** the current MIDlet suite. */
    private MIDletSuite midletSuite;
    /** loads the MIDlets from a suite's JAR in a VM specific way. */
    private MIDletLoader midletLoader;
    /** Forwards actions to the MIDlet. */
    private MIDletPeer midlet;
    /** True when a MIDlet is first being started. */
    private boolean midletStarting;
    /**
     * True when a MIDlet has called notifyDestroyed during it initial
     * startApp call.
     */
    private boolean midletDestroyedDuringStart;

    /**
     * Construct a new MIDletStateHandler object.
     */
    private MIDletStateHandler() {
    }

    /**
     * Initializes MIDlet State Handler.
     *
     * @param token security token for initilaization
     * @param theMIDletStateListener processes MIDlet states in a
     *                               VM specific way
     * @param theMidletLoader loads a MIDlet in a VM specific way
     * @param thePlatformRequestHandler the platform request handler
     */
    public void initMIDletStateHandler(
        SecurityToken token,
        MIDletStateListener theMIDletStateListener,
        MIDletLoader theMidletLoader,
        PlatformRequest thePlatformRequestHandler) {

        token.checkIfPermissionAllowed(Permissions.AMS);

        listener = theMIDletStateListener;
        midletLoader = theMidletLoader;

        MIDletPeer.initClass(this, thePlatformRequestHandler);
    }

    /**
     * Loads and starts a given MIDlet in a suite.
     * <p>
     * MIDlets are created using their no-arg constructor. Once created
     * a MIDlet is sequenced to the <code>ACTIVE</code> state.
     * <p>
     * If a Runtime exception occurs during <code>startApp</code> the
     * MIDlet will be destroyed immediately.  Its <code>destroyApp</code>
     * will be called allowing the MIDlet to cleanup.
     *
     * @param aMidletSuite the current midlet suite
     * @param classname name of MIDlet class
     *
     * @exception ClassNotFoundException if the MIDlet main class is
     * not found
     * @exception InstantiationException if the MIDlet can not be
     * created
     * @exception IllegalAccessException if the MIDlet is not
     * permitted to perform a specific operation
     * @exception IllegalStateException if a suite is already running
     * @exception MIDletStateChangeException  is thrown
     * if the <code>MIDlet</code>
     *		cannot start now but might be able to start at a
     *		later time.
     */
    public void startSuite(
            MIDletSuite aMidletSuite, String classname)
            throws ClassNotFoundException, InstantiationException,
            IllegalAccessException, MIDletStateChangeException {
        synchronized (midletLock) {
            if (midletSuite != null) {
                throw new IllegalStateException(
                    "There is already a MIDlet Suite running.");

            }

            midletSuite = aMidletSuite;
            midletStarting = true;
            midlet = createMIDlet(classname);

            try {
                midlet.startApp();
                midletStarting = false;
                if (midletDestroyedDuringStart) {
                    throw new MIDletStateChangeException();
                }
            } finally {
                if (midletStarting) {
                    /*
                     * MIDlet did not start, don't stop the exception,
                     * if any case, just cleanup
                     */
                    try {
                        midlet.destroyApp(true);
                    } catch (Throwable e) {
                        // Ignore
                    }
                }
            }
        }
    }

    /**
     * Forwards the startApp to the MIDlet.
     *
     * @exception <code>MIDletStateChangeException</code>  is thrown if the
     *                <code>MIDlet</code> cannot start now but might be able
     *                to start at a later time.
     */
    public void resumeApp() throws MIDletStateChangeException {
        synchronized (midletLock) {
            if (midlet == null) {
                return;
            }

            midlet.startApp();
        }
    }

    /**
     * Forwards pauseApp to the MIDlet.
     */
    public void pauseApp() {
        synchronized (midletLock) {
            if (midlet == null) {
                return;
            }

            midlet.pauseApp();
        }
    }

    /**
     * Forwards destoryApp to the MIDlet.
     *
     * @param unconditional the flag to pass to destroy
     *
     * @exception <code>MIDletStateChangeException</code> is thrown
     *                if the <code>MIDlet</code>
     *          wishes to continue to execute (Not enter the <i>Destroyed</i>
     *          state).
     */
    public void destroyApp(boolean unconditional)
            throws MIDletStateChangeException {
        synchronized (midletLock) {
            if (midlet == null) {
                return;
            }

            midlet.destroyApp(unconditional);
        }
    }

    /**
     * Provides a object with a mechanism to retrieve
     * <code>MIDletSuite</code> being run.
     *
     * @return MIDletSuite being run
     */
    public MIDletSuite getMIDletSuite() {
        return midletSuite;
    }

    /**
     *
     * Used by a <code>MIDletPeer</code> to notify the application management
     * software that it has entered into the
     * <i>DESTROYED</i> state.  The application management software will not
     * call the MIDlet's <code>destroyApp</code> method, and all resources
     * held by the <code>MIDlet</code> will be considered eligible for
     * reclamation.
     * The <code>MIDlet</code> must have performed the same operations
     * (clean up, releasing of resources etc.) it would have if the
     * <code>MIDlet.destroyApp()</code> had been called.
     *
     */
    void midletDestroyed() {
        if (midletStarting) {
            midletDestroyedDuringStart = true;
        } else {
            listener.midletDestroyed(getMIDletSuite(),
                midlet.getMIDlet().getClass().getName(), midlet.getMIDlet());
        }
    }

    /**
     * Used by a <code>MIDlet</code> to notify the application management
     * software that it has entered into the <i>PAUSED</i> state.
     * Invoking this method will
     * have no effect if the <code>MIDlet</code> is destroyed,
     * or if it has not yet been started. <p>
     * It may be invoked by the <code>MIDlet</code> when it is in the
     * <i>ACTIVE</i> state. <p>
     *
     * If a <code>MIDlet</code> calls <code>notifyPaused()</code>, in the
     * future its <code>startApp()</code> method may be called make
     * it active again, or its <code>destroyApp()</code> method may be
     * called to request it to destroy itself.
     */
    void midletPaused() {
        if (!midletStarting) {
            listener.midletPausedItself(getMIDletSuite(),
                midlet.getMIDlet().getClass().getName());
        }
    }

    /**
     * Used by a <code>MIDlet</code> to notify the application management
     * software that it is
     * interested in entering the <i>ACTIVE</i> state. Calls to
     * this method can be used by the application management software to
     * determine which applications to move to the <i>ACTIVE</i> state.
     * <p>
     * When the application management software decides to activate this
     * application it will call the <code>startApp</code> method.
     * <p> The application is generally in the <i>PAUSED</i> state when this is
     * called.  Even in the paused state the application may handle
     * asynchronous events such as timers or callbacks.
     */
    void resumeRequest() {
        listener.resumeRequest(getMIDletSuite(),
                               midlet.getMIDlet().getClass().getName());
    }

    /**
     * Creates a MIDlet.
     *
     * @param classname name of MIDlet class
     *
     * @return newly created MIDlet
     *
     * @exception ClassNotFoundException if the MIDlet class is
     * not found
     * @exception InstantiationException if the MIDlet cannot be
     * created
     * @exception IllegalAccessException if the MIDlet is not
     * permitted to perform a specific operation
     */
    private MIDletPeer createMIDlet(String classname) throws
            ClassNotFoundException, InstantiationException,
            IllegalAccessException {

        MIDletPeer result;

        /*
         * Just in case there is a hole we have not found.
         * Make sure there is not a new MIDlet state already created.
         */
        if (newMidletPeer != null) {
            throw new SecurityException("Recursive MIDlet creation");
        }

        newMidletPeer = new MIDletPeer();

        // if newInstance is sucessful newMidletPeer will be null
        result = newMidletPeer;
        try {
            midletLoader.newInstance(getMIDletSuite(), classname);
        } finally {
            /* Make sure the creation window is closed. */
            newMidletPeer = null;
        }
        
        return result;
    }
}
