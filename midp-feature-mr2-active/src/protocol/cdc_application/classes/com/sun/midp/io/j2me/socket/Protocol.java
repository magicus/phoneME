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
import java.net.*;
import javax.microedition.io.*;

public class Protocol extends com.sun.cdc.io.j2me.socket.Protocol {
    
    
    /** Number of input streams that were opened. */
    protected int iStreams = 0;
    /**
     * Maximum number of open input streams. Set this
     * to zero to prevent openInputStream from giving out a stream in
     * write-only mode.
     */
    protected int maxIStreams = 1;

    /*
     * Open the input stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened.
     */
    public InputStream openInputStream() throws IOException {
        if (maxIStreams == 0) {
            throw new IOException("no more input streams available");
        }
        InputStream i = super.openInputStream();
        maxIStreams--;
        iStreams++;
        return i;
    }
    
    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    /* This class overrides the setSocketOption() to allow 0 as a valid value for sendBufferSize
     * and receiveBufferSize. The underlying CDC networking layer considers 0 as illegal 
     * value and throws IAE which causes the TCK test to fail.
     */
    public void setSocketOption(byte option,  int value) throws IllegalArgumentException, IOException {
        if (option == SocketConnection.SNDBUF || option == SocketConnection.RCVBUF ) {
            if (value == 0) {
                value = 1;
                super.setSocketOption(option, value);
            }
        }            
        super.setSocketOption(option, value);
    }

    /*
     * throws SecurityException if MIDP permission check fails 
     */
    protected void checkMIDPPermission(String host, int port) {
        //The actual MIDP permission check happens here
    }

}
