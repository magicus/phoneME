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

import java.io.*;

import javax.microedition.rms.*;

import com.sun.midp.i18n.Resource;

import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.main.MIDletSuiteLoader;

import com.sun.midp.midlet.MIDletStateHandler;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Installs/Updates a test suite for CHAPI, runs the first MIDlet 
 * in the suite in a loop 
 * specified number of iterations or until the new version of the suite is not 
 * found, then removes the suite. 
 * <p> The modifications for CHAPI enable the security checks during
 * the installation by enabling the listener.
 * The security prompts are handled by the installer.
 * The listener responds without prompting to requests to install the jar and
 * and to delete the RMS storage of the application (if ask). 
 * <p>
 * The MIDlet uses these application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: URL for the test suite
 *   <li>arg-1: Used to override the default domain used when installing
 *    an unsigned suite. The default is maximum to allow the runtime API tests
 *    be performed automatically without tester interaction.
 *    <li>arg-2: Integer number, specifying how many iterations to run
 *    the suite. If argument is not given or less then zero, then suite
 *    will be run until the new version of the suite is not found.
 * </ol>
 * <p>
 * If arg-0 is not given then a form will be used to query the tester for
 * the arguments.</p>
 */
public class CHAutoTester extends AutoTester implements InstallListener {

    /**
     * Installs and performs the tests.
     *
     * @param midletSuiteStorage MIDletSuiteStorage object
     * @param inp_installer Installer object
     * @param inp_url URL of the test suite
     */
    public void installAndPerformTests(
        MIDletSuiteStorage midletSuiteStorage,
        Installer inp_installer, String inp_url) {

	installListener = this;
	super.installAndPerformTests(midletSuiteStorage, inp_installer, url);
    }

    /**
     * Called with the current state of the install so the user can be
     * asked to override the warning. To get the warning from the state
     * call {@link InstallState#getLastException()}. If false is returned,
     * the last exception in the state will be thrown and
     * {@link Installer#wasStopped()} will return true if called.
     *
     * @param state current state of the install.
     *
     * @return true this installer does not stop.
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
     * @return true; this installer always continues
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
	// Ignore
    }

    /**
     * Called with the current state of the install so the user can be
     * asked to confirm if the RMS data should be kept for new version of
     * an updated suite.
     *
     * @param state current state of the install.
     *
     * @return false; never keep the RMS data if asked.
     */
    public boolean keepRMS(InstallState state) {
	return false;
    }

    /**
     * Called with the current state of the install so the user can be
     * asked to confirm the authentication path.
     * If false is returned, the an I/O exception thrown and
     * {@link Installer#wasStopped()} will return true if called.
     *
     * @param state current state of the install.
     *
     * @return true to continue the install
     */
    public boolean confirmAuthPath(InstallState state) {
        return true;
    }
}
