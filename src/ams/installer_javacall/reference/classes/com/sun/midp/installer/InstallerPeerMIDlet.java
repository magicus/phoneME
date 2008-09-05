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

package com.sun.midp.installer;

import javax.microedition.midlet.MIDlet;
import com.sun.midp.configurator.Constants;
import com.sun.midp.midlet.MIDletSuite;

/**
 * A MIDlet passing the installer's requests and responses between
 * Java and native code. Allows to install midlet either from
 * an http(s):// or from a file:// URL, or from a file given by
 * a local path.
 * <p>
 * The MIDlet uses certain application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: an ID of this application
 *   <li>arg-1: URL of the midlet suite to install
 *   <li>arg-2: a storage ID where to save the suite's jar file
 */
public class InstallerPeerMIDlet extends MIDlet implements InstallListener,
        Runnable {
    /**
     * Code of the "Warn User" request to the native callback.
     */
    private static final int RQ_WARN_USER            = 1;

    /**
     * Code of the "Confirm Jar Download" request to the native callback. 
     */
    private static final int RQ_CONFIRM_JAR_DOWNLOAD = 2;

    /**
     * Code of the "Update Installation Status" request to the native callback. 
     */
    private static final int RQ_UPDATE_STATUS        = 3;

    /**
     * Code of the "Ask If The Suite Data Should Be Retained"
     * request to the native callback.
     */
    private static final int RQ_ASK_KEEP_RMS         = 4;

    /**
     * Code of the "Confirm Authorization Path" request to the native callback.
     */
    private static final int RQ_CONFIRM_AUTH_PATH    = 5;

    /**
     * Code of the "Confirm Redirection" request to the native callback.
     */
    private static final int RQ_CONFIRM_REDIRECT     = 6;

    /** ID assigned to this application by the application manager */
    private int appId; 

    /**
     * Create and initialize the MIDlet.
     */
    public InstallerPeerMIDlet() {
        new Thread(this).start();
    }

    /**
     * Start.
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
    }

    /** Installs a new MIDlet suite. */
    public void run() {
        Installer installer = null;
        String installerClassName = null;
        final String supportedUrlTypes[] = {
            "http",  "HttpInstaller",
            "https", "HttpInstaller",
            "file",  "FileInstaller"
        };

        // parse the arguments
        String arg0 = getAppProperty("arg-0");
        boolean err = false;
System.out.println(">>> arg0 = '" + arg0 + "'");
        if (arg0 != null) {
            try {
                appId = Integer.parseInt(arg0);
            } catch (NumberFormatException nfe) {
                err = true;
            }
        } else {
            err = true;
        }

        if (err) {
System.out.println(">>> (1) ERROR!");
            reportFinished0(-1, MIDletSuite.UNUSED_SUITE_ID,
                            "Application ID is not given or invalid.");
            notifyDestroyed();
            return;
        }

        String url = getAppProperty("arg-1");
System.out.println(">>> url = '" + url + "'");
        if (url == null) {
            reportFinished0(appId, MIDletSuite.UNUSED_SUITE_ID,
                            "URL to install from is not given.");
            notifyDestroyed();
            return;
        }

        int storageId = Constants.INTERNAL_STORAGE_ID;
        String strStorageId = getAppProperty("arg-2");
        if (strStorageId != null) {
            try {
                storageId = Integer.parseInt(strStorageId);
            } catch (NumberFormatException nfe) {
                // Intentionally ignored
            }
        }

        // If a scheme is omitted, handle the url
        // as a file on the local file system.
        final String scheme = Installer.getUrlScheme(url, "file");

        for (int i = 0; i < supportedUrlTypes.length << 1; i++, i++) {
            if (supportedUrlTypes[i].equals(scheme)) {
                installerClassName = "com.sun.midp.installer." +
                    supportedUrlTypes[i+1];
                break;
            }
        }

        if (installerClassName != null) {
            try {
                installer = (Installer)
                    Class.forName(installerClassName).newInstance();
            } catch (Throwable t) {
                // Intentionally ignored: 'installer' is already null
            }
        }

        if (installer == null) {
            final String errMsg = "'" + scheme + "' URL type is not supported.";
System.out.println(errMsg);
            reportFinished0(appId, MIDletSuite.UNUSED_SUITE_ID, errMsg);
            notifyDestroyed();
            return;
        }

        // install the suite
        int lastInstalledSuiteId;
        int len = url.length();
        boolean jarOnly = (len >= 4 &&
            ".jar".equalsIgnoreCase(url.substring(len - 4, len)));
        String errMsg = null;

        try {
            if (jarOnly) {
                lastInstalledSuiteId = installer.installJar(url, null,
                    storageId, false, false, this);
            } else {
System.out.println(">>> Installing JAD...");
                lastInstalledSuiteId =
                    installer.installJad(url, storageId, false, false, this);
System.out.println(">>> lastInstalledSuiteId = " + lastInstalledSuiteId);
            }
        } catch (Throwable t) {
t.printStackTrace();
            errMsg = "Error installing the suite: " + t.getMessage();
            lastInstalledSuiteId = MIDletSuite.UNUSED_SUITE_ID;
System.out.println(">>> errMsg");
        }

System.out.println(">>> exiting...");
        notifyDestroyed();
        reportFinished0(appId, lastInstalledSuiteId, errMsg);
    }

    /*
     * Implementation of the InstallListener interface.
     * It calls the corresponding native listener and
     * blocks the installer thread until the answer arrives
     * from the side using this installer.
     */

    /**
     * Called with the current state of the install so the user can be
     * asked to override the warning. To get the warning from the state
     * call {@link InstallState#getLastException()}. If false is returned,
     * the last exception in the state will be thrown and
     * {@link Installer#wasStopped()} will return true if called.
     *
     * @param state current state of the install.
     *
     * @return true if the user wants to continue, false to stop the install
     */
    public boolean warnUser(InstallState state) {
        sendNativeRequest0(RQ_WARN_USER, convertInstallState(state),
                           -1, null);
        /*
         * This Java thread is blocked here until the answer arrives
         * in native, then it is woken up also from the native code.
         */
        return getAnswer0();
    }

    /**
     * Called with the current state of the install so the user can be
     * asked to confirm the jar download.
     * If false is returned, the an I/O exception thrown and
     * {@link Installer#wasStopped()} will return true if called.
     *
     * @param state current state of the install.
     *
     * @return true if the user wants to continue, false to stop the install
     */
    public boolean confirmJarDownload(InstallState state) {
        sendNativeRequest0(RQ_CONFIRM_JAR_DOWNLOAD, convertInstallState(state),
                           -1, null);
        /*
         * This Java thread is blocked here until the answer arrives
         * in native, then it is woken up also from the native code.
         */
        return getAnswer0();
    }

    /**
     * Called with the current status of the install. See
     * {@link Installer} for the status codes.
     *
     * @param status current status of the install.
     * @param state current state of the install.
     */
    public void updateStatus(int status, InstallState state) {
        sendNativeRequest0(RQ_UPDATE_STATUS, convertInstallState(state),
                           status, null);
    }

    /**
     * Called with the current state of the install so the user can be
     * asked to confirm if the RMS data should be kept for new version of
     * an updated suite.
     * If false is returned, the an I/O exception thrown and
     * {@link Installer#wasStopped()} will return true if called.
     *
     * @param state current state of the install.
     *
     * @return true if the user wants to keep the RMS data for the next suite
     */
    public boolean keepRMS(InstallState state) {
        sendNativeRequest0(RQ_ASK_KEEP_RMS, convertInstallState(state),
                           -1, null);
        /*
         * This Java thread is blocked here until the answer arrives
         * in native, then it is woken up also from the native code.
         */
        return getAnswer0();
    }

    /**
     * Called with the current state of the install so the user can be
     * asked to confirm the authentication path.
     * If false is returned, the an I/O exception thrown and
     * {@link Installer#wasStopped()} will return true if called.
     *
     * @param state current state of the install.
     *
     * @return true if the user wants to continue, false to stop the install
     */
    public boolean confirmAuthPath(InstallState state) {
        sendNativeRequest0(RQ_CONFIRM_AUTH_PATH, convertInstallState(state),
                           -1, null);
        /*
         * This Java thread is blocked here until the answer arrives
         * in native, then it is woken up also from the native code.
         */
        return getAnswer0();
    }

    /**
     * Called with the current state of the install and the URL where the
     * request is attempted to be redirected so the user can be asked
     * to confirm if he really wants to install from the new location.
     * If false is returned, the an I/O exception thrown and
     * {@link com.sun.midp.installer.Installer#wasStopped()}
     * will return true if called.
     *
     * @param state       current state of the install.
     * @param newLocation new url of the resource to install.
     * 
     * @return true if the user wants to continue, false to stop the install
     */
    public boolean confirmRedirect(InstallState state, String newLocation) {
        sendNativeRequest0(RQ_CONFIRM_REDIRECT, convertInstallState(state),
                           -1, newLocation);
        /*
         * This Java thread is blocked here until the answer arrives
         * in native, then it is woken up also from the native code.
         */
        return getAnswer0();
    }

    /**
     * Converts the given InstallState object into NativeInstallState
     * to facilitate access to it from the native code. 
     *
     * @param state the state object to convert
     *
     * @return NativeInstallState object corresponding to the given
     *         InstallState object
     */
    private NativeInstallState convertInstallState(InstallState state) {
System.out.println(">>> Converting installation state...");

        NativeInstallState nis = new NativeInstallState();

        nis.appId = appId;
        nis.suiteId = state.getID();
        nis.jarUrl = state.getJarUrl();
        nis.suiteName = state.getSuiteName();
        nis.jarSize = state.getJarSize();

        nis.authPath = state.getAuthPath();

        InvalidJadException ije = state.getLastException();
        if (ije == null) {
            nis.exceptionCode = NativeInstallState.NO_EXCEPTIONS;
        } else {
            nis.exceptionCode = ije.getReason();
        }

        // IMPL_NOTE: not implemented yet
        nis.suiteProperties = null;

        return nis;
    }

    // Native methods.

    /**
     * Sends a request of type defined by the given request code to
     * the party that uses this installer via the native callback.
     *
     * Note: only some of parameters are used, depending on the request code
     *
     * @param requestCode code of the request to the native callback
     * @param state       current installation state
     * @param status      current status of the installation, -1 if not used
     * @param newLocation new url of the resource to install; null if not used
     */
    private native void sendNativeRequest0(int requestCode,
                                           NativeInstallState state,
                                           int status, String newLocation);

    /**
     * Returns yes/no answer from the native callback.
     *
     * @return yes/no answer from the native callback
     */
    private native boolean getAnswer0();

    /**
     * Reports to the party using this installer that
     * the operation has been completed.
     *
     * @param appId this application ID
     * @param suiteId ID of the newly installed midlet suite, or
     *                MIDletSuite.UNUSED_SUITE_ID if the installation
     *                failed
     * @param errMsg error message if the installation failed, null otherwise
     */
    private native void reportFinished0(int appId, int suiteId, String errMsg);
}

/**
 * Storage for InstallState fields that should be passed to native.
 */
class NativeInstallState {
    /**
     * exceptionCode value indicating that there are no exceptions.
     */
    public static final int NO_EXCEPTIONS = -1;

    public int appId;
    public int exceptionCode;
    public int suiteId;
    public String[] suiteProperties;
    public String jarUrl;
    public String suiteName;
    public int    jarSize;
    public String[] authPath;
}
