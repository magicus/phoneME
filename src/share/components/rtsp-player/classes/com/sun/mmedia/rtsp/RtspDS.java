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
import java.util.Random;
import javax.microedition.media.Player;
import javax.microedition.media.MediaException;
import javax.microedition.media.protocol.SourceStream;

import com.sun.mmedia.protocol.BasicDS;

import com.sun.mmedia.rtsp.protocol.*;

public class RtspDS extends BasicDS
{
    private Random rnd = new Random( System.currentTimeMillis() );

    private RtspConnection connection  = null;
    private InputStream    inputStream = null;
    private RtspUrl        url         = null;
    private int            seqNum      = 0;

    public RtspUrl getUrl() {
        return url;
    }

    public void setLocator( String ml ) throws MediaException {
        try {
            url = new RtspUrl( ml );
            super.setLocator( ml );
        } catch( IOException ioe ) {
            throw new MediaException( ioe.toString() );
        }
    }

    public synchronized void connect() throws IOException {
        if( null == connection ) {
            try {
                connection = new RtspConnection( this );

                seqNum = rnd.nextInt();

                sendRequest( RtspOutgoingRequest.createDescribe( seqNum, url ) );
                waitForResponse();

                sendRequest( RtspOutgoingRequest.createSetup( seqNum, url, allocPort() ) );
                waitForResponse();

                // TODO: determine actual content type
                setContentType( "audio/x-wav" );
            } catch( RuntimeException re ) {
                connection = null;
                throw re;
            } catch( Exception ex ) {
                connection = null;
                throw new IOException( "failed to connect to " +
                                locator + " : " + ex.getMessage() );
            }
        }
    }

    public synchronized void disconnect() {
        if( null != connection ) {
            connection.close();
            connection = null;
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

    //=========================================================================

    /**
     * Highest and lowest port values allowed for RTP port pairs.
     */
    private static final int MIN_PORT = 1024;  // inclusive
    private static final int MAX_PORT = 65536; // exclusive
    private static int nextPort = MIN_PORT;

    private static int allocPort() {
        int retVal = nextPort;

        // TODO: check if these ports are actually available

        if( ++nextPort == MAX_PORT ) nextPort = MIN_PORT;
        return retVal;
    }

    //=========================================================================

    private Object msgWaitEvent = new Object();
    private boolean responseReceived = false;

    /** 
     * This method is called by RtspConnection when message is received
     */
    protected void processIncomingMessage( byte[] msg ) {
        System.out.println( "--------- incoming ---------\n"
                            + new String( msg ) +
                            "----------------------------\n" );

        synchronized( msgWaitEvent ) {
            responseReceived = true;
            msgWaitEvent.notifyAll();
            seqNum++;
        }
    }

    private boolean sendRequest( RtspOutgoingRequest request ) {
        synchronized( msgWaitEvent ) {
            if( !responseReceived ) {
                // prevent sending next request while sill waiting for reply
                return false;
            } else {
                responseReceived = false;
                return connection.sendData( request.getBytes() );
            }
        }
    }

    private boolean waitForResponse() {
        boolean ok;
        synchronized( msgWaitEvent ) {
            try {
                msgWaitEvent.wait( 5000 );
                ok = responseReceived;
            } catch( InterruptedException e ) {
                ok = false;
            }
        }
        return ok;
    }
}
