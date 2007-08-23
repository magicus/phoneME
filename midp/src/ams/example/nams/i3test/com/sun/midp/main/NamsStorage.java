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

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * A wrapper for NAMS MIDlet API.
 */
public class NamsStorage {
    
    /** size of the NAMS storage */
    public static final int NAMS_STORAGE_SIZE = 20;

    /** free entry in the storage */
    public static final int NAMS_STATE_NO = 0;
    /** midlet state is active */
    public static final int NAMS_STATE_ACTIVE = 1;
    /** midlet state is paused */
    public static final int NAMS_STATE_PAUSED = 2;
    /** midlet state is destroyed */
    public static final int NAMS_STATE_DESTROYED = 3;
    /** midlet failed to start */
    public static final int NAMS_STATE_ERROR = 4;
    /** storage entry is reserved for new (starting) midlet */
    public static final int NAMS_STATE_RESERVED = 5;
    
    /**
     * returns isolate where NAMS Notifier is running
     * (where to send notification events)
     *
     * @param midletSuiteStorage MIDletSuiteStorage object
     * @param inp_installer Installer object
     * @param inp_url URL of the test suite
     *
     * @return isolate Id
     */
    public static native int getNotifierIsolateId();
    
    /**
     * returns state of a given midlet
     *
     * @param id the ID of the midlet
     * @return midlet state
     */
    static native int getMIDletState(int id);
    /**
     * returns display status of a given midlet
     *
     * @param id the ID of the midlet
     * @return display status
     */
    static native boolean getDisplayStatus(int id);
    /**
     * returns display status change request status of a given midlet
     * (not used now)
     *
     * @param id the ID of the midlet
     * @return display status change request 
     */
    static native boolean getDisplayChangeRequest(int id);
    /**
     * returns midlet state change tracking status of a given midlet
     *
     * @param id the ID of the midlet
     * @return true if notification shall be sent for 
     *         midlet state changes of the given midlet
     *         false - otherwise
     */
    static native boolean getMIDletStateTrack(int id);
    /**
     * returns display status change tracking status of a given midlet
     *
     * @param id the ID of the midlet
     * @return true if notification shall be sent for 
     *         display status changes of the given midlet
     *         false - otherwise
     */
    static native boolean getDisplayStatusTrack(int id);
    
    /**
     * stores state of a given midlet in NAMS Storage
     *
     * @param id the ID of the midlet
     * @param state new state of the midlet
     */
    static native void setMIDletState(int id, int state);
    /**
     * stores display status a given midlet in NAMS Storage
     *
     * @param id the ID of the midlet
     * @param status new display status of the midlet
     */
    static native void setDisplayStatus(int id, boolean status);
    /**
     * stores display status change request a given midlet in NAMS Storage
     *
     * @param id the ID of the midlet
     * @param request new display status change request of the midlet
     */
    static native void setDisplayChangeRequest(int id, boolean request);
    /**
     * stores midlet state traking a given midlet in NAMS Storage
     *
     * @param id the ID of the midlet
     * @param track new midlet state vhange traning of the midlet
     */
    static native void setMIDletStateTrack(int id, boolean track);
    /**
     * stores display status traking a given midlet in NAMS Storage
     *
     * @param id the ID of the midlet
     * @param track new display status vhange traning of the midlet
     */
    static native void setDisplayStatusTrack(int id, boolean track);

}
    
