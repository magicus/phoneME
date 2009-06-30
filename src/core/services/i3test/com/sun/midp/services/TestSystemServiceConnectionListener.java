/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
import com.sun.midp.links.*;
import java.io.*;
import java.util.*;
import com.sun.cldc.isolate.*;
import com.sun.midp.i3test.TestCase;

/**
 * Tests for system service requesting functionality
 */
public class TestSystemServiceConnectionListener extends TestCase {
    public final static String SERVICE_ID = "42";

    static private class SecurityTrusted
            implements ImplicitlyTrustedClass {};

    private static SecurityToken token = 
        SecurityInitializer.requestToken(new SecurityTrusted());

    class SimpleSystemService implements SystemService, Runnable {
        private final static String testString = "just a test string";

        Thread serviceThread = null;
        SystemServiceConnection con = null;
        boolean stringsMatch = false;

        public String getServiceID() {
            return SERVICE_ID;
        }

        public void start() {
        }

        public void stop() {
            try {
                serviceThread.join();
            } catch (InterruptedException e) {
            }
        }

        public void acceptConnection(SystemServiceConnection con) {
            this.con = con;

            serviceThread = new Thread(this);
            serviceThread.start();
        }

        public void run() {
            try {
                // send test string to client
                SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();
                msg.getDataOutput().writeUTF(testString);
                con.send(msg);

                // get a response string from client
                msg = (SystemServiceDataMessage)con.receive();
                String responseString = msg.getDataInput().readUTF();

                // compare strings
                stringsMatch = testString.toUpperCase().equals(responseString);
            } catch (Throwable t) {
            }
        }
    }

    void testRemote() 
        throws IsolateStartupException,
               InterruptedIOException,
               IOException,
               ClosedLinkException {

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        SimpleSystemService service = new SimpleSystemService();
        manager.registerService(service);

        SystemServiceRequestHandler requestHandler = 
            new SystemServiceRequestHandler(manager);

        Isolate serviceIsolate = Isolate.currentIsolate();
        Isolate clientIsolate = new Isolate(
                "com.sun.midp.services.SystemServiceIsolate", null);
        clientIsolate.start();

        IsolateSystemServiceRequestHandler isolateRequestHandler = 
            requestHandler.newIsolateRequestHandler(clientIsolate);

        Link namedPortalLink = Link.newLink(serviceIsolate, clientIsolate);
        Link[] clientLinks = { namedPortalLink };
        LinkPortal.setLinks(clientIsolate, clientLinks);
        SystemServiceLinkPortal.sendLinks(namedPortalLink);

        requestHandler.handleIsolateRequests(isolateRequestHandler);

        clientIsolate.waitForExit();
        
        manager.shutdown();

        assertTrue("Strings match", service.stringsMatch);
    }

    void testLocal() {
        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        SimpleSystemService service = new SimpleSystemService();
        manager.registerService(service);

        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);

        SystemServiceConnection con = null;
        con = serviceRequestor.requestService(
                TestSystemService.SERVICE_ID);

        try {
            // receive string from service
            SystemServiceDataMessage msg = (SystemServiceDataMessage)con.receive();
            String testString = msg.getDataInput().readUTF();

            // convert string to upper case and sent it back to service
            msg = SystemServiceMessage.newDataMessage();
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
               IsolateStartupException,
               ClosedLinkException,
               IOException {

        declare("testRemote");
        testRemote();

        declare("testLocal");
        testLocal();
    }
}
