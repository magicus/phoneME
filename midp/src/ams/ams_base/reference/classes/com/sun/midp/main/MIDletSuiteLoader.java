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

package com.sun.midp.main;

import java.io.IOException;

import javax.microedition.lcdui.AlertType;

import com.sun.midp.configurator.Constants;

import com.sun.midp.events.EventQueue;

import com.sun.midp.installer.InternalMIDletSuiteImpl;

import com.sun.midp.io.j2me.push.PushRegistryImpl;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.lcdui.DisplayContainer;
import com.sun.midp.lcdui.DisplayEventProducer;
import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.DisplayEventHandlerFactory;
import com.sun.midp.lcdui.LCDUIEventListener;
import com.sun.midp.lcdui.RepaintEventProducer;
import com.sun.midp.lcdui.ItemEventConsumer;
import com.sun.midp.lcdui.SystemAlert;

import com.sun.midp.midlet.MIDletEventProducer;
import com.sun.midp.midlet.MIDletEventListener;
import com.sun.midp.midlet.MIDletPeer;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteStorage;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.SecurityInitializer;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.midp.content.CHManager;
import com.sun.midp.wma.WMACleanupMonitor;

import com.sun.midp.automation.AutomationInitializer;

/**
 * The first class loaded in VM by midp_run_midlet_with_args to initialize
 * internal security the internal AMS classes and start a MIDlet suite.
 * <p>
 * In SVM mode it handles all MIDlet suites (AMS and internal romized,
 * and application).
 * <p>
 * In MVM mode it only handles the first MIDlet suite isolate which is used by
 * the MIDP AMS and other internal MIDlets.
 */
public class MIDletSuiteLoader {

    /** The unique ID of the last MIDlet suite to run. */
    static String lastMidletSuiteToRun;

    /** The class name of the last MIDlet to run. */
    static String lastMidletToRun;

    /**
     * Called at the initial start of the VM.
     * Initializes internal security and any other AMS classes related
     * classes before starting the MIDlet.
     *
     * @param args not used, a {@link CommandState} object is obtained
     *             using a native method instead of the argument.
     */
    public static void main(String args[]) {
        CommandState state;
        MIDletSuite midletSuite = null;

	/*
	 * WARNING: Don't add any calls before this !
	 *
	 * Register AMS Isolate ID in native global variable.
	 * Since native functions rely on this value to distinguish
	 * whether Java AMS is running, this MUST be called before any 
	 * other native functions from this Isolate. I.E. This call
	 * must be the first thing this main make.
	 */
        registerAmsIsolateId();

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_CORE,
                           "MIDlet suite loader started");
        }
        // current isolate & AMS isolate is the same object
        int amsIsolateId = MIDletSuiteLoader.getAmsIsolateId();
        int currentIsolateId = MIDletSuiteLoader.getIsolateId();
        vmBeginStartUp(currentIsolateId);

        // Throws SecurityException if already called,
        SecurityToken internalSecurityToken = SecurityInitializer.init();
        EventQueue eventQueue =
            EventQueue.getEventQueue(internalSecurityToken);

        // create all needed event-related objects but not initialize ...
        MIDletEventProducer midletEventProducer = 
            new MIDletEventProducer(
                internalSecurityToken,
                eventQueue);

        MIDletControllerEventProducer midletControllerEventProducer = 
            new MIDletControllerEventProducer(
                internalSecurityToken,
                eventQueue,
                amsIsolateId,
                currentIsolateId);
        
        DisplayEventProducer displayEventProducer = 
            new DisplayEventProducer(
                internalSecurityToken,
                eventQueue);
        
        RepaintEventProducer repaintEventProducer = new RepaintEventProducer(
            internalSecurityToken, 
            eventQueue);
        MIDletProxyList midletProxyList = new MIDletProxyList(eventQueue);
        
        DisplayContainer displayContainer = new DisplayContainer(
            internalSecurityToken, currentIsolateId);
        
        DisplayEventHandler displayEventHandler = 
            DisplayEventHandlerFactory.getDisplayEventHandler(
            internalSecurityToken);
        /*
         * Bad style of type casting, but 
         * DisplayEventHandlerImpl implement both 
         * DisplayEventHandler & ItemEventConsumer I/Fs
         */
        ItemEventConsumer itemEventConsumer = 
                (ItemEventConsumer)displayEventHandler;
        
	    MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        
        MIDletEventListener midletEventListener = new MIDletEventListener(
            internalSecurityToken, 
            eventQueue, 
            displayContainer);
                
        LCDUIEventListener lcduiEventListener = new LCDUIEventListener(
            internalSecurityToken, 
            eventQueue, 
            itemEventConsumer);
        
        // do all initialization for already created event-related objects ...
        MIDletProxy.initClass(midletEventProducer);
        MIDletProxyList.initClass(midletProxyList);
            
        AmsUtil.initClass(midletProxyList, midletControllerEventProducer);
        
        displayEventHandler.initDisplayEventHandler(
            internalSecurityToken,
            eventQueue,
            displayEventProducer,
            midletControllerEventProducer,
            repaintEventProducer,
            displayContainer);
        
        MIDletPeer.initClass(
            internalSecurityToken,
            displayEventHandler, 
            midletStateHandler, 
            midletControllerEventProducer);
        midletStateHandler.initMIDletStateHandler(
            internalSecurityToken,
            displayEventHandler, 
            midletControllerEventProducer,
            displayContainer);
        
        AutomationInitializer.init(eventQueue, 
                midletControllerEventProducer);

        state = CommandState.getCommandState();

        try {
            String temp;
            MIDletSuiteStorage midletSuiteStorage;

            state.status = Constants.MIDLET_CONSTRUCTOR_FAILED;

            if (state.suiteID == null) {
                if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                    Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                       "The suite ID for the " +
                       "MIDlet suite was not given");
                }

                state.status = Constants.MIDLET_SUITE_NOT_FOUND;
                return;
            }

            if (state.midletClassName == null) {
                if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                    Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                       "The class name for the " +
                       "MIDlet was not given");
                }

                state.status = Constants.MIDLET_CLASS_NOT_FOUND;
                return;
            }

            midletSuiteStorage = MIDletSuiteStorage.
                getMIDletSuiteStorage(internalSecurityToken);
           
            ExecuteMIDletEventListener.startListening(
                internalSecurityToken, 
                displayEventHandler, 
                eventQueue);

            // Start inbound connection watcher thread.
            PushRegistryImpl.startListening();

            // Initialize the Content Handler Monitor of MIDlet exits
            CHManager.getManager(internalSecurityToken).initCleanupMonitor(
                midletProxyList);

            // Initialize WMA's cleanup monitor
            WMACleanupMonitor.init(midletProxyList);

            if (state.suiteID.equals("internal")) {
                // assume a class name of a MIDlet in the classpath
                midletSuite = InternalMIDletSuiteImpl.create(
                    null,
                    state.suiteID);

                // This is for Manager MIDlet.
                if (Constants.MEASURE_STARTUP || state.logoDisplayed) {
                    midletSuite.setTempProperty(internalSecurityToken,
                                                "logo-displayed", "");
                } else {
                    state.logoDisplayed = true;
                }
            } else {
                midletSuite =
                    midletSuiteStorage.getMIDletSuite(state.suiteID, false);
            }

            if (midletSuite == null) {
                displayException(displayEventHandler, 
                    Resource.getString(
                 ResourceConstants.AMS_MIDLETSUITELDR_MIDLETSUITE_NOTFOUND));
                state.status = Constants.MIDLET_SUITE_NOT_FOUND;
                return;
            }

            if (!midletSuite.isEnabled()) {
                displayException(displayEventHandler, Resource.getString(
                  ResourceConstants.AMS_MIDLETSUITELDR_MIDLETSUITE_DISABLED));
                state.status = Constants.MIDLET_SUITE_DISABLED;
                return;
            }

            if (state.arg0 != null) {
                midletSuite.setTempProperty(internalSecurityToken,
                                            "arg-0", state.arg0);
                state.arg0 = null;
            }

            if (state.arg1 != null) {
                midletSuite.setTempProperty(internalSecurityToken,
                                            "arg-1", state.arg1);
                state.arg1 = null;
            }

            if (state.arg2 != null) {
                midletSuite.setTempProperty(internalSecurityToken,
                                            "arg-2", state.arg2);
                state.arg2 = null;
            }

            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                               "MIDlet suite loader starting a suite");
            }

            vmEndStartUp(currentIsolateId);
            midletStateHandler.startSuite(midletSuite, 0,
                  state.midletClassName);

            if (midletProxyList.shutdownInProgress()) {
                /*
                 * The MIDlet was shutdown by either the OS or the
                 * push system. Set the command state to signal this
                 * to the native AMS code.
                 */
                state.status = CommandState.SHUTDOWN;
                midletProxyList.waitForShutdownToComplete();
            } else {
                state.status = CommandState.OK;
            }

            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_CORE,
                   "MIDlet suite loader exiting");
            }
        } catch (Throwable t) {
            state.status =
                MIDletSuiteLoader.handleException(displayEventHandler, t);
        } finally {
            /*
             * Shutdown the event queue gracefully to process any events
             * that may be in the queue currently.
             */
            eventQueue.shutdown();

	    /*
	     * The midletSuite is not closed because the other
	     * active threads may be depending on it.
	     * For example, Display uses isTrusted to update
	     * screen icons.
	     * A native finalizer will take care of unlocking
	     * the native locks.	     
	     */

            state.suiteID = null;
            state.midletClassName = null;

            if (state.status != CommandState.SHUTDOWN) {
                if (lastMidletSuiteToRun != null) {
                    state.lastSuiteID = lastMidletSuiteToRun;
                    state.lastMidletClassName = lastMidletToRun;
                }

                // Check to see if we need to run a selected suite next
                if (AmsUtil.nextMidletSuiteToRun != null) {
                    state.suiteID = AmsUtil.nextMidletSuiteToRun;
                    state.midletClassName = AmsUtil.nextMidletToRun;

                    state.arg0 = AmsUtil.arg0ForNextMidlet;
                    state.arg1 = AmsUtil.arg1ForNextMidlet;
                    state.arg2 = AmsUtil.arg2ForNextMidlet;
                } else if (state.lastSuiteID != null) {
                    state.suiteID = state.lastSuiteID;
                    state.midletClassName = state.lastMidletClassName;

                    /* Avoid an endless loop. */
                    state.lastSuiteID = null;
                    state.lastMidletClassName = null;

                    /*
                     * This could an bad JAD from an auto test suite,
                     * so make sure the status to OK, the native
                     * code will run the last suite.
                     */
                    state.status = CommandState.OK;
                }
            }

            state.save();

            /*
             * Return specific non-zero number so the native AMS code can
             * know that this is graceful exit and not VM abort.
             */
            CommandState.exitInternal(CommandState.MAIN_EXIT);
        }
    }

    /**
     * Common exception handling for MIDlet suite loading.
     *
     * @param handler display event handler to draw displays
     * @param exception Exception to handle
     *
     * @return status code for the exception
     */
    static int handleException(DisplayEventHandler handler, 
            Throwable exception) {
        String logMsg;
        int msgId;
        int status;
        String exceptionMsg;

        if (exception instanceof ClassNotFoundException) {
            logMsg = "MIDlet class(s) not found: " + exception.getMessage();
            msgId =
                ResourceConstants.AMS_MIDLETSUITELDR_CANT_LAUNCH_MISSING_CLASS;
            status = Constants.MIDLET_CLASS_NOT_FOUND;
        } else if (exception instanceof InstantiationException) {
            logMsg = "MIDlet instance(s) could not be created: " +
                exception.getMessage();
            msgId =
                ResourceConstants.AMS_MIDLETSUITELDR_CANT_LAUNCH_ILL_OPERATION;
            status = Constants.MIDLET_INSTANTIATION_EXCEPTION;
        } else if (exception instanceof IllegalAccessException) {
            logMsg = "MIDlet class(s) could not be accessed: " +
                exception.getMessage();
            msgId = 
                ResourceConstants.AMS_MIDLETSUITELDR_CANT_LAUNCH_ILL_OPERATION;
            status = Constants.MIDLET_ILLEGAL_ACCESS_EXCEPTION;
        } else if (exception instanceof OutOfMemoryError) {
            logMsg = "The MIDlet has run out of memory";
            msgId = ResourceConstants.AMS_MIDLETSUITELDR_QUIT_OUT_OF_MEMORY;
            status = Constants.MIDLET_OUT_OF_MEM_ERROR;
        } else {
            if (Logging.TRACE_ENABLED) {
                Logging.trace(exception,
                             "Exception caught in MIDletSuiteLoader");
            }

            logMsg = "Unexpected exception caught in MIDletSuiteLoader";
            msgId = ResourceConstants.AMS_MIDLETSUITELDR_UNEXPECTEDLY_QUIT;
            status = Constants.MIDLET_CONSTRUCTOR_FAILED;
        }

        if (Logging.REPORT_LEVEL <= Logging.ERROR) {
            Logging.report(Logging.ERROR, LogChannels.LC_CORE, logMsg);
        }

        if (exception.getMessage() == null) {
            exceptionMsg = Resource.getString(msgId);
        } else {
            exceptionMsg = Resource.getString(msgId) + "\n\n" +
                exception.getMessage();
        }

        if (Configuration.getIntProperty("DisableStartupErrorAlert", 0) ==
                0) {
            displayException(handler, exceptionMsg);
        }

        return status;
    }

    /**
     * Display an exception to the user.
     *
     * @param handler display event handler to draw displays
     * @param exceptionMsg exception message
     */
    static void displayException(DisplayEventHandler handler, 
        String exceptionMsg) {

        SystemAlert alert = new SystemAlert(handler, "Exception",
              exceptionMsg, null,
              AlertType.ERROR);

        alert.waitForUser();
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invokation will be executed
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
    public static boolean execute(String id, String midlet,
                                  String displayName) {
        return executeWithArgs(id, midlet, displayName, null, null, null);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invokation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     *                      application manager
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
    public static boolean execute(SecurityToken securityToken,
            String id, String midlet, String displayName) {
        return executeWithArgs(securityToken, id, midlet, displayName, null,
                               null, null);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invokation will be executed
     * when the current application is terminated.
     *
     * @param id ID of an installed suite
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
    public static boolean executeWithArgs(String id, String midlet,
            String displayName, String arg0, String arg1, String arg2) {

        return executeWithArgs(null, id, midlet, displayName, arg0, arg1,
                               arg2);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invokation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     * @param id ID of an installed suite
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
    public static boolean executeWithArgs(SecurityToken securityToken,
            String id, String midlet, String displayName,
            String arg0, String arg1, String arg2) {
        return executeWithArgs(securityToken, 0, id, midlet, displayName,
                               arg0, arg1, arg2);
    }

    /**
     * Starts a MIDlet in a new Isolate or
     * queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invokation will be executed
     * when the current application is terminated.
     *
     * @param securityToken security token of the calling class
     * @param externalAppId ID of MIDlet to invoke, given by an external
     *                      application manager
     * @param id ID of an installed suite
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
    public static boolean executeWithArgs(SecurityToken securityToken,
            int externalAppId, String id, String midlet, String displayName,
            String arg0, String arg1, String arg2) {
        MIDletSuiteStorage midletSuiteStorage;

        // Note: getMIDletSuiteStorage performs an AMS permission check.
        if (securityToken != null) {
            midletSuiteStorage =
                MIDletSuiteStorage.getMIDletSuiteStorage(securityToken);
        } else {
            midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
        }

        return AmsUtil.executeWithArgs(midletSuiteStorage, externalAppId, id,
            midlet, displayName, arg0, arg1, arg2);
    }

    /**
     * Gets the unique storage name of the next MIDlet suite to run.
     *
     * @return storage name of a MIDlet suite
     */
    public static String getNextMIDletSuiteToRun() {
        return AmsUtil.nextMidletSuiteToRun;
    }

    /**
     * Gets the name of the next MIDlet to run.
     *
     * @return storage name of a MIDlet
     */
    public static String getNextMIDletToRun() {
        return AmsUtil.nextMidletToRun;
    }

    /**
     * Queues the last suite to run when there is not a next Suite
     * to run. This value will be persistent until it is used.
     * Not used in MVM mode.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     *
     * @exception SecurityException if the caller does not have permission
     *   to manage midlets
     */
    public static void setLastSuiteToRun(String id, String midlet) {
        MIDletSuite midletSuite =
            MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

        // if a MIDlet suite is not scheduled, assume the JAM is calling.
        if (midletSuite != null) {
            midletSuite.checkIfPermissionAllowed(Permissions.AMS);
        }

        lastMidletSuiteToRun = id;
        lastMidletToRun = midlet;
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
     * Register the Isolate ID of the AMS Isolate by making a native
     * method call that will call JVM_CurrentIsolateId and set
     * it in the proper native variable.
     */
    private static native void registerAmsIsolateId();

    /**
     * Send hint to VM about begin of a MIDlet startup phase within specified
     * isolate to allow the VM to fine tune its internal parameters to achieve
     * optimal peformance
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

    /** This class is not meant to be instantiated. */
    private MIDletSuiteLoader() {
    }
}
