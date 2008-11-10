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
package com.sun.midp.io.j2me.pipe.serviceProtocol;

import com.sun.cldc.isolate.Isolate;
import com.sun.midp.links.Link;
import com.sun.midp.links.LinkMessage;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.services.SystemServiceConnection;
import com.sun.midp.services.SystemServiceConnectionClosedException;
import com.sun.midp.services.SystemServiceDataMessage;
import com.sun.midp.services.SystemServiceLinkMessage;
import com.sun.midp.services.SystemServiceManager;
import com.sun.midp.services.SystemServiceMessage;
import com.sun.midp.services.SystemServiceRequestor;
import java.io.ByteArrayInputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.IOException;
import java.io.InterruptedIOException;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Implementation of low-level inter-isolate protocol to manage Pipes.
 * Both AMS-side and client-side code is provided by this class
 * to minimize chance of making protocol out-of-sync.
 */
public class PipeServiceProtocol {

    static final String SERVICE_ID = "com.sun.midp.io.pipe";
    static final int MAGIC_REGISTER_PIPE_SERVER = 0x49587001;
    static final int MAGIC_REGISTER_PIPE_CLIENT = 0x49587002;
    static final int MAGIC_CLOSE_PIPE_SERVER = 0x49587003;
    static final int MAGIC_ACCEPT_PIPE_SERVER = 0x49587004;
    static final int MAGIC_OK = 0x49587011;
    static final int MAGIC_FAIL = 0x49587012;
    static final int MAGIC_WOULDBLOCK = 0x49587013;
    private static final boolean DEBUG = false;
    private static long nextEndpointIdToIssue;
    private int debugInstanceId;
    private static int nextDebugInstanceId;
    private SecurityToken token;
    private String serverName;
    private String serverVersionRequested;
    private String serverVersionActual;
    private Link inboundLink;
    private Link outboundLink;
    private static SystemServiceConnection conn;
    private long serverInstanceId;
    private Link acceptLink;

    private PipeServiceProtocol(SecurityToken token) {
        this.token = token;
        debugInstanceId = nextDebugInstanceId++;
    }

    public static synchronized PipeServiceProtocol getService(SecurityToken token) {
        if (conn == null) {
            SystemServiceRequestor serviceRequestor =
                    SystemServiceRequestor.getInstance(token);
            conn = serviceRequestor.requestService(SERVICE_ID);
            if (conn == null) {
                if (DEBUG)
                    debugPrintS(" ERR: service not found");
                throw new IllegalStateException("Pipe service not found");
            }
        }

        PipeServiceProtocol service = new PipeServiceProtocol(token);

        return service;
    }

    public static void registerService(SecurityToken token) {
        Dispatcher dispatcher = new Dispatcher(token);
        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.registerService(dispatcher);
        if (DEBUG)
            debugPrintS(" service registered");
    }

    public synchronized void connectClient(String serverName, String serverVersionRequested)
            throws IOException {
        if (DEBUG)
            debugPrint(" connectClient " + serverName + ' ' + serverVersionRequested);
        try {
            SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();
            DataOutput out = msg.getDataOutput();
            out.writeInt(MAGIC_REGISTER_PIPE_CLIENT);
            out.writeUTF(serverName);
            out.writeUTF(serverVersionRequested);
            out.writeLong(Isolate.currentIsolate().uniqueId());
            conn.send(msg);

            msg = (SystemServiceDataMessage) conn.receive();
            DataInput in = msg.getDataInput();
            int result = in.readInt();
            if (result != MAGIC_OK) {
                throw new ConnectionNotFoundException(in.readUTF());
            }
            serverVersionActual = in.readUTF();

            this.serverName = serverName;
            this.serverVersionRequested = serverVersionRequested;
            SystemServiceLinkMessage linkMsg = (SystemServiceLinkMessage) conn.receive();
            inboundLink = linkMsg.getLink();
            linkMsg = (SystemServiceLinkMessage) conn.receive();
            outboundLink = linkMsg.getLink();
        } catch (SystemServiceConnectionClosedException cause) {
            if (DEBUG) {
                debugPrint(" ERR:");
                cause.printStackTrace();
            }
            throw new IOException("Cannot communicate with pipe service: " + cause);
        }
        if (DEBUG)
            debugPrint(" connectClient " + serverName + ' ' + serverVersionRequested + " completed");
    }

    public synchronized void connectServer(String serverName, String serverVersion)
            throws IOException {
        if (DEBUG)
            debugPrint(" connectServer " + serverName + ' ' + serverVersion);
        try {
            SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();
            DataOutput out = msg.getDataOutput();
            out.writeInt(MAGIC_REGISTER_PIPE_SERVER);
            out.writeUTF(serverName);
            out.writeUTF(serverVersion);
            out.writeLong(Isolate.currentIsolate().uniqueId());
            conn.send(msg);

            msg = (SystemServiceDataMessage) conn.receive();
            DataInput in = msg.getDataInput();
            int result = in.readInt();
            if (result != MAGIC_OK) {
                throw new ConnectionNotFoundException(in.readUTF());
            }
            serverInstanceId = in.readLong();

            this.serverName = serverName;
            this.serverVersionActual = serverVersion;
        } catch (SystemServiceConnectionClosedException cause) {
            if (DEBUG) {
                debugPrint(" ERR:");
                cause.printStackTrace();
            }
            throw new IOException("Cannot communicate with pipe service: " + cause);
        }
        if (DEBUG)
            debugPrint(" connectServer " + serverName + ' ' + serverVersion + " completed");
    }

    public PipeServiceProtocol acceptByServer() throws IOException {
        if (DEBUG)
            debugPrint(" acceptByServer");
        try {
            synchronized (this) {
                SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();
                DataOutput out = msg.getDataOutput();
                out.writeInt(MAGIC_ACCEPT_PIPE_SERVER);
                out.writeLong(serverInstanceId);
                conn.send(msg);
                SystemServiceLinkMessage linkMsg = (SystemServiceLinkMessage) conn.receive();
                acceptLink = linkMsg.getLink();
            }

            // now block waiting for client to come
            LinkMessage lMsg = acceptLink.receive();
            DataInput in = new DataInputStream(new ByteArrayInputStream(lMsg.extractData()));
            if (in.readInt() == MAGIC_REGISTER_PIPE_CLIENT) {
                PipeServiceProtocol service = new PipeServiceProtocol(token);
                service.serverVersionRequested = in.readUTF();
                lMsg = acceptLink.receive();
                service.inboundLink = lMsg.extractLink();
                lMsg = acceptLink.receive();
                service.outboundLink = lMsg.extractLink();

                if (DEBUG)
                    debugPrint(" acceptByServer completed");

                return service;
            } else {
                throw new IOException();
            }
        } catch (SystemServiceConnectionClosedException cause) {
            if (DEBUG) {
                debugPrint(" ERR:");
                cause.printStackTrace();
            }
            throw new IOException("Cannot communicate with pipe service: " + cause);
        } catch (InterruptedIOException cause) {
            if (DEBUG) 
                debugPrint(" acceptByServer aborted for " + serverInstanceId);
            throw cause;
        } catch (IOException cause) {
            if (DEBUG) {
                debugPrint(" ERR:");
                cause.printStackTrace();
            }
            throw new IOException("Cannot communicate with pipe service: " + cause);
        }
    }

    public synchronized void closeServer() throws IOException {
        if (DEBUG)
            debugPrint(" closeServer");
        
        // close accept link if there is one. causes abort of pending acceptByServer
        try {
            if (acceptLink != null)
                acceptLink.close();
        } catch (Exception e) {
            // ignored
        }

        // send notification to the pipe service
        try {
            SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();
            DataOutput out = msg.getDataOutput();
            out.writeInt(MAGIC_CLOSE_PIPE_SERVER);
            out.writeLong(serverInstanceId);
            conn.send(msg);

            msg = (SystemServiceDataMessage) conn.receive();
            DataInput in = msg.getDataInput();
            int result = in.readInt();
            if (result != MAGIC_OK) {
                throw new IOException(in.readUTF());
            }
        } catch (SystemServiceConnectionClosedException cause) {
            if (DEBUG) {
                debugPrint(" ERR:");
                cause.printStackTrace();
            }
            throw new IOException("Cannot communicate with pipe service: " + cause);
        }
    }

    public void closeClient() throws IOException {
        if (inboundLink != null)
            inboundLink.close();
        if (outboundLink != null)
            outboundLink.close();
    }

    private void debugPrint(String msg) {
        System.out.println("[pipe " + Isolate.currentIsolate().id() + '/' + debugInstanceId + "] " + msg);
    }

    static void debugPrintS(String msg) {
        System.out.println("[pipe " + Isolate.currentIsolate().id() + "] " + msg);
    }

    static int parseVersion(String str) {
        int dot1 = str.indexOf('.');
        int dot2 = str.indexOf('.', dot1 + 1);
        if (dot1 < 1 || dot2 == dot1 + 1)
            throw new IllegalArgumentException("Malformed server version");
        int version = 0;
        try {
            if (dot2 < 0)
                dot2 = str.length();
            version = Integer.parseInt(str.substring(0, dot1)) * 10000 +
                    Integer.parseInt(str.substring(dot1 + 1, dot2)) * 100;
            if (dot2 < str.length())
                version += Integer.parseInt(str.substring(dot2));
        } catch (NumberFormatException ex) {
            throw new IllegalArgumentException("Malformed server version");
        }
        return version;
    }

    synchronized static long generateEndpointId() {
        return nextEndpointIdToIssue++;
    }

    public Link getOutboundLink() {
        return outboundLink;
    }

    public Link getInboundLink() {
        return inboundLink;
    }

    public String getServerName() {
        return serverName;
    }

    public String getServerVersionActual() {
        return serverVersionActual;
    }

    public String getServerVersionRequested() {
        return serverVersionRequested;
    }
}
