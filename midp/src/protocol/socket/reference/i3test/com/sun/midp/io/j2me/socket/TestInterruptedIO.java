/*
 * 	
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.io.j2me.socket;

import java.io.*;
import javax.microedition.io.*;
import com.sun.midp.i3test.*;

/**
 * Test of SocketConnection function that unblocks a pending read
 * when the connection is closed.
 */
public class TestInterruptedIO extends TestCase implements Runnable {

    /** SocketConnection being tested. */
    SocketConnection connection;

    /** InputStream for the blocking read. */
    InputStream is;

    /** Thread that is blocked on the read. */
    Thread t1;

    /** Records if the expected InterruptedIOException is thrown. */
    boolean exceptionThrown = false;

    /** Records if the thread exited. */
    boolean done = false;

    /**
     * Run method to open the InputStream and block on a read.
     * When the stream/connection is closed the read should 
     * unblock with an InterruptedIOException.
     */
    public void run() {
        try {
            is = connection.openInputStream();

            // Notify indicating the InputStream is open
            synchronized (this) {
                notifyAll();
            }

            // Block on a read
            int ch = is.read();
            assertTrue("read should not have succeeded", false);
        } catch (InterruptedIOException ioe) {
            assertNotNull("Caught InterruptedIOException", ioe);
            exceptionThrown = true;
        } catch (IOException ioe) {
            assertNull("Unexpected IOException", ioe);
        } finally {
            synchronized (this) {
                done = true;     
                notifyAll();        
            }
        }
    }

    /**
     * Open the connection and start the thread to call the run
     * method.  It waits until the InputStream is open before returning.
     */
    void setUp() {
        // Create a socket connection 
        try {
            connection = (SocketConnection)
              Connector.open("socket://host.domain:80");
            //OutputStream os = connection.openOutputStream();
            //os.write("GET /midlets/midlet.jad HTTP/1.0\n\n".getBytes());
        } catch (IOException ioe) {
            assertNull("Exception during socket open", ioe);
        }
        
        assertNotNull("Verify socket open", connection);

        // Launch a thread which would be blocked for some I/O operation
        // for above socket connection
        t1 = new Thread(this);
        t1.start();

        // Wait for the InputStream to have been opened
        synchronized (this) {
            while (is == null) {
                try {
                    wait(200);
                } catch (InterruptedException e) {
                    // Ignore interrupt
                }
            } 
        }
    }
    
    /**
     * Close the open connection.
     */
    void closeConnection() {
        try {
	    if (is != null) {
		is.close();
		is = null;
	    }
	    if (connection != null) {
		connection.close();
		connection = null;
	    }
        } catch (IOException e) {
            assertNull("Exception closing a socket", e);
            e.printStackTrace();
        }
    }

    /**
     * This test creates another thread that tries to close a socket
     * connection that is in use by first thread. It is supposed to throw 
     * InterruptedIOException in this case.
     */
    void testForException() {
        closeConnection();

        synchronized (this) {
            if (!done) {
                try {
                    wait(2000);
                } catch (InterruptedException e) {
                    System.out.println("Catch interrupt");
                }
            } 
        }

        assertTrue("Verify InterruptedIOException", exceptionThrown);
    }

    /**
     * Clean up after the test. 
     */
    void tearDown() {
	closeConnection();
    }

    /**
     * Run the test by setting up the connection, and starting
     * a thread to block on a read then  forcing the 
     * connection to be closed and checking that exception occurred.
     */
    public void runTests() {
        declare("Test InterruptedIOException");

        setUp();

        testForException();

        tearDown();
    }

}
