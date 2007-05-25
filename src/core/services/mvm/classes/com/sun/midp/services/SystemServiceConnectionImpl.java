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

import com.sun.midp.links.*;
import java.io.*;
import java.lang.*;

/**
 * Implements SystemServiceConnection interface.
 */
final class SystemServiceConnectionImpl 
    implements SystemServiceConnection {
    
    /** Pair of Links for data exchange between service and client */
    private SystemServiceConnectionLinks connectionLinks = null;

    /** Listener to be notified about incoming messages */
    private SystemServiceConnectionListener listener = null;

    /**
     * Listener thread class.
     */
    class ListenerThread extends Thread {
        public void run() {
            try {
                while (true) {
                    SystemServiceMessage msg = receive();
                    listener.onMessage(msg);
                }
            } catch ( SystemServiceConnectionClosedException e) {
                listener.onConnectionClosed();
            }
        }
    }

    /** 
     * Constructor.
     *
     * @param connectionLinks pair of Links for data exchange between 
     * service and client
     */
    SystemServiceConnectionImpl(SystemServiceConnectionLinks connectionLinks) {
        this.connectionLinks = connectionLinks;
    }

    /**
     * Receives a message. Blocks until there is a message to receive.
     * On client side, it receives message from service. On service side,
     * it receives message from client.
     *
     * @return SystemServiceMessage object representing received message
     */   
    public SystemServiceMessage receive() 
        throws SystemServiceConnectionClosedException {

        try { 
            Link receiveLink = connectionLinks.getReceiveLink();
            LinkMessage msg = receiveLink.receive(); 
            byte[] data = msg.extractData();

            return new SystemServiceReadMessage(data);
        } catch (ClosedLinkException e) {
            throw new SystemServiceConnectionClosedException();
        } catch (InterruptedIOException e) {
            /*
             * In normal case, this means that client Isolate has exited.
             * Close our side of connection, and throw ConnectionClosed
             * exception.
             */
            connectionLinks.close();
            throw new SystemServiceConnectionClosedException();
        } catch (IOException e) {
            /**
             * Shouldn't happen. So, panic, and close connection.
             */
            connectionLinks.close();
            throw new SystemServiceConnectionClosedException();
        }
    }

    /**
     * Sends a message. Blocks until message is received. On client side,
     * it sends message to service. On service side, it sends message 
     * to client.
     *
     * @param msg message to send
     */   
    public void send(SystemServiceMessage msg) 
        throws SystemServiceConnectionClosedException {

        try {
            SystemServiceWriteMessage writeMsg = (SystemServiceWriteMessage)msg;
            byte[] data = writeMsg.getData();

            LinkMessage linkMsg = LinkMessage.newDataMessage(data);
            Link sendLink = connectionLinks.getSendLink();
            sendLink.send(linkMsg);
        } catch (ClosedLinkException e) {
            throw new SystemServiceConnectionClosedException();
        } catch (InterruptedIOException e) {
            /*
             * In normal case, this means that client Isolate has exited.
             * Close our side of connection, and throw ConnectionClosed
             * exception.
             */            
            connectionLinks.close();
            throw new SystemServiceConnectionClosedException();
        } catch (IOException e) {
            /**
             * Shouldn't happen. So, panic, and close connection.
             */            
            connectionLinks.close();
            throw new SystemServiceConnectionClosedException();
        }        
    }

    /**
     * Sets a listener which will be notified when message has arrived.
     *
     * @param listener listener to notify. if null, removes current
     * listener.
     */
    public void setConnectionListener(SystemServiceConnectionListener l) {
        if (listener == null) {
            listener = l;
            new ListenerThread().start();
        } else {
            throw new IllegalStateException();
        }
    }
}
