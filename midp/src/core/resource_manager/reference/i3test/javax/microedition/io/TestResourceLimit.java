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
package javax.microedition.io;

import com.sun.midp.i3test.*;
import javax.microedition.rms.RecordStore;
import javax.microedition.rms.RecordStoreException;
import com.sun.midp.configurator.Constants;

import java.io.IOException;

/**
 * Unit tests for resource limit checks
 */

public class TestResourceLimit extends TestCase {
    /**
     * URI to which the test will try to open connections.
     * Use any URI visible from your network, for example,
     * "socket://www.sun.com:80" will do in most cases
     * (but only if you do not need a proxy to connect to it);
     * "socket://localhost:80" will do if your computer is
     * running an http server. Please, contact your network
     * administrator if you have a problem that you cannot
     * resolve yourself.
     */
    static private final String otherSideUri =
            //"socket://www.sun.com:80";
            "socket://localhost:80";

    /**
     * Test for TCP client resource limit check
     */
    void testTCPClientRscLimit() throws java.io.IOException {
        boolean exceptionThrown = false;
        boolean cleanupFailed = false;
        int openCount = 0;
        SocketConnection[] sc = 
            new SocketConnection[Constants.TCP_CLI_AMS_LIMIT+1];

        try {
            // Setup
            while (openCount < Constants.TCP_CLI_AMS_LIMIT) {
                sc[openCount] = (SocketConnection) Connector.open(
                        otherSideUri);
                openCount++;
            }

            // Actual Test
            try {
                sc[openCount] = (SocketConnection) Connector.open(
                        otherSideUri);
                openCount++;
            } catch (java.io.IOException io) {
                exceptionThrown = true;
            }
        } catch (ConnectionNotFoundException cnfe) {
            fail("Exception while opening "+otherSideUri+": "+cnfe.getMessage()+
                 ".  If the host name cannot be resolved, you may " +
                 "need to change the i3test source to use a host name that" +
                 " is visible from your network.");
            return;
        } finally {
            // Cleanup
            for (int i = 0; i < openCount; i++) {
                try {
                    sc[i].close();
                } catch (java.io.IOException io) {
                    cleanupFailed = true;
                }
            }
            if (cleanupFailed) {
                throw new java.io.IOException("Cleanup failed TCP clients");
            }
        }

        assertTrue("expected exception thrown?",exceptionThrown);
    }

    /**
     * Test for TCP server resource limit check
     */
    void testTCPServerRscLimit() throws java.io.IOException {
        boolean exceptionThrown = false;
        boolean cleanupFailed = false;
        int openCount = 0;
        int localServerPort = 10000;
        ServerSocketConnection[] sc = 
            new ServerSocketConnection[Constants.TCP_SER_AMS_LIMIT+1];

        try {
            // Setup 
            while (openCount < Constants.TCP_SER_AMS_LIMIT) {
                String s = "socket://:" + Integer.toString(localServerPort + 
														   openCount);
                try {
                    sc[openCount] = (ServerSocketConnection) Connector.open(s);
                    openCount++;
                } catch (IOException e) {
                    fail("error opening "+s+" (of " +
                            Constants.TCP_SER_AMS_LIMIT +
                            " ServerSocketConnections) " +
                            "-- Probably, some connections were open before" +
                            " starting the test");
                    e.printStackTrace();
                    return; // performs "finally"
                }
            }

            // Actual Test
            try {
                String s = "socket://:" + Integer.toString(localServerPort + 
														   openCount);
                sc[openCount] = (ServerSocketConnection) Connector.open(s);
                openCount++;
            } catch (java.io.IOException io) {
                exceptionThrown = true;
            } 
        } finally {
            // Cleanup
            for (int i = 0; i < openCount; i++) {
                try {
                    sc[i].close();
                } catch (java.io.IOException io) {
                    cleanupFailed = true;
                }
            }
            if (cleanupFailed) {
                throw new java.io.IOException("Cleanup failed TCP servers");
            }
        }

        assertTrue(exceptionThrown);
    }

    /**
     * Test for datagram resource limit check
     */
    void testDatagramRscLimit() throws java.io.IOException {
        boolean exceptionThrown = false;
        boolean cleanupFailed = false;
        int openCount = 0;
        int localServerPort = 10000;
        DatagramConnection[] sc = 
            new DatagramConnection[Constants.UDP_AMS_LIMIT+1];

        try {
            // Setup 
            while (openCount < Constants.UDP_AMS_LIMIT) {
                String s = "datagram://:" + Integer.toString(localServerPort + 
															 openCount);
                sc[openCount] = (DatagramConnection) Connector.open(s);
                openCount++; 
            }

            // Actual Test
            try {
                String s = "datagram://:" + Integer.toString(localServerPort + 
															 openCount);
                sc[openCount] = (DatagramConnection) Connector.open(s);
                openCount++;
            } catch (java.io.IOException io) {
                exceptionThrown = true;
            } 
        } finally {
            // Cleanup
            for (int i = 0; i < openCount; i++) {
                try {
                    sc[i].close();
                } catch (java.io.IOException io) {
                    cleanupFailed = true;
                }
            }
            if (cleanupFailed) {
                throw new java.io.IOException("Cleanup failed datagrams");
            }
        }

        assertTrue(exceptionThrown);
    }

    /**
     * Main method that launches the unit tests
     */
    public void runTests() throws Throwable {
        declare("testTCPClientRscLimit");
        testTCPClientRscLimit();

        declare("testTCPServerRscLimit");
        testTCPServerRscLimit();

        declare("testDatagramRscLimit");
        testDatagramRscLimit();
    }

}

    
