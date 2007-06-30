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

import com.sun.midp.links.*;
import java.io.*;

import com.sun.cldc.isolate.*;
import com.sun.midp.i3test.TestCase;

/**
 * Tests for the LinkPortal class.
 */
public class TestServiceRequestProtocol extends TestCase {

    class SystemServiceRequestListenerImpl 
        implements SystemServiceRequestListener {

        // expected service ID
        String serviceID = "";

        // precreated connection between service and client
        SystemServiceConnectionLinks con = null;

        // true if connection links has been passed to client
        boolean linksPassed = false;

        SystemServiceRequestListenerImpl(String serviceID, 
                SystemServiceConnectionLinks con) {

            this.serviceID = serviceID;
            this.con = con;
            this.linksPassed = false;
        }

        public SystemServiceConnectionLinks onServiceRequest(String serviceID) {
            if (this.serviceID.equals(serviceID)) {
                // got expected service ID, return connection to this service
                return con;
            } else {
                return null;
            }
        }

        public void onLinksPassedToClient(SystemServiceConnectionLinks links) {
            linksPassed = true;
        }
    }


    class ServiceRequestHandlerThread extends Thread {
        // Links used for requesting service
        SystemServiceConnectionLinks requestLinks = null;

        // protocol for performing request
        SystemServiceRequestProtocolAMS protocol = null;

        Object result = null;
        boolean done = false;

        ServiceRequestHandlerThread(
                SystemServiceConnectionLinks requestLinks, 
                SystemServiceRequestListener requestListener) {

            this.requestLinks = requestLinks;
            this.protocol = 
                new SystemServiceRequestProtocolAMS(requestListener);

            this.start();
        }

        Object await() {
            try {
                synchronized (this) {
                    while (!done) {
                        wait();
                    }
                }
            } catch (InterruptedException ignore) { }

            return result;
        }

        public void run() {
            try {
                // wait for request
                Link receiveLink = requestLinks.getReceiveLink();
                LinkMessage msg = receiveLink.receive();

                // handle request
                protocol.handleServiceRequest(requestLinks);
            } catch (Throwable t) {
                result = t;
            }

            synchronized (this) {
                done = true;
                notifyAll();
            }
        }
    }
    
    /**
     * Tests setting and getting of actual data.
     */
    void testActual() throws InterruptedIOException, IOException {
        String serviceID = "Test service";
        Isolate is = Isolate.currentIsolate();

        Link reqC2SLink = Link.newLink(is, is);
        Link reqS2CLink = Link.newLink(is, is);
        SystemServiceConnectionLinks requestLinksAMS = 
            new SystemServiceConnectionLinks(reqS2CLink, reqC2SLink);
        SystemServiceConnectionLinks requestLinksClient = 
            new SystemServiceConnectionLinks(reqC2SLink, reqS2CLink);

        // precreated connection between service and client
        Link conSendLink = Link.newLink(is, is);
        Link conRecLink = Link.newLink(is, is);
        SystemServiceConnectionLinks links = 
            new SystemServiceConnectionLinks(conRecLink, conSendLink);

        // start listening for requests
        SystemServiceRequestListenerImpl l = 
            new SystemServiceRequestListenerImpl(serviceID, links);
        ServiceRequestHandlerThread sp = 
            new ServiceRequestHandlerThread(requestLinksAMS, l);

        // request service
        SystemServiceRequestProtocolClient protocol = 
            new SystemServiceRequestProtocolClient(requestLinksClient);
        LinkMessage emptyMsg = LinkMessage.newStringMessage("");
        reqC2SLink.send(emptyMsg);
        protocol.requestService(serviceID);

        // check that service request went without errors
        Object result = sp.await();
        assertTrue("links sent without error", result == null);
        assertTrue("links received without error", l.linksPassed == true);
        
        // check that connection returned as result of request 
        // is expected one
        SystemServiceConnectionLinks con = 
            protocol.getSystemServiceConnectionLinks();
        assertNotSame("link not same", 
                conSendLink, con.getSendLink());
        assertTrue("links equal", 
                conSendLink.equals(con.getSendLink()));
        assertNotSame("link not same", 
                conRecLink, con.getReceiveLink());
        assertTrue("links equal", 
                conRecLink.equals(con.getReceiveLink()));
    }

    /**
     * Runs all tests.
     */
    public void runTests() throws InterruptedIOException, IOException {
        declare("testActual");
        testActual();
    }
}

