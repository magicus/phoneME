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
package com.sun.midp.io.j2me.pipe;

import com.sun.midp.i3test.TestCase;
import com.sun.midp.io.j2me.pipe.serviceProtocol.PipeServiceProtocol;
import com.sun.midp.io.pipe.PipeConnection;
import com.sun.midp.io.pipe.PipeServerConnection;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.services.SystemServiceManager;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.Vector;
import javax.microedition.io.Connector;

/**
 *
 */
public class TestPipeConnection extends TestCase {

    private static SecurityToken token = SecurityTokenProvider.getToken();

    private abstract class Server implements Runnable {

        public void run() {
            try {
                PipeServerConnection serverConn =
                        (PipeServerConnection) Connector.open("pipe://:TestPipeConnection:1.0;");

                PipeConnection dataConn = (PipeConnection) serverConn.acceptAndOpen();
                communicate(dataConn);

            } catch (IOException ex) {
                ex.printStackTrace();
                fail(ex.toString());
            }
        }

        abstract void communicate(PipeConnection dataConn) throws IOException;
    }

    void testConnectionOpenClose() throws IOException {
        PipeServiceProtocol.registerService(token);

        try {
            PipeServerConnection serverConn =
                    (PipeServerConnection) Connector.open("pipe://:TestPipeConnection:1.0;");
            serverConn.close();
        } catch (IOException iOException) {
            iOException.printStackTrace();
            fail("Server open+close failed " + iOException);
        }

        boolean hasOpenedClient = true;
        try {
            PipeConnection clientConn =
                    (PipeConnection) Connector.open("pipe://*:TestPipeConnection:1.0;");
        } catch (IOException ex) {
            hasOpenedClient = false;
        }
        assertFalse("Should not have been opened client connection", hasOpenedClient);

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }

    void testConnectionOpenAcceptClose() throws IOException {
        PipeServiceProtocol.registerService(token);

        boolean hasBeenInterrupted = false;
        try {
            final PipeServerConnection serverConn =
                    (PipeServerConnection) Connector.open("pipe://:TestPipeConnection:1.0;");

            new Thread(new Runnable() {

                public void run() {
                    try {
                        Thread.sleep(100);

                        serverConn.close();
                    } catch (IOException ex) {
                        ex.printStackTrace();
                    } catch (InterruptedException ex) {
                        ex.printStackTrace();
                    }
                }
            }).start();
            try {
                serverConn.acceptAndOpen();
            } catch (InterruptedIOException ex) {
                hasBeenInterrupted = true;
            }
        } catch (IOException iOException) {
            iOException.printStackTrace();
            fail("Open+close failed " + iOException);
        }

        assertTrue("acceptAndOpen should has been interrupted", hasBeenInterrupted);

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }

    void testLocalTransfer() throws IOException, InterruptedException {
        PipeServiceProtocol.registerService(token);

        new Thread(new Server() {

            void communicate(PipeConnection dataConn) throws IOException {
                DataInputStream in = dataConn.openDataInputStream();
                DataOutputStream out = dataConn.openDataOutputStream();

                out.writeUTF("1");
                out.close();
                assertEquals("invalid data response", "2", in.readUTF());
            }
        }).start();

        Thread.sleep(100);

        try {
            PipeConnection clientConn =
                    (PipeConnection) Connector.open("pipe://*:TestPipeConnection:1.0;");

            DataInputStream in = clientConn.openDataInputStream();
            DataOutputStream out = clientConn.openDataOutputStream();

            assertEquals("invalid message from server", "1", in.readUTF());
            out.writeUTF("2");
            out.close();

        } catch (IOException ex) {
            ex.printStackTrace();
            fail(ex.toString());
        }

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }

    void testLocalTransferClose() throws IOException, InterruptedException {
        PipeServiceProtocol.registerService(token);

        Thread readerThread = new Thread(new Server() {

            public void communicate(PipeConnection dataConn) throws IOException {
                DataInputStream in = dataConn.openDataInputStream();

                assertEquals("invalid data response", "1", in.readUTF());
                assertEquals("expected EOF not found", -1, in.read());
            }
        });
        readerThread.start();

        Thread.sleep(100);

        try {
            PipeConnection clientConn =
                    (PipeConnection) Connector.open("pipe://*:TestPipeConnection:1.0;");

            DataOutputStream out = clientConn.openDataOutputStream();

            out.writeUTF("1");
            out.close();

        } catch (IOException ex) {
            ex.printStackTrace();
            fail(ex.toString());
        }
        
        readerThread.join();

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }

    void testLocalTransferDeferredOpen() throws IOException, InterruptedException {
        PipeServiceProtocol.registerService(token);

        final Vector status = new Vector(1);

        new Thread(new Server() {

            void communicate(PipeConnection dataConn) throws IOException {
                DataOutputStream out = dataConn.openDataOutputStream();

                out.writeUTF("1");
                out.close();

                assertEquals("Receiver should have not received message by this moment", 0, status.size());

                DataInputStream in = dataConn.openDataInputStream();
                assertEquals("invalid data response", "2", in.readUTF());
            }
        }).start();

        Thread.sleep(100);

        try {
            PipeConnection clientConn =
                    (PipeConnection) Connector.open("pipe://*:TestPipeConnection:1.0;");

            // give a chance for writer to write into buffer
            Thread.sleep(100);

            DataInputStream in = clientConn.openDataInputStream();
            DataOutputStream out = clientConn.openDataOutputStream();

            assertEquals("invalid message from server", "1", in.readUTF());
            status.addElement("1");

            out.writeUTF("2");
            out.close();

        } catch (IOException ex) {
            ex.printStackTrace();
            fail(ex.toString());
        }

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }

    public void runTests() throws Throwable {
        declare("testConnectionOpenClose");
        testConnectionOpenClose();

        declare("testConnectionOpenAcceptClose");
        testConnectionOpenAcceptClose();

        declare("testLocalTransfer");
        testLocalTransfer();

        declare("testLocalTransferClose");
        testLocalTransferClose();
        
        declare("testLocalTransferDeferredOpen");
        testLocalTransferDeferredOpen();
    }
}
