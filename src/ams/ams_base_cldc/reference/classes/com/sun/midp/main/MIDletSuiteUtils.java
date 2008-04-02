/*
 *
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

package com.sun.midp.main;

import com.sun.j2me.security.AccessController;

import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.SystemAlert;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

import javax.microedition.lcdui.AlertType;

/**
 * The class designed to provide utils for starting MIDlet suites,
 * and scheduling their start using VM cycling mechanism.
 */
public class MIDletSuiteUtils {

    /** The unique ID of the last MIDlet suite to run. */
    static int lastMidletSuiteToRun;

    /** The class name of the last MIDlet to run. */
    static String lastMidletToRun;

    /**
     * If not null, this will be available to the last MIDlet to run as
     * application property arg-0.
     */
    static String arg0ForLastMidlet;

    /**
     * If not null, this will be available to the last MIDlet to run as
     * application property arg-1.
     */
    static String arg1ForLastMidlet;

    /** The unique ID of the next MIDlet suite to run. */
    static int nextMidletSuiteToRun;

    /** The class of the next MIDlet to run. */
    static String nextMidletToRun;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-0.
     */
    static String arg0ForNextMidlet;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-1.
     */
    static String arg1ForNextMidlet;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-2.
     */
    static String arg2ForNextMidlet;

    /**
     * The minimum amount of memory guaranteed to be available
     * to the VM at any time; &lt; 0 if not used.
     */
    static int memoryReserved;

    /**
     * The total amount of memory that the VM can reserve; &lt; 0 if not used.
     */
    static int memoryTotal;

    /**
     * Priority to set after restarting the VM; &lt;= 0 if not used.
     */
    static int priority;

    /**
     * Name of the profile to set after restarting the VM; null if not used.
     */
    static String profileName;

    /**
     * true if the new midlet must be started in debug
     * mode, false otherwise.
     */
    static boolean isDebugMode;

    /**
     * Display an exception to the user.
     *
     * @param securityToken security token for displaying System Alert.
     * @param exceptionMsg exception message
     */
    static void displayException(SecurityToken securityToken,
				 String exceptionMsg) {

        SystemAlert alert = new SystemAlert(securityToken, "Exception", 
					    exceptionMsg, null, 
					    AlertType.ERROR);
        alert.run();
        alert.waitForUser();
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean execute(
            int id, String midlet, String displayName) {
        return executeWithArgs(
            id, midlet, displayName, null, null, null, false);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param isDebugMode true if the new midlet must be started in debug
     *                    mode, false otherwise
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean execute(
            int id, String midlet, String displayName, boolean isDebugMode) {
        return executeWithArgs(
            id, midlet, displayName, null, null, null, isDebugMode);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     *                      application manager
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean execute(
            SecurityToken securityToken, int suiteId,
            String midlet, String displayName) {

        return executeWithArgs(
            securityToken, suiteId, midlet,
            displayName, null, null, null, false);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     *                      application manager
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param isDebugMode true if the new midlet must be started in debug
     *                    mode, false otherwise
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean execute(
            SecurityToken securityToken, int suiteId,
            String midlet, String displayName, boolean isDebugMode) {

        return executeWithArgs(
            securityToken, suiteId, midlet,
            displayName, null, null, null, isDebugMode);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean executeWithArgs(
            int suiteId, String midlet, String displayName,
            String arg0, String arg1, String arg2) {

        return executeWithArgs(
            null, suiteId, midlet, displayName, arg0, arg1, arg2, false);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     * @param isDebugMode true if the new midlet must be started in debug
     *                    mode, false otherwise
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean executeWithArgs(
            int suiteId, String midlet, String displayName,
            String arg0, String arg1, String arg2, boolean isDebugMode) {

        return executeWithArgs(
            null, suiteId, midlet, displayName, arg0, arg1, arg2, isDebugMode);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean executeWithArgs(
            SecurityToken securityToken, int suiteId, String midlet,
            String displayName, String arg0, String arg1, String arg2) {

        return executeWithArgs(
            securityToken, 0, suiteId, midlet,
            displayName, arg0, arg1, arg2, false);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     * @param isDebugMode true if the new midlet must be started in debug
     *                    mode, false otherwise
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean executeWithArgs(
            SecurityToken securityToken, int suiteId, String midlet,
            String displayName, String arg0, String arg1, String arg2,
            boolean isDebugMode) {

        return executeWithArgs(
            securityToken, 0, suiteId, midlet,
            displayName, arg0, arg1, arg2, isDebugMode);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     * @param externalAppId ID of MIDlet to invoke, given by an external
     *                      application manager
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     * @param isDebugMode true if the new midlet must be started in debug
     *                    mode, false otherwise
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean executeWithArgs(
            SecurityToken securityToken, int externalAppId,
            int suiteId, String midlet, String displayName,
            String arg0, String arg1, String arg2,
            boolean isDebugMode) {

        return executeWithArgs(
            securityToken, externalAppId, suiteId, midlet, displayName,
            arg0, arg1, arg2, -1, -1, -1, null, isDebugMode);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invocation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     * @param externalAppId ID of MIDlet to invoke, given by an external
     *                      application manager
     * @param suiteId ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     * @param memoryReserved the minimum amount of memory guaranteed to be
     *             available to the isolate at any time; &lt; 0 if not used
     * @param memoryTotal the total amount of memory that the isolate can
                   reserve; &lt; 0 if not used
     * @param priority priority to set for the new isolate;
     *                 &lt;= 0 if not used
     * @param profileName name of the profile to set for the new isolate;
     *                    null if not used
     * @param isDebugMode true if the new midlet must be started in debug
     *                    mode, false otherwise
     *
     * @return true if the MIDlet suite MUST first exit before the
     * MIDlet is run
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static boolean executeWithArgs(
            SecurityToken securityToken, int externalAppId,
            int suiteId, String midlet, String displayName,
            String arg0, String arg1, String arg2,
            int memoryReserved, int memoryTotal, int priority,
            String profileName,
            boolean isDebugMode) {

        MIDletSuiteStorage midletSuiteStorage;

        // Note: getMIDletSuiteStorage performs an AMS permission check.
        if (securityToken != null) {
            midletSuiteStorage =
                MIDletSuiteStorage.getMIDletSuiteStorage(securityToken);
        } else {
            midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
        }

        return AmsUtil.executeWithArgs(
            midletSuiteStorage, externalAppId, suiteId,
            midlet, displayName, arg0, arg1, arg2,
            memoryReserved, memoryTotal, priority, profileName, isDebugMode);
    }

    /**
     * Gets the unique storage name of the next MIDlet suite to run.
     *
     * @return storage name of a MIDlet suite
     */
    public static int getNextMIDletSuiteToRun() {
        return nextMidletSuiteToRun;
    }

    /**
     * Gets the name of the next MIDlet to run.
     *
     * @return storage name of a MIDlet
     */
    public static String getNextMIDletToRun() {
        return nextMidletToRun;
    }

    /**
     * Queues the last suite to run when there is not a next Suite
     * to run. This value will be persistent until it is used.
     * Not used in MVM mode.
     * <p>
     * Method requires com.sun.midp.ams permission.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static void setLastSuiteToRun(int id, String midlet, String arg0,
            String arg1) {

        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);

        lastMidletSuiteToRun = id;
        lastMidletToRun = midlet;
        arg0ForLastMidlet = arg0;
        arg1ForLastMidlet = arg1;
    }

    /**
     * Get the Isolate ID of the AMS Isolate.
     *
     * @return Isolate ID of AMS Isolate
     */
    public static native int getAmsIsolateId();

    /**
     * Get the current Isolate ID.
     *
     * @return ID of this Isolate.
     */
    public static native int getIsolateId();

    /**
     * Check whether current Isolate is an AMS Isolate
     *
     * @return true if the current Isolate is an AMS Isolate,
     *   false otherwise.
     */
    public static native boolean isAmsIsolate();

    /**
     * Register the Isolate ID of the AMS Isolate by making a native
     * method call that will call JVM_CurrentIsolateId and set
     * it in the proper native variable.
     */
    static native void registerAmsIsolateId();

    /**
     * Send hint to VM about begin of a MIDlet startup phase within specified
     * isolate to allow the VM to fine tune its internal parameters to achieve
     * optimal perfomance
     *
     * @param midletIsolateId ID of the started MIDlet isolate
     */
    static native void vmBeginStartUp(int midletIsolateId);

    /**
     * Send hint to VM about end of a MIDlet startup phase within specified
     * isolate to allow the VM to restore its internal parameters changed on
     * startup time for better performance
     *
     * @param midletIsolateId ID of the started MIDlet isolate
     */
    static native void vmEndStartUp(int midletIsolateId);

    /**
     * Secure method to send VM hint about begin of a MIDlet startup phase
     * within specified isolate
     *
     * @param token security token with the AMS permission allowed
     * @param midletIsolateId ID of the started MIDlet isolate
     */
    static public void vmBeginStartUp(
        SecurityToken token, int midletIsolateId) {
        token.checkIfPermissionAllowed(Permissions.AMS);
        vmBeginStartUp(midletIsolateId);
    }

    /**
     * Secure method to send VM hint about end of a MIDlet startup phase
     * within specified isolate
     *
     * @param token security token with the AMS permission allowed
     * @param midletIsolateId ID of the started MIDlet isolate
     */
    static public void vmEndStartUp(
        SecurityToken token, int midletIsolateId) {
        token.checkIfPermissionAllowed(Permissions.AMS);
        vmEndStartUp(midletIsolateId);
    }

    /**
     * The method is designed to init AMS task resources. The resources
     * can be shared between all working isolates, so it is important to
     * init them before other isolate tasks will require the resources.
     *
     * The tasks other than AMS shouldn't call this method, it's guarded
     * by run-time exception.
     *
     * IMPL_NOTE: The method is temporarily loacated here, since we need
     *   to introduce new abstraction for AMS task logic and separate it
     *   from the MIDlet suite loading and execution logic. Now the method
     *   is needed to MIDletSuiteLoader & NativeAppManagerPeer classes
     *   which represent an AMS task for Java AMS and Native AMS cases
     *   correspondingly.
     */
    static void initAmsResources() {
        // Check whether caller task is an AMS task
        if (!isAmsIsolate()) {
            throw new RuntimeException(
                "Resources initialization should be done from the AMS task");
        }

        // The static initializer of the Display class will forward on
        // the Chameleon skin resources loading if Chameleon is being used.
        // It is important to load Chameleon resources from the AMS isolate
        // before other isolates will need them.
        try {
            Class.forName("javax.microedition.lcdui.Display");
        } catch (Throwable ex) {
            throw new RuntimeException(
                "Display initialization has failed");
        }
    }
}
