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

//import com.sun.mmedia.PermissionAccessor;

/**
 * BasicPlayer provides basic implementation for the Player methods.
 * Many of the methods call do<method> to do the actual work that can
 * be overridden by subclasses.
 *
 * @created    January 13, 2005
 */
public abstract class ABBBasicPlayer implements Player {
    
    /**
     * global player id
     */
    private static int pcount = -1;

    /**
     * lock object
     */
    private static Object idLock = new Object();

    /**
     * the state of this player
     */
    public int state = UNREALIZED;

    /**
     * the loopCount of this player
     */
    int loopCountSet = 1, loopCount;

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

    /**
     * this player's playerlisteners
     */
    Vector listeners = new Vector(2);
    
    /**
     * flag shows that "listeners" have been modified while 
     * player is executing callbacks from "listeners".
     */
    boolean listenersModified = false;

    /**
     * Asynchronous event mechanism.
     */
    PlayerEventQueue eventQueue = null;

    /**
     * event queue lock obj
     */
    Object evtLock = new Object();

    /**
     * player ID of this player
     */
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

    
    /**
     * Control package name
     */
    protected final static String pkgName = "javax.microedition.media.control.";

    /**
     * Centralized control management with string constants for each
     * implemented Control.
     * <p>
     * For adding a new control interfaces, follow the following steps:
     * <ol>
     *  <li>Add new control name here. If it is not in the package
     *     javax.microedition.media.control, then the full package
     *     name of the control must be used.
     *  <li>Add the control's name field to the allCtrls array (see below)
     * </ol>
     * <p>
     * Size note: it would be space saving to declare the array allCtrls
     * with the strings directly and not define the strings constants here.
     * However, it is more convenient for descendants to use
     * these constants, instead of e.g.
     * <code>&nbsp;&nbsp;&nbsp;allCtrls[4]; // RateControl</code>
     *
     * @see    #getControls()
     * @see    #doGetControl(String)
     * @see    #allCtrls
     */

    /**
     *  Description of the Field
     */
    protected final static String fpcName = "FramePositioningControl";
    /**
     *  Description of the Field
     */
    protected final static String guiName = "GUIControl";
    /**
     *  Description of the Field
     */
    protected final static String mdcName = "MetaDataControl";
    /**
     *  Description of the Field
     */
    protected final static String micName = "MIDIControl";
    /**
     *  Description of the Field
     */
    protected final static String picName = "PitchControl";
    /**
     *  Description of the Field
     */
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
    /**
     *  This one is not among public JSR135 controls, 
     *  seems that this is used only for RTSP PLayer.
     *  But its participation in "search-by-name" slows down 
     *  all players that use this array for "getControls()".
     */
    protected final static String rtspName = "RtspControl";

    /**
     * An array containing all available controls in Players
     * extending BasicPlayer.
     * A player can overwrite this array in order to change the list.
     */
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
    
    /**
     * An array containing all needed permissions in Players
     * extending BasicPlayer.
     * A player can overwrite this array in order to change the list.
     *
     * By default it is empty.
     */
    private static final int[] allPermissions = {};

    /**
     * is set by checkPermissions() to bypass further checks
     */
    private boolean isTrusted;
    
    /**
     * array of controls for a given player
     */
    private String control_names[];
    
    /**
     * array of permissions for a given player
     */
    private int permissions[];

    /**
     * the default size of the event queue
     * can be overridden by descendants
     */
    int eventQueueSize = 20;

    /**
     * flag to prevent delivering events after the CLOSED event
     */
    private boolean closedDelivered;

    /**
     * listener for media events while the midlet is in paused state.
     */ 
    private static MIDletPauseListener pauseListener = null;
    private static boolean vmPaused = false;

    /* Source data for player */
    InputStream source;
    
    /**
     * Sets the listener for media activity notifications.
     *
     * This interface can only be set once and shall only be used
     * by MIDletState.
     */
    public static void setMIDletPauseListener(MIDletPauseListener listener) {
        //System.out.println("DEBUG: about to BP.setMIDletPauseListener(" + listener + ")");
        // Only set the listener once!
        // If the listener is aleady set then return witout waring.
        if (pauseListener == null) {
            pauseListener = listener;
        }
    }

    /**
     * Informs the BasicPlayer that the VM has been paused if
     * paused is set to true - or resumed if paused is set to false
     */
    public static void pauseStateEntered(MIDletPauseListener listener,
                                         boolean paused) {
        //System.out.println("DEBUG: about to BP.pauseStateEntered(" + listener + "," + paused + ")");
        // if the listeners don't match then simply return
        if (listener != pauseListener) return;

        vmPaused = paused;

        if (vmPaused) {
            for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
                ABBBasicPlayer p = (ABBBasicPlayer)e.nextElement();
                
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
    
    /**
     *Constructor for the ABBBasicPlayer object
     */
    public ABBBasicPlayer() {
        init();
        control_names = allCtrls;
        permissions = allPermissions;
    }
    
    protected ABBBasicPlayer(String[] n, int[] p) {
        init();
        control_names = (n == null) ? allCtrls : n;
        permissions = (p == null) ? allPermissions : p;
    }
    
    private void init() {

        synchronized (idLock) {
            pcount = (pcount + 1) % 32767;
            pID = pcount;
        }
        mplayers.put(new Integer(pID), this);
    }

    /**
     * Initializes Player by Media Encodings obtained from URI and parsed.
     * To be called by Manager when new player (from URI) is being created.
     *
     * @param encodings media encodings in form "key=value", separated by '&'.
     *
     * @returns true if initialization was successful, false otherwise
     */
    public boolean initFromURL(String encodings) {
        return true;
    }
    
    /**
     * Checks if user application has all permissions needed to access player
     */
    protected final void checkPermissions() throws SecurityException {
/*
        if (isTrusted) return;
        //TBD: check locator-specific permissions ?
        for (int i = 0; i < permissions.length; ++i)
            PermissionAccessor.checkPermissions(permissions[i]);
        isTrusted = true;
 */
    }
    
    /**
     * Check to see if the Player is closed.  If the
     * unrealized boolean flag is true, check also to
     * see if the Player is UNREALIZED.
     *
     * @param  unrealized  Description of the Parameter
     */
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


    /**
     *  Description of the Method
     *
     * @param  count  Description of the Parameter
     */
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

    
    
    public void setSource(InputStream source)
         throws IOException, MediaException {
        this.source = source;
    }
    
    // JAVADOC COMMENT ELIDED
    public synchronized void realize() throws MediaException {
        //System.out.println("DEBUG: about to BP.realize() for player=" + this);
        chkClosed(false);

        if (state >= REALIZED) {
            return;
        }

        doRealize();
        
        state = REALIZED;
    }

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


    /**
     * Subclasses need to implement this to prefetch
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
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


    /**
     * Subclasses need to implement this start
     * the <code>Player</code>.
     *
     * @return    Description of the Return Value
     */
    protected abstract boolean doStart();


    /**
     * Subclasses can override this method to do the actual starting
     * of worker threads.
     */
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

        state = PREFETCHED;
        sendEvent(PlayerListener.STOPPED, new Long(getMediaTime()));
        //System.out.println("DEBUG: finished BP.stop() for player=" + this);
    }


    /**
     * Subclasses need to implement this to realize
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
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


    /**
     * Subclasses need to implement this to deallocate
     * the <code>Player</code>.
     */
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
        
        sendEvent(PlayerListener.CLOSED, null);
        mplayers.remove(new Integer(pID));
    }


    /**
     * Subclasses need to implement this to close
     * the <code>Player</code>.
     */
    protected abstract void doClose();


    // JAVADOC COMMENT ELIDED
    public synchronized long setMediaTime(long now) throws MediaException {
        //System.out.println("DEBUG: about to BP.setMediaTime(" + now + ") for player=" + this);
        chkClosed(true);

        long theDur = doGetDuration();
        if ((theDur != TIME_UNKNOWN) && (now > theDur)) {
            now = theDur;
        }

        long rtn = doSetMediaTime(now);
        EOM = false;

        //System.out.println("DEBUG: finished BP.setMediaTime(" + now + ")=" + rtn + " for player=" + this);
        return rtn;
    }


    /**
     * Subclasses need to implement this to set the media time
     * of the <code>Player</code>.
     *
     * @param  now                 Description of the Parameter
     * @return                     Description of the Return Value
     * @exception  MediaException  Description of the Exception
     */
    protected abstract long doSetMediaTime(long now) throws MediaException;


    // JAVADOC COMMENT ELIDED
    public long getMediaTime() {
        //System.out.println("DEBUG: about to BP.getMediaTime() for player=" + this);
        chkClosed(false);
        return doGetMediaTime();
    }


    /**
     * Subclasses need to implement this to get the media time
     * of the <code>Player</code>
     *
     * @return    Description of the Return Value
     */
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


    /**
     * Subclasses need to implement this to get the duration
     * of the <code>Player</code>.
     *
     * @return    Description of the Return Value
     */
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
    
    /**
     *  Description of the Method
     *
     * @param  evtName  Description of the Parameter
     * @param  evtData  Description of the Parameter
     */
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
    /**
     *  Gets the controls attribute of the BasicPlayer object
     *
     * @return    The controls value
     */
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


    /**
     * The worker method to actually obtain the control.
     *
     * @param  type  the class name of the <code>Control</code>.
     * @return       <code>Control</code> for the class or interface
     * name.
     */
    protected abstract Control doGetControl(String type);


    /**
     * For global PlayerID management
     *
     * @param  pid  Description of the Parameter
     * @return      Description of the Return Value
     */
    public static ABBBasicPlayer get(int pid) {
        return (ABBBasicPlayer) (mplayers.get(new Integer(pid)));
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
        //System.out.println("DEBUG: about to BP.pauseAll()");
        if (mplayers == null) {
            return;
        }

        for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
            ABBBasicPlayer p = (ABBBasicPlayer) e.nextElement();

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


    /**
     *  Resumes all media players' activities.
     *
     *  Players that were in STARTED state when pause
     *  was called will resume playing at the media time
     *  they were stopped and deallocated.
     */
    public static void resumeAll() {
        //System.out.println("DEBUG: about to BP.resumeAll()");
        if (mplayers == null || pstates.size() == 0) {
            return;
        }
        
        for (Enumeration e = mplayers.elements(); e.hasMoreElements();) {
            ABBBasicPlayer p = (ABBBasicPlayer) e.nextElement();

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
    

    /**
     * Implementation method for VolumeControl
     *
     * @param  ll  Description of the Parameter
     * @return     Description of the Return Value
     */
    public int doSetLevel(int ll) {
        return ll;
    }

    // JAVADOC COMMENT ELIDED
    public String getContentType() {
        chkClosed(true);
        return "";
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
    /**
     * the player instance
     */
    private ABBBasicPlayer p;
    /**
     * event info array
     */
    private EventQueueEntry evt;

    /**
     * The constructor
     *
     * @param  p  the instance of BasicPlayer intending to post event to
     *        this event queue.
     */
    PlayerEventQueue(ABBBasicPlayer p) {
        this.p = p;
        evt = null;
        //System.out.println("DEBUG: Created Player Event Queue ! player=" + p);
        start();
    }

    /**
     * Put an event in the event queue and wake up the thread to
     * deliver it.  If the event queue is filled, block.
     *
     * @param  evtName  Description of the Parameter
     * @param  evtData  Description of the Parameter
     */
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


    /**
     * Event handling thread.
     */
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

