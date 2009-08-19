/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

import java.util.logging.Level;
import java.util.logging.Logger;
import  javax.microedition.media.*;
import  javax.microedition.media.control.*;
import  javax.microedition.media.protocol.SourceStream;
import  javax.microedition.media.protocol.DataSource;
import  java.util.Enumeration;
import  java.util.Hashtable;
import  java.util.Vector;
import  com.sun.j2me.app.AppPackage;
import  com.sun.j2me.app.AppIsolate;
import  com.sun.j2me.log.Logging;
import  com.sun.j2me.log.LogChannels;

import java.io.IOException;

public final class HighLevelPlayer implements Player, TimeBase, StopTimeControl {

    /** Unknown media format */
    static final String MEDIA_FORMAT_UNKNOWN = "UNKNOWN";
    /** Unsupported media format */
    static final String MEDIA_FORMAT_UNSUPPORTED = "UNSUPPORTED";
    /** Device Tone format */
    static final String MEDIA_FORMAT_DEVICE_TONE = "DEVICE_TONE";
    /** Device MIDI format */
    static final String MEDIA_FORMAT_DEVICE_MIDI = "DEVICE_MIDI";
    /** Audio Capture format*/
    static final String MEDIA_FORMAT_CAPTURE_AUDIO = "CAPTURE_AUDIO";
    /** Video Capture format*/
    static final String MEDIA_FORMAT_CAPTURE_VIDEO = "CAPTURE_VIDEO";
    /** Radio Capture format*/
    static final String MEDIA_FORMAT_CAPTURE_RADIO = "CAPTURE_RADIO";
    /** Tone format */
    static final String MEDIA_FORMAT_TONE = "TONE";

    /**
     * Control package name
     */
    final static String pkgName = "javax.microedition.media.control.";

    /**
     * Centralized control management with string constants for each
     * implemented Control.
     * <p>
     * For adding a new control interfaces, follow the following steps:
     * <ol>
     *  <li>Add new control name here. If it is not in the package
     *     javax.microedition.media.control, then the full package
     *     name of the control must be used.
     *  <li>Add the control's name field to the allJsr135Ctrls array (see below)
     * </ol>
     * <p>
     * Size note: it would be space saving to declare the array allJsr135Ctrls
     * with the strings directly and not define the strings constants here.
     * However, it is more convenient for descendants to use
     * these constants, instead of e.g.
     * <code>&nbsp;&nbsp;&nbsp;allJsr135Ctrls[4]; // RateControl</code>
     *
     * @see    #getControls()
     * @see    #doGetNewControl(String)
     * @see    #allJsr135Ctrls
     */

    final static String fpcName = "FramePositioningControl";
    /**
     *  Description of the Field
     */
    final static String mdcName = "MetaDataControl";
    /**
     *  Description of the Field
     */
    final static String micName = "MIDIControl";
    /**
     *  Description of the Field
     */
    final static String picName = "PitchControl";
    /**
     *  Description of the Field
     */
    final static String racName = "RateControl";
    /**
     *  Description of the Field
     */
    final static String recName = "RecordControl";
    /**
     *  Description of the Field
     */
    final static String stcName = "StopTimeControl";
    /**
     *  Description of the Field
     */
    final static String tecName = "TempoControl";
    /**
     *  Description of the Field
     */
    final static String guiName = "GUIControl";
    /**
     *  Description of the Field
     */
    final static String vicName = "VideoControl";
    /**
     *  Description of the Field
     */
    final static String rtspName = "RtspControl";

    /**
     *  Description of the Field
     */
    final static String tocName = "ToneControl";
    /**
     *  Description of the Field
     */
    final static String dtocName = "com.sun.mmedia.control.DualToneControl";
    /**
     *  Description of the Field
     */
    final static String vocName = "VolumeControl";

    /**
     * An array containing all available JSR-135 controls in possible Players
     */
    private final static String[] allJsr135Ctrls = {
        fpcName,
        mdcName,
        micName,
        picName,
        racName,
        recName,
        stcName,
        tecName,
        guiName,
        vicName,
        rtspName,
        tocName,
        dtocName,
        vocName,
        };

    /**
     * An array containing all available controls
     * in possible Players, including JSR-234 controls
     * if available.
     * DO NOT USE DIRECTLY! Use getPossibleControlNames() instead.
     */
    private static Vector possibleControlNames = null;


    public PlayerStateSubscriber state_subscriber = null;

    private LowLevelPlayer lowLevelPlayer;
    
    DataSource  source;
    SourceStream stream;

    private String  mediaFormat = null;
    boolean handledByDevice = false;
    private boolean handledByJava = false;
    private int hNative = 0;      // handle of native API library


    /**
     * the loopCount of this player
     */
    int loopCountSet = 1, loopCount;

    private Vector listeners = new Vector(2);

    /**
     * hastable to map playerID to instances
     */
    private static Hashtable mplayers = new Hashtable(4);
    /**
     * table of player states
     */
    private static Hashtable pstates = new Hashtable();
    /**
     * table of media times
     */
    private static Hashtable mtimes = new Hashtable();

    /**
     * VM paused?
     */
    private static boolean vmPaused = false;

    /**
     * global player id
     */
    private static int pcount = -1;
    /**
     * player ID of this player
     */
    protected int pID = 0;
    /**
     * lock object
     */
    final private static Object idLock = new Object();
    
    /**
     * The task that is used to send the System Volume Changed event.
     */
    private static Runnable changeSystemVolumeTask = null;
    
    /**
     * System Volume level. This value will be set for all players from this VM.
     */
    private static int systemVolume = 100;

    // Init native library
    private native int nInit(int appId, int pID, String URI)
                                            throws MediaException, IOException;
    // Terminate native library
    private native int nTerm(int handle);
    // Get Media Format
    private native String nGetMediaFormat(int handle);
    // Need media Download in Java side?
    private native boolean nIsHandledByDevice(int handle);

    // Realize native player
    private native void nRealize(int handle, String mime) throws
            MediaException;

    private static String PL_ERR_SH = "Cannot create a Player: ";
    
    synchronized void abort( String msg )
    {
        if( CLOSED != getState() )
        {
            close();
            sendEvent(PlayerListener.ERROR, msg );
        }
    }

    private void setHandledByJava()
    {
        handledByJava = true;
        if( 0 != hNative )
        {
            nTerm( hNative );
            hNative = 0;
        }
    }

    /**
     * Constructor 
     */
    public HighLevelPlayer(DataSource source) throws MediaException, IOException {
        // Get current application ID to support MVM
        int appId = AppIsolate.getIsolateId();

        synchronized (idLock) {
            pcount = (pcount + 1) % 32767;
            pID = pcount;
        }

        locator = source.getLocator();
        hNative = nInit(appId, pID, locator);

        mplayers.put(new Integer(pID), this);

        mediaFormat     = nGetMediaFormat(hNative);

        if( mediaFormat.equals( MEDIA_FORMAT_UNSUPPORTED ) ) {
            /* verify if handled by Java */
            mediaFormat = Configuration.getConfiguration().ext2Format(source.getLocator());
            if( mediaFormat == null || mediaFormat.equals( MEDIA_FORMAT_UNSUPPORTED ) ) {
                nTerm(hNative);
                hNative = 0;
                throw new MediaException("Unsupported Media Format:" + mediaFormat + " for " + source.getLocator());
            } else {
                setHandledByJava();
            }
        }

        if (locator != null && mediaFormat.equals(MEDIA_FORMAT_UNKNOWN)) {
            if (locator.equals(Manager.TONE_DEVICE_LOCATOR)) {
                mediaFormat = MEDIA_FORMAT_DEVICE_TONE;
                handledByDevice = true;
            } else if (locator.equals(Manager.MIDI_DEVICE_LOCATOR)) {
                mediaFormat = MEDIA_FORMAT_DEVICE_MIDI;
                handledByDevice = true;
            }
        } else if (locator != null && locator.startsWith(Configuration.CAPTURE_LOCATOR)) {
            if (locator.startsWith(Configuration.AUDIO_CAPTURE_LOCATOR)) {
                mediaFormat = MEDIA_FORMAT_CAPTURE_AUDIO;
            } else if (locator.startsWith(Configuration.VIDEO_CAPTURE_LOCATOR)) {
                mediaFormat = MEDIA_FORMAT_CAPTURE_VIDEO;
            } else if (locator.startsWith(Configuration.RADIO_CAPTURE_LOCATOR)) {
                mediaFormat = MEDIA_FORMAT_CAPTURE_RADIO;
            }
            handledByDevice = true;
        }

        if (!handledByJava && !handledByDevice) {
            handledByDevice = nIsHandledByDevice(hNative);
        }

        this.source = source;

        if (!handledByDevice) {
            source.connect();
            SourceStream[] streams = source.getStreams();
            if (null == streams) {
                throw new MediaException("DataSource.getStreams() returned null");
            } else if (0 == streams.length) {
                throw new MediaException("DataSource.getStreams() returned an empty array");
            } else if (null == streams[0]) {
                throw new MediaException("DataSource.getStreams()[0] is null");
            } else {
                if (streams.length > 1 && Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "*** DataSource.getStreams() returned " + streams.length + 
                        " streams, only first one will be used!");
                }

                stream = streams[0];
                if( 0 == stream.getContentLength() )
                {
                    throw new MediaException("Media size is zero");
                }
            }
        }

        // Set event listener
        new MMEventListener();
        state = UNREALIZED;
    }

    void receiveRSL()
    {
        if( null != lowLevelPlayer )
        {
            // Avoid blocking the event listener thread
            new Thread() {
                public void run() {
                    lowLevelPlayer.doReceiveRSL();
                }
            }.start();
        }
    }

    void notifySnapshotFinished()
    {
        if( null != lowLevelPlayer )
        {
            lowLevelPlayer.doNotifySnapshotFinished();
        }
    }
            
    void continueDownload() {
        /* predownload media data to fill native buffers */
        if ( null != directInputThread ) {
            directInputThread.requestData();
        }
    }

    public int getNativeHandle()
    {
        return hNative;
    }

    void setNativeHandleToNull()
    {
        hNative = 0;
    }

    /**
     * TimeBase related functions.
     */
    private long origin = 0;
    private long offset = 0;
    private long time = 0;
    private long sysOffset = 0;
    final private Object timeLock = new Object();
    private boolean useSystemTime = true;

    /**
     * Get the current time of this <code>TimeBase</code>.  The values
     * returned must be non-negative and non-decreasing over time.
     *
     * @return    the current <code>TimeBase</code> time in microseconds.
     */
    public long getTime() {
        synchronized (timeLock) {
            if (useSystemTime) {

                time = origin + (System.currentTimeMillis() * 1000L -
                    sysOffset) - offset;
            } else {

                time = origin + getMediaTime() - offset;
            }
            return time;
        }
    }

    // NOTE: Right now, HighLevelPlayer implements StopTimeControl, but only
    // provides partial implementation. That means each individual player
    // must complete the implementation. I.E., somehow it must keep polling
    // current media time and compare it to the set stop time. If the time
    // is reached, it should call doStop and satev.

    /**
     * For StopTimeControl - initially reset
     */
    private long stopTime = StopTimeControl.RESET;

    /**
     * Returns the current stop time
     *
     * @return    The stopTime value
     */
    public long getStopTime() {
        return stopTime;
    }

    /**
     * the state of this player
     */
    private int state = UNREALIZED;

    private void setState( int state )
    {
        this.state = state;
    }

    /**
     * Sets the stop time
     *
     * @param  time  The new stopTime value
     */
    public synchronized void setStopTime(long time) {
        if ( getState() == STARTED) {
            /*
             *  If the stop time has already been set and its being set again,
             *  throw an exception
             */
            if (stopTime != StopTimeControl.RESET && time != StopTimeControl.RESET) {
                throw new IllegalStateException("StopTime already set");
            }
            /*
             *  If the new stop time is before the current media time,
             *  stop the player and send an event
             */
            if (time < getMediaTime()) {
                try {
                    lowLevelPlayer.doPreStop();
                    lowLevelPlayer.doStop();
                } catch (MediaException e) {
                    // Not much we can do here.
                }
                satev();
                /*
                 *  Send STOPPED_AT_TIME event
                 */
                return;
            }
        }
        stopTime = time;
        lowLevelPlayer.doSetStopTime(stopTime);
    }

    /**
     * Send STOPPED_AT_TIME event. Call this after stopping the player
     */
    void satev() {
        // Update the time base to use the system time
        // before stopping.
        updateTimeBase(false);
        setState( PREFETCHED );
        stopTime = StopTimeControl.RESET;
        sendEvent(PlayerListener.STOPPED_AT_TIME, new Long(getMediaTime()));
    }

    /**
     * flag to prevent delivering events after the CLOSED event
     */
    private boolean closedDelivered;

    /**
     * Asynchronous event mechanism.
     */
    EvtQ evtQ = null;

    /**
     * event queue lock obj
     */
    final Object evtLock = new Object();

    /**
     * Send event to player
     *
     * @param  evt      event type
     * @param  evtData  event data
     */
    public void sendEvent(String evt, Object evtData) {
        //  There's always one listener for EOM -- itself.
        //  "Deliver" the CLOSED event so that the evtQ thread may terminate
        if (listeners.size() == 0 && !evt.equals( PlayerListener.END_OF_MEDIA )
                                  && !evt.equals( PlayerListener.CLOSED )
                                  && !evt.equals(PlayerListener.ERROR)) {
            return;
        }

        // Safeguard against sending events after CLOSED event to avoid
        // deadlock in event delivery thread.
        if (closedDelivered) {
            return;
        }

        // Deliver the event to the listeners.
        synchronized (evtLock) {
            if (evtQ == null) {
                evtQ = new EvtQ(this);
            }
            evtQ.sendEvent(evt, evtData);
            // try to let listener run
            Thread.currentThread().yield();
        }

        if (evt == PlayerListener.CLOSED || evt == PlayerListener.ERROR) {
            closedDelivered = true;
        }
    }

    /**
     * This method needs to be called when the Player transitions
     * back and forth the STARTED and STOPPED states.  This is
     * to make sure that the correct time base time can be computed.
     *
     * @param  started  Description of the Parameter
     */
    public void updateTimeBase(boolean started) {
        synchronized (timeLock) {
            origin = getTime();
            useSystemTime = !started;
            if (started) {
                // Computes the starting offset based on the media time.
                offset = getMediaTime();
            } else {
                // Computes the starting offset based on the system time.
                offset = System.currentTimeMillis() * 1000L - sysOffset;
            }
        }
    }

    //protected MediaDownload mediaDownload = null;
    private DirectInputThread directInputThread;

    /**
     * Check to see if the Player is closed.  If the
     * unrealized boolean flag is true, check also to
     * see if the Player is UNREALIZED.
     *
     * @param  unrealized  Description of the Parameter
     */
    private void chkClosed(boolean unrealized) {
        int state = getState();
        if (state == CLOSED || (unrealized && state == UNREALIZED)) {
            throw new IllegalStateException("Can't invoke the method at the " +
                (state == CLOSED ? "closed" : "unrealized") +
                " state");
        }
    }

    /**
     * Constructs portions of the <code>Player</code> without
     * acquiring the scarce and exclusive resources.
     * This may include examining media data and may
     * take some time to complete.
     * <p>
     * When <code>realize</code> completes successfully, 
     * the <code>Player</code> is in the
     * <i>REALIZED</i> state.
     * <p>
     * If <code>realize</code> is called when the <code>Player</code> is in
     * the <i>REALIZED</i>, <i>PREFETCHTED</i> or <i>STARTED</i> state,
     * the request will be ignored.
     *
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @exception MediaException Thrown if the <code>Player</code> cannot
     * be realized.
     * @exception SecurityException Thrown if the caller does not
     * have security permission to realize the <code>Player</code>.
     *
     */
    public synchronized void realize() throws MediaException {
        chkClosed(false);

        if (getState() >= REALIZED) {
            return;
        }

        String type = source.getContentType();
        if (type == null && stream != null && stream.getContentDescriptor() != null) {
            type = stream.getContentDescriptor().getContentType();
        }

        directInputThread = null;

        if( !handledByJava )
        {
            if( !handledByDevice && !mediaFormat.equals(MEDIA_FORMAT_TONE) )
            {
            /* predownload media data to recognize media format and/or
               specific media parameters (e.g. duration) */

                directInputThread = new DirectInputThread( this );
                directInputThread.start();
            }

            final String t = type;
            runAsync( new AsyncTask() {
                public void run() throws MediaException {
                    nRealize(hNative, t);
                }
            } );
        }

        System.out.println( "HighLevelPlayer: realize() resumed" );
        if (!handledByDevice && !handledByJava) {
            mediaFormat = nGetMediaFormat(hNative);
            if (mediaFormat.equals(MEDIA_FORMAT_UNSUPPORTED)) {
                String format;
                /* verify if handled by Java */
                if (type != null &&
                        (format = Configuration.getConfiguration().mime2Format(type)) != null && 
                        !format.equals(MEDIA_FORMAT_UNKNOWN) && 
                        !format.equals(MEDIA_FORMAT_UNSUPPORTED)) {
                    mediaFormat = format;
                    setHandledByJava();
                } else {
                    throw new MediaException("Unsupported media format ('" + type + "','" + mediaFormat + "')");
                }
            }
        }

        if (mediaFormat.equals(MEDIA_FORMAT_UNKNOWN)) {        
            /* ask media format if unknown */
            mediaFormat = nGetMediaFormat(hNative);

            if (mediaFormat.equals(MEDIA_FORMAT_UNKNOWN)) {
                throw new MediaException("Unknown Media Format");
            }
            if (mediaFormat.equals(MEDIA_FORMAT_UNSUPPORTED)) {
                throw new MediaException("Unsupported Media Format");
            }
        }

        /* create Implementation Player */
        lowLevelPlayer = getPlayerFromType( mediaFormat );

        lowLevelPlayer.doRealize();

        setState( REALIZED );

        if (null != state_subscriber) {
            state_subscriber.PlayerRealized(this);
        }
    }


    /**
     *  Gets the playerFromType attribute of the Manager class
     *
     * @param  type                Description of the Parameter
     * @return                     The playerFromType value
     * @exception  IOException     Description of the Exception
     * @exception  MediaException  Description of the Exception
     */
    private LowLevelPlayer getPlayerFromType(String type) throws MediaException {

        if ( type == null ) {
            throw new MediaException(PL_ERR_SH + "MediaFormat is not determined");
        }

        if ("GIF".equals(type)) {
            return new GIFPlayer( this );
        } else if (DirectPlayer.nIsToneControlSupported(hNative)) {
            return new DirectTone( this );
        } else if (DirectPlayer.nIsMIDIControlSupported(hNative)) {
            return new DirectMIDI( this );
        } else {
            return new DirectPlayer( this );
        }              

    }

    private boolean isBlockedUntilEvent = false;

    private interface AsyncTask {
        public void run() throws MediaException;
    }

    private void runAsync( AsyncTask task ) throws MediaException
    {
        final Object lock = getAsyncExecLock();
        if (null != lock) {
            synchronized (lock) {
                task.run();
                isBlockedUntilEvent = true;
                while (isBlockedUntilEvent) {
                    try {
                        lock.wait();
                    } catch (InterruptedException ex) {
                    }
                }
                System.out.println(
                        "HighLevelPlayer: runAndWaitIfAsync() unblocked");
            }
        } else {
            task.run();
        }
    }

    private Object getAsyncExecLock()
    {
        return directInputThread;
    }

    void unblockOnEvent()
    {
        final Object lock = getAsyncExecLock();
        if( null != lock )
        {
            synchronized( lock )
            {
                isBlockedUntilEvent = false;
                lock.notify();
            }
        }
    }
    
    /**
     * Acquires the scarce and exclusive resources
     * and processes as much data as necessary
     * to reduce the start latency.
     * <p>
     * When <code>prefetch</code> completes successfully, 
     * the <code>Player</code> is in
     * the <i>PREFETCHED</i> state.
     * <p>
     * If <code>prefetch</code> is called when the <code>Player</code>
     * is in the <i>UNREALIZED</i> state,
     * it will implicitly call <code>realize</code>.
     * <p>
     * If <code>prefetch</code> is called when the <code>Player</code> 
     * is already in the <i>PREFETCHED</i> state, the <code>Player</code>
     * may still process data necessary to reduce the start
     * latency.  This is to guarantee that start latency can
     * be maintained at a minimum. 
     * <p>
     * If <code>prefetch</code> is called when the <code>Player</code> 
     * is in the <i>STARTED</i> state,
     * the request will be ignored.
     * <p>
     * If the <code>Player</code> cannot obtain all 
     * of the resources it needs, it throws a <code>MediaException</code>.
     * When that happens, the <code>Player</code> will not be able to
     * start.  However, <code>prefetch</code> may be called again when
     * the needed resource is later released perhaps by another
     * <code>Player</code> or application.
     *
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @exception MediaException Thrown if the <code>Player</code> cannot
     * be prefetched.
     * @exception SecurityException Thrown if the caller does not
     * have security permission to prefetch the <code>Player</code>.
     *
     */
    public synchronized void prefetch() throws MediaException {
        if (getState() >= PREFETCHED) {
            return;
        }

        if (getState() < REALIZED) {
            realize();
        }

        if (vmPaused) {
            return;
        }

        /* prefetch native player */
        /* predownload media data to fill native buffers */
        runAsync( new AsyncTask() {
            public void run() throws MediaException {
                lowLevelPlayer.doPrefetch();
            }
        });

        System.out.println("HighLevelPlayer: Prefetch resumed");
        
        VolumeControl vc = ( VolumeControl )getControl(
                pkgName + vocName);
        if (vc != null && (vc.getLevel() == -1)) {
               vc.setLevel(100);
        }

        setState( PREFETCHED );
        
        if (null != state_subscriber) {
            state_subscriber.PlayerPrefetched(this);
        }
    };

    /**
     * the flag to indicate whether the Player is currently paused at EOM.
     * If true, the Player will seek back to the beginning when
     * restarted.
     */
    boolean EOM = false;

    /**
     * the flag to indicate looping after EOM.
     */
    boolean loopAfterEOM = false;

    private boolean hasZeroDuration()
    {
        return ( isDevicePlayer() && !hasToneSequenceSet ) ;
    }

    /**
     * Starts the <code>Player</code> as soon as possible.
     * If the <code>Player</code> was previously stopped
     * by calling <code>stop</code> or reaching a preset
     * stop time, it will resume playback
     * from where it was previously stopped.  If the 
     * <code>Player</code> has reached the end of media,
     * calling <code>start</code> will automatically
     * start the playback from the start of the media.
     * <p>
     * When <code>start</code> returns successfully, 
     * the <code>Player</code> must have been started and 
     * a <code>STARTED</code> event will 
     * be delivered to the registered <code>PlayerListener</code>s.
     * However, the <code>Player</code> is not guaranteed to be in
     * the <i>STARTED</i> state.  The <code>Player</code> may have
     * already stopped (in the <i>PREFETCHED</i> state) because 
     * the media has 0 or a very short duration.
     * <p>
     * If <code>start</code> is called when the <code>Player</code>
     * is in the <i>UNREALIZED</i> or <i>REALIZED</i> state,
     * it will implicitly call <code>prefetch</code>.
     * <p>
     * If <code>start</code> is called when the <code>Player</code>
     * is in the <i>STARTED</i> state, 
     * the request will be ignored.
     *
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @exception MediaException Thrown if the <code>Player</code> cannot
     * be started.
     * @exception SecurityException Thrown if the caller does not
     * have security permission to start the <code>Player</code>.
     */
    public synchronized void start() throws MediaException {
        DirectDebugOut.nDebugPrint("HighLevelPlayer.start() entered");
        if (getState() >= STARTED) {
            return;
        }

        if (getState() < REALIZED) {
            realize();
        }

        if (getState() < PREFETCHED) {
            prefetch();
        }

        if ( hasZeroDuration() ) {
            sendEvent(PlayerListener.STARTED, new Long(0));
            sendEvent( PlayerListener.END_OF_MEDIA, new Long(0) );
            return;
        }

        if (vmPaused) {
            DirectDebugOut.nDebugPrint("HighLevelPlayer.start() is returning because vmPaused");
            return;
        }
        
        // Update the time base to use the player's
        // media time before starting.
        updateTimeBase(true);

        // Check for any preset stop time.
        if (stopTime != StopTimeControl.RESET) {
            if (stopTime <= getMediaTime()) {
                satev();
                // Send STOPPED_AT_TIME event
                return;
            }
        }

        // If it's at the EOM, it will automatically
        // loop back to the beginning.
        if (EOM) {
            try {
            setMediaTime(0);
            } catch(Exception e) {
                // do nothing...
            }
        }

        runAsync( new AsyncTask() {
            public void run() throws MediaException {
                if (!lowLevelPlayer.doStart()) {
                    throw new MediaException("start");
                }
            }
        });

        setState( STARTED );
        sendEvent(PlayerListener.STARTED, new Long(getMediaTime()));

        // Finish any pending startup stuff in subclass
        // Typically used to start any threads that might potentially
        // generate events before the STARTED event is delivered
        lowLevelPlayer.doPostStart();

        DirectDebugOut.nDebugPrint("HighLevelPlayer.start() is returning");

    };

    protected boolean hasToneSequenceSet = false;

    public boolean isCapturePlayer()
    {
        return (mediaFormat.equals(MEDIA_FORMAT_CAPTURE_AUDIO) ||
                mediaFormat.equals(MEDIA_FORMAT_CAPTURE_VIDEO) ||
                mediaFormat.equals(MEDIA_FORMAT_CAPTURE_RADIO) );
    }

    boolean isDevicePlayer()
    {
        return (mediaFormat.equals(MEDIA_FORMAT_DEVICE_MIDI) ||
                mediaFormat.equals(MEDIA_FORMAT_DEVICE_TONE));
    }

    /**
     * Stops the <code>Player</code>.  It will pause the playback at
     * the current media time.
     * <p>
     * When <code>stop</code> returns, the <code>Player</code> is in the 
     * <i>PREFETCHED</i> state.
     * A <code>STOPPED</code> event will be delivered to the registered
     * <code>PlayerListener</code>s.
     * <p>
     * If <code>stop</code> is called on
     * a stopped <code>Player</code>, the request is ignored.
     *
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @exception MediaException Thrown if the <code>Player</code>
     * cannot be stopped.
     */
    public synchronized void stop() throws MediaException {
        chkClosed(false);

        loopAfterEOM = false;

        if (getState() < STARTED) {
            return;
        }
        lowLevelPlayer.doPreStop();

        runAsync( new AsyncTask() {
            public void run() throws MediaException {
                lowLevelPlayer.doStop();
            }
        });

        // Update the time base to use the system time
        // before stopping.
        updateTimeBase(false);

        setState( PREFETCHED );
        sendEvent(PlayerListener.STOPPED, new Long(getMediaTime()));
    }

    /**
     * Release the scarce or exclusive
     * resources like the audio device acquired by the <code>Player</code>.
     * <p>
     * When <code>deallocate</code> returns, the <code>Player</code>
     * is in the <i>UNREALIZED</i> or <i>REALIZED</i> state.
     * <p>
     * If the <code>Player</code> is blocked at
     * the <code>realize</code> call while realizing, calling
     * <code>deallocate</code> unblocks the <code>realize</code> call and
     * returns the <code>Player</code> to the <i>UNREALIZED</i> state.
     * Otherwise, calling <code>deallocate</code> returns the
     * <code>Player</code> to  the <i>REALIZED</i> state.
     * <p>
     * If <code>deallocate</code> is called when the <code>Player</code>
     * is in the <i>UNREALIZED</i> or <i>REALIZED</i>
     * state, the request is ignored.
     * <p>
     * If the <code>Player</code> is <code>STARTED</code>
     * when <code>deallocate</code> is called, <code>deallocate</code>
     * will implicitly call <code>stop</code> on the <code>Player</code>.
     *
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     */
    public synchronized void deallocate() {
        chkClosed(false);

        loopAfterEOM = false;

        if (getState() < PREFETCHED) {
            return;
        }

        if (getState() == STARTED) {
            try {
                stop();
            } catch (MediaException e) {
                // Not much we can do here.
            }
        }

        try {
            runAsync(new AsyncTask() {

                public void run() throws MediaException {
                    lowLevelPlayer.doDeallocate();
                }
            });
        } catch (MediaException ex) {}
        
        if (stream != null) {
            // if stream is not seekable, just return
            if (SourceStream.NOT_SEEKABLE != stream.getSeekType()) {
                try {
                    // seek to start position
                    stream.seek(0);
                } catch(IOException e) {
                    // System.out.println("[direct] doDeallocate seek IOException");
                }
            }
        }

        if( null != state_subscriber &&
            ( getState() == PREFETCHED || getState() == STARTED ) ) {
            state_subscriber.PlayerDeallocated( this );
        }

        setState( REALIZED );
    };

    /**
     * Check for the multimedia snapshot permission.
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     */
    public void checkSnapshotPermission() {
        try {
            PermissionAccessor.checkPermissions( getLocator(), PermissionAccessor.PERMISSION_SNAPSHOT );
        } catch( InterruptedException e ) {
            throw new SecurityException( "Interrupted while trying to ask the user permission" );
        }
    }

    /**
     * Close the <code>Player</code> and release its resources.
     * <p>
     * When the method returns, the <code>Player</code> is in the
     * <i>CLOSED</i> state and can no longer be used.
     * A <code>CLOSED</code> event will be delivered to the registered
     * <code>PlayerListener</code>s.
     * <p>
     * If <code>close</code> is called on a closed <code>Player</code>
     * the request is ignored.
     */
    public synchronized void close() {
        if (getState() == CLOSED) {
            return;
        }

        if (getState() == STARTED) {
            try {
                stop();
            } catch (MediaException e) {
                // Not much we can do here.
            }
        }

        if( null != directInputThread )
        {
            directInputThread.close();
        }
        
        if( null != lowLevelPlayer )
        {
            lowLevelPlayer.doClose();
        }
        else if(hNative != 0) {
            nTerm(hNative);
            hNative = 0;
        }


        setState( CLOSED );

        if (source != null) {
            source.disconnect();
            source = null;
        }

        /* close native part of unrealized player */
        
        sendEvent(PlayerListener.CLOSED, null);
        mplayers.remove(new Integer(pID));
    }
    
    /**
     * Sets the <code>TimeBase</code> for this <code>Player</code>.
     * <p>
     * A <code>Player</code> has a default <code>TimeBase</code> that
     * is determined by the implementation. 
     * To reset a <code>Player</code> to its default 
     * <code>TimeBase</code>, call <code>setTimeBase(null)</code>.
     *
     * @param master The new <CODE>TimeBase</CODE> or 
     * <CODE>null</CODE> to reset the <code>Player</code>
     * to its default <code>TimeBase</code>.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>UNREALIZED</i>, <i>STARTED</i> or <i>CLOSED</i> state.
     * @exception MediaException Thrown if
     * the specified <code>TimeBase</code> cannot be set on the 
     * <code>Player</code>.
     * @see #getTimeBase
     */
    public void setTimeBase(TimeBase master) throws MediaException {
        chkClosed(true);
        if (state == STARTED) {
            throw new IllegalStateException("Cannot call setTimeBase on a player in the STARTED state");
        }

        if (master == null) {
            return;
        }
        if (master != this) {
            throw new MediaException("Incompatible TimeBase");
        }
    };

    /**
     * Gets the <code>TimeBase</code> that this <code>Player</code> is using.
     * @return The <code>TimeBase</code> that this <code>Player</code> is using.
     * @see #setTimeBase
     *
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>UNREALIZED</i> or <i>CLOSED</i> state.
     */
    public TimeBase getTimeBase() {
        chkClosed(true);
        return this;
    };

    private long mediaTimeSet = TIME_UNKNOWN;

    void notifyMediaTimeSet( long mediaTimeSet )
    {
        this.mediaTimeSet = mediaTimeSet;
    }

    private long getMediaTimeSet()
    {
        return mediaTimeSet;
    }

    /**
     * Sets the <code>Player</code>'s&nbsp;<i>media time</i>.
     * <p>
     * For some media types, setting the media time may not be very
     * accurate.  The returned value will indicate the 
     * actual media time set.
     * <p>
     * Setting the media time to negative values will effectively
     * set the media time to zero.  Setting the media time to
     * beyond the duration of the media will set the time to
     * the end of media.
     * <p>
     * There are some media types that cannot support the setting
     * of media time.  Calling <code>setMediaTime</code> will throw
     * a <code>MediaException</code> in those cases.
     * 
     * @param now The new media time in microseconds.
     * @return The actual media time set in microseconds.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>UNREALIZED</i> or <i>CLOSED</i> state.
     * @exception MediaException Thrown if the media time
     * cannot be set.
     * @see #getMediaTime
     */
    public synchronized long setMediaTime(long now) throws MediaException {
        chkClosed(true);

        if (now < 0) {
            now = 0;
        }

        // Time-base-time needs to be updated if player is started.
        if (getState() == STARTED) {
            origin = getTime();
        }

        long theDur = lowLevelPlayer.doGetDuration();
        if ((theDur != TIME_UNKNOWN) && (now > theDur)) {
            now = theDur;
        }

        final long timeToSet = now;
        runAsync( new AsyncTask() {
            public void run() throws MediaException {
                lowLevelPlayer.doSetMediaTime( timeToSet );
            }
        } );

        long rtn = getMediaTimeSet();

        System.out.println( "HighLevelPlayer: setMediaTime resumed" );
        
        EOM = false;

        // Time-base-time needs to be updated if player is started.
        if (getState() == STARTED) {
            offset = rtn;
        }

        return rtn;
    };

    public void setSnapshotQuality( int quality )
    {
    }

    /**
     * Gets this <code>Player</code>'s current <i>media time</i>.
     * <p>
     * <code>getMediaTime</code> may return <code>TIME_UNKNOWN</code> to
     * indicate that the media time cannot be determined. 
     * However, once <code>getMediaTime</code> returns a known time 
     * (time not equals to <code>TIME_UNKNOWN</code>), subsequent calls 
     * to <code>getMediaTime</code> must not return 
     * <code>TIME_UNKNOWN</code>.
     *
     * @return The current <i>media time</i> in microseconds or 
     * <code>TIME_UNKNOWN</code>.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @see #setMediaTime
     */
    public long getMediaTime() {
        chkClosed(false);
        long time = TIME_UNKNOWN;
        if( hasZeroDuration() )
        {
            return 0;
        }
        if( null != lowLevelPlayer )
        {
            time = lowLevelPlayer.doGetMediaTime();
        }
        return time;
    };

    /**
     * Gets the current state of this <code>Player</code>.
     * The possible states are: <i>UNREALIZED</i>,
     * <i>REALIZED</i>, <i>PREFETCHED</i>, <i>STARTED</i>, <i>CLOSED</i>.
     * 
     * @return The <code>Player</code>'s current state.
     */
    public int getState() {
        return state;
    };

    /**
     * Get the duration of the media.
     * The value returned is the media's duration
     * when played at the default rate.
     * <br>
     * If the duration cannot be determined (for example, the
     * <code>Player</code> is presenting live
     * media)  <CODE>getDuration</CODE> returns <CODE>TIME_UNKNOWN</CODE>.
     *
     * @return The duration in microseconds or <code>TIME_UNKNOWN</code>.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     */
    public long getDuration() {
        chkClosed(false);
        long dur = TIME_UNKNOWN;
        if( hasZeroDuration() )
        {
            return 0;
        }
        if( null != lowLevelPlayer )
        {
            dur = lowLevelPlayer.doGetDuration();
        }
        return dur;
    };

    /**
     * Locator string
     */
    private String locator;

    public String getLocator()
    {
        return locator;
    }

    /**
     * Get the content type of the media that's
     * being played back by this <code>Player</code>.
     * <p>
     * See <a href="Manager.html#content-type">content type</a>
     * for the syntax of the content type returned.
     *
     * @return The content type being played back by this 
     * <code>Player</code>.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>UNREALIZED</i> or <i>CLOSED</i> state.
     */
    public String getContentType() {

        String type = null;

        chkClosed(true);

        if (source == null) {
            // Call helper function to get a content type
            type = DefaultConfiguration.getContentType(locator);
        } else {
            type = source.getContentType();
        }

        if( null == type )
        {
            type = lowLevelPlayer.doGetContentType();
        }

        return type;

    };


    /**
     * Set the number of times the <code>Player</code> will loop
     * and play the content.
     * <p>
     * By default, the loop count is one.  That is, once started,
     * the <code>Player</code> will start playing from the current
     * media time to the end of media once.
     * <p>
     * If the loop count is set to N where N is bigger than one,
     * starting the <code>Player</code> will start playing the
     * content from the current media time to the end of media.
     * It will then loop back to the beginning of the content 
     * (media time zero) and play till the end of the media.
     * The number of times it will loop to the beginning and 
     * play to the end of media will be N-1.
     * <p>
     * Setting the loop count to 0 is invalid.  An 
     * <code>IllegalArgumentException</code> will be thrown.
     * <p>
     * Setting the loop count to -1 will loop and play the content
     * indefinitely.
     * <p>
     * If the <code>Player</code> is stopped before the preset loop
     * count is reached either because <code>stop</code> is called or
     * a preset stop time (set with the <code>StopTimeControl</code>) 
     * is reached, calling <code>start</code> again will
     * resume the looping playback from where it was stopped until it
     * fully reaches the preset loop count. 
     * <p> 
     * An <i>END_OF_MEDIA</i> event will be posted 
     * every time the <code>Player</code> reaches the end of media.
     * If the <code>Player</code> loops back to the beginning and
     * starts playing again because it has not completed the loop
     * count, a <i>STARTED</i> event will be posted.
     * 
     * @param count indicates the number of times the content will be
     * played.  1 is the default.  0 is invalid.  -1 indicates looping 
     * indefintely.
     * @exception IllegalArgumentException Thrown if the given
     * count is invalid.
     * @exception IllegalStateException Thrown if the 
     * <code>Player</code> is in the <i>STARTED</i> 
     * or <i>CLOSED</i> state. 
     */

    public void setLoopCount(int count) {
        chkClosed(false);

        if (getState() == STARTED) {
            throw new IllegalStateException("setLoopCount");
        }

        if (count == 0 || count < -1) {
            throw new IllegalArgumentException("setLoopCount");
        }

        loopCountSet = count;
        loopCount = count;
    }


    /**
     * Add a player listener for this player.
     *
     * @param playerListener the listener to add.
     * If <code>null</code> is used, the request will be ignored.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @see #removePlayerListener
     */
    public void addPlayerListener(PlayerListener playerListener) {
        chkClosed(false);
        if (playerListener != null) {
            listeners.addElement(playerListener);
        }
    };

    /**
     * Remove a player listener for this player.
     *
     * @param playerListener the listener to remove.
     * If <code>null</code> is used or the given 
     * <code>playerListener</code> is not a listener for this
     * <code>Player</code>, the request will be ignored.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * is in the <i>CLOSED</i> state.
     * @see #addPlayerListener
     */
    public void removePlayerListener(PlayerListener playerListener) {
        chkClosed(false);
        listeners.removeElement(playerListener);
    };


    private Hashtable htControls = new Hashtable();
    private Control [] controls = null;


    /**
     *  Gets the controls attribute of the HighLevelPlayer object
     *
     * @return    The controls value
     */
    public final Control[] getControls() {
        chkClosed(true);

        synchronized( this )
        {
            if( null == controls )
            {
                Vector v = new Vector(3);
                // average maximum number of controls

                Enumeration ctrlNames = getPossibleControlNames().elements();
                while( ctrlNames.hasMoreElements() ) {
                    Object c = getControl( ( String )ctrlNames.nextElement() );
                    if ((c != null) && !v.contains(c)) {
                        v.addElement(c);
                    }
                }
                controls = new Control[v.size()];
                v.copyInto( controls );
            }
        }

        return controls;
    }

    public boolean isRadioPlayer()
    {
        return ( null != source.getLocator() &&
            source.getLocator().startsWith(
                DefaultConfiguration.RADIO_CAPTURE_LOCATOR ) );
    }

    public boolean isCameraPlayer()
    {
        return mediaFormat.equals(MEDIA_FORMAT_CAPTURE_VIDEO);
    }

    /**
     * Returns the array containing all the available JSR-135 and
     * JSR-234 (if available) control names
     * that may be supported by HighLevelPlayer
     */
    private static synchronized Vector getPossibleControlNames()
    {
        if( null == possibleControlNames )
        {
            possibleControlNames = new Vector( allJsr135Ctrls.length );

            for( int i = 0; i < allJsr135Ctrls.length; i++ )
            {
                possibleControlNames.addElement(
                        getFullControlName( allJsr135Ctrls[ i ] ) );
            }
            Jsr234Proxy ammsProxy = Jsr234Proxy.getInstance();
            if( ammsProxy.isJsr234Available() )
            {
                String [] jsr234Names =
                        ammsProxy.getJsr234PlayerControlNames();
                for( int i = 0; i < jsr234Names.length; i++ )
                {
                    possibleControlNames.addElement( jsr234Names[ i ] );
                }
            }

        }
        return possibleControlNames;
    }

    // Prepend the package name if the type given does not
    // have the package prefix.
    private static String getFullControlName( String name )
    {
        if( name.indexOf('.') < 0  )
        {
            return pkgName + name;
        }
        else {
            return name;
        }
    }
    /**
     * Gets the <code>Control</code> that supports the specified
     * class or interface. The full class
     * or interface name should be specified.
     * <code>Null</code> is returned if the <code>Control</code>
     * is not supported.
     *
     * @param  type  Description of the Parameter
     * @return       <code>Control</code> for the class or interface
     * name.
     */
    public Control getControl(String type) {
        chkClosed(true);

        if (type == null) {
            throw new IllegalArgumentException();
        }

        String fullName = getFullControlName( type );

        Control c = null;
        synchronized( this )
        {
            c = ( Control )htControls.get( fullName );
            if( null == c &&
                null == controls &&
                getPossibleControlNames().contains( fullName ) )
            {
                c = lowLevelPlayer.doGetNewControl( fullName );
                if( null != c )
                {
                    htControls.put( fullName, c );
                }
            }
        }
        return c;
    }

    /**
     * For global PlayerID management
     *
     * @param  pid  Description of the Parameter
     * @return      Description of the Return Value
     */
    public static HighLevelPlayer get(int pid) {
        return (HighLevelPlayer) (mplayers.get(new Integer(pid)));
    }

    /**
     * Send system volume changed event to all of the player from this VM
     */
    public static void sendSystemVolumeChanged(int volume) {
        if (mplayers == null) {
            return;
        }

        systemVolume = volume;
        
        if (changeSystemVolumeTask == null) {
            changeSystemVolumeTask = new Runnable() {
                private boolean isRunning = false;
                private int waiting = 0;
                public void run() {
                    synchronized (this) {
                        if (isRunning) {
                            /* the task is already running */
                            waiting++;
                            try {
                                this.wait();
                            } catch (InterruptedException ie) {
                                return;
                            } finally {
                                waiting--;
                            }
                        } else {
                            isRunning = true;
                        }
                    }
                    for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
                        if (waiting > 0) {
                            /* New task is waiting. Exitting */
                            break;
                        }
                        HighLevelPlayer p = (HighLevelPlayer) e.nextElement();
                        /* Send event to player if this player is in realized state (or above) */
                        int state = p.getState();
                        if (state >= Player.REALIZED) {
                            VolumeControl vc = (VolumeControl)p.getControl("VolumeControl");
                            if (vc != null && vc instanceof DirectVolume) {
                                ((DirectVolume)vc).setSystemVolume(systemVolume);
                            }
                        }
                    }
                    synchronized (this) {
                        if (waiting > 0) {
                            this.notify();
                        } else {
                            isRunning = false;
                        }
                    }
                }
            };
        }
        new Thread(changeSystemVolumeTask).start();
    }

    /**
     *  Pauses and deallocates all media players.
     *
     *  After this call all players are either in realized
     *  or unrealized state.  
     *
     *  Resources are being released during deallocation.
     */
    public static void pauseAll() {
        vmPaused = true;
    
        if (mplayers == null) {
            return;
        }

        for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
            HighLevelPlayer p = (HighLevelPlayer) e.nextElement();

            int state = p.getState();
            long time = p.getMediaTime();
            
            // save the player's state
            pstates.put(p, new Integer(state));
            // save the player's media time
            mtimes.put(p, new Long(time));

            try {
                // Stop the player
                if (state == Player.STARTED) {
                    p.stop();
                }
            } catch(MediaException ex) {
            }
        }
    }


    /**
     *  Resumes all media players' activities.
     *
     *  Players that were in STARTED state when pause
     *  was called will resume playing at the media time
     *  they were stopped and deallocated.
     */
    public static void resumeAll() {
        vmPaused = false;
        
        if (mplayers == null || pstates.size() == 0) {
            return;
        }
        
        for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
            HighLevelPlayer p = (HighLevelPlayer) e.nextElement();

            int state = ((Integer) pstates.get(p)).intValue();
            long time = ((Long) mtimes.get(p)).longValue();

            switch (state) {
                /*
                case Player.PREFETCHED:
                    try {
                        p.prefetch();
                        p.setMediaTime(time);
                    } catch (MediaException ex) {
                    }
                    break;
                */
                case Player.STARTED:
                    try {
                        //p.realize();
                        //p.prefetch();
                        if (p.getState() != Player.STARTED) {
                            p.setMediaTime(time);
                            p.start();
                        }
                    } catch (MediaException ex) {
                    }
                    break;
            }
        }

        // clear player states and media times
        pstates.clear();
        mtimes.clear();
    }

    /**
     * the default size of the event queue
     * can be overridden by descendants
     */
    int eventQueueSize = 20;

    /**
     *  Description of the Method
     */
    private synchronized void doLoop() {
        // If a loop count is set, we'll loop back to the beginning.
        if ((loopCount > 1) || (loopCount == -1)) {
            try {
                setMediaTime(0);
            } catch(MediaException e){
                // Do nothing...
            }
            try {
                if (loopCount > 1) {
                    loopCount--;
                }
                start();
            } catch (MediaException ex) {
                // System.out.println("[basic] doLoop exception " + ex.getMessage());
                loopCount = 1;
            }
        } else if (loopCountSet > 1) {
            loopCount = loopCountSet;
        }

        loopAfterEOM = false;
    }

    /**
     * The thread that's responsible for delivering Player events.
     * This class lives for only 5 secs.  If no event comes in
     * 5 secs, it will exit.
     *
     * @created    January 13, 2005
     */
    private class EvtQ extends Thread {

        /**
         * the player instance
         */
        private HighLevelPlayer p;
        /**
         * event type array
         */
        private String[] evtQ;
        /**
         * event data array
         */
        private Object[] evtDataQ;
        /**
         * head and tail pointer of the event queue
         */
        private int head, tail;

        /**
         * The constructor
         *
         * @param  p  the instance of HighLevelPlayer intending to post event to
         *        this event queue.
         */
        EvtQ(HighLevelPlayer p) {
            this.p = p;
            evtQ = new String[p.eventQueueSize];
            evtDataQ = new Object[p.eventQueueSize];
            start();
        }

        /**
         * Put an event in the event queue and wake up the thread to
         * deliver it.  If the event queue is filled, block.
         *
         * @param  evt      Description of the Parameter
         * @param  evtData  Description of the Parameter
         */
        synchronized void sendEvent(String evt, Object evtData) {

            // Wait if the event queue is full.
            // This potentially will block the Player's main thread.
            while ((head + 1) % p.eventQueueSize == tail) {
                try {
                    wait(1000);
                } catch (Exception e) {
                }
            }
            evtQ[head] = evt;
            evtDataQ[head] = evtData;
            if (++head == p.eventQueueSize) {
                head = 0;
            }
            notifyAll();
            // try to let other threads run
            Thread.currentThread().yield();
        }

        /**
         * Event handling thread.
         */
        public void run() {

            String evt = "";
            Object evtData = null;
            boolean evtToGo = false;
            // true if there is an event to send
            // true if at least one event is sent,
            // in case that posting the initial event
            // takes a long time
            boolean evtSent = false;

            for (;;) {

                synchronized (this) {

                    // If the queue is empty, we'll wait
                    if (head == tail) {
                        try {
                            wait(1000);
                        } catch (Exception e) {
                        }
                    }
                    if (head != tail) {
                        evt = evtQ[tail];
                        evtData = evtDataQ[tail];
                        // For garbage collection.
                        evtDataQ[tail] = null;
                        evtToGo = true;
                        if (++tail == p.eventQueueSize) {
                            tail = 0;
                        }
                        notifyAll();
                    } else {
                        evtToGo = false;
                    }

                }
                // synchronized this

                if (evtToGo) {
                    // First, check and handle EOM.
                    if (evt == PlayerListener.END_OF_MEDIA) {
                        synchronized (p) {
                            p.EOM = true;
                            p.loopAfterEOM = false;

                            if (p.getState() > Player.PREFETCHED) {
                                p.updateTimeBase(false);
                                p.setState(Player.PREFETCHED);
                                if (p.loopCount > 1 || p.loopCount == -1) {

                                    p.loopAfterEOM = true;
                                }
                            }
                        }
                    }

                    // Notify the PlayerListeners.
                    Enumeration en;
                    synchronized (p.listeners) {
                        en = p.listeners.elements();
                    }

                    PlayerListener l;

                    Player src_p = p;

                    while (en.hasMoreElements()) {
                        try {
                            l = (PlayerListener) en.nextElement();
                            l.playerUpdate(src_p, evt, evtData);
                        } catch (Exception e) {
                            e.printStackTrace();
                            System.err.println("Error in playerUpdate " +
                                    "while delivering event " + evt + ": " + e);
                        }
                    }

                    if (p.loopAfterEOM) {
                        // We'll need to loop back because looping was set.
                        p.doLoop();
                    }

                    evtSent = true;

                }

                // We'll exit the event thread if we have already sent one
                // event and there's no more event after 5 secs; or if the
                // Player is closed.

                if (evt == PlayerListener.CLOSED || evt == PlayerListener.ERROR) {
                    // will cause a deadlock if the queue
                    // is full
                    synchronized (p.evtLock) {
                        p.evtQ = null;
                        break;
                        // Exit the event thread.
                    }
                }

                synchronized (this) {
                    if (head == tail && evtSent && !evtToGo) {
                        synchronized (p.evtLock) {
                            p.evtQ = null;
                            break;
                            // Exit the event thread.
                        }
                    }
                }
            }
        }
    }
}

