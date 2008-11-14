/*
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

package com.sun.midp.io.j2me.datagram;

import java.io.*;
import javax.microedition.io.*;
import com.sun.midp.i3test.*;

/**
 * Test for a blocked read operation for a datagram. 
 */ 

public class TestDatagramWouldBlock extends TestCase implements Runnable {

    /** Port number for datagram server socket */
    static final int PORT = 1234;

    /** Message string sent from client to server */
    static final String strTestMsg = "klaatu barada nikto\r\n";

    /** Server datagram */
    UDPDatagramConnection server;

    /** client datagram */
    UDPDatagramConnection client;

    /** No of bytes in a message */
    private int numBytesToSend;

    /** Datagrm sent from client */
    private Datagram sendDatagram = null;

    /** Datagram received at server */
    private Datagram receivedDatagram = null;

    /** Flag that indicates that data is read at server */
    boolean readData = false;

    /** Thread that is blocked on the read */
    Thread t1;

    /**
     * Run method to open the server datagram and block on a read.
     */
    public void run() {
        try {
            // Create the server listening datagram socket 
            server = (UDPDatagramConnection) 
                     Connector.open("datagram://:"+PORT);
            // Notify indicating that server is available
            synchronized (this) {
                notifyAll();
            }

            receivedDatagram = createDatagramToReceive();

            server.receive(receivedDatagram);
        } catch (IOException ioe) {
            System.out.println("TestDatagram reader thread failed with:");
            ioe.printStackTrace();
        } finally {
            synchronized (this) {
                readData = true;
                notifyAll();
            }
        }
    }
   
    /**
     * Open the client connection and start the thread to call the run
     * method. It waits until the server is available.
     */
    void setUp() {
        try {
            // Create client datagram socket
            client = (UDPDatagramConnection)
                Connector.open("datagram://localhost:"+PORT);

            // Launch a thread which would be blocked for read operation
            // from above datagram connection
            t1 = new Thread(this);
            t1.start();

            // Wait for the server to become available
            synchronized (this) {
                while (server == null) {
                    try {
                        // Wait for sufficiently long time so that server
                        // is blocked for read operation
                        wait(1200);
                    } catch (InterruptedException e) {
                        // Ignore interrupt
                    }
                }
            }
        } catch (IOException ioe) {
            System.out.println("TestDatagram setUp failed with:");
            ioe.printStackTrace();
        }
    }

    /**
     * In this test, client datagram socket sends a packet to server. 
     * Upon receiving the packet at server, message is printed back.
     */
    void testWouldBlock() {
        assertNotNull("Verify datagram socket open", client);
        sendDatagram = createDatagramToSend("datagram://localhost:"+PORT);

        try {
            client.send(sendDatagram);
        } catch (IOException ioe) {
            cleanUp();
            System.out.println("Cannot send datagram, send thrown " + ioe);
            return;
        }

        synchronized (this) {
            if (!readData) {
                try {
                    wait(2000);
                } catch (InterruptedException e) {
                    System.out.println("Catch interrupt");
                }
            }
        }

        byte[] buf = receivedDatagram.getData();

        String rcvdMsg = new String(buf);
	// IMPL_NOTE: System.out.println("Received msg : " + rcvdMsg);
        assertTrue("Verify data received at server", readData);
    }

    /**
     * Clean up after the test. 
     */
    void cleanUp() {
        try {
            client.close();
            server.close();
        } catch (IOException e) {
            System.out.println("TestSocket cleanUp failed with:");
            e.printStackTrace();
        }
        receivedDatagram = null;
        sendDatagram = null;
    }

    /**
     * Creates a datagram packet to be sent by client
     *
     * @param address Target host address
     * @return Datagram object to be sent
     */
    private Datagram createDatagramToSend(String address) {
        sendDatagram = null;

        numBytesToSend = strTestMsg.length();
        byte[] buf = new byte[numBytesToSend];

        buf = strTestMsg.getBytes();

        try {
            sendDatagram = client.newDatagram(buf, numBytesToSend, address);
        } catch (IOException ioe) {
            cleanUp();
            System.out.println("Cannot create Datagram. IOException: " + ioe);
        }

        return sendDatagram;
    }

    /**
     * Creates a datagram packet to be received at server
     *
     * @return Empty object to receive datagram
     */
    private Datagram createDatagramToReceive() {
        receivedDatagram = null;
        numBytesToSend = strTestMsg.length();

        try {
            receivedDatagram = server.newDatagram(numBytesToSend);
        } catch (IOException ioe) {
            cleanUp();
            System.out.println("Cannot create Datagram. IOException: " + ioe);
        }
        
        return receivedDatagram;
    }

    /**
     * Run the test by setting up datagram client in main thread and datagram
     * server in a separate thread. The server thread blocks for read 
     * operation. The client writes a data packet to server. The test passes
     * successfully when server receives the data back from client properly.
     */
    public void runTests() {
        setUp();

        declare("TestDatagramWouldBlock");
        testWouldBlock();

        cleanUp();
    }

}


