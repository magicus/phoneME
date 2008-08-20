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
import com.sun.mmedia.RTPPlayer;
import com.sun.mmedia.protocol.CommonDS;
import com.sun.mmedia.rtsp.*;
import java.util.*;

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

    // media time -- time from beginning of media. When player
    //               is stopped/paused, mt is also stopped/paused.
    // IMPL_NOTE: 1) current implementation uses system timer
    //            to measure media time. This is not very good.
    //            2) mediaTime is updated when doGetMediaTime()
    //               or doSetMediaTime() is called.
    //
    private long mediaTime;   // microseconds, default is 0

    // hypotetic moment in system time when mediaTime was 0.
    private long startedTime; // microseconds, default is 0

    private RtspCtrl rtspControl; // default is NULL
    private boolean setup_ok; // default is FALSE
    private boolean data_received; // default is FALSE

    private final boolean RTSP_DEBUG = false;

    public RTSPPlayer() {
        rtspControl = new RtspCtrl(this);
    }

    public String getContentType()
    {
        chkClosed(true);

        // IMPL_NOTE:
        // The following is a temporary solution. This will work fine
        // as long as our RTSP/RTP implementation supports only one
        // payload type, namely audio/mp3. When other content types
        // are enabled, either this method should return "application/sdp",
        // or we should get actual content type from javacall after player
        // is realized and first packets are receievd.
        return "audio/mp3";
    }

    public void sendEvent(String evt, Object evtData)
    {
        // IMPL_NOTE:
        // In current version media time is measured in 
        // Java using system timer. Consequently, EOM
        // event arriving from javacall doesn't have correct
        // media time set, so we patch event data with correct 
        // value here

        if ( evt.equals( PlayerListener.END_OF_MEDIA ))
            super.sendEvent(evt, new Long(doGetMediaTime()));
        else
            super.sendEvent(evt, evtData);
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
                        players[i] = new RTPPlayer( this );
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

            // start the RTSP session
            rtspManager.setStartPos( mediaTime );
            started = rtspManager.rtspStart();
            
            if( !started) {
                rtspControl.setStatus( rtspManager.getProcessError());
            } else {
                startedTime = System.currentTimeMillis() * 1000 - mediaTime;
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
        boolean must_restart = started;

        if(started) doStop();

        mediaTime = now;
    
        if(must_restart) doStart();

        return now;
    }


    /**
     * Retrieves the current media time.
     *
     * @return    the media time in microseconds.
     */
    protected final long doGetMediaTime() {
        if (started) {
            mediaTime = System.currentTimeMillis() * 1000 - startedTime;
        }

        return mediaTime;
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

        if (type.startsWith(BasicPlayer.pkgName))
        {
            String type_name = type.substring(BasicPlayer.pkgName.length());

            if (type_name.equals(BasicPlayer.vocName) || 
                type_name.equals(BasicPlayer.vicName))
            {
                // retrieve Volume and Video Controls from child [RTP] players
                for (int i = 0; i < numberOfTracks; i++)
                {
                    control = players[i].getControl(type_name);

                    if (control != null) break;
                }
            }
            else if (type_name.equals(BasicPlayer.rtspName))
            {
                // RTSP Control
                control = rtspControl;
            }
            else if (type_name.equals(BasicPlayer.stcName))
            {
                control = this;
            }
        }
        return control;
    }

    // StopTimeControl implementation.

    private Timer stopTimer;

    protected void doSetStopTime(long time)
    {
        if (time == StopTimeControl.RESET && stopTimer != null)
        {
            stopTimer.cancel();
            stopTimer = null;
        }
        else if (state == STARTED)
        {
            long currentTime = doGetMediaTime();
            long duration = doGetDuration();
            if (currentTime != TIME_UNKNOWN && time >= currentTime)
            {
                if (stopTimer != null) stopTimer.cancel();
                stopTimer = new Timer();
                long scheduleTime = ( time - currentTime ) / 1000;
                if (scheduleTime <= 0) scheduleTime = 1;
                stopTimer.schedule(new StopTimeCtrlTask(), scheduleTime);
            }
        }
    }

    protected void doPostStart()
    {
        if (stopTime != StopTimeControl.RESET)
        {
            if (stopTimer != null) stopTimer.cancel();
            long currentTime = doGetMediaTime();
            long scheduleTime = ( stopTime - currentTime ) / 1000;
            if (scheduleTime <= 0) scheduleTime = 1;
            stopTimer = new Timer();
            stopTimer.schedule(new StopTimeCtrlTask(), scheduleTime);
        }
    }

    protected void doPreStop()
    {
        if (stopTimer != null)
        {
            stopTimer.cancel();
            stopTimer = null;
        }
    }

    class StopTimeCtrlTask extends TimerTask
    {
        public void run()
        {
            synchronized (RTSPPlayer.this)
            {
                long mt = doGetMediaTime();
                long dur = doGetDuration();

                while (mt < stopTime && mt < dur)
                {
                    try
                    {
                        java.lang.Thread.sleep(10);
                    }
                    catch (InterruptedException e)
                    {
                        // just skip it
                    }

                    mt = doGetMediaTime();
                }
                doPreStop();
                doStop();
            }
            satev();
        }
    }
}


