/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.io.j2me.serversocket;

import com.sun.midp.security.SecurityToken;

import java.io.IOException;

public class Socket extends com.sun.midp.io.j2me.serversocket.Socket {
    private int listeningPort = -1;

    /**
     * Opens a port to listen on.
     *
     * @param port       TCP to listen on
     *
     * @exception IOException  if some other kind of I/O error occurs
     */
    public void open(int port, SecurityToken token) throws IOException {
        super.open(port, token);
        listeningPort = getLocalPort();
    }

    /**
     * Returns a connection that represents a server side
     * socket connection.
     * <p>
     * Polling the native code is done here to allow for simple
     * asynchronous native code to be written. Not all implementations
     * work this way (they block in the native code) but the same
     * Java code works for both.
     *
     * @return     a socket to communicate with a client.
     *
     * @exception  IOException  if an I/O error occurs when creating the
     *                          input stream
     */
    synchronized protected com.sun.midp.io.j2me.socket.Protocol getProtocolClass()
        throws IOException {

        return new com.sun.kvem.io.j2me.socket.Protocol(listeningPort);
    }
}
