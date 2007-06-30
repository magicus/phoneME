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
import com.sun.midp.links.*;
import java.io.*;
import java.util.*;
import com.sun.cldc.isolate.*;
import com.sun.midp.i3test.TestCase;

public class TestSystemServiceManager extends TestCase {
    public final static String SERVICE_ID = "42";

    private static SecurityToken token = SecurityTokenProvider.getToken();

    class DummySystemService implements SystemService {
        boolean wasRequested = false;
        boolean wasStarted = false;
        boolean wasStopped = false;

        public String getServiceID() {
            return SERVICE_ID;
        }

        public void start() {
            wasStarted = true;
        }

        public void stop() {
            wasStopped = true;
        }

        public void acceptConnection(SystemServiceConnection connection) {
            wasRequested = true;
        }
    }

    void testActual() {
        SystemServiceManager serviceManager = 
            SystemServiceManager.getInstance(token);
        DummySystemService service = new DummySystemService();
        serviceManager.registerService(service);
        
        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);
        serviceRequestor.requestService(SERVICE_ID);

        assertTrue("Service started", service.wasStarted);
        assertTrue("Service requested", service.wasRequested);
    }

    /**
     * Runs all tests.
     */
    public void runTests() {
        declare("testActual");
        testActual();
    }
}
