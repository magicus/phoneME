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

package com.sun.midp.suspend;

import com.sun.midp.i3test.TestCase;
import com.sun.midp.suspend.test.TestUtil;

import javax.microedition.io.Connector;
import javax.microedition.io.StreamConnectionNotifier;
import javax.microedition.io.StreamConnection;
import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

/**
 * Tests connection behavior within suspend/resume cycles.
 */
public class TestConnectionSuspend extends TestCase {
    private Side server;
    private Side client;
    
    private void init() {
        TestUtil.setNoVMSuspendMode();
        server = new ServerSide();
        TestUtil.sleep(1000);
        client = new ClientSide();
        client.waitReady();
        server.waitReady();
    }


    public void runTests() throws Throwable {
        init();

        declare("Test connection before Suspend");

        assertTrue("positive write", client.canWrite());
        assertTrue("positive read", server.canRead());

        declare("Test connection after Suspend");
        TestUtil.suspendMidp();
        TestUtil.sleep();

        assertTrue("negative write", !client.canWrite());
        assertTrue("negative read", !server.canRead());

        declare("Test connection after Resume");
        TestUtil.resumeMidp();
        TestUtil.sleep();

        assertTrue("negative write2", !client.canWrite());
        assertTrue("negative read2", !server.canRead());

        client.closeAll();
        server.closeAll();
    }
}

abstract class Side implements Runnable {
    private StreamConnection conn;
    private DataInputStream in;
    private DataOutputStream out;
    private final Object lock = new Object();
    private String error;
    static final int port = 33133;

    void closeAll() {
        try {
            conn.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    abstract StreamConnection connect() throws IOException;

    boolean canRead() {
        try {
            in.readInt();
        }
        catch (IOException e) {
            return false;
        }
        return true;
    }

    boolean canWrite() {
        try {
            out.writeInt(1);
            out.flush();
        }
        catch (IOException e) {
            return false;
        }
        return true;
    }

    void waitReady() {
        synchronized (lock) {
            if (null == conn && null == error) {
                try {
                    lock.wait();
                }
                catch (InterruptedException e) {
                    error = "interrupted";
                }
            }
        }

        if (error != null) {
            throw new RuntimeException(error);
        }
    }

    public void run() {
        synchronized (lock) {
            try {
                    conn = connect();
                    in = conn.openDataInputStream();
                    out = conn.openDataOutputStream();
                }
            catch (IOException e) {
                error = e.getMessage();
            }
            finally {
                lock.notify();
            }
        }
    }

    Side() {
        new Thread(this).start();
    }
}

class ServerSide extends Side {
    StreamConnectionNotifier notif;

    StreamConnection connect() throws IOException {
        notif = (StreamConnectionNotifier)
                Connector.open("socket://:" + port);
        return notif.acceptAndOpen();
    }
    void closeAll() {
        try {
            notif.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        super.closeAll();
    }
}

class ClientSide extends Side {
    StreamConnection connect() throws IOException {
        return (StreamConnection) Connector.open("socket://localhost:" + port);
    }
}
