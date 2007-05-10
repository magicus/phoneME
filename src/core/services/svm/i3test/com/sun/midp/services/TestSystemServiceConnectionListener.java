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

package com.sun.midp.services;

import com.sun.midp.security.*;
import java.io.*;
import java.util.*;
import com.sun.midp.i3test.TestCase;

/**
 * Tests for system service requesting functionality
 */
public class TestSystemServiceConnectionListener extends TestCase {
    private static SecurityToken token = SecurityTokenProvider.getToken(); 
    private final static String testString = "just a test string";   

    class ConnectionListener implements SystemServiceConnectionListener {
        private SystemServiceConnection con = null;
        private boolean stringSent = false;

        String responseString = null;
        boolean connectionClosed = false;

        ConnectionListener(SystemServiceConnection con) {
            this.con = con;
        }

        public void onMessage(SystemServiceMessage msg) {
            try {
                // string received from client
                String str = msg.getDataInput().readUTF();
                
                if (!stringSent) {
                    // send test string to client
                    msg = SystemServiceMessage.newMessage();
                    msg.getDataOutput().writeUTF(testString);
                    con.send(msg);

                    stringSent = true;
                } else {
                    // string received from client is response string
                    responseString = str;
                }

            } catch (Throwable t) {
                System.err.println("Exception: " + t);
                t.printStackTrace();
            }
        }

        public void onConnectionClosed() {
            connectionClosed = true;
        }
    }
    
    class SimpleSystemService implements SystemService {
        final static String SERVICE_ID = "42";

        private ConnectionListener listener = null;

        boolean stringsMatch = false;
        boolean connectionClosed = false;

        public String getServiceID() {
            return SERVICE_ID;
        }

        public void start() {
        }

        public void stop() {
            // compare strings
            String responseString = listener.responseString; 
            stringsMatch = testString.toUpperCase().equals(responseString);

            connectionClosed = listener.connectionClosed;
        }

        public void acceptConnection(SystemServiceConnection con) {

            listener = new ConnectionListener(con);
            con.setConnectionListener(listener);
        }
    }


    void testLocal() {
        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        SimpleSystemService service = new SimpleSystemService();
        manager.registerService(service);

        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);

        SystemServiceConnection con = null;
        con = serviceRequestor.requestService(
                SimpleSystemService.SERVICE_ID);

        try {
            // send an empty string to service to start messages exchange
            SystemServiceMessage msg = SystemServiceMessage.newMessage();
            msg.getDataOutput().writeUTF("");
            con.send(msg);
            
            // receive string from service
            msg = con.receive();
            String testString = msg.getDataInput().readUTF();

            // convert string to upper case and sent it back to service
            msg = SystemServiceMessage.newMessage();
            msg.getDataOutput().writeUTF(testString.toUpperCase());
            con.send(msg);
        } catch (Throwable t) {
            System.err.println("Exception: " + t);
        }

        manager.shutdown();

        assertTrue("Strings match", service.stringsMatch);
    }

    /**
     * Runs all tests.
     */
    public void runTests() 
        throws InterruptedIOException,
               IOException {

        declare("testLocal");
        testLocal();
    }
}


