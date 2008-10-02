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

public class RtspDS extends BasicDS
{
    private static final int RESPONSE_TIMEOUT = 5000;

    private static final int MIN_PORT = 1024;  // inclusive
    private static final int MAX_PORT = 65536; // exclusive

    private static Random rnd = new Random( System.currentTimeMillis() );

    private Object msgWaitEvent = new Object();
    private RtspIncomingMessage response = null;

    private RtspConnection connection  = null;
    private boolean        started     = false;
    private RtspUrl        url         = null;
    private int            seqNum      = 0;
    private String         sessionId   = null;
    private String         contentBase = null;
    private RtspRange      range       = null;

    private int sessionTimeout = 60; // in seconds, 60 is default according to the spec

    private RtspSS[] streams = null;

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

                if( !sendRequest( RtspOutgoingRequest.DESCRIBE( seqNum, url ) ) ) {
                    throw new IOException( "RTSP DESCRIBE request failed" );
                }

                contentBase = response.getContentBase();

                SdpSessionDescr sdp = response.getSdp();
                if( null == sdp ) throw new IOException( "no SDP data received" );

                SdpMediaAttr range_attr = sdp.getSessionAttribute( "range" );

                if( null != range_attr ) {
                    try {
                        range = new RtspRange( range_attr.getValue() );
                    } catch( NumberFormatException e ) {
                        range = null;
                    }
                }

                int num_tracks = sdp.getMediaDescriptionsCount();
                if( 0 == num_tracks ) num_tracks = 1;

                streams = new RtspSS[ num_tracks ];

                String mediaControl;
                RtspOutgoingRequest setup_request;

                // sessionId is null at this point
                for( int i = 0; i < num_tracks; i++ ) {

                    // TODO: allocPort() should allocate actual UDP port, not just number
                    int client_data_port = allocPort();

                    if( i < sdp.getMediaDescriptionsCount() ) {
                        mediaControl = sdp.getMediaDescription( i ).getMediaAttribute( "control" ).getValue();
                    } else {
                        mediaControl = null;
                    }

                    if( null != contentBase ) {
                        setup_request = RtspOutgoingRequest.SETUP( seqNum, contentBase, mediaControl,
                                                                   sessionId, client_data_port );
                    } else {
                        setup_request = RtspOutgoingRequest.SETUP( seqNum, url, sessionId, client_data_port );
                    }

                    if( !sendRequest( setup_request ) ) {
                        throw new IOException( "SETUP request failed" );
                    }

                    if( 0 == i ) {
                        sessionId = response.getSessionId();
                        Integer timeout = response.getSessionTimeout();
                        if( null != timeout ) {
                            sessionTimeout = timeout.intValue();
                        }
                    }

                    RtspTransportHeader th = response.getTransportHeader();

                    if( th.getClientDataPort() != client_data_port ) {
                        // TODO: returned value for client data port may be different.
                        // in this case an attempt should be made to re-allocate UDP
                        // port accordingly. Note that this attempt may fail.
                        client_data_port = th.getClientDataPort();
                    }

                    streams[ i ] = new RtspSS( mediaControl, client_data_port );
                }

                // TODO: determine actual content type
                setContentType( "audio/x-wav" );

                start();

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
        /*
        if( null != connection ) {
            try {
                sendRequest( RtspOutgoingRequest.TEARDOWN( seqNum, url, sessionId ) );
            } catch( InterruptedException e ) {
                Thread.currentThread().interrupt();
            } finally {
                connection.close();
                connection = null;
            }
        }
        */
    }

    public synchronized void start() throws IOException {
        if( null == connection ) throw new IllegalStateException( "RTSP: Not connected" );
        try {
            started = sendRequest( RtspOutgoingRequest.PLAY( seqNum, url, sessionId ) );
        } catch( InterruptedException e ) {
            throw new IOException( "start aborted: " + e.getMessage() );
        }
    }

    public synchronized void stop() throws IOException {
        if( null == connection || !started ) return;
        try {
            sendRequest( RtspOutgoingRequest.PAUSE( seqNum, url, sessionId ) );
        } catch( InterruptedException e ) {
            throw new IOException( "stop aborted: " + e.getMessage() );
        }
    }

    public synchronized SourceStream[] getStreams() {
        if( null == connection ) throw new IllegalStateException( "RTSP: Not connected" );
        return new SourceStream[] { null }; //streams;
    }

    public synchronized long getDuration() {
        if( null != range ) {
            float from = range.getFrom();
            float to = range.getTo();
            if( RtspRange.NOW != from && RtspRange.END != to ) {
                return (long)( ( to - from ) * 1000.0 );
            }
        }

        return Player.TIME_UNKNOWN;
    }

    public String getContentType() {
        if( null == connection ) throw new IllegalStateException( "RTSP: Not connected" );
        return contentType;
    }

    //=========================================================================

    private static int allocPort() {
        int retVal = nextPort;

        // TODO: check if these ports are actually available

        nextPort += 2;

        if( nextPort >= MAX_PORT ) {
            nextPort = MIN_PORT;
        }

        return retVal;
    }

    //=========================================================================

    /** 
     * This method is called by RtspConnection when message is received
     */
    protected void processIncomingMessage( byte[] bytes ) {
        synchronized( msgWaitEvent ) {
            RtspIncomingMessage msg;
            try {
                msg = new RtspIncomingMessage( bytes );
            } catch( Exception e ) {
                e.printStackTrace();
                msg = null;
            }
            Integer cseq = msg.getCSeq();
            // response may not have CSeq defined if status is not '200 OK'.
            if( null == cseq || cseq.intValue() == seqNum ) {
                response = msg;
                msgWaitEvent.notifyAll();
                seqNum++;
            }
        }
    }

    /**
     * blocks until response is received or timeout period expires
     */
    private boolean sendRequest( RtspOutgoingRequest request ) 
        throws InterruptedException, IOException {

        boolean ok = false;

        synchronized( msgWaitEvent ) {
            response = null;
            if( connection.sendData( request.getBytes() ) ) {
                try {
                    msgWaitEvent.wait( RESPONSE_TIMEOUT );
                    ok = ( null != response );
                } catch( InterruptedException e ) {
                    throw e;
                }
            }
        }

        if( ok && !response.getStatusCode().equals( "200" ) ) {
            throw new IOException( "RTSP error " + response.getStatusCode()
                                   + ": '" + response.getStatusText() + "'" );
        }

        return ok;
    }
}
