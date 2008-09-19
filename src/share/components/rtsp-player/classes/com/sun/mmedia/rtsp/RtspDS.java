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

import com.sun.mmedia.sdp.*;
import com.sun.mmedia.rtsp.protocol.*;

public class RtspDS extends BasicDS
{
    private static final int MIN_PORT = 1024;  // inclusive
    private static final int MAX_PORT = 65536; // exclusive

    private Random rnd = new Random( System.currentTimeMillis() );

    private Object msgWaitEvent = new Object();
    private RtspIncomingMessage response = null;

    private RtspConnection connection = null;
    private InputStream    inputStream = null;
    private RtspUrl        url         = null;
    private int            seqNum      = 0;
    private String         sessionId   = null;

    private static int nextPort = MIN_PORT;

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

                SdpParser sdp = response.getSdp();

                if( null != sdp ) {
                    int  start_time = 0; // in milliseconds
                    int  end_time   = 0; // in milliseconds
                    long duration   = 0; // in milliseconds

                    MediaAttribute range = sdp.getSessionAttribute( "range" );

                    if( null != range ) {
                        String value = range.getValue();
                        if( value.startsWith( "npt" ) ) {
                            try {
                                int start = value.indexOf( '=' ) + 1;
                                int end = value.indexOf( '-' );
                                String startTime = value.substring( start, end ).trim();
                                String endTime = value.substring( end + 1 ).trim();

                                int dotIndex = endTime.indexOf( "." );

                                if( dotIndex < 0 ) {
                                    end_time = Integer.parseInt( endTime ) * 1000;
                                } else {
                                    String p1 = "0" + endTime.substring( 0, dotIndex );
                                    String p2 = endTime.substring( dotIndex + 1 );
                                    end_time = Integer.parseInt( p1 ) * 1000;
                                    end_time += Integer.parseInt( ( p2 + "000" ).substring( 0, 3 ) );
                                }

                                duration = (long)( ( end_time - start_time ) * 1000 );
                            } catch( NumberFormatException e ) {
                                duration = 0;
                            }
                        }
                        System.out.println( "## range:" + start_time + "-" + end_time );
                    }
                }

                sendRequest( RtspOutgoingRequest.createSetup( seqNum, url, allocPort() ) );

                sessionId = "4268";

                sendRequest( RtspOutgoingRequest.createPlay( seqNum, url, sessionId ) );

                // TODO: determine actual content type
                setContentType( "audio/x-wav" );

            } catch( InterruptedException e ) {
                connection = null;
                Thread.currentThread().interrupt();
                throw new IOException( "connect to " +
                                locator + " aborted: " + e.getMessage() );
            } catch( IOException e ) {
                connection = null;
                throw new IOException( "failed to connect to " +
                                locator + " : " + e.getMessage() );
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

    private static int allocPort() {
        int retVal = nextPort;

        // TODO: check if these ports are actually available

        if( ++nextPort == MAX_PORT ) nextPort = MIN_PORT;
        return retVal;
    }

    //=========================================================================

    /** 
     * This method is called by RtspConnection when message is received
     */
    protected void processIncomingMessage( byte[] bytes ) {
        synchronized( msgWaitEvent ) {
            response = new RtspIncomingMessage( bytes );
            msgWaitEvent.notifyAll();
            seqNum++;
        }
    }

    /**
     * blocks until response is received or timeout period expires
     */
    private boolean sendRequest( RtspOutgoingRequest request ) 
        throws InterruptedException {

        boolean ok = false;
        synchronized( msgWaitEvent ) {
            response = null;
            if( connection.sendData( request.getBytes() ) ) {
                try {
                    msgWaitEvent.wait( 5000 );
                    ok = ( null != response );
                } catch( InterruptedException e ) {
                    throw e;
                }
            }
        }
        return ok;
    }
}
