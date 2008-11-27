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

import com.sun.midp.links.*;
import java.io.*;
import java.util.*;
import com.sun.cldc.isolate.*;
import com.sun.midp.i3test.TestCase;

/**
 * Tests for system service requesting functionality
 */
public class TestSystemServiceRequest extends TestCase {
    public final static String SERVICE_ID = "42";

    class DummySystemServiceManager extends SystemServiceManager {
        SystemService service = null;

        DummySystemServiceManager() {
        }

        public void registerService(SystemService service) {
            this.service = service;
        }

        public SystemService getService(String serviceID) {
            if (service.getServiceID().equals(serviceID)) {
                return service;
            } else {
                return null;
            }
        }

        public void shutdown() {
        }
    }

    class DummySystemService implements SystemService {
        boolean wasRequested = false;

        public String getServiceID() {
            return SERVICE_ID;
        }

        public void start() {
        }

        public void stop() {
        }

        public void acceptConnection(SystemServiceConnection connection) {
            wasRequested = true;
        }
    }

    void testRemote() 
        throws IsolateStartupException,
               InterruptedIOException,
               IOException,
               ClosedLinkException {

        SystemServiceManager serviceManager = new DummySystemServiceManager();
        DummySystemService service = new DummySystemService();
        serviceManager.registerService(service);

        SystemServiceRequestHandler requestHandler = 
            new SystemServiceRequestHandler(serviceManager);

        Isolate serviceIsolate = Isolate.currentIsolate();
        Isolate clientIsolate = new Isolate(
                "com.sun.midp.services.SystemServiceRequestIsolate", null);
        clientIsolate.start();

        IsolateSystemServiceRequestHandler isolateRequestHandler = 
            requestHandler.newIsolateRequestHandler(clientIsolate);

        SystemServiceConnectionLinks links = isolateRequestHandler.getSendReceiveLinks();
        Link[] linkArray = new Link[2];
        linkArray[0] = links.getSendLink();
        linkArray[1] = links.getReceiveLink();
        LinkPortal.setLinks(clientIsolate, linkArray);

        requestHandler.handleIsolateRequests(isolateRequestHandler);

        clientIsolate.waitForExit();
        
        assertTrue("Service requested", service.wasRequested);
    }

    void testLocal() {
        SystemServiceManager serviceManager = new DummySystemServiceManager();
        DummySystemService service = new DummySystemService();
        serviceManager.registerService(service);

        SystemServiceRequestorLocal serviceRequestor = 
            new SystemServiceRequestorLocal(serviceManager);
        serviceRequestor.requestService(SERVICE_ID);
    
        assertTrue("Service requested", service.wasRequested);

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
