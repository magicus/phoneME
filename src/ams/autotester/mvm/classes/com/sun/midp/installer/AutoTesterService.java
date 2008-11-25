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
 * Implements AutoTester service
 */
class AutoTesterService implements SystemService, Runnable  {
    /** Our service ID */
    final static String SERVICE_ID = "AUTOTESTER";

    /** Handshake string: sent from service to client */
    final static String HANDSHAKE_SERVICE = "AUTOTESTER_SERVICE";

    /** Handshake string: sent from client to service */
    final static String HANDSHAKE_CLIENT = "AUTOTESTER_CLIENT";

    /** 
     * Connection between service and client. 
     * This service expects only one such connection.
     */
    private SystemServiceConnection con = null;    

    /** URL of the test suite. */    
    private String url;

    /** Security domain to assign to unsigned suites. */    
    private String domain;

    /** How many iterations to run the suite */
    private int loopCount;

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
     * @param connection connection between client and service
     */
    public void acceptConnection(SystemServiceConnection inp_con) {
        synchronized (this) {
            // only one connection is allowed and expected
            if (con != null) {
                return;
            }

            con = inp_con;
        }

        initSession();
        new Thread(this).start();
    }

    public void run() {
        String handshake = receiveHandshake();
        if (!handshake.equals(HANDSHAKE_CLIENT)) {
            endSession();            
            return;
        }

        sendResponse(HANDSHAKE_SERVICE);

        receiveTestRunParams();
        if (url == null) {
            endSession();            
            return;
        }

        AutoTesterHelper helper = new AutoTesterHelper(url, domain, loopCount);

        String response = "";
        try {
            helper.installAndPerformTests();
        } catch (Throwable t) {
            int suiteId = helper.getTestSuiteId();
            response = helper.getInstallerExceptionMessage(suiteId, t);
        }

        sendResponse(response);

        endSession();
    }

    private void initSession() {
        url = null;
        domain = null;
        loopCount = -1;
    }

    private void endSession() {
        synchronized (this) {
            con = null;
        }        
    }

    /**
     * Receives handshake string. We don't have to do the handshake,
     * because connection between service and client is considered
     * trusted and reliable, but we do it anyway to protect us from
     * unexpected failures.
     */
    private String receiveHandshake() {
        String handshake = "";
        
        SystemServiceDataMessage msg = null;
        try {
            msg = (SystemServiceDataMessage)con.receive();
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }

        if (msg != null) {
            try {
                handshake = msg.getDataInput().readUTF();
            } catch (IOException ex) {
                // ignore
            }
        }

        return handshake;
    }

    private void receiveTestRunParams() {
        SystemServiceDataMessage msg = null;
        try {
            msg = (SystemServiceDataMessage)con.receive();
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }

        if (msg == null) {
            return;
        }

        try {
            url = msg.getDataInput().readUTF();
            if (url.length() == 0) {
                url = null;
            }

            domain = msg.getDataInput().readUTF();
            if (domain.length() == 0) {
                domain = null;
            }

            loopCount = msg.getDataInput().readInt();
        } catch (IOException ex) {
            // ignore
        }
    }

    private void sendResponse(String response) {
        SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();

        try {
            msg.getDataOutput().writeUTF(response);
        } catch (IOException ex) {
            // ignore
        }

        try {
            con.send(msg);
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }
    }
}

