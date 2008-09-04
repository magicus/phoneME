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
import com.sun.midp.installer.*;
import com.sun.midp.configurator.Constants;

/**
 * A MIDlet passing the installer's requests and responses between
 * Java and native code. Allows to install midlet either from
 * an http(s):// or from a file:// URL, or from a file given by
 * a local path.
 * <p>
 * The MIDlet uses certain application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: currently must be "I" == install
 *   (for consistency with GraphicalInstaller)</li>
 *   <li>arg-1: URL of the midlet suite to install
 *   <li>arg-2: a storage ID where to save the suite's jar file
 */
public class InstallerPeerMIDlet extends MIDlet implements InstallListener,
        Runnable {
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
        if (arg0 == null || !"I".equals(arg0)) {
            showUsage();
            notifyDestroyed();
            return;
        }

        String url = getAppProperty("arg-1");
        if (url == null) {
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
            //final String errMsg = "'" + scheme + "' URL type is not supported.";
            notifyDestroyed();
            return;
        }

        // install the suite
        int lastInstalledMIDletId;
        int len = url.length();
        boolean jarOnly = (len >= 4 &&
            ".jar".equalsIgnoreCase(url.substring(len - 4, len)));

        try {
            if (jarOnly) {
                lastInstalledMIDletId = installer.installJar(url, null,
                    storageId, false, false, this);
            } else {
                lastInstalledMIDletId =
                    installer.installJad(url, storageId, false, false, this);
            }

            System.out.println("The suite was succesfully installed, ID: " +
                               lastInstalledMIDletId);
        } catch (Throwable t) {
            //final String errMsg = "Error installing the suite: " + t.getMessage();
        }

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
        return true;
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
        return true;
    }

    /**
     * Called with the current status of the install. See
     * {@link Installer} for the status codes.
     *
     * @param status current status of the install.
     * @param state current state of the install.
     */
    public void updateStatus(int status, InstallState state) {
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
        return true;
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
        return true;
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
        return true;
    }
}
