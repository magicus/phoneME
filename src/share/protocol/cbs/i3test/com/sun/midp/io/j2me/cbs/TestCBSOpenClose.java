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

package com.sun.midp.io.j2me.cbs;

// Classes
import com.sun.midp.i3test.TestCase;
import javax.microedition.io.Connector;
import javax.wireless.messaging.MessageConnection;

// Exceptions
import java.io.IOException;

/**
 * Tests if an CBS connection can simply be open and closed without sending or
 * receiving any messages. This test also tries to find failures when closing a
 * connection more than once (A valid case).
 */ 
public class TestCBSOpenClose extends TestCase implements Runnable {

    /** The fully qualified name of this test. */
    private final String TEST_NAME = this.getClass().getName();


    /** The device ID ("phone number") of the recipient. */
    private final String CBS_CLIENT_DEVICE_ID = "+5559999";

    /** The application ID of the recipient. */
    private final int CBS_CLIENT_PORT = 12345;

    /** The CBS client address with a host name (Illegal). */
    private final String CBS_CLIENT_ADDRESS1 =
        "cbs://" + CBS_CLIENT_DEVICE_ID + ":" + CBS_CLIENT_PORT;

    /** The CBS client address without a host name (Legal). */
    private final String CBS_CLIENT_ADDRESS2 =
        "cbs://:" + CBS_CLIENT_PORT;


    /** The CBS connection. */
    private MessageConnection con = null;

    /** Test passed/failed flag. */
    private boolean passed = false;


    /**
     * Set up the physical connection.
     */
    void setUp() {

        // Try an illegal address first.
        try {
            createClientConnection(CBS_CLIENT_ADDRESS1);
        } catch (IllegalArgumentException iae) {
            // This is a good thing. Cannot have a host name in address.
        } catch (IOException ioe) {
            System.out.println(TEST_NAME + " set-up failed:");
            ioe.printStackTrace();
        }

        // Now try a legal address.
        try {
            createClientConnection(CBS_CLIENT_ADDRESS2);
        } catch (IOException ioe) {
            System.out.println(TEST_NAME + " set-up failed:");
            ioe.printStackTrace();
        }
    }

    /**
     * Simply close the open connection. If successful, this test passes.
     */
    public void run() {

        try {

            closeConnection();

            /* 
             * Close connection again. This is allowed by the spec
             * and shouldn't cause any exceptions to be thrown.
             */
	    closeConnection();

            passed = true;
            assertTrue("This indicates we passed.", passed);

        } catch (Exception e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
        }
    }

    /**
     * Provide clean-up services, following the run of this test.
     */
    void cleanUp() {
        closeConnection();
    }

    /**
     * Create and open an CBS connection.
     *
     * @param clientAddress The CBS address of the client.
     */
    private void createClientConnection(String clientAddress)
        throws IOException {

        con = (MessageConnection)Connector.open(clientAddress);
    }

    /**
     * Close the CBS connection.
     */
    private void closeConnection() {
        try {
            con.close();
        } catch (IOException ioe) {
            // Fail silently.
        } catch (Exception e) {
            // Fail silently.
        }
    }

    /**
     * Main entry point.
     */
    public void runTests() {

        setUp();

        declare(TEST_NAME);

        run();

        cleanUp();
    }

}

