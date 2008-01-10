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

import com.sun.jme.remoting.CommException;
import com.sun.jme.remoting.DefaultNamedObjectRegistry;
import com.sun.jme.remoting.NamedObjectRegistry;
import com.sun.midp.odd.UEIProxyListener;
import com.sun.midp.odd.remoting.RemotingThread;
import com.sun.midp.installer.*;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

/**
 * Core functionality of ODD. 1. Receive requests from the UEI-Proxy (via the
 * UEIProxyListener), and reply back. 2. Execute the request using JAMS internal
 * APIs
 * 
 * @author Roy Ben Hayun
 */
// TODO: Alexey Z - Usage of JAMS internal APIs according to incoming requests
public class ODTEngine implements UEIProxyListener {

    //
    // State machine constants
    //

    /**
     * Engine states (by array items value): 0. No server running 1. Server
     * started. waiting for connections. 2. Hanshake 3. Pin authentication
     * 
     */
    // TODO: Roy - to implement
    private static final int[] ENGINE_STATES = new int[] { 0, 1, 2, 3, 4, 5, 6,
            7, 8, 9 };

    //
    // Members
    //

    /**
     * Installer
     */
    private Installer installer = new HttpInstaller();

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

    private final RemotingThread server;
    
    private String generatedPin;

    //
    // Life cycle
    //

    /**
     * Creates a new instance of ODTEngine
     */
    public ODTEngine(ProgressScreen progressScreen, AgentSettings settings) {
        this.settings = settings;
        this.progressScreen = progressScreen;
        DefaultNamedObjectRegistry nor = new DefaultNamedObjectRegistry();
        nor.registerRemotableObject("UEIProxyListener", this,
                UEIProxyListener.class);
        this.server = new RemotingThread(nor, progressScreen);
    }

    //
    // UI triggered Operations
    //

    /**
     * Start the server
     */
    void startAcceptingConnections() {
        stateIndex = 1;
        server.start();
    }

    /**
     * Stop the server
     */
    void stopAcceptingConnections() {
        stateIndex = 0;
        server.stop();
    }

    /**
     * Perform controlled shutdown (e.g., terminate any running installations,
     * sessions etc)
     */
    void shutdown() {
        stateIndex = 0;
        stopAcceptingConnections();
    }

    boolean isServerRunning() {
        return server.isRunning();
    }

    //
    // Operations
    //

    /**
     * Ensure that the currently received request is as expected. For example,
     * "Run" must come after "Install"
     */
    private void validateEngineState() {
        // TODO: Roy - ensure current request matches next state in line
        boolean valid = true;
        if (!valid) {
            progressScreen.log("invalid request");
            // TODO: Alexey Z - cleanup session resource (e.g., terminate
            // MIDlet, uninstall etc)
            stopAcceptingConnections();
        }
    }

    /**
     * Display Alert with pin. User is required to type the pin in a dialog that
     * pops on the PC screen, by the UEI-Proxy.
     */
    private void displayPin() {
        long pin = System.currentTimeMillis() % 9999;
        if (pin < 1000) {
            pin += 1000; //make it 4 digits
        }
        generatedPin = String.valueOf(pin);
        progressScreen.log("Enter pin "+generatedPin+" on PC side.");
    }

    //
    // UEIProxyListener implementation
    //

    private int installApplication(String url) {
        stateIndex = 4;
        validateEngineState();
        progressScreen.log("installing suite...");

        // install the suite
        InvalidJadException exceptionThrown = null;
        int len = url.length();
        boolean jarOnly = (len >= 4 && ".jar".equalsIgnoreCase(url.substring(
                len - 4, len)));

        // installation listener (can be moved from here)
        final class ODTInstallListener implements InstallListener {
            public boolean warnUser(InstallState state) {
                if (settings.silentInstallation) {
                    return true;
                }
                return false;
            }

            public boolean confirmJarDownload(InstallState state) {
                // if (settings.silentInstallation) {
                // return true;
                // }
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
        int midletId = -1;

        try {
            if (jarOnly) {
                midletId = installer.installJar(url, null, storageId, false,
                        false, new ODTInstallListener());
            } else {
                midletId = installer.installJad(url, storageId, false, false,
                        new ODTInstallListener());
            }
        } catch (InvalidJadException ex) {
            ex.printStackTrace();
            exceptionThrown = ex;
        } catch (Throwable t) {
            exceptionThrown = new InvalidJadException(
                    InvalidJadException.JAD_NOT_FOUND, "Unknown error.");
        }

        // TODO: Jan Sterba - send response according to installation result
        // exceptionThrown is null if the installation succeeded
        // otherwise an error code can be retrieved through
        // exceptionThrown.getReason() (see InvalidJadException class).

        progressScreen.log("installation status: "
                + ((exceptionThrown == null) ? "SUCCESS!" : "FAILURE, code "
                        + exceptionThrown.getReason()));
        return midletId;
    }

    public void uninstallApplication(int appId) {
        stateIndex = 6;
        validateEngineState();
        progressScreen.log("uninstalling suite...");

        if (appId != MIDletSuite.UNUSED_SUITE_ID) {
            try {
                MIDletSuiteStorage.getMIDletSuiteStorage().remove(appId);
            } catch (Exception e) {
                // ignore
            }
        }

        // TODO: Jan Sterba - send response (success/failure)
        // TODO: Jan Sterba - drop off the current connection (disconnect).
        progressScreen.log("waiting for connection...");
    }

    public int getAPIVersion() throws CommException {
        stateIndex = 2;
        validateEngineState();
        progressScreen.clear();
        progressScreen.log("handshaking...");
        return API_VERSION;
    }

    public boolean requiresPinAuthentication() throws CommException {
        if (settings.pinRequired) {
            displayPin();
            return true;
        } else {
            return false;
        }
    }

    public boolean verifyPin(String pinNumber) throws CommException {
        stateIndex = 3;
        validateEngineState();
        boolean result = false;
        if (generatedPin == null) {
            result = !settings.pinRequired;
        } else {
            result = generatedPin.equals(pinNumber.trim());
        }
        if (result) {
            progressScreen.log("pin authenticated");            
        } else {
            progressScreen.log("pin authentication failed");
        }
        return result;
    }

    public void runApplication(String url) throws CommException {
        stateIndex = 5;
        validateEngineState();
        progressScreen.log("starting midlet");
        int appId = installApplication(url);
        // TODO: Alexey Z - run MIDlet in debug mode
        uninstallApplication(appId);
    }

    public void debugApplication(String url) throws CommException {
        stateIndex = 5;
        validateEngineState();
        progressScreen.log("starting midlet in debug mode");
        int appId = installApplication(url);
        // TODO: Alexey Z - run MIDlet in non-debug mode
        uninstallApplication(appId);
    }

}
