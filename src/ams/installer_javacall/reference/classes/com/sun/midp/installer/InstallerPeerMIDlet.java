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

import java.util.Vector;

import javax.microedition.midlet.MIDlet;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.midletsuite.MIDletSuiteCorruptedException;
import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.InstallInfo;

import com.sun.midp.events.*;

import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.ImplicitlyTrustedClass;

import com.sun.midp.configurator.Constants;

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
public class InstallerPeerMIDlet extends MIDlet
        implements EventListener, InstallListener, Runnable {
    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {}

    /*
     * IMPL_NOTE: the following request codes must have the same values as
     *            the corresponding JAVACALL_INSTALL_REQUEST_* constants
     *            defined in javacall_ams_installer.h.
     */

    /**
     * Code of the "Warn User" request to the native callback.
     */
    private static final int RQ_WARN_USER            = 1;

    /**
     * Code of the "Confirm Jar Download" request to the native callback. 
     */
    private static final int RQ_CONFIRM_JAR_DOWNLOAD = 2;

    /**
     * Code of the "Ask If The Suite Data Should Be Retained"
     * request to the native callback.
     */
    private static final int RQ_ASK_KEEP_RMS         = 3;

    /**
     * Code of the "Confirm Authorization Path" request to the native callback.
     */
    private static final int RQ_CONFIRM_AUTH_PATH    = 4;

    /**
     * Code of the "Confirm Redirection" request to the native callback.
     */
    private static final int RQ_CONFIRM_REDIRECT     = 5;

    /**
     * Code of the "Update Installation Status" request to the native callback.
     *
     * Note that it is not used in java_ams_install_ask/answer and is treated
     * as other requests for handiness. If this value is changed, the
     * corresponding value in the native implementation of sendNativeRequest0()
     * must also be changed.
     */
    private static final int RQ_UPDATE_STATUS        = 6;

    /** Provides API to install midlets. */
    private Installer installer;

    /** ID assigned to this application by the application manager */
    private int appId; 

    /**
     * Create and initialize the MIDlet.
     */
    public InstallerPeerMIDlet() {
        SecurityToken internalSecurityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());

        EventQueue eventQueue = EventQueue.getEventQueue(internalSecurityToken);

        eventQueue.registerEventListener(
            EventTypes.NATIVE_ENABLE_OCSP_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_CHECK_OCSP_ENABLED_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_UNBLOCK_INSTALLER, this);

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
        String installerClassName = null;
        final String supportedUrlTypes[] = {
            "http",  "HttpInstaller",
            "https", "HttpInstaller",
            "file",  "FileInstaller"
        };
        String suiteName = null;

        // parse the arguments
        String arg0 = getAppProperty("arg-0");
        boolean err = false;
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
            reportFinished0(-1, MIDletSuite.UNUSED_SUITE_ID, -1,
                            "Application ID is not given or invalid.");
            notifyDestroyed();
            return;
        }

        String url = getAppProperty("arg-1");
        if (url == null) {
            reportFinished0(appId, MIDletSuite.UNUSED_SUITE_ID, -1,
                            "URL to install from is not given.");
            notifyDestroyed();
            return;
        }

        // check if this is not URL but an integer, so this is a suite update
        // and this parameter is a suite ID
        int suiteId = MIDletSuite.UNUSED_SUITE_ID;
        String errStr = null;

        try {
            suiteId = Integer.parseInt(url);

            try {
                // get URL and suite name from the suite properties
                MIDletSuiteImpl midletSuite =
                    MIDletSuiteStorage.getMIDletSuiteStorage().getMIDletSuite(
                        suiteId, false);
                InstallInfo installInfo = midletSuite.getInstallInfo();

                if (midletSuite.getNumberOfMIDlets() == 1) {
                    MIDletInfo midletInfo =
                        new MIDletInfo(midletSuite.getProperty("MIDlet-1"));
                    suiteName = midletInfo.name;
                } else {
                    suiteName =
                        midletSuite.getProperty(MIDletSuite.SUITE_NAME_PROP);
                }

                url = installInfo.getDownloadUrl();
                midletSuite.close();
            } catch (MIDletSuiteCorruptedException msce) {
                errStr = "MIDlet suite is corrupted.";
            } catch (MIDletSuiteLockedException msle) {
                errStr = "MIDlet suite is locked.";
            }

            if (errStr != null) {
                reportFinished0(appId, suiteId, -1, errStr);
                notifyDestroyed();
                return;
            }

        } catch (NumberFormatException nfe) {
            // ignore
        }

        int storageId = Constants.UNUSED_STORAGE_ID;
        String strStorageId = getAppProperty("arg-2");

        if (strStorageId != null) {
            try {
                storageId = Integer.parseInt(strStorageId);
            } catch (NumberFormatException nfe) {
                // Intentionally ignored
            }
        }

        if (storageId == Constants.UNUSED_STORAGE_ID) {
            // apply defaults
            storageId = Constants.INTERNAL_STORAGE_ID;
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
            reportFinished0(appId, MIDletSuite.UNUSED_SUITE_ID, -1, errMsg);
            notifyDestroyed();
            return;
        }

        // install the suite
        int lastInstalledSuiteId = MIDletSuite.UNUSED_SUITE_ID;
        int len = url.length();
        boolean jarOnly = (len >= 4 &&
            ".jar".equalsIgnoreCase(url.substring(len - 4, len)));
        String errMsg = null;
        int errCode = 0; // no errors by default

        try {
            if (jarOnly) {
                lastInstalledSuiteId = installer.installJar(url, suiteName,
                    storageId, false, false, this);
            } else {
                lastInstalledSuiteId =
                    installer.installJad(url, storageId, false, false, this);
            }
        } catch (InvalidJadException ije) {
            errCode = ije.getReason();
            errMsg = ije.getExtraData();
        } catch (Throwable t) {
            errCode = -1;
            errMsg = "Error installing the suite: " + t.getMessage();
        }

        reportFinished0(appId, lastInstalledSuiteId, errCode, errMsg);
        
        notifyDestroyed();
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
        int err = sendNativeRequest0(RQ_WARN_USER, convertInstallState(state),
                                     -1, null);
        if (err != 0) {
            return false; /* stop the installation */
        }

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
        int err = sendNativeRequest0(RQ_CONFIRM_JAR_DOWNLOAD,
                                     convertInstallState(state),
                                     -1, null);
        if (err != 0) {
            return false; /* stop the installation */
        }

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
        int err = sendNativeRequest0(RQ_ASK_KEEP_RMS,
                                     convertInstallState(state),
                                     -1, null);
        if (err != 0) {
            return true; /* keep the RMS */
        }

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
        int err = sendNativeRequest0(RQ_CONFIRM_AUTH_PATH,
                                     convertInstallState(state),
                                     -1, null);
        if (err != 0) {
            return false; /* stop the installation */
        }

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
        int err = sendNativeRequest0(RQ_CONFIRM_REDIRECT,
                                     convertInstallState(state),
                                     -1, newLocation);
        if (err != 0) {
            return false; /* stop the installation */
        }

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

        /*
         * IMPL_NOTE: only well-known attributes are supported
         *            for the installation purposes.
         */
        final String[] propNames = {
            MIDletSuite.JAR_MANIFEST,       MIDletSuite.DATA_SIZE_PROP,
            MIDletSuite.JAR_SIZE_PROP,      MIDletSuite.JAR_URL_PROP,
            MIDletSuite.SUITE_NAME_PROP,    MIDletSuite.VENDOR_PROP,
            MIDletSuite.VERSION_PROP,       MIDletSuite.DESC_PROP,
            MIDletSuite.CONFIGURATION_PROP, MIDletSuite.PROFILE_PROP,
            MIDletSuite.RUNTIME_EXEC_ENV_PROP,
            MIDletSuite.RUNTIME_EXEC_ENV_DEFAULT,
            MIDletSuite.PERMISSIONS_PROP,   MIDletSuite.PERMISSIONS_OPT_PROP
        };

        Vector v = new Vector(propNames.length * 2);

        for (int i = 0; i < propNames.length; i++) {
            String propValue = state.getAppProperty(propNames[i]);
            if (propValue != null) {
                v.addElement(propNames[i]);
                v.addElement(propValue);
            }
        }

        int arraySize = v.size();
        if (arraySize > 0) {
            nis.suiteProperties = new String[arraySize];
            for (int j = 0; j < arraySize; j++, j++) {
                nis.suiteProperties[j] = (String)v.elementAt(j);
                nis.suiteProperties[j + 1] = (String)v.elementAt(j + 1);
            }
        } else {
            nis.suiteProperties = null;
        }

        return nis;
    }

    // ==============================================================
    // ------ Implementation of the EventListener interface

    /**
     * Preprocess an event that is being posted to the event queue.
     * This method will get called in the thread that posted the event.
     *
     * @param event event being posted
     *
     * @param waitingEvent previous event of this type waiting in the
     *     queue to be processed
     *
     * @return true to allow the post to continue, false to not post the
     *     event to the queue
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        NativeEvent nativeEvent = (NativeEvent)event;
        // appId of the running installer which the event is intended for
        int installerAppId = nativeEvent.intParam1;

        // -1 - broadcast
        if ((installerAppId != appId) && (installerAppId != -1)) {
            // this event was sent to another installer, ignoring
            return;
        }

        switch (nativeEvent.getType()) {
            case EventTypes.NATIVE_UNBLOCK_INSTALLER: {
                unblockInstaller0();
                break;
            }

            case EventTypes.NATIVE_ENABLE_OCSP_REQUEST: {
                /*
                 * intParam1 - appId of the running installer,
                 * intParam2 - 0/1 to enable / disable OCSP
                 */
                boolean enable = (nativeEvent.intParam2 != 0);
                int result;

                if (installer != null) {
                    installer.enableOCSPCheck(enable);
                    result = 0;
                } else {
                    result = -1;
                }

                notifyRequestHandled0(appId,
                        EventTypes.NATIVE_ENABLE_OCSP_REQUEST,
                                result, enable);
                break;
            }

            case EventTypes.NATIVE_CHECK_OCSP_ENABLED_REQUEST: {
                boolean ocspEnabled;
                int result;

                if (installer != null) {
                    ocspEnabled = installer.isOCSPCheckEnabled();
                    result = 0;
                } else {
                    result = -1;
                    ocspEnabled = false;
                }
                
                notifyRequestHandled0(appId,
                        EventTypes.NATIVE_CHECK_OCSP_ENABLED_REQUEST,
                                result, ocspEnabled);
                break;
            }
        }
    }

    // ==============================================================
    // Native methods.

    /**
     * Sends a request of type defined by the given request code to
     * the party that uses this installer via the native callback.
     *
     * Note: only some of parameters are used, depending on the request code
     *
     * IMPL_NOTE: the request code passed to this method as an argument must
     *            be the same as the corresponding JAVACALL_INSTALL_REQUEST_*
     *            value defined in javacall_ams_installer.h.
     * 
     * @param requestCode code of the request to the native callback
     * @param state       current installation state
     * @param status      current status of the installation, -1 if not used
     * @param newLocation new url of the resource to install; null if not used
     *
     * @return 0 if no errors, a platform-specific error code otherwise
     */
    private static native int sendNativeRequest0(
            int requestCode,
            NativeInstallState state,
            int status,
            String newLocation);

    /**
     * Returns yes/no answer from the native callback.
     *
     * @return yes/no answer from the native callback
     */
    private static native boolean getAnswer0();

    /**
     * Reports to the party using this installer that
     * the installation has been completed.
     *
     * @param appId this application ID
     * @param suiteId ID of the newly installed midlet suite, or
     *                MIDletSuite.UNUSED_SUITE_ID if the installation
     *                failed
     * @param resultCode result of the installation (0 if succeeded or -1 in
     *                   case of unknown error, or one of the values defined
     *                   in InvalidJadException)
     * @param errMsg error message if the installation failed, null otherwise
     */
    private static native void reportFinished0(
            int appId, int suiteId, int resultCode, String errMsg);

    /**
     * Reports to the party using this installer that the requested
     * operation has been completed and the result (if any) is available.
     *
     * @param appId this application ID
     * @param requestCode code of the request that was handled
     * @param resultCode completion code (0 if succeeded or -1 in case
     *                   of error)
     * @param result operation-dependent result (for OCSP operations it contains
     *               the current state (enabled/disabled) of OCSP checks)
     */
    private static native void notifyRequestHandled0(
        int appId, int requestCode, int resultCode, boolean result);

    /**
     * Unblocks the installer thread.
     */
    private static native void unblockInstaller0();
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
