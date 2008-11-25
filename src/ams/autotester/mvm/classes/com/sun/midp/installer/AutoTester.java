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

import com.sun.cldc.isolate.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.main.AmsUtil;
import com.sun.midp.main.MIDletSuiteUtils;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;

import com.sun.midp.events.*;
import com.sun.midp.services.*;
import com.sun.midp.security.*;
import java.io.*;

/**
 * Installs/Updates a test suite, runs the first MIDlet in the suite in a loop
 * specified number of iterations or until the new version of the suite is not
 * found, then removes the suite.
 * <p>
 * The MIDlet uses these application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: URL for the test suite
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
public final class AutoTester extends AutoTesterBase 
    implements Runnable {

    /** We need security token for connecting to AutoTetser service */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {};

    private static SecurityToken token = null;    

    /** Connection to AutoTesterService */
    private SystemServiceConnection con = null;

    /**
     * Creates and initializes a new auto tester MIDlet.
     */
    public AutoTester() {
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
        String response;

        sendTestRunParams();
        response = receiveResponse();

        if (response != null) {
            displayInstallerError(response);
        }

        notifyDestroyed();        
    }

    /**
     * Starts the background tester.
     */
    void startBackgroundTester() {
        connectToService();

        if (con != null) {
            new Thread(this).start();
        } else {
            displayError(Resource.getString(ResourceConstants.EXCEPTION),
                "Failed to connect to AutoTester service");
        }
    }

    private static SecurityToken getSecurityToken() {
        if (token == null) {
            token = SecurityInitializer.requestToken(new SecurityTrusted());
        }

        return token;
    }

    private void connectToService() {
        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(getSecurityToken());

        con = serviceRequestor.requestService(AutoTesterService.SERVICE_ID);
    }

    private void sendTestRunParams() {
        SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();

        try {
            msg.getDataOutput().writeUTF(url == null? "" : url);
            msg.getDataOutput().writeUTF(domain == null? "" : domain);
            msg.getDataOutput().writeInt(loopCount);
        } catch (IOException ex) {
            // ignore
        }

        try {
            con.send(msg);
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }        
    }

    private String receiveResponse() {
        String response = null;

        SystemServiceDataMessage msg = null;
        try {
            msg = (SystemServiceDataMessage)con.receive();
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }

        if (msg == null) {
            return response;
        }

        try {
            response = msg.getDataInput().readUTF();
            if (response.length() == 0) {
                response = null;
            }
        } catch (IOException ex) {
            // ignore
        }

       return response; 
    }
}
