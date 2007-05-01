/*
 *  
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

package com.sun.midp.main;

import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventListener;
import com.sun.midp.events.Event;
import com.sun.midp.events.NativeEvent;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Works as a simple (display) manager to test NAMS.
 */
public class NamsNotifier implements EventListener {
    
    /** 
     * array of sync objects for midlet state change notifications - 
     * one per midlet/per state
     */
    private Object syncState[][];

    /** 
     * array of sync objects for display FG notifications - 
     * one per midlet
     */
    private Object syncFG[];

    /** 
     * array of sync objects for display BG notifications - 
     * one for all midlets
     */
    private Object syncBG;
    
    /**
     * public constructor.
     * 
     * registers itself as an event listener for TEST_EVENT
     */
    public NamsNotifier() {
        
        int i, j;
        EventQueue eventQueue;
        
        // appId = 0 is reserved for NamsManager itself
        initNamsNotifier(MIDletSuiteUtils.getIsolateId());
        
        // register self as the listener for 
        eventQueue = EventQueue.getEventQueue();
        // if (eventQueue == null)
        //     Logging.report(Logging.ERROR, LogChannels.LC_CORE,
        //         "DEBUG: Notifier: constructor - event queue is null !!!");
        eventQueue.registerEventListener(EventTypes.TEST_EVENT, this);
        
        syncState = new Object[NamsStorage.NAMS_STORAGE_SIZE][6];
        syncFG = new Object[NamsStorage.NAMS_STORAGE_SIZE];
        syncBG = new Object();
        
        for (i = 0; i < NamsStorage.NAMS_STORAGE_SIZE; ++i) {
            for (j = 0; j < 6; ++j) {
                syncState[i][j] = new Object();
            }
            syncFG[i] = new Object();
        }
    }
    
    /**
     * initializes native part of NamsNotifier
     *
     * @param isolateId isolate where NamsNotifier is run 
     *        (where to sent events)
     */
    private native void initNamsNotifier(int isolateId);
    
    /**
     * event preprocessing event routine - empty
     *
     * @param event event to check
     * @param waitingEvent event to compare with 
     *
     * @return always returns true
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * main event event processing event routine 
     *
     * @param event event to process
     *
     */
    public void process(Event event) {
        NativeEvent e = (NativeEvent)event;
        int appId = e.intParam1;
        int callbackId = e.intParam2;
        int state = e.intParam3;
        int reason = e.intParam4;
        
        switch (callbackId) {
            case 0: 
                /* BG callback - no parameters */ 
                /*
                Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                   "DEBUG: Got Notification: {" +
                   "appId=-1, " +
                   "BG=" + callbackId + ", " +
                   "x, " + 
                   "reason=" + reason + "}");
                 */
                synchronized (syncBG) { 
                    syncBG.notifyAll();
                }
                break;
            case 1: 
                /* FG callback */ 
                if (appId < 0 || appId > NamsStorage.NAMS_STORAGE_SIZE) {
                    /*
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                        "DEBUG: Notifier: FG callback - " +
                        "index out of bounds ..." +
                        " " + appId);
                     */
                } else {
                    /*
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                        "DEBUG: Got Notification: {" +
                        " appId=" + appId +  ", " +
                        "FG=" + callbackId + ", " +
                        "x, " + 
                        "reason=" + reason + "}");
                     */
                    synchronized (syncFG[appId]) { 
                        syncFG[appId].notifyAll();
                    }
                }
                break;
            case 2: 
                /* State callback */ 
                if (appId < 0 || appId >= NamsStorage.NAMS_STORAGE_SIZE ||
                    state < 1 || state > 5) {
                    /*
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                        "DEBUG: Notifier: State callback - " +
                        "index out of bounds ..." +
                         " " + appId +
                         " " + state);
                     */
                } else {
                    /*
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                        "DEBUG: Got Notification: {" +
                        " appId=" + appId +  ", " +
                        "STATE=" + callbackId + ", " +
                        "state=" + state + ", " +
                        "reason=" + reason + "}");
                     */
                    synchronized (syncState[appId][state]) { 
                        syncState[appId][state].notifyAll();
                    }
                }
                break;
        }
    }
    
    public boolean waitForBG(long timeout) {
        boolean value = true;
        
        synchronized (syncBG) {
            try {
                /*
                Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                    "DEBUG: Enter Waiting for BG");
                 */
                syncBG.wait(timeout);
            } catch (InterruptedException ie) {
                value = false;
            } catch (IllegalMonitorStateException ime) {
                Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                   "Unexpected monitor exception");
            }
        }
        return value;
    }

    public boolean waitForFG(int appId, long timeout) {
        boolean value = true;
        
        synchronized (syncFG[appId]) {
            try {
                /*
                Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                    "DEBUG: Enter Waiting for appId=" + appId + "FG");
                 */
                syncFG[appId].wait(timeout);
            } catch (InterruptedException ie) {
                value = false;
            } catch (IllegalMonitorStateException ime) {
                Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                   "Unexpected monitor exception");
            }
        }
        return value;
    }

    public boolean waitForState(int appId, int state, long timeout) {
        boolean value = true;
        
        synchronized (syncState[appId][state]) {
            try {
                /*
                Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                    "DEBUG: Enter Waiting for appId=" + appId + 
                    " state=" + state);
                 */
                syncState[appId][state].wait(timeout);
            } catch (InterruptedException ie) {
                value = false;
            } catch (IllegalMonitorStateException ime) {
                Logging.report(Logging.ERROR, LogChannels.LC_CORE,
                   "Unexpected monitor exception");
            }
        }
        return value;
    }
}
   
