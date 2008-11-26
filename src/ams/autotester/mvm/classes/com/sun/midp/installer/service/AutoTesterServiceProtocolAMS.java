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

/**
 * Service side of AutoTester service data exchange protocol. 
 */
final class AutoTesterServiceProtocolAMS {
    /** OK status constant */
    public final static String STATUS_OK = ""; 

    /** Connection between service and client */
    private SystemServiceConnection con;

    /** URL of the test suite. */
    private String url;

    /** Security domain to assign to unsigned suites. */
    private String domain;

    /** How many iterations to run the suite */
    private int loopCount;

    /**
     * Constructor.
     *
     * @param theConnection connection between service and client
     */
    AutoTesterServiceProtocolAMS(SystemServiceConnection theConnection) {
        con = theConnection;

        url = null;
        domain = null;
        loopCount = -1;
    }

    /**
     * Receives test run parameters from client.
     */
    void receiveTestRunParams() 
        throws SystemServiceConnectionClosedException, IOException {

        SystemServiceDataMessage msg = (SystemServiceDataMessage)con.receive();

        url = msg.getDataInput().readUTF();
        if (url.length() == 0) {
            url = null;
        }

        domain = msg.getDataInput().readUTF();
        if (domain.length() == 0) {
            domain = null;
        }
        
        loopCount = msg.getDataInput().readInt();
    }

    /**
     * Sends error status to client.
     *
     * @param errorMessage error message
     */
    void sendErrorStatus(String errorMessage) 
        throws SystemServiceConnectionClosedException, IOException {

        SystemServiceUtil.sendString(con, errorMessage);
    }

    /**
     * Sends OK status to client.
     */
    void sendOKStatus() 
        throws SystemServiceConnectionClosedException, IOException {

        SystemServiceUtil.sendString(con, STATUS_OK);        
    }

    /**
     * Gets URL of the test suite.
     *
     * @return URL of the test suite
     */
    String getURL() {
        return url;
    }

    /**
     * Gets security domain for the test suite.
     *
     * @return security domain
     */
    String getDomain() {
        return domain;
    }

    /**
     * Gets loop count.
     *
     * @return loop count
     */
    int getLoopCount() {
        return loopCount;
    }
}
