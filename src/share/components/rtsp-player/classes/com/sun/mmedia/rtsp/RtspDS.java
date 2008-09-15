/*
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

package com.sun.mmedia.rtsp;

import java.io.IOException;
import java.io.InputStream;
import javax.microedition.media.Player;
import javax.microedition.media.MediaException;
import javax.microedition.media.protocol.SourceStream;

import com.sun.mmedia.protocol.BasicDS;

public class RtspDS extends BasicDS
{
    private final static String userAgent = "User-Agent: MMAPI RTSP Client 1.0";

    private boolean        connected   = false;
    private RtspConnection connection  = null;
    private InputStream    inputStream = null;
    private RtspUrl        url         = null;

    public void setLocator( String ml ) throws MediaException {
        try {
            url = new RtspUrl( ml );
            super.setLocator( ml );
        } catch( IOException ioe ) {
            throw new MediaException( ioe.toString() );
        }
    }

    public synchronized void connect() throws IOException {
        if( !connected ) {
            try {
                connection = new RtspConnection( url );

                int sequenceNumber = 4268;

                String msg = "DESCRIBE rtsp://" + url.getHost() + "/" + url.getFile() +
                    " RTSP/1.0\r\n" + "CSeq: " + sequenceNumber + "\r\n" +
                    "Accept: application/sdp\r\n" + userAgent + "\r\n\r\n";

                connection.sendData( msg.getBytes() );

                // TODO: determine actual content type
                setContentType( "audio/x-wav" );

                connected = true;
            } catch( RuntimeException re ) {
                throw re;
            } catch( Exception ex ) {
                throw new IOException( "failed to connect to " +
                                locator + " : " + ex.getMessage() );
            }
        }
    }

    public synchronized void disconnect() {
        if( connected ) {
            connection = null;
            connected = false;
        }
    }

    public synchronized void start() throws IOException {
    }

    public synchronized void stop() throws IOException {
    }

    public synchronized SourceStream[] getStreams() {
        /*
        if( inputStream == null ) {
            return new SourceStream[] { null };
        }
        */
        return new SourceStream[] { null };
    }

    public synchronized long getDuration() {
        return Player.TIME_UNKNOWN;
    }
}
