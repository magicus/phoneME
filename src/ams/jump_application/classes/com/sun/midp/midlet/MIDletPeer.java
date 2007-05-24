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

import javax.microedition.io.ConnectionNotFoundException;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;


/**
 * MIDletPeer forwards updates
 * to a MIDlet. It contains references to the MIDlet itself and to its
 * corresponding.  Control methods (startApp, destroyApp,
 * pauseApp) defined here are invoked on the MIDlet object via the
 * MIDletTunnel.
 * <p>
 * All state changes are synchronized using midletStateHandler retrieved
 * from the MIDletStateHandler.
 * NotifyPaused, ResumeRequest, and NotifyDestroyed methods invoked on the
 * MIDlet cause the appropriate state change.  The MIDletStateHandler is aware
 * of changes by waiting on the midletStateHandler.
 */

public class MIDletPeer {
    /** The controller of MIDlets. */
    private static MIDletStateHandler midletStateHandler;

    /** Handles platform requests. */
    private static PlatformRequest platformRequest;

    /** The MIDletTunnel implementation from javax.microedition.midlet */
    private static MIDletTunnel tunnel;

    /**
     * Initialize the MIDletPeer class. Should only be called by the
     * MIDletPeerList (MIDletStateHandler).
     *
     * @param theMIDletStateHandler the midlet state handler
     * @param theMIDletStateListener the midlet state listener
     * @param thePlatformRequestHandler the platform request handler
     */
    static void initClass(
        MIDletStateHandler theMIDletStateHandler,
        PlatformRequest thePlatformRequestHandler) {

        midletStateHandler = theMIDletStateHandler;
        platformRequest = thePlatformRequestHandler;
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
     * @param m the midlet instance
     *
     * @return MIDletPeer instance associate with m
     */
    static MIDletPeer getMIDletPeer(MIDlet m) {
        return tunnel.getMIDletPeer(m);
    }

    /**
     * The MIDlet for which this is the peer.
     */
    protected MIDlet midlet;

    /**
     * Creates a MIDlet's peer which is registered the MIDletStateHandler.
     * Shall be called only from MIDletStateHandler.
     * <p>
     * The peer MIDlet field is set later when the MIDlet's constructor calls
     * newMidletState.
     */
    MIDletPeer() {
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
        midletStateHandler.midletDestroyed();
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
        midletStateHandler.midletPaused();
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
        midletStateHandler.resumeRequest();
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

        return platformRequest.dispatch(URL);
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
}
