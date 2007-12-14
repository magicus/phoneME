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

package com.sun.midp.odd;

import javax.microedition.lcdui.Alert;
import com.sun.midp.odd.remoting.UEIProxyListener;
import com.sun.midp.installer.*;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

/**
 * Core functionality of ODD.
 * 1. Receive requests from the UEI-Proxy (via the UEIProxyListener), and reply back.
 * 2. Execute the request using JAMS internal APIs
 *
 * @author Roy Ben Hayun
 */
//TODO: Jan Sterba - UEIProxyListener support
//TODO: Alexey Z - Usage of JAMS internal APIs according to incoming requests
public class ODTEngine implements UEIProxyListener {
    
    //
    // State machine constants
    //

    /** 
     * Engine states (by array items value):
     * 0. No server running
     * 1. Server started. waiting for connections. 
     * 2. Hanshake
     * 3. Pin authentication 
     *
     */
    //TODO: Roy - to implement
    private static final int[] ENGINE_STATES = new int[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    //
    // Members
    //

    /**
     * Installer
     */
    private Installer installer = new HttpInstaller();

    /**
     * Suite ID of the midlet that the user is going to debug.
     */
    int idOfMidletUnderDebug = MIDletSuite.UNUSED_SUITE_ID;
    
    /**
     * Engine state index
     */
    private int stateIndex = 0;
    
    /**
     * Agent settings
     */
    private final AgentSettings settings;
    
    /** 
     * Progress screen
     */
    private final ProgressScreen progressScreen;
    
    //
    // Life cycle
    //
    
    /**
     * Creates a new instance of ODTEngine
     */
    public ODTEngine(ProgressScreen progressScreen, AgentSettings settings) {
        this.settings = settings;
        this.progressScreen = progressScreen;
    }
    
    //
    // UI triggered Operations
    //
    
    /**
     * Start the server 
     */
    void startAcceptingConnections() {
        stateIndex = 1;
        //TODO: Jan Sterba - start server, ready to accept single incoming connection at a time.
        //  (any subsequent connection, gets rejected)
    }
    
    /**
     * Stop the server
     */
    void stopAcceptingConnections() {
        stateIndex = 0;
        //TODO: Jan Sterba - stop server.        
    }
    
    /**
     * Perform controlled shutdown (e.g., terminate any running installations, sessions  etc)
     */
    void shutdown() {
        stateIndex = 0;
        //TODO: Alexey Z, Jan Sterba - ensure proper shutdown
    }

    boolean isServerRunning() {
        //TODO: Jan Sterba - return true or false according to server readiness.        
        return false;
    }

    //
    // Operations
    //
    
    
    /**
     * Ensure that the currently received request is as expected.
     * For example, "Run"  must come after "Install"
     */
    private void validateEngineState() {
        //TODO: Roy - ensure current request matches next state in line
        boolean valid = true;
        if(!valid){
            progressScreen.log("invalid request");
            //TODO: Alexey Z - cleanup session resource (e.g., terminate MIDlet, uninstall etc)
            //TODO: Jan Sterba - disconnect
        }
    }

    
    /**
     * Display Alert with pin.
     * User is required to type the pin in a dialog that pops on the PC screen, by the UEI-Proxy.
     */
    private void displayPin() {
            //TODO: 1. choose random number
            //TODO: 2. display pin on screen 
    }
    
    //
    // UEIProxyListener implementation
    //
    
    
    public void handleHandshakeRequest() {
        stateIndex = 2;
        validateEngineState();
        progressScreen.clear();
        progressScreen.log("received incoming connection...");
        if (settings.pinRequired) {
            displayPin();
            //TODO: Jan Sterba - send handshake response (with Pin required)
        }
        else{
            //TODO: Jan Sterba - send handshake response  (without pin required)      
        }
    }

    public void handlePinAuthentication() {
        stateIndex = 3;
        validateEngineState();
        progressScreen.log("pin authenticated");
        //TODO: Jan Sterba - compare to Pin and send PASS/FAIL response
    }

    public void handleInstallationRequest(String url) {
        stateIndex = 4;
        validateEngineState();
        progressScreen.log("installing suite [name]...");

        // install the suite

        InvalidJadException exceptionThrown = null;
        int len = url.length();
        boolean jarOnly = (len >= 4 &&
            ".jar".equalsIgnoreCase(url.substring(len - 4, len)));

        // installation listener (can be moved from here)
        final class ODTInstallListener implements InstallListener {
            public boolean warnUser(InstallState state) {
                if (settings.silentInstallation) {
                    return true;
                }
                return false;
            }

            public boolean confirmJarDownload(InstallState state) {
                //if (settings.silentInstallation) {
                //    return true;
                //}
                return true;
            }

            public void updateStatus(int status, InstallState state) {
            }

            public boolean keepRMS(InstallState state) {
                if (settings.silentInstallation) {
                    return true;
                }
                return false;
            }

            public boolean confirmAuthPath(InstallState state) {
                if (settings.silentInstallation) {
                    return true;
                }
                return false;
            }
        }

        int storageId = Constants.INTERNAL_STORAGE_ID;
        
        try {
            if (jarOnly) {
                idOfMidletUnderDebug = installer.installJar(url, null,
                    storageId, false, false, new ODTInstallListener());
            } else {
                idOfMidletUnderDebug = installer.installJad(url, storageId,
                        false, false, new ODTInstallListener());
            }
        } catch (InvalidJadException ex) {
            exceptionThrown = ex;
        } catch (Throwable t) {
            exceptionThrown = new InvalidJadException(
                    InvalidJadException.JAD_NOT_FOUND,
                    "Unknown error.");
        }

        //TODO: Jan Sterba - send response according to installation result
        //      exceptionThrown is null if the installation succeeded
        //      otherwise an error code can be retrieved through
        //      exceptionThrown.getReason() (see InvalidJadException class).

        progressScreen.log("installation status: " +
                ((exceptionThrown == null) ? "SUCCESS!" :
                        "FAILURE, code " + exceptionThrown.getReason()));
    }

    public void handleRunRequest() {
        stateIndex = 5;
        validateEngineState();
        progressScreen.log("starting [midlet]");
        //TODO: Alexey Z - run MIDlet in non-debug mode
        //TODO: Jan Sterba - send response (success/failure)
    }

    public void handleDebugRequest() {
        stateIndex = 5;
        validateEngineState();
        progressScreen.log("starting [midlet] in debug mode");
        //TODO: Alexey Z - run MIDlet in debug mode
        //TODO: Jan Sterba - send response (success/failure)
    }

    public void handleUninstallationRequest() {
        stateIndex = 6;
        validateEngineState();
        progressScreen.log("uninstalling suite [name]...");

        if (idOfMidletUnderDebug != MIDletSuite.UNUSED_SUITE_ID) {
            try {
                MIDletSuiteStorage.getMIDletSuiteStorage().remove(
                        idOfMidletUnderDebug);
            } catch (Exception e) {
                // ignore
            }
        }

        //TODO: Jan Sterba - send response (success/failure)
        //TODO: Jan Sterba - drop off the current connection (disconnect).
        progressScreen.log("waiting for connection...");
    }
    
}
