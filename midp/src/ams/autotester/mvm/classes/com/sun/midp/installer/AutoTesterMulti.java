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

import java.util.*;

import com.sun.cldc.isolate.*;

import com.sun.midp.main.AmsUtil;

import javax.microedition.io.*;

import javax.microedition.lcdui.*;

import javax.microedition.midlet.*;

import javax.microedition.rms.*;

import com.sun.midp.io.j2me.storage.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.midlet.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.security.*;
import com.sun.midp.configurator.Constants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;


/**
 * Fetches a list of URLs pointing to test suites to install, then,
 * for each specified test suite in parallel, installs/updates the suite,
 * runs the first MIDlet in the suite in a loop specified number of iterations
 * or until the new version of the suite is not found, then removes the suite.
 * <p>
 * The MIDlet uses these application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: URL for html page with links to test suites. This html page
 *    looks like this:
 *      <a href="http://localhost/suite1.jad">Suite1</a>
 *      <a href="http://localhost/suite2.jad">Suite2</a>
 *   <li>arg-1: Used to override the default domain used when installing
 *    an unsigned suite. The default is maximum to allow the runtime API tests
 *    be performed automatically without tester interaction. The domain name
 *    may be followed by a colon and a list of permissions that must be allowed
 *    even if they are not listed in the MIDlet-Permissions attribute in the
 *    application descriptor file. Instead of the list a keyword "all" can be
 *    specified indicating that all permissions must be allowed, for example:
 *    operator:all.
 *    <li>arg-2: Integer number, specifying how many iterations to run
 *    the suite. If argument is not given or less then zero, then suite
 *    will be run until the new version of the suite is not found.
 * </ol>
 * <p>
 * If arg-0 is not given then a form will be used to query the tester for
 * the arguments.</p>
 */
public final class AutoTesterMulti extends AutoTesterBase 
    implements Runnable {

    /** AutoTesterMultiHelper instance */
    private AutoTesterMultiHelper helper;

    /**
     * Create and initialize a new auto tester MIDlet.
     */
    public AutoTesterMulti() {
        super();

        if (url != null) {
            startBackgroundTester();
        } else {
            /**
             * No URL has been provided, ask the user.
             *
             * commandAction will subsequently call startBackgroundTester.
             */
            getUrl();
        }
    }

    /** 
     * Runs the installer. 
     */
    public void run() {
        helper.installAndPerformTests();
        notifyDestroyed();        
    }


    /**
     * Starts the background tester.
     */
    void startBackgroundTester() {
        helper = new AutoTesterMultiHelper(this, url, domain, loopCount);       
        new Thread(this).start();
    }   

    /**
     * Displays message about error that happened during 
     * fetching list of suites to run.
     *
     * @param message message to display
     */
    void displayFetchInstallListError(String message) {
        Alert a = new Alert(Resource.getString(ResourceConstants.ERROR),
                message, null, AlertType.ERROR);
        a.setTimeout(Alert.FOREVER);
        display.setCurrent(a);
    }
}

