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

import javax.microedition.midlet.*;

import javax.microedition.lcdui.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.cldc.isolate.*;

import com.sun.midp.midlet.*;

import com.sun.midp.installer.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.main.*;

import com.sun.midp.io.j2me.push.*;

import com.sun.midp.configurator.Constants;


/**
 * This is an implementation of the ApplicationManager interface
 * for the MVM mode of the VM running with only one midlet at time.
 *
 * Application manager controls midlet life cycle:
 *    - installs, updates and removes midlets/midlet suites
 *    - launches and terminates midlets
 *    - displays info about a midlet/midlet suite
 *    - shuts down the AMS system
 */
public class SMMManager extends MIDlet
    implements MIDletProxyListListener, Runnable, ApplicationManager {

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

    /** True until constructed for the first time. */
    private static boolean first = true;

    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

    /** Screen that displays all installed midlets and installer */
    private AppManagerPeer appManager;

    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /** If true, MIDlet can be launched. */
    private boolean allowMidletLaunch = true;

    /** ID of the suite to run. */
    int suiteId;

    /** Class name of the MIDlet to run. */
    String className;

    /** Display name of the MIDlet to run. */
    String displayName;

    /**
     * If not null, this string will be available to the
     * MIDlet as application property arg-0
     */
    String arg0;

    /**
     * If not null, this string will be available to the
     * MIDlet as application property arg-1
     */
    String arg1;

    /**
     * If not null, this string will be available to the
     * MIDlet as application property arg-2
     */
    String arg2;


    /** UI to display error alerts. */
    DisplayError displayError;

    /**
     * Create and initialize a new Manager MIDlet.
     */
    public SMMManager() {
        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        midletProxyList = MIDletProxyList.getMIDletProxyList();
        midletProxyList.setDisplayController(
            new SMMDisplayController(midletProxyList,
                MIDletSuite.INTERNAL_SUITE_ID, this.getClass().getName()));

        PushRegistryInternal.setMvmSingleMidletMode();

        GraphicalInstaller.initSettings();

        first = (getAppProperty("logo-displayed") == null);

        Display display = Display.getDisplay(this);
        displayError = new DisplayError(display);

        // AppSelector will be set to be current at the end of its constructor
        appManager = new AppManagerPeer(this, display, displayError, first, null);

        if (first) {
            first = false;
        }

        /*
         * Listen to the MIDlet proxy list.
         * This allows us to notify the Application Selector
         * of any changes whenever switch back to the AMS.
         * The listener must be set up after finishing all
         * initialization in the constructor.
         */
        midletProxyList.addListener(this);
    }

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
     * Destroy cleans up.
     *
     * @param unconditional is ignored; this object always
     * destroys itself when requested.
     */
    public void destroyApp(boolean unconditional) {
        // IMPL_NOTE: remove this:
        GraphicalInstaller.saveSettings(null, MIDletSuite.UNUSED_SUITE_ID);

        appManager.cleanUp();

        // Ending this MIDlet ends all others.
        midletProxyList.shutdown();
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
        appManager.notifyMidletExited(midlet);
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error code
     */
    public void midletStartError(int externalAppId, int suiteId,
                                 String className, int errorCode,
                                 String errorDetails) {
        allowMidletLaunch = true;
        appManager.notifyMidletStartError(suiteId, className,
                errorCode, errorDetails);
    }


    /** Launches an application Isolate and waits for it to end. */
    public void run() {
        try {
            Isolate appIsolate = AmsUtil.startMidletInNewIsolate(suiteId,
                                     className, displayName,
                                     arg0, arg1, arg2);

            appIsolate.waitForExit();
        } finally {
            suiteId = MIDletSuite.UNUSED_SUITE_ID;
            className = null;
            displayName = null;
            arg0 = null;
            arg1 = null;
            arg2 = null;
            allowMidletLaunch = true;
        }
    }

    // ===================================================================
    // ---- Implementation of the ApplicationManager interface ------------

    /**
     * Discover and install a suite.
     */
    public void installSuite() {
        try {
            runMidlet(MIDletSuite.INTERNAL_SUITE_ID, DISCOVERY_APP,
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
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID, CA_MANAGER,
                Resource.getString(ResourceConstants.CA_MANAGER_APP));
        } catch (Exception ex) {
            displayError.showErrorAlert(Resource.getString(
                ResourceConstants.CA_MANAGER_APP), ex, null, null);
        }
    }

    /**
     * Launch the component manager.
     */
    public void launchComponentManager() {
        try {
            MIDletSuiteUtils.execute(MIDletSuite.INTERNAL_SUITE_ID, COMP_MANAGER,
                Resource.getString(ResourceConstants.COMP_MANAGER_APP));
        } catch (Exception ex) {
            displayError.showErrorAlert(Resource.getString(
                ResourceConstants.COMP_MANAGER_APP), ex, null, null);
        }
    }

    /** Launch ODT Agent. */
    public void launchODTAgent() {
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
            runMidlet(suiteInfo.suiteId, midletToRun,
                      suiteInfo.displayName);

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
        try {
            runMidlet(MIDletSuite.INTERNAL_SUITE_ID, INSTALLER,
                      null, "U", String.valueOf(suiteInfo.suiteId), null);

        } catch (Exception ex) {
            displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
        }
    }

    /**
     * Shut downt the system
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
    public void moveToForeground(RunningMIDletSuiteInfo suiteInfo, String className) {}


    /**
     * Exit the midlet with the passed in midlet suite info.
     *
     * @param suiteInfo information for the midlet to be terminated
     * @param className the running MIDlet class name
     */
    public void exitMidlet(RunningMIDletSuiteInfo suiteInfo, String className) {}


    /**
     * Handle exit of MIDlet suite (the only MIDlet in sute exited or MIDlet
     * selector exited).
     * @param suiteInfo Containing ID of exited suite
     * @param className the running MIDlet class name
     */
    public void notifySuiteExited(RunningMIDletSuiteInfo suiteInfo, String className) {}

    /**
     * Handle exit of MIDlet selector.
     * @param suiteInfo Containing ID of suite
     */
    public void notifyMIDletSelectorExited(RunningMIDletSuiteInfo suiteInfo) {}
    
    // ==============================================================
    // ----------------- PRIVATE methods ---------------------------

    /**
     * Starts a MIDlet in a new Isolate.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     */
    private void runMidlet(int id, String midlet, String displayName) {
        runMidlet(id, midlet, displayName, null, null, null);
    }

    /**
     * Starts a MIDlet in a new Isolate with the given arguments.
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
     */
    private void runMidlet(int id, String midlet,
            String displayName, String arg0, String arg1, String arg2) {
        if (!allowMidletLaunch) {
            return;
        }

        this.suiteId = id;
        this.className = midlet;
        this.displayName = displayName;
        this.arg0 = arg0;
        this.arg1 = arg1;
        this.arg2 = arg2;

        new Thread(this).start();

        allowMidletLaunch = false;
    }
}
