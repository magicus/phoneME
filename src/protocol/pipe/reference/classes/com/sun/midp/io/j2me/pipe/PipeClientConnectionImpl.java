/*
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
package com.sun.midp.io.j2me.pipe;

import com.sun.midp.io.j2me.pipe.serviceProtocol.PipeServiceProtocol;
import com.sun.midp.io.ConnectionBaseAdapter;
import com.sun.midp.io.pipe.PipeConnection;
import com.sun.midp.links.ClosedLinkException;
import com.sun.midp.links.Link;
import com.sun.midp.links.LinkMessage;
import java.io.IOException;
import java.io.InterruptedIOException;
import javax.microedition.io.Connection;
import com.sun.midp.security.SecurityToken;
import java.util.Vector;

class PipeClientConnectionImpl extends ConnectionBaseAdapter implements PipeConnection {

    private static final boolean DEBUG = true;
    
    private PipeServiceProtocol pipe;
    private SecurityToken token;
    private Object suiteId;
    private String serverName;
    private String version;
    private Link sendLink;
    private Link receiveLink;
    private byte[] bufferedMsg;
    private int bufferedMsgOffset;
    private Thread senderThread;
    private Sender sender;
    private Vector sendQueue = new Vector(1);
    private IOException sendStatus;

    PipeClientConnectionImpl(SecurityToken token, PipeServiceProtocol pipe) {
        this.pipe = pipe;
        this.token = token;
        serverName = pipe.getServerName();
        version = pipe.getServerVersionRequested();
    }

    PipeClientConnectionImpl(Object suiteId, String serverName, String version, SecurityToken token) {
        this.token = token;
        this.suiteId = suiteId;
        this.serverName = serverName;
        this.version = version;
    }

    void establishTransfer(int mode) throws IOException {
        receiveLink = pipe.getInboundLink();
        sendLink = pipe.getOutboundLink();
        sender = new Sender();
        senderThread = new Thread(sender);
        senderThread.start();

        initStreamConnection(mode);
    }

    protected void notifyClosedInput() {
        if (DEBUG)
            debugPrint("input closed");
        
        super.notifyClosedInput();
        
        receiveLink.close();
    }

    protected void notifyClosedOutput() {
        if (DEBUG)
            debugPrint("output closed");
        (new Exception()).printStackTrace();
        
        super.notifyClosedOutput();
        
        sendLink.close();
    }

    void establish(int mode) throws IOException {
        pipe = PipeServiceProtocol.getService(token);
        pipe.connectClient(serverName, version);

        establishTransfer(mode);
    }

    public Connection openPrim(String name, int mode, boolean timeouts) throws IOException {
        throw new IOException("This method should not be called because it should not exist. Please refactor");
    }

    protected void disconnect() throws IOException {
        if (DEBUG)
            debugPrint("disconnected");
        
        connectionOpen = false;
        
        synchronized (sender) {
            sender.notify();
        }
    }

    public int available() throws IOException {
        // TODO: prefetch input
        int len = bufferedMsg == null ? 0 : bufferedMsg.length - bufferedMsgOffset;

        if (DEBUG)
            debugPrint("available " + len + " bytes");
        
        return len;
    }

    protected synchronized int readBytes(byte[] b, int off, int len) throws IOException {
        if (DEBUG)
            debugPrint("readBytes len=" + len + ", can read " + (connectionOpen && iStreams > 0));
        if (!connectionOpen || iStreams == 0)
            throw new IOException();

        int originalOffset = off;
        int bytesRead;
        if (bufferedMsg != null) {
            bytesRead = readFromBuffer(b, off, len);
            len -= bytesRead;
            off += bytesRead;
        }

        while (len > 0) {
            if (!receiveLink.isOpen()) {
                if (DEBUG)
                    debugPrint("readBytes: link closed");
                closeInputStream();
                return off - originalOffset;
            }
            LinkMessage lm = receiveLink.receive();
            if (!lm.containsData())
                throw new IOException();

            bufferedMsg = lm.extractData();
            if (DEBUG)
                debugPrint("readBytes: received message. " + bufferedMsg.length + " bytes");
            bufferedMsgOffset = 0;
            bytesRead = readFromBuffer(b, off, len);
            len -= bytesRead;
            off += bytesRead;
        }
        
        if (DEBUG)
            debugPrint("readBytes: read " + (off - originalOffset) + " bytes");
        
        return off - originalOffset;
    }

    private void debugPrint(String msg) {
        System.out.println("[pipe client conn " + Integer.toHexString(hashCode()) + "] " + msg);
    }

    private int readFromBuffer(byte[] b, int off, int len) {
        int remainingData = bufferedMsg.length - bufferedMsgOffset;
        if (remainingData >= len) {
            remainingData = len;
        }
        System.arraycopy(bufferedMsg, bufferedMsgOffset, b, off, remainingData);
        bufferedMsgOffset += remainingData;
        if (bufferedMsgOffset == bufferedMsg.length) {
            bufferedMsg = null;
        }
        if (DEBUG)
            debugPrint("readFromBuffer len=" + len + ", read " + remainingData);
        return remainingData;
    }

    protected int writeBytes(byte[] b, int off, int len) throws IOException {
        if (DEBUG)
            debugPrint("writeBytes len=" + len + " can write " + (connectionOpen && oStreams > 0));
        if (!connectionOpen || oStreams == 0)
            throw new IOException();
        
        if (len == 0)
            return 0;
        
        LinkMessage lm = LinkMessage.newDataMessage(b, off, len);

        if (DEBUG)
            debugPrint("writeBytes: checking status of outbound connection");
        synchronized (sender) {
            if (sendStatus != null) {
                throw sendStatus;
            }
            
            if (DEBUG)
                debugPrint("writeBytes: sending message");
            sendQueue.addElement(lm);
        }
        
        if (DEBUG)
            debugPrint("writeBytes: wrote " + len + " bytes");
        return len;
    }

    public String getRequestedServerVersion() {
        return version;
    }

    public String getServerName() {
        return serverName;
    }

    private class Sender implements Runnable {

        public void run() {
            while (connectionOpen) {
                LinkMessage lm;
                synchronized (this) {

                    if (sendQueue.size() == 0) {
                        if (DEBUG)
                            debugPrint("Sender waiting for data");
                        try {
                            wait();
                        } catch (InterruptedException ex) {
                            ex.printStackTrace();
                        }
                    }

                    lm = (LinkMessage) sendQueue.firstElement();
                    sendQueue.removeElementAt(0);
                }

                try {
                    if (DEBUG)
                        debugPrint("Sender now will send message");
                    sendLink.send(lm);
                } catch (ClosedLinkException ex) {
                    // send link has been closed on receive side
                    sendStatus = new IOException("Outbound connection closed");
                } catch (InterruptedIOException ex) {
                    sendStatus = ex;
                } catch (IOException ex) {
                    sendStatus = ex;
                }
                if (DEBUG)
                    debugPrint("Sender send message with " + sendStatus);
                if (sendStatus != null) {
                    try {
                        sendStatus.printStackTrace();
                        closeOutputStream();
                    } catch (IOException ex1) {
                    }
                }
            }
        }
    }
}
