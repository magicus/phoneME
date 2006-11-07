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

import java.util.Vector;
import java.util.Enumeration;
import java.util.Hashtable;

import java.io.*;

import javax.microedition.media.*;
import javax.microedition.media.control.*;
import javax.microedition.media.protocol.DataSource;
import javax.microedition.media.protocol.SourceStream;

import com.sun.mmedia.PermissionAccessor;

/**
 * BasicPlayer provides basic implementation for the Player methods.
 * Many of the methods call do<method> to do the actual work that can
 * be overridden by subclasses.
 *
 * @created    January 13, 2005
 */
public abstract class BasicPlayer implements Player, TimeBase, StopTimeControl {
    
    /**
     * global player id
     */
    private static int pcount = -1;

    /**
     * lock object
     */
    private static Object idLock = new Object();

    // JAVADOC COMMENT ELIDED
    public int state = UNREALIZED;

    // JAVADOC COMMENT ELIDED
    int loopCountSet = 1, loopCount;

    // JAVADOC COMMENT ELIDED
    boolean EOM = false;

    // JAVADOC COMMENT ELIDED
    boolean loopAfterEOM = false;

    // JAVADOC COMMENT ELIDED
    Vector listeners = new Vector(2);
    
    // JAVADOC COMMENT ELIDED
    boolean listenersModified = false;

    // JAVADOC COMMENT ELIDED
    PlayerEventQueue eventQueue = null;

    // JAVADOC COMMENT ELIDED
    Object evtLock = new Object();

    // JAVADOC COMMENT ELIDED
    protected int pID = 0;

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

    
    // JAVADOC COMMENT ELIDED
    protected final static String pkgName = "javax.microedition.media.control.";

    // JAVADOC COMMENT ELIDED

    // JAVADOC COMMENT ELIDED
    protected final static String fpcName = "FramePositioningControl";
    // JAVADOC COMMENT ELIDED
    protected final static String guiName = "GUIControl";
    // JAVADOC COMMENT ELIDED
    protected final static String mdcName = "MetaDataControl";
    // JAVADOC COMMENT ELIDED
    protected final static String micName = "MIDIControl";
    // JAVADOC COMMENT ELIDED
    protected final static String picName = "PitchControl";
    // JAVADOC COMMENT ELIDED
    protected final static String racName = "RateControl";
    /**
     *  Description of the Field
     */
    protected final static String recName = "RecordControl";
    /**
     *  Description of the Field
     */
    protected final static String stcName = "StopTimeControl";
    /**
     *  Description of the Field
     */
    protected final static String tecName = "TempoControl";
    /**
     *  Description of the Field
     */
    protected final static String tocName = "ToneControl";
    /**
     *  Description of the Field
     */
    protected final static String vicName = "VideoControl";
    /**
     *  Description of the Field
     */
    protected final static String vocName = "VolumeControl";
    // JAVADOC COMMENT ELIDED
    protected final static String rtspName = "RtspControl";

    // JAVADOC COMMENT ELIDED
    private static final String[] allCtrls = {
        fpcName, /*FramePositioningControl*/
        guiName, /*GUIControl*/
        mdcName, /*MetaDataControl*/
        micName, /*MIDIControl*/
        picName, /*PitchControl*/
        racName, /*RateControl*/
        recName, /*RecordControl*/
        stcName, /*StopTimeControl*/
        tecName, /*TempoControl*/
        tocName, /*ToneControl*/
        vicName, /*VideoControl*/
        vocName, /*VolumeControl*/
        rtspName, /*(non-standard) RtspControl*/
    };
    
    // JAVADOC COMMENT ELIDED
    private static final int[] allPermissions = {};

    // JAVADOC COMMENT ELIDED
    private boolean isTrusted;
    
    // JAVADOC COMMENT ELIDED
    private String control_names[];
    
    // JAVADOC COMMENT ELIDED
    private int permissions[];
    
    // JAVADOC COMMENT ELIDED
    protected DataSource source;

    // JAVADOC COMMENT ELIDED
    protected SourceStream stream;

    // JAVADOC COMMENT ELIDED
    private TimeBase timeBase = this;

    // JAVADOC COMMENT ELIDED
    protected long stopTime = StopTimeControl.RESET;

    // JAVADOC COMMENT ELIDED
    int eventQueueSize = 20;

    // JAVADOC COMMENT ELIDED
    private boolean closedDelivered;

    // JAVADOC COMMENT ELIDED 
    private static MIDletPauseListener pauseListener = null;
    private static boolean vmPaused = false;

    // JAVADOC COMMENT ELIDED
    public static void setMIDletPauseListener(MIDletPauseListener listener) {
        //System.out.println("DEBUG: about to BP.setMIDletPauseListener(" + listener + ")");
        // Only set the listener once!
        // If the listener is aleady set then return witout waring.
        if (pauseListener == null) {
            pauseListener = listener;
        }
    }

    // JAVADOC COMMENT ELIDED
    public static void pauseStateEntered(MIDletPauseListener listener,
                                         boolean paused) {
        //System.out.println("DEBUG: about to BP.pauseStateEntered(" + listener + "," + paused + ")");
        // if the listeners don't match then simply return
        if (listener != pauseListener) return;

        vmPaused = paused;

        if (vmPaused) {
            for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
                BasicPlayer p = (BasicPlayer)e.nextElement();
                
                if (p.getState() == STARTED) {
                    notifyPauseListener("Player");
                }
            }
            /*pauseAll();
        } else {
            resumeAll();*/
        }
    }
            
    public static void notifyPauseListener(String msg) {
        if (vmPaused && pauseListener != null) {
            pauseListener.reportActivity(msg);
        }
    }
    
    // JAVADOC COMMENT ELIDED
    public BasicPlayer() {
        init();
        control_names = allCtrls;
        permissions = allPermissions;
    }
    protected BasicPlayer(String[] n, int[] p) {
        init();
        control_names = (n == null) ? allCtrls : n;
        permissions = (p == null) ? allPermissions : p;
    }
    
    private void init() {
        // Initialize sysOffset to the current time.
        // This is used for TimeBase calculations.
        sysOffset = System.currentTimeMillis() * 1000L;

        synchronized (idLock) {
            pcount = (pcount + 1) % 32767;
            pID = pcount;
        }
        mplayers.put(new Integer(pID), this);
    }

    // JAVADOC COMMENT ELIDED
    public boolean initFromURL(String encodings) {
        return true;
    }
    
    // JAVADOC COMMENT ELIDED
    protected final void checkPermissions() throws SecurityException {
        if (isTrusted) return;
        //TBD: check locator-specific permissions ?
        for (int i = 0; i < permissions.length; ++i)
            PermissionAccessor.checkPermissions(permissions[i]);
        isTrusted = true;
    }
    
    // JAVADOC COMMENT ELIDED
    protected final void chkClosed(boolean unrealized) {
        /*
         * This method is indended to be called from synchronized methods
         * (that change player's state), but in fact 
         * it is invoked from unsynchronized methods too, 
         * so, as a temporary solution,  
         * it shall eliminate access to player's state: 
         * it must get the state only once and then work with a local variable.
         */
        int theState = this.state; 
        if (theState == CLOSED || (unrealized && theState == UNREALIZED)) {
            throw new IllegalStateException("Can't invoke the method at the " +
                (theState == CLOSED ? "closed" : "unrealized") +
                " state ");
        }
    }


    // JAVADOC COMMENT ELIDED
    public synchronized void setLoopCount(int count) {
        //System.out.println("DEBUG: about to BP.setLoopCount(" + count + ") for player=" + this);
        chkClosed(false);
        
        if (state == STARTED) {
            throw new IllegalStateException("setLoopCount");
        }
        
        if (count == 0 || count < -1) {
            throw new IllegalArgumentException("setLoopCount");
        }

        loopCountSet = count;
        loopCount = count;
        
        doSetLoopCount(count);
    }


    // JAVADOC COMMENT ELIDED
    protected void doSetLoopCount(int count) {
    }

    public static final int AUDIO_NONE = 0;
    public static final int AUDIO_PCM = 1;
    public static final int AUDIO_MIDI = 2;

    public int getAudioType() {
        return AUDIO_NONE;
    }

    public void setOutput(Object output) { }
    public Object getOutput() { return null; }

    // JAVADOC COMMENT ELIDED
    public synchronized void realize() throws MediaException {
        //System.out.println("DEBUG: about to BP.realize() for player=" + this);
        chkClosed(false);

        if (state >= REALIZED) {
            return;
        }
        /*  
           Will not check permission in case that the source has not been initialized 
           In order to avoid from asking about permission to record in case of i.e. 
           Manager.createPlayer("MIDI_DEVICE_LOCATOR"); player.realize() 
           Note: if the player needs to get permission if realize but the source  
           is null (like in the case of JavaMPEG1Player2) it should overwrite  
           the realize method. 
        */
        if (source != null) { 
            checkPermissions(); 
        } 

        // BasicPlayer only handles the first stream from
        // a DataSource.
        if ((source != null) && (stream == null)) {
            stream = source.getStreams()[0];
        } else {
            if (hasDataSource) {
                state = UNREALIZED;
                throw new MediaException("Unable to realize");
            } 
        }

        doRealize();
        
        state = REALIZED;
    }

    protected boolean hasDataSource = true;

    /**
     * Subclasses need to implement this to realize
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected abstract void doRealize() throws MediaException;


    // JAVADOC COMMENT ELIDED
    public synchronized void prefetch() throws MediaException {
        //System.out.println("DEBUG: about to BP.prefetch() for player=" + this);
        chkClosed(false);

        if (state >= PREFETCHED) {
            return;
        }

        if (state < REALIZED) {
            realize();
        } else {
            //if realize has been called the permission will be checked from there
            checkPermissions();
        }

        doPrefetch();

        state = PREFETCHED;
    }


    // JAVADOC COMMENT ELIDED
    protected abstract void doPrefetch() throws MediaException;


    // JAVADOC COMMENT ELIDED
    public synchronized void start() throws MediaException {
        //System.out.println("DEBUG: about to BP.start() for player=" + this + " at time=" + getMediaTime());
        chkClosed(false);

        if (state >= STARTED) {
            return;
        }

        if (state < PREFETCHED) {
            prefetch();
        } else {
            //If prefetch has been called the permission will be checked from there
            if(!EOM && !loopAfterEOM) {
                checkPermissions();
            }
        }

        // Update the time base to use the player's
        // media time before starting.
        updateTimeBase(true);

        // Check for any preset stop time.
        if (stopTime != StopTimeControl.RESET) {
            if (stopTime <= getMediaTime()) {
                satev(); // Send STOPPED_AT_TIME event
                return;
            }
        }

        // If it's at the EOM, it will automatically
        // loop back to the beginning.
        if (EOM) 
            try {
                setMediaTime(0);
            } catch (MediaException me) {
                // Ignore, if setting media time is not supported
            }

        if (!doStart()) {
            throw new MediaException("start");
        }

        state = STARTED;
        sendEvent(PlayerListener.STARTED, new Long(getMediaTime()));

        // Finish any pending startup stuff in subclass
        // Typically used to start any threads that might potentially
        // generate events before the STARTED event is delivered
        doPostStart();
        //System.out.println("DEBUG: finished BP.start() for player=" + this);
    }


    // JAVADOC COMMENT ELIDED
    protected abstract boolean doStart();


    // JAVADOC COMMENT ELIDED
    protected void doPostStart() {
    }


    // JAVADOC COMMENT ELIDED
    public synchronized void stop() throws MediaException {
        //System.out.println("DEBUG: about to BP.stop() for player=" + this + " at time=" + getMediaTime());
        chkClosed(false);

        loopAfterEOM = false;
     
        if (state < STARTED) {
            return;
        }

        doStop();
        
        // Update the time base to use the system time
        // before stopping.
        updateTimeBase(false);

        state = PREFETCHED;
        sendEvent(PlayerListener.STOPPED, new Long(getMediaTime()));
        //System.out.println("DEBUG: finished BP.stop() for player=" + this);
    }


    // JAVADOC COMMENT ELIDED
    protected abstract void doStop() throws MediaException;


    // JAVADOC COMMENT ELIDED
    public synchronized void deallocate() {
        //System.out.println("DEBUG: about to BP.deallocate() for player=" + this);
        chkClosed(false);

        loopAfterEOM = false;

        if (state < PREFETCHED) {
            return;
        }

        if (state == STARTED) {
            try {
                stop();
            } catch (MediaException e) {
                // Not much we can do here.
                // e.printStackTrace();
            }
        }

        doDeallocate();

        EOM = true;
        
        state = REALIZED;
    }


    // JAVADOC COMMENT ELIDED
    protected abstract void doDeallocate();


    // JAVADOC COMMENT ELIDED
    public synchronized void close() {
        //System.out.println("DEBUG: about to BP.close() for player=" + this);
        if (state == CLOSED) {
            return;
        }

        deallocate();
        doClose();

        state = CLOSED;

        if (source != null) {
            source.disconnect();
        }
        
        sendEvent(PlayerListener.CLOSED, null);
        mplayers.remove(new Integer(pID));
    }


    // JAVADOC COMMENT ELIDED
    protected abstract void doClose();


    // JAVADOC COMMENT ELIDED
    public synchronized long setMediaTime(long now) throws MediaException {
        //System.out.println("DEBUG: about to BP.setMediaTime(" + now + ") for player=" + this);
        chkClosed(true);

        if (now < 0) {
            now = 0;
        }

        // Time-base-time needs to be updated if player is started.
        if (state == STARTED) {
            origin = getTime();
        }

        long theDur = doGetDuration();
        if ((theDur != TIME_UNKNOWN) && (now > theDur)) {
            now = theDur;
        }

        long rtn = doSetMediaTime(now);
        EOM = false;

        // Time-base-time needs to be updated if player is started.
        if (state == STARTED) {
            offset = rtn;
        }

        //System.out.println("DEBUG: finished BP.setMediaTime(" + now + ")=" + rtn + " for player=" + this);
        return rtn;
    }


    // JAVADOC COMMENT ELIDED
    protected abstract long doSetMediaTime(long now) throws MediaException;


    // JAVADOC COMMENT ELIDED
    public long getMediaTime() {
        //System.out.println("DEBUG: about to BP.getMediaTime() for player=" + this);
        chkClosed(false);
        return doGetMediaTime();
    }


    // JAVADOC COMMENT ELIDED
    protected abstract long doGetMediaTime();


    // JAVADOC COMMENT ELIDED
    public int getState() {
        return state;
    }


    // JAVADOC COMMENT ELIDED
    public long getDuration() {
        //System.out.println("DEBUG: about to BP.getDuration() for player=" + this);
        chkClosed(false);
        return doGetDuration();
    }


    // JAVADOC COMMENT ELIDED
    protected abstract long doGetDuration();


    // JAVADOC COMMENT ELIDED
    public void addPlayerListener(PlayerListener playerListener) {
        chkClosed(false);
        if (playerListener != null) {
            /* 
             * Excplicit "sync" is needed to raise "modified" flag. 
             * Implicit "sync" is already inside addElemet() method, 
             * so second sync from the same thread will do nothing ...
             */
            synchronized (listeners) {
                listenersModified = true;
                listeners.addElement(playerListener);
            }
        }
    }


    // JAVADOC COMMENT ELIDED
    public void removePlayerListener(PlayerListener playerListener) {
        chkClosed(false);
        if (playerListener != null) {
            /* 
             * Excplicit "sync" is needed to raise "modified" flag. 
             * Implicit "sync" is already inside removeElemet() method, 
             * so second sync from the same thread will do nothing ...
             */
            synchronized (listeners) {
                listenersModified = true;
                listeners.removeElement(playerListener);
            }
        }
    }

    final void notifyListeners(String message, Object obj) {
        Object copy[];
        synchronized (listeners) {
            copy = new Object[listeners.size()];
            listeners.copyInto(copy);
            listenersModified = false;
        }
        /*
         * TBD: raise a flag to show that we are in callbacks 
         * to detect re-entrance ...
         * (syncState object can also be used, 
         * however it protects state changes too) 
         */
        for (int i = 0; i < copy.length; i++) {
            PlayerListener listener = (PlayerListener)copy[i];
            listener.playerUpdate(this, message, obj);
        }
        /*
         * TBD: need to check for "listenersModified == true", 
         * this means that one of callbacks updated listeners ->
         * need some actions ...
         */
    }
    
    // JAVADOC COMMENT ELIDED
    public void sendEvent(String evtName, Object evtData) {
        //System.out.println("DEBUG: about to BP.sendEvent(" + evtName + "," + evtData +") for player=" + this);
        //  There's always one listener for EOM - itself (for loop procesing).
        //  "Deliver" the CLOSED/ERROR events 
        //  so that the eventQueue thread may terminate
        if (listeners.size() == 0 && 
            evtName != PlayerListener.END_OF_MEDIA &&
            evtName != PlayerListener.CLOSED && 
            evtName != PlayerListener.ERROR) {
            return;
        }

        // Safeguard against sending events after CLOSED event to avoid
        // deadlock in event delivery thread.
        if (closedDelivered) {
            return;
        }

        // Deliver the event to the listeners.
        synchronized (evtLock) {
            if (eventQueue == null) {
                eventQueue = new PlayerEventQueue(this);
            }
            // TBD: attempt to ensure "eventQueue" existence 
            // in eventQueue.sentEvent() call ...
            eventQueue.sendEvent(evtName, evtData);
        }

        if (evtName == PlayerListener.CLOSED || 
            evtName == PlayerListener.ERROR) {
            closedDelivered = true;
        }
    }

    synchronized void doFinishLoopIteration() {
        //System.out.println("DEBUG: about to BP.doFinishLoopIteration() for player=" + this);
        EOM = true;
        loopAfterEOM = false;
        if (state > Player.PREFETCHED) {

            updateTimeBase(false);

            state = Player.PREFETCHED;
            if (loopCount > 1 || loopCount == -1) {
                loopAfterEOM = true;
            }
        }
        //System.out.println("DEBUG: finished BP.doFinishLoopIteration() for player=" + this);
    }
    /**
     *  Description of the Method
     */
    synchronized void doNextLoopIteration() {
        //System.out.println("DEBUG: about to BP.doNextLoopIteration() for player=" + this);
        if (loopAfterEOM) {
            // If a loop count is set, we'll loop back to the beginning.
            if ((loopCount > 1) || (loopCount == -1)) {
                try {
                    if (setMediaTime(0) == 0) {
                        if (loopCount > 1) {
                            loopCount--;
                        }
                        start();
                    } else {
                        loopCount = 1;
                    }
                } catch (MediaException ex) {
                    loopCount = 1;
                }
            } else if (loopCountSet > 1) {
                loopCount = loopCountSet;
            }

            loopAfterEOM = false;
        }
        //System.out.println("DEBUG: finished BP.doNextLoopIteration() for player=" + this);
    }


    // "final" to verify that no subclass overrides getControls.
    // can be removed if overload necessary
    // JAVADOC COMMENT ELIDED
    public final Control[] getControls() {
        chkClosed(true);
        
        Vector v = new Vector(3);
        // average maximum number of controls
        for (int i = 0; i < control_names.length; i++) {
            Object c = getControl(control_names[i]);
            if ((c != null) && !v.contains(c)) {
                v.addElement(c);
            }
        }
        Control[] ret = new Control[v.size()];
        v.copyInto(ret);        
        return ret;
    }


    // JAVADOC COMMENT ELIDED
    public final Control getControl(String type) {
        chkClosed(true);

        if (type == null) {
            throw new IllegalArgumentException();
        }

        // Prepend the package name if the type given does not
        // have the package prefix.
        if (type.indexOf('.') < 0) {
            // for non-fully qualified control names,
            // look up the package in the allCtrls array
            for (int i = 0; i < allCtrls.length; i++) {
                if (allCtrls[i].equals(type)) {
                    // standard controls are specified
                    // without package name in allCtrls
                    return doGetControl(pkgName + type);
                } else if (allCtrls[i].endsWith(type)) {
                    // non-standard controls are with
                    // full package name in allCtrls
                    return doGetControl(allCtrls[i]);
                }
            }
        }
        return doGetControl(type);
    }


    // JAVADOC COMMENT ELIDED
    protected abstract Control doGetControl(String type);


    // JAVADOC COMMENT ELIDED
    public static BasicPlayer get(int pid) {
        return (BasicPlayer) (mplayers.get(new Integer(pid)));
    }

    // JAVADOC COMMENT ELIDED
    public static void pauseAll() {
        //System.out.println("DEBUG: about to BP.pauseAll()");
        if (mplayers == null) {
            return;
        }

        for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
            BasicPlayer p = (BasicPlayer) e.nextElement();

            int state = p.getState();
            long time = p.getMediaTime();
            
            // save the player's state
            pstates.put(p, new Integer(state));

            // save the player's media time
            mtimes.put(p, new Long(time));

            // deallocate the player
            //
            // this will implicitly stop all players
            // and release scarce resources such as
            // the audio device
            p.deallocate();
        }
    }


    // JAVADOC COMMENT ELIDED
    public static void resumeAll() {
        //System.out.println("DEBUG: about to BP.resumeAll()");
        if (mplayers == null || pstates.size() == 0) {
            return;
        }
        
        for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
            BasicPlayer p = (BasicPlayer) e.nextElement();

            int state = ((Integer) pstates.get(p)).intValue();
            long time = ((Long) mtimes.get(p)).longValue();

            switch (state) {
                case Player.PREFETCHED:
                    try {
                        //System.out.println("DEBUG: BP.resumeAll() for PREFETCHED player=" + p);
                        p.prefetch();
                        p.setMediaTime(time);
                    } catch (MediaException ex) {
                    }
                    break;
                case Player.STARTED:
                    try {
                        //System.out.println("DEBUG: BP.resumeAll() for STARTED player=" + p);
                        p.realize();
                        p.prefetch();
                        p.setMediaTime(time);
                        p.start();
                    } catch (MediaException ex) {
                    }
                    break;
            }
        }

        // clear player states and media times
        pstates.clear();
        mtimes.clear();
    }
    

    // JAVADOC COMMENT ELIDED
    public int doSetLevel(int ll) {
        return ll;
    }

    /**
     * MMAPI Full specific methods.
     *
     * @param  source              The new source value
     * @exception  IOException     Description of the Exception
     * @exception  MediaException  Description of the Exception
     */

    // JAVADOC COMMENT ELIDED
    public void setSource(DataSource source)
         throws IOException, MediaException {
        this.source = source;
    }


    // JAVADOC COMMENT ELIDED
    public void setTimeBase(TimeBase master) throws MediaException {
        //System.out.println("DEBUG: about to BP.setTimeBase(" + master + ") for player=" + this);
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
    }
    

    // JAVADOC COMMENT ELIDED
    public TimeBase getTimeBase() {
        chkClosed(true);
        return timeBase;
    }


    // JAVADOC COMMENT ELIDED
    public String getContentType() {
        chkClosed(true);
        return ((source != null) ? source.getContentType() : "");
    }


    // JAVADOC COMMENT ELIDED
    public final long getStopTime() {
        //System.out.println("DEBUG: finished BP.getStopTime()=" + stopTime + " for player=" + this);
        return stopTime;
    }


    // JAVADOC COMMENT ELIDED
    public final synchronized void setStopTime(long time) {
        //System.out.println("DEBUG: about to BP.setStopTime(" + time + ") for player=" + this);
        if (state == STARTED) {
            /*
             *  If the stop time has already been set and its being set again,
             *  throw an exception
             */
            if (stopTime != StopTimeControl.RESET && 
                time != StopTimeControl.RESET) {
                throw new IllegalStateException("StopTime already set");
            }
            /*
             *  If the new stop time is before the current media time,
             *  stop the player and send an event
             */
            if (time < getMediaTime()) {
                try {
                    doStop();
                } catch (MediaException e) {
                    // Not much we can do here.
                }
                satev(); // Send STOPPED_AT_TIME event
                //System.out.println("DEBUG: finished(stopped) to BP.setStopTime(" + time + ") for player=" + this);
                return;
            }
        }
        stopTime = time;
        doSetStopTime(stopTime);
        //System.out.println("DEBUG: finished to BP.setStopTime(" + time + ") for player=" + this);
    }


    // JAVADOC COMMENT ELIDED
    protected void doSetStopTime(long time) {
    }


    // JAVADOC COMMENT ELIDED
    protected void satev() {
        // Update the time base to use the system time
        // before stopping.
        updateTimeBase(false);
        state = PREFETCHED;
        stopTime = StopTimeControl.RESET;
        sendEvent(PlayerListener.STOPPED_AT_TIME, new Long(getMediaTime()));
    }


    // JAVADOC COMMENT ELIDED
    private long origin = 0;
    private long offset = 0;
    private long time = 0;
    private long sysOffset = 0;
    private boolean useSystemTime = true;
    private Object timeLock = new Object();


    // JAVADOC COMMENT ELIDED
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


    // JAVADOC COMMENT ELIDED
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



    // JAVADOC COMMENT ELIDED


    // JAVADOC COMMENT ELIDED
    int NOT_SEEKABLE = SourceStream.NOT_SEEKABLE;

    // JAVADOC COMMENT ELIDED
    int SEEKABLE_TO_START = SourceStream.SEEKABLE_TO_START;

    // JAVADOC COMMENT ELIDED
    int RANDOM_ACCESSIBLE = SourceStream.RANDOM_ACCESSIBLE;


    // JAVADOC COMMENT ELIDED
    protected int readFully(byte b[], int off, int len) throws IOException {
        int n = 0;
        while (n < len) {
            int count = stream.read(b, off + n, len - n);
            if (count < 0) {
                throw new IOException("premature end of stream");
            }
            n += count;
        }
        return len;
    }


    private final static int MAX_SKIP = 2048;
    private static byte[] skipArray;


    // JAVADOC COMMENT ELIDED
    protected long skipFully(int numBytes) throws IOException {
        long target = stream.tell() + numBytes;
        if (stream.getSeekType() == SourceStream.RANDOM_ACCESSIBLE) {
            if (stream.seek(target) != target) {
                throw new IOException("skipped past end");
            }
            return numBytes;
        }

        if (numBytes < 0) {
            throw new IOException("bad param");
        }

        // Allocate memory for skip array
        int toSkip = numBytes;
        int min = numBytes;
        if (min > MAX_SKIP) {
            min = MAX_SKIP;
        }
        if (skipArray == null || skipArray.length < min) {
            skipArray = new byte[min];
        }

        // Skip over numBytes
        while (toSkip > 0) {
            min = toSkip;
            if (min > MAX_SKIP) {
                min = MAX_SKIP;
            }

            if (stream.read(skipArray, 0, min) < min) {
                throw new IOException("skipped past end");
            }
            toSkip -= min;
        }

        return numBytes;
    }


    // JAVADOC COMMENT ELIDED
    protected long seekStrm(long where) throws IOException {
        return stream.seek(where);
    }


    // JAVADOC COMMENT ELIDED
    protected int getSeekType() {
        return stream.getSeekType();
    }
}

/**
 * The thread that's responsible for delivering Player events.
 * This class lives for only 5 secs.  If no event comes in
 * 5 secs, it will exit.
 *
 * @created    January 13, 2005
 */
class PlayerEventQueue extends Thread {
    // JAVADOC COMMENT ELIDED
    private BasicPlayer p;
    // JAVADOC COMMENT ELIDED
    private EventQueueEntry evt;

    // JAVADOC COMMENT ELIDED
    PlayerEventQueue(BasicPlayer p) {
        this.p = p;
        evt = null;
        //System.out.println("DEBUG: Created Player Event Queue ! player=" + p);
        start();
    }

    // JAVADOC COMMENT ELIDED
    synchronized void sendEvent(String evtName, Object evtData) {

        //System.out.println("DEBUG: about to Queue.sendEvent(" + evtName + "," + evtData +") for player=" + this);
        
        //add element to the ring ...
        if (evt == null) {
            evt = new EventQueueEntry(evtName, evtData);
        } else {
            evt.link = new EventQueueEntry(evtName, evtData, evt.link);
            evt = evt.link;
        }
        this.notifyAll();
    }


    // JAVADOC COMMENT ELIDED
    public void run() {

        String evtName = "";
        Object evtData = null;
        EventQueueEntry evtLink = null;
        
        boolean evtToGo = false; // true if there is an event to send
        
        // true if at least one event is sent,
        // in case that posting the initial event
        // takes a long time
        boolean evtSent = false;

        for (; ; ) {

            synchronized (this) {
                // TBD: use a special Object to wait/notify
                // instead of time delays 
                // (for synchronization and wake up of 
                // BasicPlayer.sendEvent(...);s threads and 
                // PlayerEventQueue.run() thread ) ? 
                //
                // If the queue is empty, we'll wait for at most
                // 5 secs.
                if (evt == null) {
                    try {
                        this.wait(5000);
                    } catch (InterruptedException ie) {
                    }
                }
                if (evt != null) {
                    evtLink = evt.link;
                    //exclude element from the ring ...
                    if (evtLink == evt) {
                        evt = null;
                    } else {
                        evt.link = evtLink.link;
                    }
                    evtToGo = true;
                    
                    evtName = evtLink.name;
                    evtData = evtLink.data;
                    
                    // For garbage collection.
                    evtLink.link = null;
                    evtLink.name = null;
                    evtLink.data = null;
                    evtLink = null;
                    
                } else {
                    evtToGo = false;
                }

            }
            // synchronized this

            if (evtToGo) {
                // TBD: move it to "sendEvent(...)" to provide loop-related
                // reaction on EOM earlier ?
                //
                // First, check and handle EOM.
                if (evtName == PlayerListener.END_OF_MEDIA) {
                    p.doFinishLoopIteration();
                }

                //System.out.println("DEBUG: about to notifyListeners(" + evtName + "," + evtData +") for player=" + p);
                // Notify the PlayerListeners.
                p.notifyListeners(evtName, evtData);

                // We'll need to loop back if looping was set.
                p.doNextLoopIteration();

                evtSent = true;

            }
            // if (evtToGo)

            // We'll exit the event thread if we have already sent one
            // event and there's no more event after 5 secs; or if the
            // Player is closed.

            if (evtName == PlayerListener.CLOSED || 
                evtName == PlayerListener.ERROR) {
                // try to nullify queue reference and exit 
                // if player is closing ...
                synchronized (p.evtLock) {
                    //System.out.println("DEBUG: Killed Player Event Queue (STOP/ERROR)! player=" + p);
                    p.eventQueue = null;
                    break; // Exit the event thread.
                }
            }

            synchronized (this) {
                // try to nullify queue reference and exit 
                // if nothing to send (but there were events in the past) ...
                if (evt == null && evtSent && !evtToGo) {
                    synchronized (p.evtLock) {
                        //System.out.println("DEBUG: Killed Player Event Queue (empty for 5 sec)! player=" + p);
                        p.eventQueue = null;
                        break; // Exit the event thread.
                    }
                }
            }
        }
    }
}

class EventQueueEntry {
    String name;
    Object data;
    EventQueueEntry link;
    
    public EventQueueEntry(String n, Object d) {
        name = n;
        data = d;
        link = this;
    }
    public EventQueueEntry(String n, Object d, EventQueueEntry l) {
        name = n;
        data = d;
        link = l;
    }
}
