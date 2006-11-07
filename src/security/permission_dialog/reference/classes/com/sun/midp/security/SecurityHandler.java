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

package com.sun.midp.security;

import javax.microedition.io.*;

import javax.microedition.lcdui.*;

import com.sun.midp.lcdui.*;

import com.sun.midp.midlet.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.events.EventQueue;

import com.sun.midp.io.j2me.storage.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Contains methods to handle with the various security state information of a
 * a MIDlet suite.
 */
public final class SecurityHandler {

    /** The security token for this class. */
    private static SecurityToken classSecurityToken;

    /** The standard security exception message. */
    public static final String STD_EX_MSG = "Application not authorized " +
                                            "to access the restricted API";

    /** Permission list. */
    private byte permissions[];

    /**
     * A flag for each permission, != 0 if permission has been asked
     * this session.
     */
    private byte permissionAsked[];

    /** Maximum permission level list. */
    private byte maxPermissionLevels[];

    /** True, if trusted. */
    private boolean trusted;

    /**
     * Creates a security domain with a list of permitted actions or no list
     * to indicate all actions. The caller must be have permission for
     * <code>Permissions.MIDP</code> or be the first caller of
     * the method for this instance of the VM.
     *
     * @param apiPermissions for the token
     * @param domain name of the security domain
     *
     * @exception SecurityException if caller is not permitted to call this
     *            method
     */
    public SecurityHandler(byte[] apiPermissions, String domain) {
        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();

        midletSuite.checkIfPermissionAllowed(Permissions.AMS);
        init(apiPermissions, domain);
    }

    /**
     * Creates a security domain with a list of permitted actions or no list
     * to indicate all actions. The caller must be have permission for
     * <code>Permissions.MIDP</code> or be the first caller of
     * the method for this instance of the VM.
     *
     * @param securityToken security token of the caller
     * @param apiPermissions for the token, can be null
     * @param domain name of the security domain
     *
     * @exception SecurityException if caller is not permitted to call this
     *            method
     */
    public SecurityHandler(SecurityToken securityToken,
            byte[] apiPermissions, String domain) {
        securityToken.checkIfPermissionAllowed(Permissions.AMS);
        init(apiPermissions, domain);
    }

    /**
     * Creates a security domain with a list of permitted actions or no list
     * to indicate all actions. The caller must be have permission for
     * <code>Permissions.MIDP</code> or be the first caller of
     * the method for this instance of the VM.
     *
     * @param apiPermissions for the token
     * @param domain name of the security domain
     *
     * @exception SecurityException if caller is not permitted to call this
     *            method
     */
    private void init(byte[] apiPermissions, String domain) {
        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();

        maxPermissionLevels =
            (Permissions.forDomain(domain))[Permissions.MAX_LEVELS];

        permissions = apiPermissions;

        permissionAsked = new byte[permissions.length];

        trusted = Permissions.isTrusted(domain);
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
     *  -1 if the status is unknown
     */
    public int checkPermission(String permission) {
        boolean found = false;
        int i;

        synchronized (this) {
            for (i = 0; i < Permissions.NUMBER_OF_PERMISSIONS; i++) {
                if (Permissions.getName(i).equals(permission)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                // report denied
                return 0;
            }

            switch (permissions[i]) {
            case Permissions.ALLOW:
            case Permissions.BLANKET_GRANTED:
                // report allowed
                return 1;

            case Permissions.SESSION:
                if (permissionAsked[i] != 0) {
                    return 1;
                }

                // fall through
            case Permissions.BLANKET:
            case Permissions.ONE_SHOT:
            case Permissions.DENY:
                // report unknown
                return -1;

            case Permissions.DENY_SESSION:
                if (permissionAsked[i] == 0) {
                    return -1;
                }
                // deny the session.
                break;

            default:
                // for safety/completeness, deny the session.
                //
                // Note: This can happen normally. Must not log
                // anything about this!
                //
                // Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI,
                //    "SecurityHandler session denied; permissions=" +
                //    permissions[i]);
                break;
            }

            // report denied
            return 0;
        }
    }

    /**
     * Check for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     * <p>
     * The title, and question strings will be translated,
     * if a string resource is available.
     * Since the strings can have substitution token in them, if there is a
     * "%" it must changed to "%%". If a string has a %1, the app parameter
     * will be substituted for it. If a string has a "%2, the resource
     * parameter will be substituted for it. If a string has a %3, the
     * extraValue parameter will be substituted for it.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param title Resource constant for the title of the dialog
     * @param question Resource constant for the question to ask the user
     * @param oneshotQuestion Resource constant for the oneshot question to
     *                        ask the user
     * @param app name of the application to insert into a string
     *        can be null if no %1 a string
     * @param resource string to insert into a string,
     *        can be null if no %2 in a string
     * @param extraValue string to insert into a string,
     *        can be null if no %3 in a string
     *
     * @return true if the permission was allow and was not allowed
     *    before
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public boolean checkForPermission(int permission, int title, int question,
        int oneshotQuestion, String app, String resource, String extraValue)
        throws InterruptedException {

        return checkForPermission(permission, title, question,
            oneshotQuestion, app, resource, extraValue, STD_EX_MSG);
    }


    /**
     * Check for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     * <p>
     * The title, question, and answer strings will be translated,
     * if a string resource is available.
     * Since the strings can have substitution token in them, if there is a
     * "%" it must changed to "%%". If a string has a %1, the app parameter
     * will be substituted for it. If a string has a "%2, the resource
     * parameter will be substituted for it. If a string has a %3, the
     * extraValue parameter will be substituted for it.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param title Resource constant for the title of the dialog
     * @param question Resource constant for the question to ask user
     * @param oneShotQuestion Resource constant for the oneshot question to
     *                        ask the user
     * @param app name of the application to insert into a string
     *        can be null if no %1 a string
     * @param resource string to insert into a string,
     *        can be null if no %2 in a string
     * @param extraValue string to insert into a string,
     *        can be null if no %3 in a string
     * @param exceptionMsg message if a security exception is thrown
     *
     * @return <code>true</code> if the permission was allowed and was
     * not allowed before; <code>false</code>, if permission is granted..
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public boolean checkForPermission(int permission, int title, int question,
        int oneShotQuestion, String app, String resource, String extraValue,
        String exceptionMsg) throws InterruptedException {

        if (permissions == null) {
            /* totally trusted, all permissions allowed */
            return false;
        }

        synchronized (this) {
            byte prevPermission;

            if (permission >= 0 && permission < permissions.length) {
                prevPermission = permissions[permission];

                switch (prevPermission) {
                case Permissions.ALLOW:
                case Permissions.BLANKET_GRANTED:
                    return false;

                case Permissions.BLANKET:
                    /* This level means the question has not been asked yet. */
                    if (askUserForPermission(classSecurityToken, trusted,
                            title, question, app, resource, extraValue)) {

                        Permissions.setPermissionGroup(permissions,
                            permission, Permissions.BLANKET_GRANTED);

                        return true;
                    }

                    Permissions.setPermissionGroup(permissions,
                        permission, Permissions.USER_DENIED);
                    break;

                case Permissions.SESSION:
                case Permissions.DENY_SESSION:
                    if (permissionAsked[permission] != 0) {
                        if (permissions[permission] == Permissions.SESSION) {
                            return false;
                        }

                        break;
                    }

                    if (askUserForPermission(classSecurityToken, trusted,
                            title, question, app, resource, extraValue)) {
                        Permissions.setPermissionGroup(permissions,
                            permission, Permissions.SESSION);

                        /*
                         * Save the fact that the question has already
                         * been asked this session.
                         */
                        Permissions.setPermissionGroup(permissionAsked,
                            permission, (byte)1 /* any non-zero number */);

                        // If same permissions as before, grant permission.
                        if (permissions[permission] == prevPermission) {
                            return false;
                        }

                        return true;
                    }

                    Permissions.setPermissionGroup(permissions,
                        permission, Permissions.DENY_SESSION);

                    /*
                     * Save the fact that the question has already
                     * been asked this session.
                     */
                    Permissions.setPermissionGroup(permissionAsked,
                        permission, (byte)1 /* any non-zero number */);
                    break;

                case Permissions.ONE_SHOT:
                case Permissions.DENY:
                    if (askUserForPermission(classSecurityToken, trusted,
                            title, oneShotQuestion, app, resource,
                            extraValue)) {
                        Permissions.setPermissionGroup(permissions,
                            permission, Permissions.ONE_SHOT);

                        if (permissions[permission] == prevPermission) {
                            return false;
                        }

                        return true;
                    }

                    // Deny permission, always.
                    Permissions.setPermissionGroup(permissions,
                        permission, Permissions.DENY);
                    break;

                default:
                    // for safety/completeness, deny the session.
                    //
                    // Note: This can happen normally. Must not log
                    // anything about this!
                    //
                    // Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI,
                    //    "SecurityHandler session denied;
                    //    prevPermission=" + prevPermission);
                    break;

                } // switch

            } // if

            throw new SecurityException(exceptionMsg);
        }
    }

    /**
     * Ask the user yes/no permission question.
     *
     * @param token security token with the permission to preempt the
     *        foreground display
     * @param trusted true to display the trusted icon, false to display the
     *                untrusted icon
     * @param title Resource constant for the title of the dialog
     * @param question Resource constant for the question to ask user
     * @param app name of the application to insert into a string
     *        can be null if no %1 a string
     * @param resource string to insert into a string,
     *        can be null if no %2 in a string
     * @param extraValue string to insert into a string,
     *        can be null if no %3 in a string
     *
     * @return true if the user says yes else false
     *
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public static boolean askUserForPermission(SecurityToken token,
            boolean trusted, int title, int question, String app,
            String resource, String extraValue) throws InterruptedException {

        PermissionDialog dialog =
            new PermissionDialog(token, trusted, title, question, app,
                                 resource, extraValue);

        return dialog.waitForAnswer();
    }

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    static void initSecurityToken(SecurityToken token) {
        if (classSecurityToken != null) {
            return;
        }

        classSecurityToken = token;
    }
}

/** Implements security permission dialog. */
class PermissionDialog implements CommandListener, MIDletEventConsumer {
    /** Caches the display manager reference. */
    private DisplayEventHandler displayEventHandler;

    /** Command object for "Yes" command. */
    private Command yesCmd =
        new Command(Resource.getString(ResourceConstants.YES),
                    Command.OK, 1);
    /** Command object for "No" command. */
    private Command noCmd =
        new Command(Resource.getString(ResourceConstants.NO),
                    Command.BACK, 1);
    /** Holds the preempt token so the form can end. */
    private Object preemptToken;

    /** Holds the answer to the security question. */
    private boolean answer;

    /**
     * Construct permission dialog.
     * <p>
     * The title, question, and answer strings will be translated,
     * if a string resource is available.
     * Since the strings can have substitution token in them, if there is a
     * "%" it must changed to "%%". If a string has a %1, the app parameter
     * will be substituted for it. If a string has a "%2, the resource
     * parameter will be substituted for it. If a string has a %3, the
     * extraValue parameter will be substituted for it.
     *
     * @param token security token with the permission to preempt the
     *        foreground display
     * @param trusted true to display the trusted icon, false to display the
     *                untrusted icon
     * @param title Resource constant for the title of the dialog
     * @param question Resource constant for the question to ask user
     * @param app name of the application to insert into a string
     *        can be null if no %1 a string
     * @param resource string to insert into a string,
     *        can be null if no %2 in a string
     * @param extraValue string to insert into a string,
     *        can be null if no %3 in a string
     *
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    PermissionDialog(SecurityToken token, boolean trusted, int title,
            int question, String app, String resource, String extraValue)
            throws InterruptedException {
        String[] substitutions = {app, resource, extraValue};
        Alert alert = new Alert(Resource.getString(title, substitutions));
        String iconFilename;
        RandomAccessStream stream;
        byte[] rawPng;
        Image icon;

        displayEventHandler =
            DisplayEventHandlerFactory.getDisplayEventHandler(token);

        if (trusted) {
            iconFilename = File.getConfigRoot() + "trusted_icon.png";
        } else {
            iconFilename = File.getConfigRoot() + "untrusted_icon.png";
        }

        stream = new RandomAccessStream(token);
        try {
            stream.connect(iconFilename, Connector.READ);
            rawPng = new byte[stream.getSizeOf()];
            stream.readBytes(rawPng, 0, rawPng.length);
            stream.disconnect();
            icon = Image.createImage(rawPng, 0, rawPng.length);
            alert.setImage(icon);
        } catch (java.io.IOException noImage) {
        }

        alert.setString(Resource.getString(question, substitutions));
        alert.addCommand(noCmd);
        alert.addCommand(yesCmd);
        alert.setCommandListener(this);
        preemptToken = displayEventHandler.preemptDisplay(this, alert, true);
    }

    /**
     * Waits for the user's answer.
     *
     * @return user's answer
     */
    boolean waitForAnswer() {
        synchronized (this) {
            if (preemptToken == null) {
                return false;
            }

            if (EventQueue.isDispatchThread()) {
                // Developer programming error
                throw new RuntimeException(
                "Blocking call performed in the event thread");
            }

            try {
                wait();
            } catch (Throwable t) {
                return false;
            }

            return answer;
        }
    }

    /**
     * Sets the user's answer and notifies waitForAnswer and
     * ends the form.
     *
     * @param theAnswer user's answer
     */
    private void setAnswer(boolean theAnswer) {
        synchronized (this) {
            answer = theAnswer;

            displayEventHandler.donePreempting(preemptToken);

            notify();
        }

    }

    /**
     * Respond to a command issued on security question form.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == yesCmd) {
            setAnswer(true);
            return;
        }

        setAnswer(false);
    }

    /**
     * Pause the current foreground MIDlet and return to the
     * AMS or "selector" to possibly run another MIDlet in the
     * currently active suite.
     * <p>
     * This is not apply to the security dialog.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletPauseEvent() {}

    /**
     * Start the currently paused state. 
     * <p>
     * This is not apply to the security dialog.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletActivateEvent() {}

    /**
     * Destroy the MIDlet given midlet.
     * <p>
     * This is not apply to the security dialog.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletDestroyEvent() {
        setAnswer(false);
    }
}
