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
package com.sun.mmedia;

import javax.microedition.media.*;
import javax.microedition.media.control.*;
import com.sun.mmedia.DirectPlayer;
import com.sun.mmedia.protocol.CommonDS;
import com.sun.mmedia.rtsp.*;

/**
 *  Description of the Class
 *
 * @created    September 11, 2002
 */
public class RTSPPlayer extends com.sun.mmedia.BasicPlayer {
    private boolean started; // default is FALSE
    private RtspManager rtspManager; // default is NULL
    private DirectPlayer players[]; // default is NULL
    private int numberOfTracks; // default is 0

    // media time in milli-seconds
    private long mediaTime; // default is 0

    // start time in milli-seconds
    private long startTime; // default is 0

    // stop time in milli-seconds
    private long stopTime; // default is 0

    private RtspCtrl rtspControl; // default is NULL
    private boolean setup_ok; // default is FALSE
    private boolean data_received; // default is FALSE

    private final boolean RTSP_DEBUG = false;

    public RTSPPlayer() {
        rtspControl = new RtspCtrl(this);
    }

    /**
     * Realizes the RTSP Player.
     *
     * @see                      Player#realize()
     * @exception MediaException Thrown if the <code>Player</code> cannot
     *                           be realized.
     */
    protected final void doRealize() throws MediaException {
        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doRealize");

        rtspControl.setStatus( "Negotiating");

        String locator = source.getLocator();

        rtspManager = new RtspManager(locator,RTSP_DEBUG);

        setup_ok = rtspManager.createConnection();

        if (setup_ok) {
            // setup the RTSP session
            setup_ok = rtspManager.rtspSetup();

            if (setup_ok) {
                numberOfTracks = rtspManager.getNumberOfTracks();

                int server_ports[] = rtspManager.getServerPorts();
                int client_ports[] = rtspManager.getClientPorts();

                players = new DirectPlayer[numberOfTracks];

                // start the RTSP server until the internal RTP players
                // can be realized.
                rtspManager.setStartPos( 0);

                CommonDS ds;

                for (int i = 0; i < numberOfTracks; i++) {
                    // setup the RTP players
                    String url = "rtp://" 
                                 + rtspManager.getServerAddress() 
                                 + ":" + client_ports[i];

                    try {
                        players[i] = new DirectPlayer(); //Manager.createPlayer(url);
                        ds = new CommonDS();
                        ds.setLocator(url);
                        ds.setContentType("content.rtp");
                        players[i].setSource(ds);
                    } catch (Exception e) {
                        throw new MediaException( e.getMessage());
                    }

                    players[i].realize();
                }
            } else {
                rtspControl.setStatus( rtspManager.getProcessError());
            }
        } else {
            rtspControl.setStatus( "Server not found [" + rtspManager.getServerAddress() + "]");
        }

        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doRealize done");
    }

    /**
     *  Prefetching (not implemented).
     *
     *  This may be an option for a future implementation. By
     *  prefetching, the startup-time as experienced by the user
     *  could be minimized. For VOD a positive side effect would
     *  be that the first frame of the video would be visible.
     *  Right now this is not guaranteed since the player may
     *  need the first frame for initialization purposes.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected final void doPrefetch() throws MediaException {
        // rtspControl.setStatus( "Buffering...");
    }


    /**
     * Retrieves the duration of the media presentation.
     *
     * The duration is communicated via the SDP protocol
     * as part of the DESCRIBE response message and stored
     * in RtspManager.
     *
     * @return    Duration in micro-seconds
     */
    public final long doGetDuration() {
        return rtspManager.getDuration();
    }


    /**
     * Starts the RTSP Player
     *
     * @return    true, if the RTSP Player was started successfully, false otherwise.
     */
    protected final boolean doStart() {
        if (RTSP_DEBUG) System.out.println("RTSP player: doStart()");
        rtspControl.setStatus( "Sun RTSP Streaming");

        if (setup_ok && !started) {
            // start the RTP players
            for (int i = 0; i < numberOfTracks; i++) {
                try {
                    players[i].start();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }

            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
            }

            // start the RTSP session
            rtspManager.setStartPos( mediaTime * 1000);
            started = rtspManager.rtspStart();
            
            if( !started) {
                rtspControl.setStatus( rtspManager.getProcessError());
            } else {
                startTime += (System.currentTimeMillis() - stopTime);
                stopTime = 0;
            }
        }

        if (RTSP_DEBUG) System.out.println("RTSP player: doStart() done");
        return started;
    }


    /**
     *  Stopping the RTSP Player.
     */
    protected final void doStop() {
        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doStop");
        // stop the RTSP session
        rtspManager.rtspStop();

        started = false;

        stopTime = System.currentTimeMillis();

        // stop the RTP players
        for (int i = 0; i < numberOfTracks; i++) {
            try {
                players[i].stop();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doStop done");
    }


    /**
     * Deallocates the RTSP Player.
     */
    protected final void doDeallocate() {
        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doDeallocate");

        // teardown of the RTSP session
        rtspManager.rtspTeardown();
        
        try {
            // stopping and deallocating all internal RTP players
            for (int i = 0; i < numberOfTracks; i++) {
                players[i].stop();
                players[i].deallocate();
            }
        } catch (MediaException e) {
            if (RTSP_DEBUG) System.out.println("Error closing RTP players");
        }

        rtspManager.closeConnection();

        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doDeallocate done");
    }


    /**
     *  Closes the RTSP Player.
     */
    protected final void doClose() {
        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doClose");

        // closing all internal RTP players
        for (int i = 0; i < numberOfTracks; i++) {
            players[i].close();
        }

        if (RTSP_DEBUG) System.out.println("[RTSPPlayer] doClose done");
    }


    /**
     * Sets the media time of the RTSP Player.
     *
     * @param  now the new media time.
     * @exception MediaException thrown if setMediaTime fails.
     * @return the new media time in microseconds.
     */
    protected final long doSetMediaTime(long now) throws MediaException {
        if (started) {	    
            doStop();
        }

        mediaTime = (now / 1000);
        startTime = System.currentTimeMillis() - mediaTime;
    
        doStart();

        return now;
    }


    /**
     * Retrieves the current media time.
     *
     * @return    the media time in microseconds.
     */
    protected final long doGetMediaTime() {
        if (started) {
            mediaTime = System.currentTimeMillis() - startTime;

            if( (mediaTime * 1000) >= rtspManager.getDuration()) {
                started = false;
                sendEvent(PlayerListener.END_OF_MEDIA, new Long(mediaTime*1000));	    
            }
        }

        return mediaTime * 1000;
    }

    /**
     *  Returns the RTSP Control object.
     *
     * @param  type  Description of the Parameter
     * @return       Description of the Return Value
     */

    public RtspCtrl getRtspControl() {
        return rtspControl;
    }

    /**
     *  Description of the Method
     *
     * @param  type  Description of the Parameter
     * @return       Description of the Return Value
     */
    protected final Control doGetControl(String type) {
        Control control =  null;

        if (/*(getState() != UNREALIZED) && */
            type.startsWith(BasicPlayer.pkgName)) {
            
            type = type.substring(BasicPlayer.pkgName.length());
            
            if (type.equals(BasicPlayer.vocName)) {
                // Volume Control
                for (int i = 0; i < numberOfTracks; i++) {
                    control = players[i].getControl( type);

                    if( control != null) {
                        break;
                    }
                }
            } else if (type.equals(BasicPlayer.rtspName)) {
                // RTSP Control
                control = rtspControl;
            } else if(type.equals(BasicPlayer.vicName)) {
                // Video Control
                for (int i = 0; i < numberOfTracks; i++) {
                    control = players[i].getControl(type);

                    if( control != null) {
                        if (RTSP_DEBUG) System.out.println( "[RTSPPlayer] got video control");
                        break;
                    }
                }
            }
        }
        return control;
    }
}


