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

package com.sun.midp.io.j2me.socket;

import java.io.*;
import javax.microedition.io.*;
import com.sun.midp.i3test.*;

public class TestNonblockingSocket extends TestCase {

    static final int PORT = 1234;
    static final String TEST_MSG = "klaatu barada nikto\r\n";

    ServerSocketConnection server;
    SocketConnection client;

    void setUp() {
	try {
            // Create the server listening socket 
            server = (ServerSocketConnection) Connector.open("socket://:"+PORT);
	    // System.out.println("Server socket opened");
	    
	    // Create client socket
	    client = (SocketConnection)
		      Connector.open("socket://localhost:"+PORT);
	    // System.out.println("Client socket opened");
	} catch (IOException ioe) {
	    System.out.println("TestSocket setUp failed with:");
	    ioe.printStackTrace();
	}
    }
    
    /**
     * This test writes to client socket before accepting server socket.
     * If client socket write is blocking, the server socket won't be 
     * responding to its write. Therefore, the accepting will fail.
     * Passing this test proves that client socket write is nonblocking.
     */
    void testNonblocking() {
	try {
	    // Write to client socket
	    // System.out.println("Writing: " + TEST_MSG);
	    OutputStream os = client.openOutputStream();
	    os.write(TEST_MSG.getBytes());
	    os.close();

            // Wait for incoming connection to server socket
	    // System.out.println("Accepting serversocket");
            SocketConnection sc = (SocketConnection) server.acceptAndOpen();
	    // System.out.println("Accepted serversocket");

	    // Read from server socket
	    InputStream is = sc.openInputStream();
	    byte buf[] = new byte[2*TEST_MSG.length()];
	    int i = 0;
	    do {
                buf[i++] = (byte) is.read();
	    } while (buf[i-1] != -1 && buf[i-1] != '\n' && i < buf.length);

	    is.close();
	    sc.close();

	    String recvd = new String(buf, 0, i);
	    // System.out.println("recvd=\""+recvd+"\"");

            assertTrue(recvd.equals(TEST_MSG));

	} catch (IOException ioe) {
	    System.out.println("TestSocket:Nonblocking test failed with:");
	    ioe.printStackTrace();
	}
    }

    void tearDown() {
	try {
	    // System.out.println("Closing connection");
            client.close();
            server.close();
	} catch (IOException e) {
	    System.out.println("TestSocket tearDown failed with:");
	    e.printStackTrace();
	}
    }

    public void runTests() {
	setUp();

        declare("Test Nonblocking");
	testNonblocking();

	tearDown();
    }

}
