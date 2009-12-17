/*
 *
 *
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

package com.sun.midp.appmanager;

import java.util.Enumeration;

import javax.microedition.midlet.*;

import javax.microedition.lcdui.*;

import com.sun.midp.i18n.*;

import com.sun.midp.midlet.*;

import com.sun.midp.installer.*;

import com.sun.midp.main.*;

import com.sun.midp.configurator.Constants;

import com.sun.midp.events.EventQueue;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.events.EventTypes;

/**
 * This is an implementation of the ApplicationManager interface
 * for the MVM mode of the VM capable of running with
 * more than 1 midlet concurrently.
 *
 * Application manager controls midlet life cycle:
 *    - installs, updates and removes midlets/midlet suites
 *    - launches, moves to foreground and terminates midlets
 *    - displays info about a midlet/midlet suite
 *    - shuts down the AMS system
 */
public class MVMManager extends MIDlet
    implements MIDletProxyListListener,
                DisplayControllerListener, 
                ApplicationManager,
                ODTControllerEventConsumer,
                AMSServicesEventConsumer,
                InstallerEventConsumer,
                InstallerResultEventConsumer
{

    /** Constant for the discovery application class name. */
    private static final String DISCOVERY_APP =
        "com.sun.midp.installer.DiscoveryApp";
    /** Constant for the graphical installer class name. */
    private static final String INSTALLER =
        "com.sun.midp.installer.GraphicalInstaller";
    /** Constant for the CA manager class name. */
    private static final String CA_MANAGER =
        "com.sun.midp.appmanager.CaManager";
    /** Constant for the Component manager class name. */
    private static final String COMP_MANAGER =
        "com.sun.midp.appmanager.ComponentManager";
    /** Constant for the ODT Agent class name. */
    private static final String ODT_AGENT =
        "com.sun.midp.odd.ODTAgentMIDlet";

    /** True until constructed for the first time. */
    private static boolean first = true;

    /** Screen that displays all installed midlets and installer */
    private AppManagerPeer appManager;

    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /** UI to display error alerts. */
    private DisplayError displayError;

    /** If on device debug is active, ID of the suite under debug. */
    private int suiteUnderDebugId = MIDletSuite.UNUSED_SUITE_ID;

    /** A thread waiting until a midlet is terminated. Can be null. */
    private Thread waitForDestroyThread;

    /** A synchronization object. */
    private static final Object waitForDestroy = new Object();

    /**
     * Create and initialize a new MVMManager MIDlet.
     */
    public MVMManager() {
        MIDletProxy thisMidlet;

        EventQueue eq = EventQueue.getEventQueue();
        new ODTControllerEventListener(eq, this);
        new AMSServicesEventListener(eq, this);
        new InstallerEventListener(eq, this);
        new InstallerResultEventListener(eq, this);

        midletProxyList = MIDletProxyList.getMIDletProxyList();

        // The proxy for this MIDlet may not have been create yet.
        for (; ; ) {
            thisMidlet = midletProxyList.findMIDletProxy(
                MIDletSuite.INTERNAL_SUITE_ID, this.getClass().getName());

            if (thisMidlet != null) {
                break;
            }

            try {
                Thread.sleep(10);
            } catch (InterruptedException ie) {
                // ignore
            }
        }

        MVMDisplayController dc = new MVMDisplayController(
            midletProxyList, thisMidlet); 
        midletProxyList.setDisplayController(dc);
        dc.addListener(this);

        IndicatorManager.init(midletProxyList);

        GraphicalInstaller.initSettings();

        first = (getAppProperty("logo-displayed") == null);

        Display display = Display.getDisplay(this);
        displayError = new DisplayError(display);

        // AppManagerUI will be set to be current at the end of its constructor
        appManager = new AppManagerPeer(this, display, displayError, first,
                                        null);

        /*
         * Listen to the MIDlet proxy list.
         * This allows us to notify the Application Selector
         * of any changes whenever switch back to the AMS.
         * The listener must be set up after finishing all
         * initialization in the constructor.
         */
        midletProxyList.addListener(this);

        processArguments();

        if (first) {
            first = false;
        }
    }

    /**
     * Processes MIDP_ENABLE_ODD_EVENT
     */
    public void handleEnableODDEvent() {
        appManager.showODTAgent();
    }

    /**
     * Processes MIDP_ODD_START_MIDLET_EVENT
     *
     * @param suiteId ID of the midlet suite
     * @param className class name of the midlet to run
     * @param displayName display name of the midlet to run
     * @param debugMode debug option for the MIDlet to be launched, one of:
     * MIDP_NO_DEBUG, MIDP_DEBUG_SUSPEND, MIDP_DEBUG_NO_SUSPEND
     */
    public void handleODDStartMidletEvent(int suiteId, String className,
                                          String displayName,
                                          int debugMode) {
        /* For the case of showing MIDlet selector, we need AMS to have
         * foreground. */
        MIDletProxy thisMidlet = midletProxyList.findMIDletProxy(
            MIDletSuite.INTERNAL_SUITE_ID, this.getClass().getName());
        midletProxyList.setForegroundMIDlet(thisMidlet);

        if (suiteUnderDebugId != MIDletSuite.UNUSED_SUITE_ID) {
            /*
             * IMPL NOTE: this forces only one running MIDlet in debug mode -
             * the VM currently does not support more MIDlets in debug mode
             * at the same time.
             */
            debugMode = Constants.MIDP_NO_DEBUG;
        }

        try {
            appManager.launchSuite(suiteId, className, debugMode, true);
            if (debugMode != Constants.MIDP_NO_DEBUG) {
                suiteUnderDebugId = suiteId;
            }
        } catch (Exception ex) {
            displayError.showErrorAlert(displayName, ex, null, null);
        }
    }

    /**
     * Processes MIDP_ODD_EXIT_MIDLET_EVENT.
     *
     * @param suiteId ID of the midlet suite
     * @param className class name of the midlet to exit or <code>NULL</code>
     *      if all MIDlets from the suite should be exited
     */
    public void handleODDExitMidletEvent(final int suiteId, 
                                         final String className) {
        appManager.exitSuite(suiteId, className);
    }
    
    /**
     * Processes MIDP_ODD_SUITE_INSTALLED_EVENT. This event indicates that
     * a new MIDlet suite has been installed by ODT agent. It is signal for 
     * the application manager to update the displayed list of MIDlets.
     * 
     * @param suiteId ID of the newly installed MIDlet suite       
     */
    public void handleODDSuiteInstalledEvent(int suiteId) {
        appManager.notifySuiteInstalled(suiteId);
    }

    /**
     * Processes MIDP_ODD_SUITE_REMOVED_EVENT. This event indicates that
     * an installed MIDlet suite has been removed by ODT agent. It is signal for 
     * the application manager to update the displayed list of MIDlets.
     * 
     * @param suiteId ID of the removed MIDlet suite          
     */
    public void handleODDSuiteRemovedEvent(int suiteId) {
        appManager.notifySuiteRemoved(suiteId);
    }


    /**
     * Processes MIDP_ODD_REQUEST_INSTALLATION_EVENT.
     *
     * @param url URL to install from
     * @param foce Installation "force" flag
     */
    public void handleODDRequestInstallationEvent(String url, boolean force) {
        String displayName =
                Resource.getString(ResourceConstants.INSTALL_APPLICATION);
        try {
            appManager.suppressNextRunMIDletQuestion();
            MIDletSuiteUtils.executeWithArgs(MIDletSuite.INTERNAL_SUITE_ID,
                                      INSTALLER,
                                      displayName,
                                      force ? "FI" : "I", url,
                                      null);
        } catch (Exception ex) {
            displayError.showErrorAlert(displayName, ex, null, null);
            handleODDInstallationDoneEvent(INSTALLER_RESULT_FAILURE,
                    MIDletSuite.UNUSED_SUITE_ID, ex.getClass().getName(),
                    ex.getMessage(), 0, null);
        }
    }


    /**
     * Processes MIDP_ODD_INSTALATION_DONE_EVENT.
     *
     * @param result One of INSTALLATION_RESULT_* values.
     * @param suiteId Id of installed suite, UNUSED_SUITE_ID in case of
     *                unsuccessful installation
     * @param exception Name of the thrown exception if any
     * @param message Message of the thrown exception if any
     * @param reason Reason of the failure if applicable on the thrown exception
     * @param extraData Extra data if applicable on the thrown exception
     */
    public void handleODDInstallationDoneEvent(int result, int suiteId,
            String exception, String message, int reason, String extraData) {

        if (result != INSTALLER_RESULT_SUCCESS) {
            appManager.allowRunMIDletQuestion();
        }
        
        MIDletProxy odtAgentMidlet = midletProxyList.findMIDletProxy(
            MIDletSuite.INTERNAL_SUITE_ID, ODT_AGENT);

        if (odtAgentMidlet == null) {
            return;
        }

        EventQueue eq = EventQueue.getEventQueue();
        NativeEvent event = new NativeEvent(
                EventTypes.MIDP_INSTALLATION_DONE_EVENT);
        event.intParam1    = result;
        event.intParam2    = suiteId;
        event.stringParam1 = exception;
        event.stringParam2 = message;
        event.intParam3    = reason;
        event.stringParam3 = extraData;

        eq.sendNativeEventToIsolate(event,
                odtAgentMidlet.getIsolateId());
    }

    /**
     * Processes MIDP_KILL_MIDLETS_EVENT.
     *
     * @param suiteId ID of the midlet suite from which the midlets
     *                must be killed
     * @param isolateId ID of the isolate requested this operation
     */
    public void handleKillMIDletsEvent(int suiteId, int isolateId) {
        try {
            int timeout = 1000 * Configuration.getIntProperty(
                    "destoryMIDletTimeout", 5);
            Enumeration proxies = midletProxyList.getMIDlets();

            while (proxies.hasMoreElements()) {
                MIDletProxy mp = (MIDletProxy)proxies.nextElement();
                if (mp.getSuiteId() != suiteId) {
                    continue;
                }

                // kill the running midlet and wait until it is destroyed
                mp.destroyMidlet();

                try {
                    synchronized(waitForDestroy) {
                        waitForDestroy.wait(timeout);
                    }
                } catch (InterruptedException ie) {
                    // ignore
                }
            }
        } catch (Exception ex) {
            displayError.showErrorAlert(null, ex, null, null);
        } finally {
            // notify the listeners that the midlets were killed
            NativeEvent event = new NativeEvent(
                    EventTypes.MIDP_MIDLETS_KILLED_EVENT);
            event.intParam1 = suiteId;

            EventQueue eq = EventQueue.getEventQueue();
            eq.sendNativeEventToIsolate(event, isolateId);
        }
    }

    /**
     * Processes RESTART_MIDLET_EVENT.
     *
     * @param externalAppId application ID assigned by externall App Manager;
     *                      not used if there is no external manager
     * @param suiteId ID of the midlet suite
     * @param className class name of the midlet to restart
     * @param displayName display name of the midlet to restart
     */
    public void handleRestartMIDletEvent(int externalAppId, int suiteId,
                                         String className, String displayName) {
        try {
            final MIDletProxy mp = midletProxyList.findMIDletProxy(suiteId,
                                                                   className);
            if (mp != null) {
                final int id = suiteId;
                final String nameOfClass  = className;
                final String nameOfMIDlet = displayName;

                // wait until the midlet is destroyed
                waitForDestroyThread = new Thread() {
                    public void run() {
                        try {
                            int timeout = 1000 *
                                Configuration.getIntProperty(
                                    "destoryMIDletTimeout", 5);

                            synchronized(waitForDestroyThread) {
                                wait(timeout);
                            }
                        } catch(InterruptedException ie) {
                            // ignore
                        }

                        MIDletSuiteUtils.execute(id, nameOfClass,
                                                 nameOfMIDlet);
                    }
                };

                waitForDestroyThread.start();

                mp.destroyMidlet();
            } else {
                // the midlet is not running, just starting it
                MIDletSuiteUtils.execute(suiteId, className,
                                         displayName);
            }
        } catch (Exception ex) {
            displayError.showErrorAlert(displayName, ex, null, null);
        }
    }

    // =================================================================
    // ---------- Operations that can be performed on Midlets ----------

    /**
     * Start app; there is nothing that needs to be done at start up.
     */
    public void startApp() {
    }

    /**
     * Pause; there are no resources that need to be released.
     */
    public void pauseApp() {
    }

    /**
     * Destroy midlet. Cleans up the resources used.
     *
     * @param unconditional is ignored; this object always
     * destroys itself when requested.
     */
    public void destroyApp(boolean unconditional) {
        /*
         * Save user settings such as currently selected MIDlet
         * This may not be needed since we are always running
         * IMPL_NOTE: remove this
         */
        GraphicalInstaller.saveSettings(null, MIDletSuite.UNUSED_SUITE_ID);

        appManager.cleanUp();

        // Ending the MIDlet ends all others.
        midletProxyList.shutdown();
    }

    // ==============================================================
    // ------ Implementation of the DisplayControllerListener interface
    /**
     * Called when going to select midlet to
     * bring it to foreground.
     *
     * @param onlyFromLaunchedList true if midlet should
     *        be selected from the list of already launched midlets,
     *        if false then possibility to launch midlet is needed.
     */
    public void selectForeground(boolean onlyFromLaunchedList) {
        appManager.showMidletSwitcher(onlyFromLaunchedList);
    }


    // ==============================================================
    // ------ Implementation of the MIDletProxyListListener interface

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        appManager.notifyMidletStarted(midlet);
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
        appManager.notifyMidletStateChanged(midlet);
    }
    
    /**
     * Called when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        synchronized(waitForDestroy) {
            waitForDestroy.notify();
        }
        
        appManager.notifyMidletExited(midlet);
    }

    /**
     * Requests ODT agent to exit running commands for the given suite.
     * @param suiteInfo Suite whose commands should be exited.
     */
    private void exitODTCommand(RunningMIDletSuiteInfo suiteInfo) {
        MIDletProxy odtAgentMidlet = midletProxyList.findMIDletProxy(
            MIDletSuite.INTERNAL_SUITE_ID, ODT_AGENT);

        if (odtAgentMidlet != null) {
            EventQueue eq = EventQueue.getEventQueue();
            NativeEvent event = new NativeEvent(
                    EventTypes.MIDP_ODD_MIDLET_EXITED_EVENT);
            event.intParam1    = suiteInfo.suiteId;
            event.stringParam1 = suiteInfo.midletToRun;
            eq.sendNativeEventToIsolate(event,
                    odtAgentMidlet.getIsolateId());
        }

        if (suiteUnderDebugId == suiteInfo.suiteId) {
            suiteUnderDebugId = MIDletSuite.UNUSED_SUITE_ID;
        }
    }
    
    /**
     * Called when a suite exited (last running MIDlet in suite exited).
     * @param suiteInfo Suite which just exited
     * @param className the running MIDlet class name
     */
    public void notifySuiteExited(RunningMIDletSuiteInfo suiteInfo, String className) {
        if (!suiteInfo.isLocked()) {
            exitODTCommand(suiteInfo);
        }
        
        appManager.notifySuiteExited(suiteInfo);

        if (waitForDestroyThread != null) {
            synchronized(waitForDestroyThread) {
                waitForDestroyThread.notify();
                waitForDestroyThread = null;
            }
        }
    }

    /**
     * Handle exit of MIDlet selector.
     * @param suiteInfo Containing ID of suite
     */
    public void notifyMIDletSelectorExited(RunningMIDletSuiteInfo suiteInfo) {
        if (!suiteInfo.isLocked()) {
            exitODTCommand(suiteInfo);
        }
        
        appManager.notifyMIDletSelectorExited(suiteInfo);
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId,
				 String className, int errorCode, String errorDetails) {
        appManager.notifyMidletStartError(suiteId, className,
            errorCode, errorDetails);
    }

    // ==============================================================
    // ------ Implementation of the MIDletProxyListListener interface

    /** Discover and install a suite. */
    public void installSuite() {
        try {
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                DISCOVERY_APP,
                Resource.getString(ResourceConstants.INSTALL_APPLICATION));
        } catch (Exception ex) {
            displayError.showErrorAlert(Resource.getString(
                                  ResourceConstants.INSTALL_APPLICATION),
                                  ex, null, null);
        }
    }

    /** Launch the CA manager. */
    public void launchCaManager() {
        try {
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                CA_MANAGER,
                Resource.getString(ResourceConstants.CA_MANAGER_APP));
        } catch (Exception ex) {
            displayError.showErrorAlert(Resource.getString(
                ResourceConstants.CA_MANAGER_APP), ex, null, null);
        }
    }

    /** Launch the Component manager. */
    public void launchComponentManager() {
        try {
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                COMP_MANAGER,
                Resource.getString(ResourceConstants.COMP_MANAGER_APP));
        } catch (Exception ex) {
            displayError.showErrorAlert(Resource.getString(
                ResourceConstants.COMP_MANAGER_APP), ex, null, null);
        }
    }

    /** Launch the ODT agent. */
    public void launchODTAgent() {
        launchODTAgent(null);
    }

    /**
     * Launches a suite.
     *
     * @param suiteInfo information for suite to launch
     * @param midletToRun class name of the MIDlet to launch
     */
    public void launchSuite(RunningMIDletSuiteInfo suiteInfo,
                            String midletToRun) {

        if (Constants.MEASURE_STARTUP) {
            System.err.println("Application Startup Time: Begin at "
                                +System.currentTimeMillis());
        }

        try {
            // Create an instance of the MIDlet class
            // All other initialization happens in MIDlet constructor
            MIDletSuiteUtils.execute(suiteInfo.suiteId, midletToRun, null, 
                                     suiteInfo.debugMode);
        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }
 
    /**
     * Update a suite.
     *
     * @param suiteInfo information for suite to update
     */
    public void updateSuite(RunningMIDletSuiteInfo suiteInfo) {
        /*
         * Setting arg 0 to "U" signals that arg 1 is a suite ID for updating.
         */
        try {
            MIDletSuiteUtils.executeWithArgs(MIDletSuite.INTERNAL_SUITE_ID,
                                      INSTALLER,
                                      suiteInfo.displayName,
                                      "U", String.valueOf(suiteInfo.suiteId),
                                      null);
        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }

    /**
     * Shut down the system
     */
    public void shutDown() {
        midletProxyList.shutdown();
    }

    /**
     * Bring the midlet with the passed in midlet suite info to the
     * foreground.
     *
     * @param suiteInfo information for the midlet to be put to foreground
     * @param className the running MIDlet class name
     */
    public void moveToForeground(RunningMIDletSuiteInfo suiteInfo, String className) {
        try {

            if (Constants.MEASURE_STARTUP) {
                System.err.println("Switch To Foreground Time: Begin at " +
                    System.currentTimeMillis());
            }

            if (suiteInfo != null) {
                midletProxyList.setForegroundMIDlet(suiteInfo.getProxyFor(className));
            }

        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }


    /**
     * Exit the midlet with the passed in midlet suite info.
     *
     * @param suiteInfo information for the midlet to be terminated
     * @param className the running MIDlet class name
     */
    public void exitMidlet(RunningMIDletSuiteInfo suiteInfo, String className) {
        try {
            if (suiteInfo != null) {
                MIDletProxy proxy = suiteInfo.getProxyFor(className);
                if (proxy != null) {
                    proxy.destroyMidlet();
                }
            }

        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }

    /**
     * Processes the arguments of this MIDletSuite.
     */
    private void processArguments() {
        for (int i = 0; i < 3; ++i) {
            final String argValue = getAppProperty("arg-" + i);
            if (argValue == null) {
                // no more arguments
                break;
            }

            if (argValue.equals("-runodtagent")) {
                launchODTAgent(null);
            } else if (argValue.startsWith("-runodtagent:")) {
                final int colonIndex = argValue.indexOf(':');
                final String odtAgentSettings = 
                        argValue.substring(colonIndex + 1);
                launchODTAgent(odtAgentSettings);
            }
        }
    }

    /**
     * Launches the ODT agent with the given settings.
     * 
     * @param odtAgentSettings string containing the ODT agent settings or
     *      <code>null</code> if the default settings should be used
     */
    private void launchODTAgent(final String odtAgentSettings) {
        try {
            MIDletSuiteUtils.executeWithArgs(
                    MIDletSuite.INTERNAL_SUITE_ID,
                    ODT_AGENT,
                    Resource.getString(ResourceConstants.ODT_AGENT_MIDLET),
                    odtAgentSettings, null, null);
        } catch (final Exception ex) {
            displayError.showErrorAlert(
                    Resource.getString(ResourceConstants.ODT_AGENT_MIDLET), 
                    ex, null, null);
        }
    }
}
