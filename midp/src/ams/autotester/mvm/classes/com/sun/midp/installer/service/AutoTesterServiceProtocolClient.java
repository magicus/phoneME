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

/**
 * Client side of AutoTester service data exchange protocol.
 */
public final class AutoTesterServiceProtocolClient {
    /** OK status constant */
    public final static String STATUS_OK = 
        AutoTesterServiceProtocolAMS.STATUS_OK;

    /** Connection between service and client */
    private SystemServiceConnection con;

    /**
     * Establishes connection to AutoTester service.
     *
     * @param token security token
     * @return AutoTesterServiceProtocolAMS instance if connection
     * has been established, null otherwise
     */
    public static AutoTesterServiceProtocolClient connectToService(SecurityToken token) {
        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);

        SystemServiceConnection con = serviceRequestor.requestService(
                AutoTesterService.SERVICE_ID);
        if (con == null) {
            return null;
        }

        return new AutoTesterServiceProtocolClient(con);
    }

    /**
     * Sends test run parameters to service.
     *
     * @param theURL URL of the test suite
     * @param theDomain security domain to assign to unsigned suites
     * @param theLoopCount how many iterations to run the suite
     */
    public void sendTestRunParams(String theURL, String theDomain, 
            int theLoopCount) 
        throws SystemServiceConnectionClosedException, IOException {

        SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();
        msg.getDataOutput().writeUTF(theURL == null? "" : theURL);
        msg.getDataOutput().writeUTF(theDomain == null? "" : theDomain);
        msg.getDataOutput().writeInt(theLoopCount);

        con.send(msg);
    }

    /**
     * Receives status string from service.
     *
     * @return status strin received
     */
    public String receiveStatus() 
        throws SystemServiceConnectionClosedException, IOException {

        return SystemServiceUtil.receiveString(con);
    }

    /**
     * Constructor.
     *
     * @param theConnection AutoTester service connection
     */
    private AutoTesterServiceProtocolClient(SystemServiceConnection 
            theConnection) {
        con = theConnection;
    }    
}
