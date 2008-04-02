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


package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.io.*;
import java.net.*;

/**
 * This class represents a socket transport for JDWP. Actually
 * is used as proxy that communicates with
 * <code>SocketTransportImpl</code> that works in another thread and
 * performs all the job for communicating with VM being debugged via
 * JDWP.
 *
 * @see jdwp.SocketTransportImpl
 * @see jdwp.Transport
 */
class SocketTransport extends Transport {

    /**
     * A <code>SocketTransportImpl</code> object that performs all
     * the communications.
     */
    private SocketTransportImpl socketImpl;

    /**
     * Connects to the specified port number on the named host. For this
     * task this mehtod initializes a <code>SocketTransportImpl</code> object
     * that works in another thread and manipulates it.
     *
     * @param ServerName a host where VM being debugged is running
     * @param PortNumber a TCP/IP port number to communicate with VM being
     * debugged
     */
    public void attachToServer(String ServerName, int PortNumber)
    throws UnknownHostException, IOException {
        socketImpl = new SocketTransportImpl();
        socketImpl.initAsClient(ServerName, PortNumber, Replies);
        synchronized(socketImpl){
            socketImpl.start();
            try{
                socketImpl.wait();
            }catch(InterruptedException e){
            }
        }
        if(!socketImpl.isStarted()){
            throw new IOException("Connection is not established");
        }
        //System.out.println("Attached");
    }

    /**
     * Closes socket and streams.
     */
    public void done() throws IOException {
        if(socketImpl != null){
            socketImpl.done();
            socketImpl = null;
        }
    }

    /**
     * Writes the specified byte to the socket.
     */
    public void write(int b) throws IOException {
        socketImpl.write(b);
    }
    
    /**
     * Stores the received JDWP replies to the <code>Replies</code>
     * vector.
     *
     * @see jdwp.Transport#Replies
     */
    public void receive() throws IOException {
        socketImpl.receive();
    }
    
    /**
     * Returns a number of bytes that are available for reading.
     *
     * @return a number of bytes that are available for reading
     */
    public int available()
    throws IOException{
        return socketImpl.availableInPrivateBuffer();
    }
    
    /**
     * Reads the next byte from the buffer. This byte is already read from
     * the socket and stored in some buffer.
     *
     * @return a next byte from the buffer
     *
     * @see jdwp.SocketTransportImpl#readNextFromPrivateBuffer()
     */
    public int read()
    throws IOException{
        return socketImpl.readNextFromPrivateBuffer();
    }
    
    /**
     * Initialtes handshake procedure. This procedure is performed
     * immediately after establishing TCP/IP connection and consist on
     * sending and receiving "JDWP-Handshake" string.
     */
    public void Handshake() throws IOException{
        socketImpl.startHandShake(new String("JDWP-Handshake").getBytes().length);
        super.Handshake();
    }
}
