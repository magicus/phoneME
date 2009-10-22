/*
 *
 *
 * Copyright 2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.installer;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.events.Event;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventListener;
import com.sun.midp.ams.VMUtils;

import com.sun.midp.appmanager.InstallerResultEventConsumer;

/**
 * This class handles installation result by notifying AMS.
 */
public final class InstallerResultHandler {

    /**
     * Send installation result to AMS.
     * @param result Result code, as defined by constants in
     *               InstallerResultEventConsumer
     * @param suiteId Installed suite ID (in case of success)
     * @param exception Thrown exception class name (in case of failure)
     * @param message Thrown exception message (in case of failure)
     * @param reason Failure reason (in case of InvalidJadException)
     * @param extraData Failure extra data (in case of InvalidJadException)
     */
    private static void sendInstallationResult(int result, int suiteId,
            String exception, String message, int reason, String extraData) {

        EventQueue eq = EventQueue.getEventQueue();
        NativeEvent event = new NativeEvent(
                EventTypes.MIDP_INSTALLATION_DONE_EVENT);
        event.intParam1    = result;
        event.intParam2    = suiteId;
        event.stringParam1 = exception;
        event.stringParam2 = message;
        event.intParam3    = reason;
        event.stringParam3 = extraData;

        eq.sendNativeEventToIsolate(event, VMUtils.getAmsIsolateId());

    }

    /**
     * Notify that installation request failed.
     * @param reason Failure reason.
     */
    public static void notifyRequestFailed(Throwable reason) {
        if (reason instanceof InvalidJadException) {
            InvalidJadException ije = (InvalidJadException) reason;
            sendInstallationResult(
                    InstallerResultEventConsumer.INSTALLER_RESULT_FAILURE,
                    MIDletSuite.UNUSED_SUITE_ID, "InvalidJadException",
                    ije.getMessage(), ije.getReason(), ije.getExtraData());
        } else {
            sendInstallationResult(
                    InstallerResultEventConsumer.INSTALLER_RESULT_FAILURE,
                    MIDletSuite.UNUSED_SUITE_ID, reason.getClass().getName(),
                    reason.getMessage(), 0, null);
        }
    }

    /**
     * Notify that installation request succeeded.
     * @param id Id of the installed suite
     */
    public static void notifyRequestSucceeded(int id) {
        sendInstallationResult(
                InstallerResultEventConsumer.INSTALLER_RESULT_SUCCESS, id,
                null, null, 0, null);
    }

    /**
     * Notify that installation request was canceled.
     */
    public static void notifyRequestCanceled() {
        sendInstallationResult(
                InstallerResultEventConsumer.INSTALLER_RESULT_CANCELED,
                MIDletSuite.UNUSED_SUITE_ID, null, null, 0, null);
    }

}
