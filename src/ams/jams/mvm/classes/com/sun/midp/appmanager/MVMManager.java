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

package com.sun.midp.appmanager;

import javax.microedition.midlet.*;

import javax.microedition.lcdui.*;

import com.sun.midp.i18n.*;

import com.sun.midp.midlet.*;

import com.sun.midp.installer.*;

import com.sun.midp.midletsuite.*;

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
                ODTControllerEventConsumer {

    /** Constant for the discovery application class name. */
    private static final String DISCOVERY_APP =
        "com.sun.midp.installer.DiscoveryApp";
    /** Constant for the graphical installer class name. */
    private static final String INSTALLER =
        "com.sun.midp.installer.GraphicalInstaller";
    /** Constant for the CA manager class name. */
    private static final String CA_MANAGER =
        "com.sun.midp.appmanager.CaManager";
    /** Constant for the ODT Agent class name. */
    private static final String ODT_AGENT =
        "com.sun.midp.odd.ODTAgentMIDlet";

    /** True until constructed for the first time. */
    private static boolean first = true;

    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

    /** Screen that displays all installed midlets and installer */
    private AppManagerUI appManagerUI;

    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /** UI to display error alerts. */
    private DisplayError displayError;

    /** If on device debug is active, ID of the suite under debug. */
    private int suiteUnderDebugId = MIDletSuite.UNUSED_SUITE_ID;

    /**
     * Create and initialize a new MVMManager MIDlet.
     */
    public MVMManager() {
        MIDletProxy thisMidlet;

        EventQueue eq = EventQueue.getEventQueue();
        new ODTControllerEventListener(eq, this);

        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        /*
         * Listen to the MIDlet proxy list.
         * this allows us to notify the Application Selector
         * of any changes whenever switch back to the AMS.
         */
        midletProxyList = MIDletProxyList.getMIDletProxyList();
        midletProxyList.addListener(this);

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
        appManagerUI = new AppManagerUI(this, display, displayError, first,
                                        null);

        if (first) {
            first = false;
        }
    }

    /**
     * Processes MIDP_ENABLE_ODD_EVENT
     */
    public void handleEnableODDEvent() {
        appManagerUI.showODTAgent();
    }

    /**
     * Processes MIDP_ODD_START_MIDLET_EVENT
     *
     * @param suiteId ID of the midlet suite
     * @param className class name of the midlet to run
     * @param displayName display name of the midlet to run
     * @param isDebugMode true if the midlet must be started in debug mode,
     *                    false otherwise
     */
    public void handleODDStartMidletEvent(int suiteId, String className,
                                          String displayName,
                                          boolean isDebugMode) {
        if (suiteUnderDebugId != MIDletSuite.UNUSED_SUITE_ID) {
            // suite is already under debug
            return;
        }

        try {
            MIDletSuiteUtils.execute(suiteId, className,
                                     displayName, isDebugMode);
            suiteUnderDebugId = suiteId;
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

        appManagerUI.cleanUp();

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
        appManagerUI.showMidletSwitcher(onlyFromLaunchedList);
    }


    // ==============================================================
    // ------ Implementation of the MIDletProxyListListener interface

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        appManagerUI.notifyMidletStarted(midlet);
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
        appManagerUI.notifyMidletStateChanged(midlet);
    }

    /**
     * Called when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        if (suiteUnderDebugId == midlet.getSuiteId()) {
            MIDletProxy odtAgentMidlet = midletProxyList.findMIDletProxy(
                MIDletSuite.INTERNAL_SUITE_ID, ODT_AGENT);

            if (odtAgentMidlet != null) {
                EventQueue eq = EventQueue.getEventQueue();
                NativeEvent event = new NativeEvent(
                        EventTypes.MIDP_ODD_MIDLET_EXITED_EVENT);
                event.intParam1    = suiteUnderDebugId;
                event.stringParam1 = odtAgentMidlet.getClassName();
                eq.sendNativeEventToIsolate(event,
                        odtAgentMidlet.getIsolateId());
            }

            suiteUnderDebugId = MIDletSuite.UNUSED_SUITE_ID;
        }

        appManagerUI.notifyMidletExited(midlet);
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
        appManagerUI.notifyMidletStartError(suiteId, className,
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

    /** Launch the ODT agent. */
    public void launchODTAgent() {
        try {
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID,
                ODT_AGENT,
                Resource.getString(ResourceConstants.ODT_AGENT_MIDLET));
        } catch (Exception ex) {
            displayError.showErrorAlert(Resource.getString(
                ResourceConstants.ODT_AGENT_MIDLET), ex, null, null);
        }
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
            MIDletSuiteUtils.execute(suiteInfo.suiteId, midletToRun, null);
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
     */
    public void moveToForeground(RunningMIDletSuiteInfo suiteInfo) {
        try {

            if (Constants.MEASURE_STARTUP) {
                System.err.println("Switch To Foreground Time: Begin at " +
                    System.currentTimeMillis());
            }

            if (suiteInfo != null) {
                midletProxyList.setForegroundMIDlet(suiteInfo.proxy);
            }

        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }


    /**
     * Exit the midlet with the passed in midlet suite info.
     *
     * @param suiteInfo information for the midlet to be terminated
     */
    public void exitMidlet(RunningMIDletSuiteInfo suiteInfo) {
        try {
            if (suiteInfo != null) {
                suiteInfo.proxy.destroyMidlet();
            }

        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }

}
