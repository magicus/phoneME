/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.IOException;

import javax.microedition.io.Connection;
import javax.microedition.io.ConnectionNotFoundException;
import javax.microedition.io.Connector;
import javax.microedition.io.HttpConnection;

import javax.microedition.lcdui.Display;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

import com.sun.midp.io.Util;

import com.sun.midp.lcdui.DisplayAccess;
import com.sun.midp.lcdui.DisplayEventHandler;

import com.sun.midp.main.Configuration;
import com.sun.midp.main.MIDletSuiteLoader;
import com.sun.midp.main.MIDletControllerEventProducer;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;

import com.sun.midp.configurator.Constants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * MIDletPeer maintains the current state of the MIDlet and forwards updates
 * to it.  It contains references to the MIDlet itself and to its
 * corresponding Display instance.  Control methods (startApp, destroyApp,
 * pauseApp) defined here are invoked on the MIDlet object via the
 * MIDletTunnel.
 * <p>
 * All state changes are synchronized using midletStateHandler retrieved
 * from the MIDletStateHandler.
 * NotifyPaused, ResumeRequest, and NotifyDestroyed methods invoked on the
 * MIDlet cause the appropriate state change.  The MIDletStateHandler is aware
 * of changes by waiting on the midletStateHandler.
 */

public class MIDletPeer implements MIDletEventConsumer {

    /** Class name of the installer use for plaformRequest. */
    static final String INSTALLER_CLASS =
        "com.sun.midp.installer.GraphicalInstaller";

    /** Media-Type for valid application descriptor files. */
    static final String JAD_MT = "text/vnd.sun.j2me.app-descriptor";

    /** Media-Type for valid Jar file. */
    static final String JAR_MT_1 = "application/java";

    /** Media-Type for valid Jar file. */
    static final String JAR_MT_2 = "application/java-archive";

    /*
     * Implementation state; the states are in priority order.
     * That is, a higher number indicates a preference to be
     * selected for activating sooner.  This allows the MIDlet state handler
     * to make one pass over the known MIDlets and pick the
     * "best" MIDlet to activate.
     */

    /**
     * State of the MIDlet is Paused; it should be quiescent
     */
    static final int PAUSED = 0;

    /**
     * State of the MIDlet is Active
     */
    static final int ACTIVE = 1;

    /**
     * State of the MIDlet when resumed by the display manager
     */
    static final int ACTIVE_PENDING = 2;

    /**
     * State of the MIDlet when paused by the display manager
     */
    static final int PAUSE_PENDING = 3;

    /**
     * State of the MIDlet with destroy pending
     */
    static final int DESTROY_PENDING = 4;

    /**
     * State of the MIDlet is Destroyed
     */
    static final int DESTROYED = 5;

    /** This class has a different security domain than the application. */
    private static SecurityToken classSecurityToken;

    /** Cached reference to the MIDletControllerEventProducer. */
    private static MIDletControllerEventProducer midletControllerEventProducer;

    /** The controller of MIDlets. */
    private static MIDletStateHandler midletStateHandler;

    /**
     * The controller of Displays.
     */
    private static DisplayEventHandler displayEventHandler;

    /** The MIDletTunnel implementation from javax.microedition.midlet */
    private static MIDletTunnel tunnel;

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class
     */
    public static void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }

    /**
     * Initialize the MIDletPeer class. Should only be called by the
     * MIDletPeerList (MIDletStateHandler).
     *
     * @param token security token for initilaization
     * @param  theDisplayEventHandler the display event handler
     * @param  theMIDletStateHandler the midlet state handler
     * @param theMIDletControllerEventProducer event producer
     */
    public static void initClass(
        SecurityToken token,
        DisplayEventHandler theDisplayEventHandler,
        MIDletStateHandler theMIDletStateHandler,
        MIDletControllerEventProducer theMIDletControllerEventProducer) {

        token.checkIfPermissionAllowed(Permissions.MIDP);

        displayEventHandler = theDisplayEventHandler;
        midletStateHandler = theMIDletStateHandler;
        midletControllerEventProducer = theMIDletControllerEventProducer;
    }
    /**
     * Sets up the reference to the MIDletTunnel implementation.
     * This must be called exactly once during system initialization.
     *
     * @param token security token for authorizing the caller
     * @param t the MIDletTunnel implementation
     */
    public static void setMIDletTunnel(SecurityToken token, MIDletTunnel t) {
        token.checkIfPermissionAllowed(Permissions.MIDP);

        tunnel = t;
    }

    /**
     * Returns the MIDletPeer object corresponding to the given
     * midlet instance.
     *
     * @param token security token for authorizing the caller
     * @param m the midlet instance
     *
     * @return MIDletPeer instance associate with m
     */
    public static MIDletPeer getMIDletPeer(SecurityToken token, MIDlet m) {
        token.checkIfPermissionAllowed(Permissions.MIDP);

        return tunnel.getMIDletPeer(m);
    }

    /**
     * The applications current state.
     */
    private int state;

    /**
     * The DisplayAccessor for this MIDlet.
     */
    protected DisplayAccess accessor;

    /**
     * The Display for this MIDlet.
     */
    protected Display display;

    /**
     * ID of the Display for this MIDlet.
     */
    protected int displayId;

    /**
     * The MIDlet for which this is the state.
     */
    protected MIDlet midlet;

    /**
     * Creates a MIDlet's peer which is registered the MIDletStateHandler
     * and DisplayEventHandler.
     * Shall be called only from MIDletStateHandler.
     *
     * The peer MIDlet field is set later when the MIDlet's constructor calls
     * newMidletState.
     */
    MIDletPeer() {
        state = ACTIVE_PENDING;        // So it will be made active soon

        // Force the creation of the Display
        accessor = displayEventHandler.createDisplay(classSecurityToken, this);
        display = accessor.getDisplay();
        displayId = accessor.getDisplayId();

        if (midletStateHandler.getMIDletSuite().isTrusted()) {
            accessor.setTrustedIcon(classSecurityToken, true);
        }
    }

    /**
     * Get the MIDlet for which this holds the state.
     *
     * @return the MIDlet; will not be null.
     */
    public MIDlet getMIDlet() {
        return midlet;
    }

    /**
     * Get the Display for this MIDlet.
     *
     * @return the Display of this MIDlet.
     */
    public Display getDisplay() {
        return display;
    }

    /**
     * Forwards startApp to the MIDlet.
     *
     * @exception <code>MIDletStateChangeException</code>  is thrown if the
     *                <code>MIDlet</code> cannot start now but might be able
     *                to start at a later time.
     */
    void startApp() throws MIDletStateChangeException {
        tunnel.callStartApp(midlet);
    }

    /**
     * Forwards pauseApp to the MIDlet.
     *
     */
    void pauseApp() {
        tunnel.callPauseApp(midlet);
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
     *          This exception is ignored if <code>unconditional</code>
     *          is equal to <code>true</code>.
     */
    void destroyApp(boolean unconditional)
        throws MIDletStateChangeException {
        tunnel.callDestroyApp(midlet, unconditional);
    }

    /**
     *
     * Used by a <code>MIDlet</code> to notify the application management
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
    public final void notifyDestroyed() {
        synchronized (midletStateHandler) {
            state = DESTROYED;
            midletStateHandler.notify();
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
    public final void notifyPaused() {
        int oldState;

        synchronized (midletStateHandler) {
            oldState = state;

            /*
             * do not notify the midletStateHandler,
             * since there is nothing to do
             */
            setStateWithoutNotify(PAUSED);
        }

        // do work after releasing the lock
        if (oldState == ACTIVE) {
            midletControllerEventProducer.sendMIDletPauseNotifyEvent(
                displayId);
        }
    }

    /**
     * Provides a <code>MIDlet</code> with a mechanism to retrieve
     * <code>MIDletSuite</code> for this MIDlet.
     *
     * @return MIDletSuite for this MIDlet
     */
    public final MIDletSuite getMIDletSuite() {
        return midletStateHandler.getMIDletSuite();
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

    public final void resumeRequest() {
        midletControllerEventProducer.sendMIDletResumeRequest(displayId);
    }

    /**
     * Requests that the device handle (e.g. display or install)
     * the indicated URL.
     *
     * <p>If the platform has the appropriate capabilities and
     * resources available, it SHOULD bring the appropriate
     * application to the foreground and let the user interact with
     * the content, while keeping the MIDlet suite running in the
     * background. If the platform does not have appropriate
     * capabilities or resources available, it MAY wait to handle the
     * URL request until after the MIDlet suite exits. In this case,
     * when the requesting MIDlet suite exits, the platform MUST then
     * bring the appropriate application to the foreground to let the
     * user interact with the content.</p>
     *
     * <p>This is a non-blocking method. In addition, this method does
     * NOT queue multiple requests. On platforms where the MIDlet
     * suite must exit before the request is handled, the platform
     * MUST handle only the last request made. On platforms where the
     * MIDlet suite and the request can be handled concurrently, each
     * request that the MIDlet suite makes MUST be passed to the
     * platform software for handling in a timely fashion.</p>
     *
     * <p>If the URL specified refers to a MIDlet suite (either an
     * Application Descriptor or a JAR file), the request is
     * interpreted as a request to install the named package. In this
     * case, the platform's normal MIDlet suite installation process
     * SHOULD be used, and the user MUST be allowed to control the
     * process (including cancelling the download and/or
     * installation). If the MIDlet suite being installed is an
     * <em>update</em> of the currently running MIDlet suite, the
     * platform MUST first stop the currently running MIDlet suite
     * before performing the update. On some platforms, the currently
     * running MIDlet suite MAY need to be stopped before any
     * installations can occur.</p>
     *
     * <p>If the URL specified is of the form
     * <code>tel:&lt;number&gt;</code>, as specified in <a
     * href="http://rfc.net/rfc2806.html">RFC2806</a>, then the
     * platform MUST interpret this as a request to initiate a voice
     * call. The request MUST be passed to the &quot;phone&quot;
     * application to handle if one is present in the platform.</p>
     *
     * <p>Devices MAY choose to support additional URL schemes beyond
     * the requirements listed above.</p>
     *
     * <p>Many of the ways this method will be used could have a
     * financial impact to the user (e.g. transferring data through a
     * wireless network, or initiating a voice call). Therefore the
     * platform MUST ask the user to explicitly acknowledge each
     * request before the action is taken. Implementation freedoms are
     * possible so that a pleasant user experience is retained. For
     * example, some platforms may put up a dialog for each request
     * asking the user for permission, while other platforms may
     * launch the appropriate application and populate the URL or
     * phone number fields, but not take the action until the user
     * explicitly clicks the load or dial buttons.</p>
     *
     * @return true if the MIDlet suite MUST first exit before the
     * content can be fetched.
     *
     * @param URL The URL for the platform to load.
     *
     * @exception ConnectionNotFoundException if
     * the platform cannot handle the URL requested.
     *
     */
    public final boolean platformRequest(String URL)
            throws ConnectionNotFoundException {
        if ("".equals(URL)) {
            if (Configuration.getIntProperty(
                    "useJavaInstallerForPlaformRequest", 0) != 0) {
                /*
                 * This is request to try to cancel the last request.
                 *
                 * If the next MIDlet to run is the installer then it can be
                 * cancelled.
                 */
                if (INSTALLER_CLASS.equals(
                    MIDletSuiteLoader.getNextMIDletToRun())) {
                    /*
                     * Try to cancel the installer midlet. Note this call only
                     * works now because suite are not run concurrently and
                     * must be queued to be run after this MIDlet is
                     * destroyed.
                     * This cancel code can be remove when the installer is
                     * runs concurrently with this suite.
                     */
                    MIDletSuiteLoader.execute(classSecurityToken, null,
                                              null, null);
                    return false;
                }
            }

            /*
             * Give the platform a chance to cancel the request.
             * Note: if the application was launched already this will
             * not have any effect.
             */
            dispatchPlatformRequest("");
            return false;
        }

        /*
         * Remove this "if", when not using the Installer MIDlet,
         * or the native installer will not be launched.
         */
        if (Configuration.getIntProperty(
                "useJavaInstallerForPlaformRequest", 0) != 0) {
            if (isMidletSuiteUrl(URL)) {
                return dispatchMidletSuiteUrl(URL);
            }
        }

        return dispatchPlatformRequest(URL);
    }

    /**
     * Find out if the given URL is a JAD or JAR HTTP URL by performing a
     * HTTP head request and checking the MIME type.
     *
     * @param url The URL for to check
     *
     * @return true if the URL points to a MIDlet suite
     */

    private boolean isMidletSuiteUrl(String url) {
        Connection conn = null;
        HttpConnection httpConnection = null;
        String profile;
        int space;
        String configuration;
        String locale;
        int responseCode;
        String mediaType;

        try {
            conn = Connector.open(url, Connector.READ);
        } catch (IllegalArgumentException e) {
            return false;
        } catch (IOException e) {
            return false;
        }

        try {
            if (!(conn instanceof HttpConnection)) {
                // only HTTP or HTTPS are supported
                return false;
            }

            httpConnection = (HttpConnection)conn;

            httpConnection.setRequestMethod(HttpConnection.HEAD);

            httpConnection.setRequestProperty("Accept", "*/*");

            profile = System.getProperty("microedition.profiles");
            space = profile.indexOf(' ');
            if (space != -1) {
                profile = profile.substring(0, space);
            }

            configuration = System.getProperty("microedition.configuration");
            httpConnection.setRequestProperty("User-Agent",
                "Profile/" + profile + " Configuration/" + configuration);

            httpConnection.setRequestProperty("Accept-Charset",
                                              "UTF-8, ISO-8859-1");

            /* locale can be null */
            locale = System.getProperty("microedition.locale");
            if (locale != null) {
                httpConnection.setRequestProperty("Accept-Language", locale);
            }

            responseCode = httpConnection.getResponseCode();

            if (responseCode != HttpConnection.HTTP_OK) {
                return false;
            }

            mediaType = Util.getHttpMediaType(httpConnection.getType());
            if (mediaType == null) {
                return false;
            }

            if (mediaType.equals(JAD_MT) || mediaType.equals(JAR_MT_1) ||
                    mediaType.equals(JAR_MT_2)) {
                return true;
            }

            return false;
        } catch (IOException ioe) {
            return false;
        } finally {
            try {
                conn.close();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                                  "Exception while closing  connection");
                }
            }
        }
    }

    /**
     * Dispatches the a JAD or JAD HTTP URL to the Graphical Installer.
     *
     * @param url The URL to dispatch
     *
     * @return true if the MIDlet suite MUST first exit before the
     * content can be fetched.
     */
    private boolean dispatchMidletSuiteUrl(String url) {
        return MIDletSuiteLoader.executeWithArgs(classSecurityToken,
            "internal", INSTALLER_CLASS, "MIDlet Suite Installer", "I",
            url, null);
    }

    /**
     * Passes the URL to the native handler.
     *
     * @param url The URL for the platform to load.
     *
     * @return true if the MIDlet suite MUST first exit before the
     * content can be fetched.
     *
     * @exception ConnectionNotFoundException if
     * the platform cannot handle the URL requested.
     */
    public native final boolean dispatchPlatformRequest(String url) throws
        ConnectionNotFoundException;

    /**
     * Change the state and notify.
     * Check to make sure the new state makes sense.
     * Changes to the status are protected by the midletStateHandler.
     * Any change to the state notifies the midletStateHandler.
     *
     * @param newState new state of the MIDlet
     */
    void setState(int newState) {
        synchronized (midletStateHandler) {
            setStateWithoutNotify(newState);
            midletStateHandler.notify();
        }
    }

    /**
     * Get the status of the specified permission.
     * If no API on the device defines the specific permission
     * requested then it must be reported as denied.
     * If the status of the permission is not known because it might
     * require a user interaction then it should be reported as unknown.
     *
     * @param permission to check if denied, allowed, or unknown.
     * @return 0 if the permission is denied; 1 if the permission is allowed;
     *         -1 if the status is unknown
     */
    public int checkPermission(String permission) {
        return getMIDletSuite().checkPermission(permission);
    }

    /**
     * Change the state without notifying the MIDletStateHandler.
     * Check to make sure the new state makes sense.
     * <p>
     * To be called only by the MIDletStateHandler or MIDletState while holding
     * the lock on midletStateHandler.
     *
     * @param newState new state of the MIDlet
     */
    void setStateWithoutNotify(int newState) {
        switch (state) {
        case DESTROYED:
            // can't set any thing else
            return;

        case DESTROY_PENDING:
            if (newState != DESTROYED) {
                // can only set DESTROYED
                return;
            }

            break;

        case PAUSED:
            if (newState == PAUSE_PENDING) {
                // already paused by app
                return;
            }

            break;

        case PAUSE_PENDING:
            if (newState == ACTIVE_PENDING) {
                /*
                 * pausedApp has not been called so the state
                 * can be set to active to cancel the pending pauseApp.
                 */
                state = ACTIVE;
                return;
            }

            break;

        case ACTIVE:
            if (newState == ACTIVE_PENDING) {
                // already active
                return;
            }

            break;

        case ACTIVE_PENDING:
            if (newState == PAUSE_PENDING) {
                /*
                 * startApp has not been called so the state
                 * can be set to paused to cancel the pending startApp.
                 */
                state = PAUSED;
                return;
            }

            break;
        }

        state = newState;
    }

    /**
     * Get the state.
     *
     * @return current state of the MIDlet.
     */
    int getState() {
        synchronized (midletStateHandler) {
            return state;
        }
    }

    /**
     * Pause a MIDlet.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletPauseEvent() {
        setState(MIDletPeer.PAUSE_PENDING);
    }

    /**
     * Activate a MIDlet.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletActivateEvent() {
        setState(MIDletPeer.ACTIVE_PENDING);
    }

    /**
     * Destroy a MIDlet.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletDestroyEvent() {
        setState(MIDletPeer.DESTROY_PENDING);
    }

}
