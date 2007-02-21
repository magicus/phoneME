/*
 *
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

package com.sun.midp.suspend;

import com.sun.midp.main.*;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;
import com.sun.midp.lcdui.DisplayEventHandlerFactory;
import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.SystemAlert;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import javax.microedition.lcdui.AlertType;
import java.util.Vector;

/**
 * Main system of the current isolate that contains all 
 * pausable subsystems in current isolate. 
 * There is a singleton instance in each isolate. The 
 * instance kept in the AMS isolate is a special one and 
 * belongs to <code>MIDPSystem</code> subtype.
 */
public class SuspendSystem extends AbstractSubsystem {
    /**
     * Listeners interested in suspend/resume operations.
     */
    private final Vector listeners = new Vector(1, 2);

    /**
     * Main subsystem that implements suspend actions for
     * whole MIDP system. It works in the AMS Isolate. There
     * is the only instance for all isolates.
     */
    private static class MIDPSystem extends SuspendSystem
            implements MIDletProxyListListener {
        /**
         * A flag to determine if at least one MIDlet has been
         * destroyed during last suspend processing.
         */
        private boolean midletKilled = false;

        /**
         * A flag to determine if at least one MIDlet has been
         * successfully paused during last suspend processing.
         */
        private boolean midletPaused = false;

        /**
         * The MIDlet proxy list.
         */
        MIDletProxyList mpl =
                MIDletProxyList.getMIDletProxyList(classSecurityToken);

        /**
         * Constructs the only instance.
         */
        private MIDPSystem() {
            state = ACTIVE;

            mpl.addListener(this);
            addListener(mpl);
        }

        /**
         * Initiates MIDPSystem suspend operations.
         */
        public synchronized void suspend() {
            SuspendTimer.start(mpl);
            super.suspend();
        }

        /**
         * Performs MIDPSystem-specific suspend operations.
         */
        protected synchronized void suspendImpl() {
            SuspendTimer.stop();
        }

        /**
         * Performs MIDPSystem-specific resume operations.
         */
        protected synchronized void resumeImpl() {
            midletKilled = false;
            midletPaused = false;
            alertIfAllMidletsKilled();
        }

        /**
         * Shows proper alert if all user midlets were killed by a preceding
         * suspend operation, and the event is not reported yet.
         */
        private void alertIfAllMidletsKilled() {
            if (allMidletsKilled()) {
                String title = Resource.getString(
                    ResourceConstants.SR_ALL_KILLED_ALERT_TITLE, null);
                String msg = Resource.getString(
                    ResourceConstants.SR_ALL_KILLED_ALERT_MSG, null);
                DisplayEventHandler disp = DisplayEventHandlerFactory.
                        getDisplayEventHandler(classSecurityToken);
                SystemAlert alert = new SystemAlert(disp, title, msg,
                        null, AlertType.WARNING);

                alert.runInNewThread();
            }
        }

        /**
         * Notifies of system suspend.
         */
        protected void suspended() {
            super.suspended();
            suspended0(!midletPaused && midletKilled);
        }

        /**
         * Notifies native functionality that MIDP activities in java
         * have been suspended.
         * @param allMidletsKilled true to indicate that all user MIDlets
         *        were killed by suspend routines.
         */
        protected native void suspended0(boolean allMidletsKilled);

        /**
         * Determines if at least one of preceding suspension operations
         * killed all user MIDlets and  the condition has not been checked
         * since that time.
         * @return true if a suspension operation killed all user MIDlets
         *         and the condition has not been checked yet, false
         *         otherwise. This method returns true only once for one
         *         event.
         */
        protected native boolean allMidletsKilled();

        /**
         * Receives notifications on MIDlet updates and removes corresponding
         * MIDlet proxy from suspend dependencies if required.
         * @param midlet MIDletProxy that represents the MIDlet updated
         * @param reason kind of changes that took place, see
         */
        public void midletUpdated(MIDletProxy midlet, int reason) {
            if (reason == MIDletProxyListListener.RESOURCES_SUSPENDED) {
                if (MIDletSuiteUtils.getAmsIsolateId() != midlet.getIsolateId()) {
                    midletPaused = true;
                }
                removeSuspendDependency(midlet);
            }
        }

        /**
         * Receives MIDlet removal notification and removes corresponding
         * MIDlet proxy from suspend dependencies.
         * @param midlet MIDletProxy that represents the MIDlet removed
         */
        public void midletRemoved(MIDletProxy midlet) {
            midletKilled = true;
            removeSuspendDependency(midlet);
        }

        /**
         * Called from the proxy list to notify of new MIDlet appearance.
         */
        public void midletAdded(MIDletProxy midlet) {
            if (MIDletSuiteUtils.getAmsIsolateId() == midlet.getIsolateId()) {
                alertIfAllMidletsKilled();
            }
        }

        /**
         * Not used. MIDletProxyListListener interface method.
         */
        public void midletStartError(int externalAppId, int suiteId,
                                     String className, int error) {}
    }

    /**
     * The singleton instance.
     */
    private static SuspendSystem instance =
        MIDletSuiteUtils.getIsolateId() == MIDletSuiteUtils.getAmsIsolateId()?
                new MIDPSystem() : new SuspendSystem();

    /**
     * Retrieves the singleton instance.
     * @param token security token that identifies caller permissions for
     *        accessing this API
     * @return the singleton instance
     */
    public static SuspendSystem getInstance(SecurityToken token) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        return instance;
    }

    /**
     * Constructs an instance.
     */
    private SuspendSystem() {}

    /**
     * Registers a listener interested in system suspend/resume operations.
     * IMPL_NOTE: method for removing listeners is not needed currently.
     *
     * @param listener the listener to be added
     */
    public void addListener(SuspendSystemListener listener) {
        synchronized (listeners) {
            listeners.addElement(listener);
        }
    }

    /**
     * Notifies listeners of system suspend.
     */
    protected void suspended() {
        synchronized (listeners) {
            for (int i = listeners.size() - 1; i >= 0; i-- ) {
                SuspendSystemListener listener =
                        (SuspendSystemListener)listeners.elementAt(i);
                listener.midpSuspended();
            }
        }
    }

    /**
     * Notifies listeners of system resume.
     */
    protected void resumed() {
        synchronized (listeners) {
            for (int i = listeners.size() - 1; i >= 0; i-- ) {
                SuspendSystemListener listener =
                        (SuspendSystemListener)listeners.elementAt(i);
                listener.midpResumed();
            }
        }
    }
}
