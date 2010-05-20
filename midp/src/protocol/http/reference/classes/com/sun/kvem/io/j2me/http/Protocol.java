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

package com.sun.kvem.io.j2me.http;

import com.sun.kvem.netmon.HttpAgent;
import com.sun.kvem.netmon.StreamConnectionStealer;
import com.sun.midp.io.j2me.http.StreamConnectionElement;

import javax.microedition.io.StreamConnection;
import java.io.IOException;

/**
 * The class subclass the http protocol of the midp. By overriding
 * the connect method it wrapps the connection stream returnd with
 * a StreamConnectionStealer that "steals" the data along the way and
 * send it to the HttpAgent and from there to the network monitor gui.
 *
 *@author ah123546
 *@created December 25, 2001
 *@version 
 * @see com.sun.kem.netmon.StreamConnectionStealer
 */
public class Protocol extends com.sun.midp.io.j2me.http.Protocol {


    protected StreamConnection connect() throws IOException {
    StreamConnection con = super.connect();
    String theUrl;
    if (url.port != -1) {
        theUrl = protocol + "://" + url.host + ":" + url.port;
    } else {
        theUrl = protocol + "://" + url.host;
    }
    
    StreamConnectionStealer newCon;
    if ((con instanceof StreamConnectionElement) &&
            ((StreamConnectionElement)con).getBaseConnection() instanceof
                    StreamConnectionStealer) {
        ((StreamConnectionStealer)
                    ((StreamConnectionElement)con).getBaseConnection()).reset(
                            theUrl);
        return con;
    } else if (con instanceof StreamConnectionStealer) {
        newCon = (StreamConnectionStealer)con;
        newCon.resetURL(theUrl);
    } else {
        newCon = new StreamConnectionStealer(theUrl, con, 
              HttpAgent.instance());
    }
    /*
     * Because StreamConnection.open*Stream cannot be called twice
     * the HTTP connect method may have already open the streams
     * to connect to the proxy and saved them in the field variables
     * already.
     */
    if (streamOutput != null) {
        newCon.setStreams(streamInput,streamOutput);
        streamInput = newCon.openDataInputStream();
        streamOutput = newCon.openDataOutputStream();
    }

    return newCon;
    }
    
    protected void disconnect() throws IOException {
        super.disconnect();
        if (connReused) {
            /*
             * connection was not closed and went to connection pool,
             * but we need the stealer to be disconnected otherwise
             * the connection will be still active in network monitor.
             */
            StreamConnection sc = getStreamConnection();
            if (sc instanceof StreamConnectionElement) {
                sc = ((StreamConnectionElement) sc).getBaseConnection();
            }
            if (sc instanceof StreamConnectionStealer) {
                ((StreamConnectionStealer) sc).disconnect();
            }
        }
    }
    
    
    
    /**
     * I override this method because we need to know when a new message
     * starts. Thats true for a new connection and for a reused connection.
     */
    protected void streamConnect() throws IOException {
    super.streamConnect();
    StreamConnection con = getStreamConnection();

    if(con instanceof StreamConnectionElement){
        con = ((StreamConnectionElement)con).getBaseConnection();
    }
    StreamConnectionStealer stealCon = (StreamConnectionStealer)con;
    stealCon.newMsg();
    }
}

