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

import com.sun.midp.services.*;
import java.io.*;
import com.sun.midp.security.*;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;


/**
 * Implements AutoTester service
 */
class AutoTesterService implements SystemService, Runnable  {
    /** Our service ID */
    final static String SERVICE_ID = "AUTOTESTER";

    /** Service-side data exchange protocol instance */
    private AutoTesterServiceProtocolAMS protocol = null;    


    /**
     * Gets unique service identifier.
     *
     * @return unique String service identifier
     */    
    public String getServiceID() {
        return SERVICE_ID;
    }

    /**
     * Starts service. Called when service is about to be
     * requested for the first time. 
     */    
    public void start() {
    }

    /**
     * Shutdowns service.
     */
    public void stop() {
    }

    /**
     * Accepts connection. When client requests a service, first,
     * a connection between client and service is created, and then
     * it is passed to service via this method to accept it and
     * start doing its thing. Note: you shouldn't block in this
     * method.
     *
     * @param theConnection connection between client and service
     */
    public void acceptConnection(SystemServiceConnection theConnection) {
        synchronized (this) {
            // only one connection is allowed and expected
            if (protocol != null) {
                return;
            }

            protocol = new AutoTesterServiceProtocolAMS(theConnection);
        }

        new Thread(this).start();
    }

    public void run() {
        try {
            protocol.receiveTestRunParams();
            String url = protocol.getURL();
            String domain = protocol.getDomain();
            int loopCount = protocol.getLoopCount();

            if (url == null) {
                Logging.report(Logging.CRITICAL, LogChannels.LC_CORE, 
                    "AutoTester service: no test URL");
                return;
            }

            AutoTesterHelper helper = new AutoTesterHelper(
                    url, domain, loopCount);

            String errorMessage = null;
            try {
                helper.installAndPerformTests();
            } catch (Throwable t) {
                int suiteId = helper.getTestSuiteId();

                // may return null, this means that exception should be ignored
                errorMessage = helper.getInstallerExceptionMessage(suiteId, t);
            } finally {
                try {
                    if (errorMessage == null) {
                        protocol.sendOKStatus();
                    } else {
                        protocol.sendErrorStatus(errorMessage);
                    }
                } catch (InterruptedIOException ioex) {
                    // ignore: client already exited before service got
                    // ack that client has received message we sent
                }                                
            }
        } catch (SystemServiceConnectionClosedException ccex) {
            Logging.report(Logging.CRITICAL, LogChannels.LC_CORE, 
                "Connection to AutoTester service client closed unexpectedly");
        } catch (Exception ex) {
            Logging.report(Logging.CRITICAL, LogChannels.LC_CORE, 
                "AutoTester service exception: " + ex.getMessage());
        } finally {
            synchronized (this) {
                protocol = null;
            }
        }
    }
}

